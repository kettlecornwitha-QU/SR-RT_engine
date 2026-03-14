# Architecture

This document gives a high-level map of how the project is organized. It is not meant to describe every class in detail. The goal is to make it easier to answer questions like:

- Where should a new feature live?
- Which files are responsible for a render request?
- What is shared between the CLI and the GUIs?

## Big Picture

The project has four main layers:

1. Core renderer and scene system in C++
2. CLI/orchestration layer in C++
3. Desktop GUIs in Python
4. Packaging and release tooling for standalone macOS apps

At runtime, the GUIs are frontends that launch the same `raytracer` binary used by the CLI. The Python side handles UX, presets, video timelines, and packaged-app convenience. The C++ side handles scene building, ray tracing, image output, and most rendering features.

## Top-Level Layout

### C++ headers and source

- [include](/Users/lukewalker/dev/SR-RT_engine/include)
- [src](/Users/lukewalker/dev/SR-RT_engine/src)

These contain the renderer proper.

Major areas:

- [include/math](/Users/lukewalker/dev/SR-RT_engine/include/math) / [src](/Users/lukewalker/dev/SR-RT_engine/src)
  Core math types such as `Vec3`, ray-related support, and shared geometric helpers.
- [include/core](/Users/lukewalker/dev/SR-RT_engine/include/core)
  Shared hit/intersection structures used across primitives and the integrator.
- [include/geometry](/Users/lukewalker/dev/SR-RT_engine/include/geometry) / [src/geometry](/Users/lukewalker/dev/SR-RT_engine/src/geometry)
  Primitives such as spheres, planes, rectangles, triangles, and constant media.
- [include/materials](/Users/lukewalker/dev/SR-RT_engine/include/materials) / [src/materials](/Users/lukewalker/dev/SR-RT_engine/src/materials)
  BSDFs and emissive materials.
- [include/accel](/Users/lukewalker/dev/SR-RT_engine/include/accel) / [src/accel](/Users/lukewalker/dev/SR-RT_engine/src/accel)
  Acceleration structures, currently centered around BVH construction/traversal.
- [include/scene](/Users/lukewalker/dev/SR-RT_engine/include/scene) / [src/scene](/Users/lukewalker/dev/SR-RT_engine/src/scene)
  Scene assembly, presets, layout helpers, color policies, model loading, and scene registry metadata.
- [include/render](/Users/lukewalker/dev/SR-RT_engine/include/render) / [src/render](/Users/lukewalker/dev/SR-RT_engine/src/render)
  Camera logic, sampling, integrator, renderer, output pipeline, denoising, quality metrics, atmosphere, and aberration.
- [include/app](/Users/lukewalker/dev/SR-RT_engine/include/app) / [src/app](/Users/lukewalker/dev/SR-RT_engine/src/app)
  CLI parsing and run-configuration logic.

### CLI entrypoint

- [main.cpp](/Users/lukewalker/dev/SR-RT_engine/main.cpp)

`main.cpp` is intentionally much thinner than it used to be. It now mostly:

- parses CLI arguments
- handles special modes like image comparison and scene-registry printing
- builds a scene by name
- applies run configuration
- calls the render/output pipeline

The heavy logic has been pushed into `src/app`, `src/scene`, and `src/render`.

### Python GUIs

- [tools/gui](/Users/lukewalker/dev/SR-RT_engine/tools/gui)

This directory contains the Image Renderer and Video Renderer frontends plus supporting modules. These GUIs do not reimplement the renderer. They build option sets, call the packaged or local `raytracer` binary, and manage user interaction.

### Packaging / release tooling

- [package_standalone_apps.command](/Users/lukewalker/dev/SR-RT_engine/package_standalone_apps.command)
- [.github/workflows/macos-apps.yml](/Users/lukewalker/dev/SR-RT_engine/.github/workflows/macos-apps.yml)
- [RELEASE.md](/Users/lukewalker/dev/SR-RT_engine/RELEASE.md)

These are responsible for building standalone macOS `.app` bundles and publishing release artifacts.

## Render Flow

The basic still-image render flow is:

1. User picks options in the CLI or Image Renderer GUI.
2. Parsed options are converted into a `RenderSettings`-style configuration.
3. A scene name is resolved through the scene registry.
4. The selected scene builder populates geometry, materials, lights, and camera defaults.
5. The renderer builds the BVH.
6. The integrator traces rays and accumulates the image and AOVs.
7. The output pipeline writes image files and optionally runs denoising.

The video render flow adds one more layer:

