#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import os
import re
import shutil
import subprocess
import tempfile
import threading
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, List, Optional

from PySide6.QtCore import QThread, Signal

from gui_runtime import packaged_runtime
from oidn_postprocess import can_run_oidn_postprocess, read_ppm, run_oidn_postprocess, write_png
from video_config_builder import RenderTaskConfig
from video_formula import ExpressionError, ExpressionEvaluator, float_text, velocity_vector_to_turns


class RenderWorker(QThread):
    progress = Signal(int, int, str)
    failed = Signal(str)
    paused = Signal(bool)
    done = Signal(str)
    aborted = Signal(str)

    def __init__(self, cfg: RenderTaskConfig) -> None:
        super().__init__()
        self.cfg = cfg
        self._abort = False
        self._pause = False
        self._lock = threading.Lock()
        self.eval = ExpressionEvaluator()
        self._current_render_proc: Optional[subprocess.Popen[str]] = None
        self._current_encoder_proc: Optional[subprocess.Popen[bytes]] = None

    def request_abort(self) -> None:
        with self._lock:
            self._abort = True
            render_proc = self._current_render_proc
        self._terminate_process(render_proc)

    def request_pause(self, pause: bool) -> None:
        with self._lock:
            self._pause = pause
        self.paused.emit(pause)

    def _is_abort(self) -> bool:
        with self._lock:
            return self._abort

    def _is_pause(self) -> bool:
        with self._lock:
            return self._pause

    def _evaluate_definitions(self, frame: int, fps: int) -> Dict[str, float]:
        env: Dict[str, float] = {
            "frame": float(frame),
            "fps": float(fps),
            "time": float(frame) / float(fps),
        }
        for line in self.cfg.definitions:
            if "=" not in line:
                continue
            name, expr = line.split("=", 1)
            name = name.strip()
            expr = expr.strip()
            if not name or not expr:
                continue
            if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
                raise ExpressionError(f"Invalid definition name: {name}")
            env[name] = self.eval.eval(expr, env)
        return env

    def _select_row(self, rows: List[Dict[str, object]], frame: int, env: Dict[str, float]) -> Dict[str, object]:
        if len(rows) == 1:
            return rows[0]

        thresholds: List[int] = []
        for i in range(len(rows) - 1):
            expr = str(rows[i].get("range_expr") or "").strip()
            if not expr:
                raise ExpressionError(f"Missing range end expression for row {i + 1}")
            thresholds.append(int(self.eval.eval(expr, env)))

        if frame <= thresholds[0]:
            return rows[0]
        for i in range(1, len(rows) - 1):
            if frame <= thresholds[i]:
                return rows[i]
        return rows[-1]

    def _eval_optional(self, expr: str, env: Dict[str, float]) -> Optional[float]:
        t = expr.strip()
        if not t:
            return None
        return self.eval.eval(t, env)

    def _now_utc_iso(self) -> str:
        return datetime.now(timezone.utc).isoformat()

    def _raytracer_binary_identity(self, path: Optional[Path] = None) -> Dict[str, object]:
        path = path or self.cfg.raytracer_bin
        info: Dict[str, object] = {"path": str(path)}
        try:
            st = path.stat()
            info["size_bytes"] = int(st.st_size)
            info["mtime_unix"] = float(st.st_mtime)
        except Exception:
            return info

        try:
            h = hashlib.sha256()
            with path.open("rb") as f:
                while True:
                    chunk = f.read(1024 * 1024)
                    if not chunk:
                        break
                    h.update(chunk)
            info["sha256"] = h.hexdigest()
        except Exception:
            pass
        return info

    def _write_manifest(self, manifest_path: Path, manifest: Dict[str, Any]) -> None:
        manifest_path.parent.mkdir(parents=True, exist_ok=True)
        tmp = manifest_path.with_suffix(manifest_path.suffix + ".tmp")
        with tmp.open("w", encoding="utf-8") as f:
            json.dump(manifest, f, indent=2)
        tmp.replace(manifest_path)

    def _freeze_runtime_bundle(self, destination_dir: Path) -> tuple[Path, Optional[Path]]:
        destination_dir.mkdir(parents=True, exist_ok=True)
        source_dir = self.cfg.raytracer_bin.parent
        include_names = {"raytracer", "oidnDenoise"}
        include_prefixes = ("libOpenImageDenoise", "libtbb")

        for entry in source_dir.iterdir():
            name = entry.name
            if name in include_names or any(name.startswith(prefix) for prefix in include_prefixes):
                dest = destination_dir / name
                if entry.is_symlink():
                    target = os.readlink(entry)
                    if dest.exists() or dest.is_symlink():
                        dest.unlink()
                    os.symlink(target, dest)
                elif entry.is_file():
                    shutil.copy2(entry, dest)

        frozen_binary = destination_dir / "raytracer"
        if not frozen_binary.exists():
            # Non-packaged/dev path may not have dependencies adjacent; at minimum keep the binary snapshot.
            frozen_binary = destination_dir / "raytracer_snapshot"
            shutil.copy2(self.cfg.raytracer_bin, frozen_binary)
        frozen_binary.chmod(frozen_binary.stat().st_mode | 0o111)

        frozen_oidn = destination_dir / "oidnDenoise"
        if frozen_oidn.exists():
            frozen_oidn.chmod(frozen_oidn.stat().st_mode | 0o111)
            return frozen_binary, frozen_oidn
        return frozen_binary, None

    def _find_ffmpeg(self) -> str:
        env_ffmpeg = os.environ.get("SR_RT_FFMPEG_BIN", "").strip()
        ffmpeg = env_ffmpeg if env_ffmpeg and Path(env_ffmpeg).exists() else shutil.which("ffmpeg")
        if not ffmpeg:
            for candidate in ["/opt/homebrew/bin/ffmpeg", "/usr/local/bin/ffmpeg"]:
                if Path(candidate).exists():
                    ffmpeg = candidate
                    break
        if not ffmpeg:
            raise RuntimeError("ffmpeg not found. Install ffmpeg or add it to PATH.")
        return ffmpeg

    def _start_streaming_encoder(self, ffmpeg: str, video_path: Path) -> subprocess.Popen[bytes]:
        enc_cmd = [
            ffmpeg,
            "-y",
            "-loglevel",
            "error",
            "-framerate",
            str(self.cfg.fps),
            "-f",
            "image2pipe",
            "-vcodec",
            "ppm",
            "-i",
            "-",
            "-c:v",
            "libx264",
            "-pix_fmt",
            "yuv420p",
            str(video_path),
        ]
        proc = subprocess.Popen(
            enc_cmd,
            stdin=subprocess.PIPE,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
        )
        proc._sr_rt_command = enc_cmd  # type: ignore[attr-defined]
        return proc

    def _final_frame_path(self, output_base: Path, denoise_enabled: bool) -> Path:
        return output_base.with_name(output_base.name + ("_denoised" if denoise_enabled else "")).with_suffix(".ppm")

    def _stream_frame_to_encoder(self, encoder: subprocess.Popen[bytes], frame_path: Path) -> None:
        if encoder.stdin is None:
            raise RuntimeError("ffmpeg encoder stdin is not available.")
        try:
            encoder.stdin.write(frame_path.read_bytes())
            encoder.stdin.flush()
        except BrokenPipeError:
            stderr = b""
            if encoder.stderr is not None:
                try:
                    stderr = encoder.stderr.read()
                except Exception:
                    stderr = b""
            details = stderr.decode("utf-8", errors="replace").strip()
            raise RuntimeError(f"ffmpeg encoder terminated unexpectedly.\n{details}")

    def _save_png_copy(self, source_ppm: Path, destination_png: Path) -> None:
        width, height, pixels = read_ppm(source_ppm)
        write_png(destination_png, width, height, pixels)

    def _cleanup_frame_outputs(self, output_base: Path) -> None:
        paths = [
            output_base.with_suffix(".ppm"),
            output_base.with_name(output_base.name + "_denoised").with_suffix(".ppm"),
            output_base.with_name(output_base.name + "_albedo").with_suffix(".ppm"),
            output_base.with_name(output_base.name + "_normal").with_suffix(".ppm"),
            output_base.with_suffix(".exr"),
            output_base.with_suffix(".png"),
        ]
        for path in paths:
            try:
                path.unlink()
            except FileNotFoundError:
                pass
            except Exception:
                pass

    def _finalize_encoder(self, encoder: subprocess.Popen[bytes]) -> List[str]:
        if encoder.stdin is not None and not encoder.stdin.closed:
            encoder.stdin.close()
        stderr_data = b""
        if encoder.stderr is not None:
            stderr_data = encoder.stderr.read()
        rc = encoder.wait()
        details = stderr_data.decode("utf-8", errors="replace").strip()
        command = getattr(encoder, "_sr_rt_command", None)
        if rc != 0:
            raise RuntimeError(f"ffmpeg encode failed:\n{details[-1200:]}")
        return list(command) if command is not None else []

    def _terminate_encoder(self, encoder: Optional[subprocess.Popen[bytes]]) -> None:
        if encoder is None:
            return
        try:
            if encoder.stdin is not None and not encoder.stdin.closed:
                encoder.stdin.close()
        except Exception:
            pass
        if encoder.poll() is None:
            try:
                encoder.terminate()
                encoder.wait(timeout=5)
            except Exception:
                try:
                    encoder.kill()
                except Exception:
                    pass

    def _terminate_process(self, proc: Optional[subprocess.Popen[str]]) -> None:
        if proc is None:
            return
        if proc.poll() is None:
            try:
                proc.terminate()
                proc.wait(timeout=5)
            except Exception:
                try:
                    proc.kill()
                except Exception:
                    pass

    def _handle_abort(
        self,
        *,
        manifest: Dict[str, Any],
        manifest_path: Optional[Path],
        run_start: float,
        frames_rendered: int,
        frames_streamed: int,
        video_path: Path,
        encoder: Optional[subprocess.Popen[bytes]],
    ) -> None:
        partial_video_path = ""
        manifest["status"] = "aborted"
        manifest["ended_utc"] = self._now_utc_iso()
        manifest["duration_sec"] = time.time() - run_start
        manifest.setdefault("progress", {})["frames_rendered"] = frames_rendered
        manifest.setdefault("progress", {})["frames_streamed"] = frames_streamed

        if frames_streamed > 0 and encoder is not None:
            try:
                enc_cmd = self._finalize_encoder(encoder)
                manifest["ffmpeg"] = {
                    **manifest.get("ffmpeg", {}),
                    "command": enc_cmd,
                    "mode": "streaming",
                }
                self._current_encoder_proc = None
                if video_path.exists() and video_path.stat().st_size > 0:
                    partial_video_path = str(video_path)
                    manifest.setdefault("outputs", {})["partial_video_path"] = partial_video_path
                    manifest.setdefault("outputs", {})["video_size_bytes"] = int(video_path.stat().st_size)
            except Exception as exc:
                manifest["abort_finalize_error"] = str(exc)
        else:
            try:
                if video_path.exists() and video_path.stat().st_size == 0:
                    video_path.unlink()
            except Exception:
                pass

        if manifest_path is not None:
            self._write_manifest(manifest_path, manifest)
        self.aborted.emit(partial_video_path)

    def run(self) -> None:
        manifest_path: Optional[Path] = None
        manifest: Dict[str, Any] = {}
        run_start = time.time()
        temp_work_dir: Optional[Path] = None
        frozen_binary_dir: Optional[Path] = None
        encoder: Optional[subprocess.Popen[bytes]] = None
        frames_streamed = 0
        try:
            packaged_python_denoise = packaged_runtime() and self.cfg.denoise and can_run_oidn_postprocess()
            init_env = self._evaluate_definitions(frame=0, fps=self.cfg.fps)
            total_frames = int(self.eval.eval(self.cfg.total_frames_expr, init_env))
            if total_frames <= 0:
                raise ExpressionError("Total number of frames must be > 0")

            stamp = time.strftime("%Y%m%d_%H%M%S")
            base_name = f"video_{self.cfg.scene_name}_{stamp}"
            video_path = self.cfg.output_dir / f"{base_name}.mp4"
            manifest_path = self.cfg.output_dir / f"{base_name}.manifest.json"
            work_dir = Path(tempfile.mkdtemp(prefix="sr_rt_video_frame_"))
            temp_work_dir = work_dir
            if self.cfg.keep_frames:
                frames_dir = self.cfg.output_dir / f"{base_name}_frames"
                frames_dir.mkdir(parents=True, exist_ok=True)
            else:
                frames_dir = None

            frozen_binary_dir = Path(tempfile.mkdtemp(prefix="sr_rt_bin_"))
            frozen_binary, frozen_oidn = self._freeze_runtime_bundle(frozen_binary_dir)
            ffmpeg = self._find_ffmpeg()
            encoder = self._start_streaming_encoder(ffmpeg, video_path)
            self._current_encoder_proc = encoder

            manifest = {
                "manifest_schema_version": 1,
                "status": "started",
                "started_utc": self._now_utc_iso(),
                "scene": {
                    "scene_name": self.cfg.scene_name,
                    "palette": self.cfg.palette,
                    "lighting_preset": self.cfg.lighting_preset,
                },
                "video": {
                    "fps": self.cfg.fps,
                    "total_frames": total_frames,
                    "total_frames_expr": self.cfg.total_frames_expr,
                },
                "formulas": {
                    "definitions": self.cfg.definitions,
                    "camera_location_rows": self.cfg.location_rows,
                    "camera_velocity_rows": self.cfg.velocity_rows,
                    "camera_orientation_rows": self.cfg.orientation_rows,
                },
                "render_settings": {
                    "width": self.cfg.width,
                    "height": self.cfg.height,
                    "spp": self.cfg.spp,
                    "max_depth": self.cfg.max_depth,
                    "adaptive": self.cfg.adaptive,
                    "adaptive_threshold": self.cfg.adaptive_threshold,
                    "denoise": self.cfg.denoise,
                    "ppm_denoised_only": self.cfg.ppm_denoised_only,
                    "atmosphere_enabled": self.cfg.atmosphere_enabled,
                    "atmosphere_density": self.cfg.atmosphere_density,
                    "atmosphere_color": self.cfg.atmosphere_color,
                    "scatter_spacing_override": self.cfg.scatter_spacing_override,
                    "scatter_max_radius_override": self.cfg.scatter_max_radius_override,
                    "scatter_growth_scale_override": self.cfg.scatter_growth_scale_override,
                    "big_scatter_spacing_override": self.cfg.big_scatter_spacing_override,
                    "big_scatter_max_radius_override": self.cfg.big_scatter_max_radius_override,
                    "row_scatter_xmax_override": self.cfg.row_scatter_xmax_override,
                    "row_scatter_z_front_override": self.cfg.row_scatter_z_front_override,
                    "row_scatter_z_back_override": self.cfg.row_scatter_z_back_override,
                    "row_scatter_replacement_rate_override": self.cfg.row_scatter_replacement_rate_override,
                    "keep_frames": self.cfg.keep_frames,
                    "options_schema_version": self.cfg.options_schema_version,
                    "scene_registry_schema_version": self.cfg.scene_registry_schema_version,
                },
                "outputs": {
                    "video_path": str(video_path),
                    "frames_dir": str(frames_dir) if frames_dir is not None else None,
                    "work_dir": str(work_dir),
                    "streaming_encode": True,
                    "manifest_path": str(manifest_path),
                },
                "raytracer_binary": {
                    "source": self._raytracer_binary_identity(self.cfg.raytracer_bin),
                    "frozen_for_job": self._raytracer_binary_identity(frozen_binary),
                },
                "ffmpeg": {"path": ffmpeg, "mode": "streaming"},
                "progress": {
                    "frames_rendered": 0,
                    "frames_expected": total_frames,
                },
            }
            self._write_manifest(manifest_path, manifest)

            for frame in range(total_frames):
                if self._is_abort():
                    self._handle_abort(
                        manifest=manifest,
                        manifest_path=manifest_path,
                        run_start=run_start,
                        frames_rendered=frame,
                        frames_streamed=frames_streamed,
                        video_path=video_path,
                        encoder=encoder,
                    )
                    encoder = None
                    return
                while self._is_pause():
                    if self._is_abort():
                        self._handle_abort(
                            manifest=manifest,
                            manifest_path=manifest_path,
                            run_start=run_start,
                            frames_rendered=frame,
                            frames_streamed=frames_streamed,
                            video_path=video_path,
                            encoder=encoder,
                        )
                        encoder = None
                        return
                    time.sleep(0.1)

                env = self._evaluate_definitions(frame, self.cfg.fps)
                loc_row = self._select_row(self.cfg.location_rows, frame, env)
                vel_row = self._select_row(self.cfg.velocity_rows, frame, env)
                ori_row = self._select_row(self.cfg.orientation_rows, frame, env)

                loc_fields = loc_row["fields"]
                vel_fields = vel_row["fields"]
                ori_fields = ori_row["fields"]

                cam_x = self._eval_optional(str(loc_fields.get("x", "")), env)
                cam_y = self._eval_optional(str(loc_fields.get("y", "")), env)
                cam_z = self._eval_optional(str(loc_fields.get("z", "")), env)

                vx = self._eval_optional(str(vel_fields.get("x", "")), env) or 0.0
                vy = self._eval_optional(str(vel_fields.get("y", "")), env) or 0.0
                vz = self._eval_optional(str(vel_fields.get("z", "")), env) or 0.0
                beta, ab_yaw, ab_pitch = velocity_vector_to_turns(vx, vy, vz)

                cam_pitch = self._eval_optional(str(ori_fields.get("pitch", "")), env) or 0.0
                cam_yaw = self._eval_optional(str(ori_fields.get("yaw", "")), env) or 0.0

                output_base = work_dir / f"frame_{frame:05d}"
                cmd = [
                    str(frozen_binary),
                    "--scene",
                    self.cfg.scene_name,
                    "--output-base",
                    str(output_base),
                    "--width",
                    str(self.cfg.width),
                    "--height",
                    str(self.cfg.height),
                    "--spp",
                    str(self.cfg.spp),
                    "--max-depth",
                    str(self.cfg.max_depth),
                    "--adaptive",
                    "1" if self.cfg.adaptive else "0",
                    "--adaptive-threshold",
                    float_text(self.cfg.adaptive_threshold),
                    "--denoise",
                    "0" if packaged_python_denoise else ("1" if self.cfg.denoise else "0"),
                    "--ppm-denoised-only",
                    "0" if packaged_python_denoise else ("1" if self.cfg.ppm_denoised_only else "0"),
                    "--atmosphere",
                    "1" if self.cfg.atmosphere_enabled else "0",
                    "--atmosphere-density",
                    float_text(self.cfg.atmosphere_density),
                    "--atmosphere-color",
                    self.cfg.atmosphere_color,
                    "--aberration-speed",
                    float_text(beta),
                    "--aberration-yaw-turns",
                    float_text(ab_yaw),
                    "--aberration-pitch-turns",
                    float_text(ab_pitch),
                    "--cam-yaw-turns",
                    float_text(cam_yaw),
                    "--cam-pitch-turns",
                    float_text(cam_pitch),
                ]
                if self.cfg.supports_lighting_preset:
                    cmd += ["--lighting-preset", self.cfg.lighting_preset]
                if self.cfg.scene_name.startswith("big_scatter"):
                    cmd += ["--big-scatter-palette", self.cfg.palette]
                if self.cfg.scatter_spacing_override is not None:
                    cmd += ["--scatter-spacing", float_text(self.cfg.scatter_spacing_override)]
                if self.cfg.scatter_max_radius_override is not None:
                    cmd += ["--scatter-max-radius", float_text(self.cfg.scatter_max_radius_override)]
                if self.cfg.scatter_growth_scale_override is not None:
                    cmd += ["--scatter-growth-scale", float_text(self.cfg.scatter_growth_scale_override)]
                if self.cfg.big_scatter_spacing_override is not None:
                    cmd += ["--big-scatter-spacing", float_text(self.cfg.big_scatter_spacing_override)]
                if self.cfg.big_scatter_max_radius_override is not None:
                    cmd += ["--big-scatter-max-radius", float_text(self.cfg.big_scatter_max_radius_override)]
                if self.cfg.row_scatter_xmax_override is not None:
                    cmd += ["--row-scatter-xmax", float_text(self.cfg.row_scatter_xmax_override)]
                if self.cfg.row_scatter_z_front_override is not None:
                    cmd += ["--row-scatter-z-front", float_text(self.cfg.row_scatter_z_front_override)]
                if self.cfg.row_scatter_z_back_override is not None:
                    cmd += ["--row-scatter-z-back", float_text(self.cfg.row_scatter_z_back_override)]
                if self.cfg.row_scatter_replacement_rate_override is not None:
                    cmd += ["--row-scatter-replacement-rate", float_text(self.cfg.row_scatter_replacement_rate_override)]
                if cam_x is not None:
                    cmd += ["--cam-x", float_text(cam_x)]
                if cam_y is not None:
                    cmd += ["--cam-y", float_text(cam_y)]
                if cam_z is not None:
                    cmd += ["--cam-z", float_text(cam_z)]

                frame_env = os.environ.copy()
                frame_env["DYLD_LIBRARY_PATH"] = str(frozen_binary_dir)
                if frozen_oidn is not None:
                    frame_env["SR_RT_OIDN_DENOISE_BIN"] = str(frozen_oidn)

                proc = subprocess.Popen(
                    cmd,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True,
                    env=frame_env,
                )
                self._current_render_proc = proc
                stdout_text, stderr_text = proc.communicate()
                self._current_render_proc = None

                if self._is_abort():
                    self._handle_abort(
                        manifest=manifest,
                        manifest_path=manifest_path,
                        run_start=run_start,
                        frames_rendered=frame,
                        frames_streamed=frames_streamed,
                        video_path=video_path,
                        encoder=encoder,
                    )
                    encoder = None
                    return

                if proc.returncode != 0:
                    err_tail = (stderr_text or stdout_text or "")[-800:]
                    raise RuntimeError(f"Frame {frame} failed.\nCommand: {' '.join(cmd)}\n{err_tail}")

                if packaged_python_denoise:
                    ok, denoise_msg, _ = run_oidn_postprocess(output_base)
                    if not ok:
                        raise RuntimeError(f"Frame {frame} denoise failed.\n{denoise_msg}")

                final_frame_path = self._final_frame_path(output_base, self.cfg.denoise)
                if not final_frame_path.exists():
                    raise RuntimeError(f"Frame {frame} completed but final frame output was not found: {final_frame_path}")

                self._stream_frame_to_encoder(encoder, final_frame_path)
                frames_streamed += 1

                if frames_dir is not None:
                    saved_png = frames_dir / f"frame_{frame:05d}.png"
                    self._save_png_copy(final_frame_path, saved_png)

                self._cleanup_frame_outputs(output_base)

                manifest.setdefault("progress", {})["frames_rendered"] = frame + 1
                if manifest_path is not None and ((frame + 1) % 25 == 0 or frame + 1 == total_frames):
                    self._write_manifest(manifest_path, manifest)

                self.progress.emit(frame + 1, total_frames, f"Rendered frame {frame + 1}/{total_frames}")

            enc_cmd = self._finalize_encoder(encoder)
            encoder = None

            manifest["status"] = "completed"
            manifest["ended_utc"] = self._now_utc_iso()
            manifest["duration_sec"] = time.time() - run_start
            manifest.setdefault("progress", {})["frames_rendered"] = total_frames
            manifest["ffmpeg"] = {"path": ffmpeg, "command": enc_cmd, "mode": "streaming"}
            try:
                manifest.setdefault("outputs", {})["video_size_bytes"] = int(video_path.stat().st_size)
            except Exception:
                pass
            if manifest_path is not None:
                self._write_manifest(manifest_path, manifest)

            self.done.emit(str(video_path))
        except Exception as exc:
            manifest["status"] = "failed"
            manifest["ended_utc"] = self._now_utc_iso()
            manifest["duration_sec"] = time.time() - run_start
            manifest["error"] = str(exc)
            if manifest_path is not None:
                try:
                    self._write_manifest(manifest_path, manifest)
                except Exception:
                    pass
            self.failed.emit(str(exc))
        finally:
            self._current_render_proc = None
            self._current_encoder_proc = None
            self._terminate_encoder(encoder)
            if temp_work_dir is not None and temp_work_dir.exists():
                shutil.rmtree(temp_work_dir, ignore_errors=True)
            if frozen_binary_dir is not None and frozen_binary_dir.exists():
                shutil.rmtree(frozen_binary_dir, ignore_errors=True)
