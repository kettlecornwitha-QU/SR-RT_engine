# SR-RT_engine

Single-file CPU path tracer with:
- BVH acceleration + triangles + OBJ loading
- GGX metal + dielectric + Lambertian + emissive
- Direct light sampling + MIS + Russian roulette
- Adaptive sampling
- AOV outputs (`albedo`, `normal`)
- Optional OIDN denoising (`--denoise`)

## For Users

If you just want to use the renderers on an Apple Silicon Mac, the intended path is:
- go to **GitHub Releases**
- download the packaged `.app.zip` files
- unzip and open the apps

Expected release assets:
- `Image Renderer.app.zip`
- `Video Renderer.app.zip`
- `SHA256SUMS.txt`

Notes:
- packaged apps are currently targeted at **Apple Silicon macOS**
- first launch may trigger normal macOS security prompts
- packaged app outputs are written under `~/Pictures/SR-RT Engine/outputs`

## For Developers

If you want to build, modify, or package the project yourself, use the sections below.

## Build From Source

### CMake (recommended)

```bash
cmake -S . -B build
cmake --build build -j
```

Binary:
- `build/raytracer`

### OIDN integration

`CMakeLists.txt` auto-detects OpenImageDenoise via:
- `find_package(OpenImageDenoise CONFIG QUIET)`

If found:
- links OIDN target
- defines `ENABLE_OIDN=1`
- `--denoise 1` runs real denoising

If not found:
- build still succeeds
- `--denoise 1` prints a clean "OIDN not enabled" message and skips denoise

Disable OIDN lookup explicitly:

```bash
cmake -S . -B build -DRAYTRACER_ENABLE_OIDN=OFF
```

## Run

Render:

```bash
./build/raytracer --scene spheres --output-base out/spheres
```

Denoise (if OIDN is linked):

```bash
./build/raytracer --scene spheres --denoise 1 --output-base out/spheres
```

Compare two PPM outputs:

```bash
./build/raytracer --compare references/spheres_ref.ppm out/spheres.ppm --rmse-threshold 0.035 --ssim-threshold 0.96
```

## Build Standalone macOS Apps

The repo can now build frozen `.app` bundles for the Image Renderer and Video Renderer so end users do not need Python installed separately.

Packaging dependencies:

```bash
python3 -m pip install -r requirements-gui.txt
```

Then build the standalone apps:

```bash
./package_standalone_apps.command
```

Output:
- `dist_apps/Image Renderer.app`
- `dist_apps/Video Renderer.app`

What gets bundled:
- the GUI Python runtime via PyInstaller
- `build/raytracer`
- OIDN runtime libraries, if they can be found from the renderer's linked rpaths or common install locations
- `ffmpeg` for the video app, if found on the packaging machine

Current caveat:
- packaged denoise now prefers the bundled `oidnDenoise` subprocess path for stability

## Automated Release Packaging

There is now a GitHub Actions workflow at:
- `.github/workflows/macos-apps.yml`

What it does:
- installs build/package dependencies on macOS
- downloads OIDN 2.4.1 for Apple Silicon
- builds `build/raytracer`
- packages:
  - `Image Renderer.app`
  - `Video Renderer.app`
- zips both apps and uploads them as workflow artifacts

Release flow:
- run manually with `workflow_dispatch`, or
- push a tag like `v0.1.0`

For tag builds, the workflow also publishes:
- `Image Renderer.app.zip`
- `Video Renderer.app.zip`
- `SHA256SUMS.txt`

Current assumption:
- this workflow targets Apple Silicon macOS packaging via `macos-14`

## Regression Helper

```bash
./quality_regression.sh --generate-reference
./quality_regression.sh
```