1. The Video Renderer GUI evaluates formulas for each frame.
2. It freezes the job configuration into a per-render snapshot.
3. Each frame launches the same `raytracer` binary with frame-specific camera values.
4. Rendered frames are optionally denoised, then encoded into a video with `ffmpeg`.

That snapshot/freeze step is important: once a video render begins, later source edits or scene tweaks should not affect frames already queued for that job.

## Core Renderer Modules

### Geometry

Geometry lives primarily in:

- [src/geometry](/Users/lukewalker/dev/SR-RT_engine/src/geometry)

This layer is responsible for:

- ray/primitive intersection
- surface normal and hit-record generation
- primitive-specific bounds used by the BVH

Examples:

- spheres
- planes
- rectangles
- triangles
- constant media wrappers

If a new primitive is added, this is usually where it starts.

### Materials

Materials live in:

- [src/materials](/Users/lukewalker/dev/SR-RT_engine/src/materials)

This layer defines scattering and emission behavior. It includes baseline diffuse, transmission, conductors, coating models, subsurface, sheen, and participating media support.

Examples:

- Lambertian
- GGX metal
- anisotropic GGX metal
- conductor metals
- dielectric
- clearcoat
- coated plastic
- thin transmission
- sheen
- subsurface
- isotropic medium

If you are changing appearance rather than layout, this is usually the first place to look.

### Acceleration

Acceleration currently centers on BVH support:

- [src/accel/bvh.cpp](/Users/lukewalker/dev/SR-RT_engine/src/accel/bvh.cpp)

This layer reduces intersection cost by organizing primitives hierarchically. Scene builders fill the world with primitives first, and the BVH is built once before rendering.

### Rendering

Rendering logic is mostly in:

- [src/render/integrator.cpp](/Users/lukewalker/dev/SR-RT_engine/src/render/integrator.cpp)
- [src/render/renderer.cpp](/Users/lukewalker/dev/SR-RT_engine/src/render/renderer.cpp)
- [src/render/sampling.cpp](/Users/lukewalker/dev/SR-RT_engine/src/render/sampling.cpp)
- [src/render/output_pipeline.cpp](/Users/lukewalker/dev/SR-RT_engine/src/render/output_pipeline.cpp)
- [src/render/output.cpp](/Users/lukewalker/dev/SR-RT_engine/src/render/output.cpp)

Responsibilities are roughly split like this:

- `integrator.cpp`
  Light transport logic and recursive path contribution
- `renderer.cpp`
  Multithreaded image rendering, tile scheduling, progress reporting, and adaptive sampling coordination
- `sampling.cpp`
  Sampling helpers and BSDF/light sampling support
- `camera_controls.cpp`, `optics.cpp`, `aberration.cpp`, `atmosphere.cpp`
  Camera orientation, thin-lens effects, relativistic aberration, and atmospheric effects
- `output_pipeline.cpp` / `output.cpp`
  File writing, AOV output, denoising, and summary/reporting support

### Quality and Testing

Supporting quality logic lives in:

- [src/render/quality.cpp](/Users/lukewalker/dev/SR-RT_engine/src/render/quality.cpp)
- [tests/aberration_test.cpp](/Users/lukewalker/dev/SR-RT_engine/tests/aberration_test.cpp)

This covers image comparison metrics such as RMSE/SSIM and targeted unit-style coverage for aberration mapping.

## Scene System

The scene system is one of the most important architectural pieces because it connects geometry, materials, lighting, defaults, and GUI metadata.

### Registry and dispatch

- [src/scene/scene_registry.cpp](/Users/lukewalker/dev/SR-RT_engine/src/scene/scene_registry.cpp)
- [src/scene/scene_builder.cpp](/Users/lukewalker/dev/SR-RT_engine/src/scene/scene_builder.cpp)

The registry defines:

- canonical scene names
- scene-family grouping
- variant labels
- default variant ordering

This metadata is used both by the CLI and the GUIs.

### Scene presets and defaults

- [src/scene/presets/scene_configs.cpp](/Users/lukewalker/dev/SR-RT_engine/src/scene/presets/scene_configs.cpp)
- [src/scene/presets/scene_params.cpp](/Users/lukewalker/dev/SR-RT_engine/src/scene/presets/scene_params.cpp)

These files separate two related ideas:

- `ScenePresetConfig`
  Camera and render-setting defaults for a scene
- parameter structs like `RowScatterParams`, `ScatterParams`, `BigScatterParams`
  Geometry/layout knobs for specific scene builders

This split keeps render defaults separate from object-placement logic.

### Scene builders

- [src/scene/builders](/Users/lukewalker/dev/SR-RT_engine/src/scene/builders)

