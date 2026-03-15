# GUI Guide

This document explains the two desktop GUIs included with SR-RT_engine:

- `Image Renderer`
- `Video Renderer`

Both apps are available as:
- source-driven Python GUIs during development
- packaged macOS `.app` releases for end users

For local packaged-app testing, the canonical app bundles live in:

- `dist_apps/Image Renderer.app`
- `dist_apps/Video Renderer.app`

The root-level `.app` bundles in the repo are no longer the preferred set to maintain.

## Image Renderer

The Image Renderer is the simpler of the two GUIs. It is meant for interactive still-image testing.

### Main controls

Left panel:
- `Scene`: choose the base scene
- `Variant`: choose a scene variant when the selected scene has multiple variants
- `Big Scatter Palette`: only relevant for `big_scatter`
- `Player Velocity`: either:
  - `Speed + Yaw + Pitch`
  - `Vector (vx, vy, vz)`
- `Camera View`:
  - optional camera position override: `x`, `y`, `z`
  - camera orientation: `yaw turns`, `pitch turns`
- `Settings...`: opens the render settings dialog
- `Render`: starts a still-image render

Right panel:
- rendered image preview
- if the rendered image is larger than the preview area, it is scaled to fit the pane while preserving aspect ratio

### Settings dialog

The shared render settings dialog currently includes:
- image width / height
- samples per pixel (`SPP`)
- max depth
- adaptive sampling toggle + threshold
- lighting preset
- denoise toggle
- `Only write denoised PPM`
- atmosphere / fog toggle, density, and color
- scene layout override controls for:
  - scatter spacing / radius / growth
  - big scatter spacing / max radius
  - row scatter layout parameters

### Output behavior

Image GUI renders are written to a temporary GUI output directory during rendering.

Development runtime:
- OS temp directory under a folder like `sr_rt_gui`

Packaged app runtime:
- the app still renders to temporary files first
- packaged outputs intended for the user are written under:
  - `~/Pictures/SR-RT Engine/outputs`

The GUI displays:
- the denoised image if a denoised output exists
- otherwise the base `.ppm`

### Denoising in packaged apps

In packaged builds, denoising is performed as a GUI-side post-process:
- render base beauty/albedo/normal outputs
- convert temporary PPM inputs to PFM
- run bundled `oidnDenoise`
- convert the denoised result back to `_denoised.ppm`

This is more reliable than asking the bundled renderer binary to denoise internally.

## Video Renderer

The Video Renderer is designed for formula-driven animation rendering.

### Left panel

Scene controls:
- `Scene`
- `Variant`
- `Color Palette`

Video controls:
- `FPS`
- `Total number of frames`

Status/info:
- current preset name
- settings summary
- render status text

Actions:
- `Settings...`
- `Render`
- `Resume` when a paused render is resumable

### Main scrollable panel

The right side contains four major sections:

1. `Definitions`
- add named formulas like:
  - `halfway = arcosh(distance/2 + 1)`
- available built-in variables include:
  - `frame`
  - `fps`
  - `time`

2. `Camera Location`
- timed rows with `x`, `y`, `z`

3. `Camera Velocity (scene frame)`
- timed rows with `x`, `y`, `z`

4. `Camera Orientation (turns)`
- timed rows with `pitch`, `yaw`

Each timed section supports multiple ranges so different formulas can apply across different frame intervals.
Of note, `Camera Velocity` is completely decoupled from `Camera Location`, so the user is free to specify formulas that are completely unphysical. For example, I can enter a `Camera Location` formula to make the camera slowly move up in the `y` direction along with a `Camera Velocity` formula to make the renderer think the camera is barreling forward in the `-z` direction at 99% the speed of light.

### Presets

The Video Renderer supports:
- `Save Preset`
- `Save Preset As...`
- `Load Preset`

Preset files now live in:
- `presets/video/`

These presets are intended to be project-owned examples, not temporary render output.

### Rendering behavior

When a video render starts:
- the renderer/runtime binary is snapshotted into a temporary runtime bundle
- bundled dynamic libraries are copied alongside it
- this isolates the running job from later code changes

That means:
- editing scene code after pressing `Render` does not affect the already-running video job

### Overlay controls

During active rendering the Video Renderer shows an overlay with:
- progress bar
- current status text
- `Abort`
- `Pause`

If paused:
- the overlay is dismissed
- the main UI becomes visible again for inspection
- `Resume` becomes available

If aborted:
- rendering stops
- the app returns to its normal pre-render state instead of closing

### Output behavior

Packaged app outputs go to:
- `~/Pictures/SR-RT Engine/outputs`

Video rendering now uses a streamed encode pipeline:
- one long-running `ffmpeg` process is started near the beginning of the job
- each frame is rendered into a temporary working area
- the frame is optionally denoised
- the final per-frame image is streamed immediately into `ffmpeg`
- the temporary working files for that frame are then cleaned up

Depending on settings:
- the final video is written there
- if `Keep frames` is on, final per-frame images are also saved there as `.png`
- if `Keep frames` is off, the render normally leaves only the final video and manifest behind

Video renders also write a manifest JSON describing the job:
- scene
- formulas
- settings
- outputs
- binary identity
- progress metadata

If aborted:
- the app still returns to its normal pre-render state
- if enough frames have already been encoded, the partial `.mp4` is preserved in `outputs/` when possible
- the manifest records that the job was aborted and whether a partial video was kept

## Shared behavior

### Scene / variant model

Both GUIs use the shared scene registry:
- scenes appear in the scene dropdown
- variants appear in the variant dropdown
- this avoids the older problem where variants showed up as top-level scenes

### Lighting presets

Both GUIs expose lighting preset selection through the shared settings dialog.

### Outputs and releases

For end users, the intended way to use the GUIs is through packaged releases:
- GitHub Releases:
  - `Image Renderer.app.zip`
  - `Video Renderer.app.zip`

### Current platform target

The packaged GUI distribution path currently targets:
- Apple Silicon macOS

## Development entry points

Image GUI:

```bash
python3 tools/gui/raytracer_gui.py
```

Video GUI:

```bash
python3 tools/gui/raytracer_animation_gui.py
```

Standalone app packaging:

```bash
python3 -m pip install -r requirements-gui.txt
./package_standalone_apps.command
```
