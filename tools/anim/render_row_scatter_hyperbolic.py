#!/usr/bin/env python3
import argparse
import math
import shutil
import subprocess
from pathlib import Path


def frame_z(frame_number: int, fps: float, start: float, distance: float, halfway: float) -> float:
    tau = fps
    mid = int(tau * halfway)
    if frame_number <= mid:
        return start - math.cosh(frame_number / tau) - 1.0
    return math.cosh(frame_number / tau - 2.0 * halfway) - 1.0 + start - distance


def frame_speed(frame_number: int, fps: float, halfway: float) -> float:
    mid = int(fps * halfway)
    if frame_number <= mid:
        return math.tanh(frame_number / fps)
    return -math.tanh(frame_number / fps - 2.0 * halfway)


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="Render row_scatter hyperbolic camera path and encode MP4."
    )
    p.add_argument("--binary", default="build/raytracer", help="Path to raytracer binary")
    p.add_argument(
        "--output-dir",
        default="outputs/row_scatter_hyperbolic",
        help="Directory for frames and video",
    )
    p.add_argument("--width", type=int, default=1920)
    p.add_argument("--height", type=int, default=1080)
    p.add_argument("--spp", type=int, default=128)
    p.add_argument("--max-depth", type=int, default=5)
    p.add_argument("--fps", type=int, default=60)
    p.add_argument("--adaptive", type=int, choices=[0, 1], default=1)
    p.add_argument("--denoise", type=int, choices=[0, 1], default=1)
    p.add_argument("--threads", type=int, default=0, help="0 keeps renderer default")
    p.add_argument("--start", type=float, default=76.0)
    p.add_argument("--end", type=float, default=-150.0)
    p.add_argument("--camera-y", type=float, default=1.2)
    p.add_argument("--frame-limit", type=int, default=-1, help="Render only first N frames")
    p.add_argument("--ffmpeg", default="", help="Explicit ffmpeg binary path")
    p.add_argument("--keep-frames", action="store_true", help="Keep rendered frame files after encoding")
    p.add_argument("--dry-run", action="store_true")
    p.add_argument("--skip-video", action="store_true")
    return p


def main() -> int:
    args = build_parser().parse_args()
    binary = Path(args.binary)
    if not binary.exists():
        print(f"Raytracer binary not found: {binary}")
        return 1

    out_dir = Path(args.output_dir)
    frames_dir = out_dir / "frames"
    frames_dir.mkdir(parents=True, exist_ok=True)

    start = args.start
    end = args.end
    distance = abs(end - start)
    halfway = math.acosh(distance / 2.0 + 1.0)
    total_last_frame = int(2.0 * args.fps * halfway)
    if args.frame_limit > 0:
        total_last_frame = min(total_last_frame, args.frame_limit - 1)

    print(
        f"start={start}, end={end}, distance={distance}, halfway={halfway:.6f}, "
        f"frames=0..{total_last_frame} ({total_last_frame + 1} total)"
    )

    for frame in range(total_last_frame + 1):
        z = frame_z(frame, float(args.fps), start, distance, halfway)
        speed = frame_speed(frame, float(args.fps), halfway)
        yaw_turns = 0.0
        pitch_turns = 0.0
        camera_yaw_turns = (frame / float(args.fps) / halfway) % 1.0

        output_base = frames_dir / f"frame_{frame:05d}"
        cmd = [
            str(binary),
            "--scene",
            "row_scatter",
            "--output-base",
            str(output_base),
            "--width",
            str(args.width),
            "--height",
            str(args.height),
            "--spp",
            str(args.spp),
            "--max-depth",
            str(args.max_depth),
            "--adaptive",
            str(args.adaptive),
            "--denoise",
            str(args.denoise),
            "--ppm-denoised-only",
            "1",
            "--cam-x",
            "0.0",
            "--cam-y",
            str(args.camera_y),
            "--cam-z",
            f"{z:.12g}",
            "--cam-yaw-turns",
            f"{camera_yaw_turns:.12g}",
            "--cam-pitch-turns",
            "0.0",
            "--aberration-speed",
            f"{speed:.12g}",
            "--aberration-yaw-turns",
            f"{yaw_turns:.12g}",
            "--aberration-pitch-turns",
            f"{pitch_turns:.12g}",
        ]
        if args.threads > 0:
            cmd += ["--threads", str(args.threads)]

        print(
            f"[{frame:05d}/{total_last_frame:05d}] z={z:.6f} beta={speed:.6f} "
            f"-> {output_base.name}"
        )
        if args.dry_run:
            print("  " + " ".join(cmd))
            continue

        proc = subprocess.run(cmd, capture_output=True, text=True)
        if proc.returncode != 0:
            print("Render failed:")
            print(proc.stdout)
            print(proc.stderr)
            return proc.returncode

    if args.skip_video:
        print("Skipping video encode (--skip-video).")
        return 0

    ffmpeg = None
    if args.ffmpeg:
        candidate = Path(args.ffmpeg)
        if candidate.exists():
            ffmpeg = str(candidate)
    if not ffmpeg:
        ffmpeg = shutil.which("ffmpeg")
    if not ffmpeg:
        for candidate in ["/opt/homebrew/bin/ffmpeg", "/usr/local/bin/ffmpeg"]:
            if Path(candidate).exists():
                ffmpeg = candidate
                break
    if not ffmpeg:
        print("ffmpeg not found on PATH (or common Homebrew locations). Frames are ready under:", frames_dir)
        return 0

    image_suffix = "_denoised.ppm" if args.denoise == 1 else ".ppm"
    pattern = str(frames_dir / f"frame_%05d{image_suffix}")
    video_path = out_dir / "row_scatter_hyperbolic.mp4"
    ffmpeg_cmd = [
        ffmpeg,
        "-y",
        "-framerate",
        str(args.fps),
        "-i",
        pattern,
        "-c:v",
        "libx264",
        "-pix_fmt",
        "yuv420p",
        str(video_path),
    ]
    print("Encoding video:", video_path)
    enc = subprocess.run(ffmpeg_cmd, capture_output=True, text=True)
    if enc.returncode != 0:
        print("ffmpeg encode failed:")
        print(enc.stdout)
        print(enc.stderr)
        return enc.returncode

    if not args.keep_frames:
        image_suffix = "_denoised.ppm" if args.denoise == 1 else ".ppm"
        removed = 0
        for frame in range(total_last_frame + 1):
            for suffix in [
                image_suffix,
                "_albedo.ppm",
                "_normal.ppm",
                ".exr",
            ]:
                path = frames_dir / f"frame_{frame:05d}{suffix}"
                if path.exists():
                    path.unlink()
                    removed += 1
        print(f"Removed temporary frame outputs ({removed} files). Use --keep-frames to keep them.")

    print("Done:", video_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