Builders assemble concrete scenes. They are where geometry, materials, and scene-specific policies come together.

Examples:

- `scatter_builder.cpp`
- `big_scatter_builder.cpp`
- `row_scatter_builder.cpp`
- `row_scatter_material_policy.cpp`

Related helpers live in:

- [src/scene/layout](/Users/lukewalker/dev/SR-RT_engine/src/scene/layout)
- [src/scene/color](/Users/lukewalker/dev/SR-RT_engine/src/scene/color)
- [src/scene/random](/Users/lukewalker/dev/SR-RT_engine/include/scene/random)

This is where placement math, color policy, and seeded randomness were intentionally extracted to keep builders readable.

### Lighting presets

- [src/scene/lighting_presets.cpp](/Users/lukewalker/dev/SR-RT_engine/src/scene/lighting_presets.cpp)

Lighting rigs are centralized here so scenes can reuse them by name. This makes it much easier to compare materials under the same light setup without duplicating light placement code.

### Model loading

- [src/scene/model_loader.cpp](/Users/lukewalker/dev/SR-RT_engine/src/scene/model_loader.cpp)

This handles imported scene/model data such as Sponza-style content when external assets are available.

## CLI Layer

The CLI support is split into focused modules:

- [src/app/cli.cpp](/Users/lukewalker/dev/SR-RT_engine/src/app/cli.cpp)
- [src/app/arg_parsers.cpp](/Users/lukewalker/dev/SR-RT_engine/src/app/arg_parsers.cpp)
- [src/app/options_schema.cpp](/Users/lukewalker/dev/SR-RT_engine/src/app/options_schema.cpp)
- [src/app/run_config.cpp](/Users/lukewalker/dev/SR-RT_engine/src/app/run_config.cpp)

This layer is responsible for:

- parsing flags
- centralizing enum/string parsing
- exporting machine-readable option schema for the GUIs
- applying CLI overrides onto scene defaults

One useful architectural pattern in this project is that the GUIs query the binary for its option schema and scene registry instead of hardcoding every rule independently.

## GUI Layer

The Python GUIs are intentionally thin frontends over the C++ renderer.

### Shared support modules

Notable shared modules in [tools/gui](/Users/lukewalker/dev/SR-RT_engine/tools/gui):

- [gui_runtime.py](/Users/lukewalker/dev/SR-RT_engine/tools/gui/gui_runtime.py)
  Runtime path resolution for repo mode vs packaged-app mode
- [gui_constants.py](/Users/lukewalker/dev/SR-RT_engine/tools/gui/gui_constants.py)
  Shared UI constants and layout values
- [scene_controls.py](/Users/lukewalker/dev/SR-RT_engine/tools/gui/scene_controls.py)
  Shared scene/variant/palette UI wiring
- [settings_summary.py](/Users/lukewalker/dev/SR-RT_engine/tools/gui/settings_summary.py)
  Shared settings-summary formatting
- [render_settings_dialog.py](/Users/lukewalker/dev/SR-RT_engine/tools/gui/render_settings_dialog.py)
  Shared settings modal used by both apps
- [options_schema.py](/Users/lukewalker/dev/SR-RT_engine/tools/gui/options_schema.py)
  Python-side loading of the renderer’s exported options schema
- [scene_registry.py](/Users/lukewalker/dev/SR-RT_engine/tools/gui/scene_registry.py)
  Python-side loading/fallback handling for scene metadata

### Image Renderer

Main files:

- [raytracer_gui.py](/Users/lukewalker/dev/SR-RT_engine/tools/gui/raytracer_gui.py)
- [image_config_builder.py](/Users/lukewalker/dev/SR-RT_engine/tools/gui/image_config_builder.py)
- [image_app_entry.py](/Users/lukewalker/dev/SR-RT_engine/tools/gui/image_app_entry.py)
- [oidn_postprocess.py](/Users/lukewalker/dev/SR-RT_engine/tools/gui/oidn_postprocess.py)

The image GUI is responsible for:

- building a render command/config from form state
- launching a single render process
- loading the resulting image into the preview pane
- running packaged-app denoise postprocess support when needed

### Video Renderer

Main files:

