#!/usr/bin/env python3
from __future__ import annotations

import os
import shutil
import subprocess
from pathlib import Path
from typing import List, Optional


def find_ffmpeg() -> str:
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


class StreamingVideoEncoder:
    def __init__(self, *, ffmpeg: str, fps: int, video_path: Path) -> None:
        self.ffmpeg = ffmpeg
        self.fps = fps
        self.video_path = video_path
        self._command = [
            ffmpeg,
            "-y",
            "-loglevel",
            "error",
            "-framerate",
            str(fps),
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
        self._proc = subprocess.Popen(
            self._command,
            stdin=subprocess.PIPE,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
        )

    @property
    def command(self) -> List[str]:
        return list(self._command)

    @property
    def process(self) -> Optional[subprocess.Popen[bytes]]:
        return self._proc

    def feed_ppm_file(self, frame_path: Path) -> None:
        if self._proc is None or self._proc.stdin is None:
            raise RuntimeError("ffmpeg encoder stdin is not available.")
        try:
            self._proc.stdin.write(frame_path.read_bytes())
            self._proc.stdin.flush()
        except BrokenPipeError:
            stderr = b""
            if self._proc.stderr is not None:
                try:
                    stderr = self._proc.stderr.read()
                except Exception:
                    stderr = b""
            details = stderr.decode("utf-8", errors="replace").strip()
            raise RuntimeError(f"ffmpeg encoder terminated unexpectedly.\n{details}")

    def finalize(self) -> List[str]:
        if self._proc is None:
            return self.command
        if self._proc.stdin is not None and not self._proc.stdin.closed:
            self._proc.stdin.close()
        stderr_data = b""
        if self._proc.stderr is not None:
            stderr_data = self._proc.stderr.read()
        rc = self._proc.wait()
        details = stderr_data.decode("utf-8", errors="replace").strip()
        if rc != 0:
            raise RuntimeError(f"ffmpeg encode failed:\n{details[-1200:]}")
        self._proc = None
        return self.command

    def terminate(self) -> None:
        if self._proc is None:
            return
        try:
            if self._proc.stdin is not None and not self._proc.stdin.closed:
                self._proc.stdin.close()
        except Exception:
            pass
        if self._proc.poll() is None:
            try:
                self._proc.terminate()
                self._proc.wait(timeout=5)
            except Exception:
                try:
                    self._proc.kill()
                except Exception:
                    pass
        self._proc = None
