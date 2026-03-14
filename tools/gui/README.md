# Raytracer GUI (MVP)

This is a lightweight desktop GUI wrapper around the raytracer binary. In development it uses `./build/raytracer`; in packaged macOS apps it can also use a bundled renderer binary.

## Features

- Scene selection + variant selection (for scene families like `scatter` and `materials`)
- Player velocity input in two modes:
  - `speed + yaw/pitch`
  - vector components `(vx, vy, vz)` (converted internally)
- Camera view yaw/pitch controls
- Fog toggle + density slider + color input
- Render button + Cancel behavior
- Large image preview pane
- Basic render settings (resolution, spp, max depth, adaptive, denoise)

## Output behavior

- GUI renders are written to a temporary folder:
  - `/tmp/sr_rt_gui` (or your OS temp equivalent)
- This avoids cluttering your project `outputs/` folder during iteration.

## Requirements

- Python 3.10+
- `PySide6`

Install:

```bash
python3 -m pip install PySide6
```

## Run

From the project root:

```bash
python3 tools/gui/raytracer_gui.py
```

The GUI expects the renderer binary at:

`./build/raytracer`

If needed, build first:

```bash
make -C build -j
```

## Standalone macOS apps

To build self-contained `.app` bundles:

```bash
python3 -m pip install -r requirements-gui.txt
./package_standalone_apps.command
```

This creates:
- `dist_apps/Image Renderer.app`
- `dist_apps/Video Renderer.app`

The standalone builds bundle:
- the Python GUI runtime
- the raytracer binary
- OIDN runtime libraries, if found during packaging
- `ffmpeg` for the video app, if `ffmpeg` is available when packaging

In packaged apps, denoise prefers the bundled `oidnDenoise` subprocess path for better stability.
