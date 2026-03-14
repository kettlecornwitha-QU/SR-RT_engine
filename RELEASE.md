# Release Guide

This document is the maintainer checklist for producing downloadable macOS app releases for SR-RT_engine.

## Scope

Current release target:
- Apple Silicon macOS

Current release artifacts:
- `Image Renderer.app.zip`
- `Video Renderer.app.zip`
- `SHA256SUMS.txt`

## Before Releasing

Recommended checks:
- build the renderer locally
- open both packaged apps locally
- confirm the Image Renderer can render with denoise enabled
- confirm the Video Renderer can render a short test clip
- skim the README user-facing instructions

Helpful local checks:

```bash
cmake -S . -B build
cmake --build build -j
python3 -m pip install -r requirements-gui.txt
./package_standalone_apps.command
```

## Local Packaging

Use this when you want to produce local `.app` bundles without publishing a GitHub release.

```bash
python3 -m pip install -r requirements-gui.txt
./package_standalone_apps.command
```

Expected outputs:
- `dist_apps/Image Renderer.app`
- `dist_apps/Video Renderer.app`

To zip them manually:

```bash
mkdir -p release_artifacts
ditto -c -k --keepParent "dist_apps/Image Renderer.app" "release_artifacts/Image Renderer.app.zip"
ditto -c -k --keepParent "dist_apps/Video Renderer.app" "release_artifacts/Video Renderer.app.zip"
shasum -a 256 \
  "release_artifacts/Image Renderer.app.zip" \
  "release_artifacts/Video Renderer.app.zip" \
  > "release_artifacts/SHA256SUMS.txt"
```

## GitHub Actions Packaging

Automated packaging is defined in:
- `.github/workflows/macos-apps.yml`

The workflow:
- installs build dependencies
- downloads OIDN 2.4.1 for Apple Silicon
- builds `build/raytracer`
- packages the standalone apps
- zips them
- uploads artifacts

## Manual Test Run On GitHub

Use this to verify the workflow without cutting a public release.

Steps:
1. Push your branch to GitHub.
2. Open the repository Actions tab.
3. Run `Build macOS Apps` using `workflow_dispatch`.
4. Download the uploaded artifacts.
5. Open and test the apps on an Apple Silicon Mac.

## Publishing A Release

The workflow publishes release assets automatically when you push a tag matching:
- `v*`

Example:

```bash
git tag v0.1.0
git push origin v0.1.0
```

Expected release assets:
- `Image Renderer.app.zip`
- `Video Renderer.app.zip`
- `SHA256SUMS.txt`

## After Publishing

Recommended follow-up:
- download the release assets from GitHub Releases
- verify the checksums
- open both apps from the downloaded zips
- confirm the README still matches the release behavior

## Known Assumptions

- packaging is currently built around Apple Silicon runners (`macos-14`)
- packaged denoise uses the bundled `oidnDenoise` subprocess path
- video packaging expects `ffmpeg` to be available during packaging