- [video_window.py](/Users/lukewalker/dev/SR-RT_engine/tools/gui/video_window.py)
- [video_config_builder.py](/Users/lukewalker/dev/SR-RT_engine/tools/gui/video_config_builder.py)
- [video_worker.py](/Users/lukewalker/dev/SR-RT_engine/tools/gui/video_worker.py)
- [video_formula.py](/Users/lukewalker/dev/SR-RT_engine/tools/gui/video_formula.py)
- [video_widgets.py](/Users/lukewalker/dev/SR-RT_engine/tools/gui/video_widgets.py)
- [video_preset_manager.py](/Users/lukewalker/dev/SR-RT_engine/tools/gui/video_preset_manager.py)
- [video_app_entry.py](/Users/lukewalker/dev/SR-RT_engine/tools/gui/video_app_entry.py)

The video GUI adds logic the image GUI does not need:

- formula evaluation for time-varying parameters
- multi-section camera timelines
- preset save/load support
- render-job freezing and snapshotting
- per-frame rendering and final video encoding

The compatibility launcher:

- [raytracer_animation_gui.py](/Users/lukewalker/dev/SR-RT_engine/tools/gui/raytracer_animation_gui.py)

exists mainly so older entrypoints still work after the video GUI was split into smaller modules.

## Shared Schema Pattern

One of the more important architectural decisions in the project is that defaults and legal choices are not only embedded in the Python GUIs.

Instead:

- the C++ binary exports an options schema
- the C++ binary exports scene registry metadata
- the GUIs consume that data and layer UI behavior on top

Benefits:

- fewer drift bugs between CLI and GUI behavior
- easier packaging, because the GUI can ask the current renderer binary what it supports
- easier future refactors, because renderer-side defaults stay authoritative

## Packaging Architecture

Standalone app packaging is built around the idea that the Python GUIs should work both:

- directly from the source tree
- from a packaged `.app` bundle

Key pieces:

- [package_standalone_apps.command](/Users/lukewalker/dev/SR-RT_engine/package_standalone_apps.command)
  Builds standalone app bundles and embeds runtime assets
- [image_app_entry.py](/Users/lukewalker/dev/SR-RT_engine/tools/gui/image_app_entry.py)
- [video_app_entry.py](/Users/lukewalker/dev/SR-RT_engine/tools/gui/video_app_entry.py)
  Set packaged-runtime environment variables so the GUI can find bundled binaries and helpers
- [gui_runtime.py](/Users/lukewalker/dev/SR-RT_engine/tools/gui/gui_runtime.py)
  Resolves local-vs-bundled paths
- [.github/workflows/macos-apps.yml](/Users/lukewalker/dev/SR-RT_engine/.github/workflows/macos-apps.yml)
  Automates packaging and release artifact publishing in CI

The packaged apps bundle:

- the Python GUI runtime
- the `raytracer` binary
- `ffmpeg` when available for video encoding
- OIDN support needed for denoise workflows

## Where To Put New Work

As a rule of thumb:

- New primitive: `include/geometry` + `src/geometry`
- New material: `include/materials` + `src/materials`
- New camera or integrator behavior: `src/render`
- New scene family or scene layout logic: `src/scene/builders`, `src/scene/layout`, `src/scene/color`, `src/scene/presets`
- New CLI flag or override behavior: `src/app`
- New GUI control that affects both apps: shared module in `tools/gui`
- New packaging/distribution change: packaging scripts, app entrypoints, or GitHub Actions workflow

## Recommended Reading Order

If you are trying to understand the codebase for the first time, this is a good order:

1. [README.md](/Users/lukewalker/dev/SR-RT_engine/README.md)
2. [docs/SCENES.md](/Users/lukewalker/dev/SR-RT_engine/docs/SCENES.md)
3. [main.cpp](/Users/lukewalker/dev/SR-RT_engine/main.cpp)
4. [src/app](/Users/lukewalker/dev/SR-RT_engine/src/app)
5. [src/scene/scene_builder.cpp](/Users/lukewalker/dev/SR-RT_engine/src/scene/scene_builder.cpp) and [src/scene/scene_registry.cpp](/Users/lukewalker/dev/SR-RT_engine/src/scene/scene_registry.cpp)
6. One representative builder such as [row_scatter_builder.cpp](/Users/lukewalker/dev/SR-RT_engine/src/scene/builders/row_scatter_builder.cpp)
7. [src/render/renderer.cpp](/Users/lukewalker/dev/SR-RT_engine/src/render/renderer.cpp) and [src/render/integrator.cpp](/Users/lukewalker/dev/SR-RT_engine/src/render/integrator.cpp)
8. [tools/gui/README.md](/Users/lukewalker/dev/SR-RT_engine/tools/gui/README.md), [docs/GUI.md](/Users/lukewalker/dev/SR-RT_engine/docs/GUI.md), and the relevant GUI entrypoints

That path gives a good balance between top-down understanding and concrete implementation details.
