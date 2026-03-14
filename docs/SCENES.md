# Scenes

This project now has a mix of single-purpose scenes and scene families with multiple material variants. This guide is meant to answer two practical questions quickly:

- What is each scene for?
- Which scene should I use when I want to test a specific effect?

The current GUI default is `row_scatter`.

## Scene Families

Some entries are really families rather than one fixed scene. In the GUIs, these show up as one scene plus a variant menu.

### `scatter`

`scatter` is the medium-scale sphere testbed. It places many spheres on a ground plane around the camera and is useful for quick material comparisons with a manageable render cost.

Notable traits:

- Mixed-material default variant
- Spheres use blue shades on one side of the camera and red shades on the other
- Sphere size increases with distance from the camera
- Good for testing aberration, reflections, and broad material behavior

Available variants:

- `mixed`
- `lambertian`
- `ggx_metal`
- `anisotropic_ggx_metal`
- `thin_transmission`
- `sheen`
- `clearcoat`
- `dielectric`
- `coated_plastic`
- `subsurface`
- `conductor_copper`
- `conductor_aluminum`
- `isotropic_volume`

Best used for:

- Fast visual A/B tests
- Relativistic aberration experiments
- Seeing how one material family behaves across many sphere positions

### `big_scatter`

`big_scatter` is the larger and more varied version of `scatter`. It spreads geometry across a much larger radius and mixes multiple primitive types.

Notable traits:

- Uses spheres, cubes, and tetrahedra
- Extends outward to a much larger scene radius
- Uses more curated, pleasant color palettes
- Higher geometric variety than `scatter`

Available variants:

- `mixed`
- `lambertian`
- `ggx_metal`
- `anisotropic_ggx_metal`
- `thin_transmission`
- `sheen`
- `clearcoat`
- `dielectric`
- `coated_plastic`
- `subsurface`
- `conductor_copper`
- `conductor_aluminum`
- `isotropic_volume`

Palette support:

- `big_scatter` is the main scene family that currently exposes color-palette choices in the GUIs.

Best used for:

- Testing material behavior across different primitive shapes
- Broader scene-scale aberration tests
- Stressing scene layout and BVH traversal more than `scatter`

## Single Scenes

### `row_scatter`

`row_scatter` is the current flagship test scene and the GUI default. It lays out rows of spheres across the ground with strong structure, mixed materials by row, and replacement spheres for visual noise.

Notable traits:

- Default lighting is `cornell-ceiling-plus-key` when using `scene-default`
- Rows are distributed across a wide `z` range
- Left side spheres are blue-toned, right side spheres are red-toned
- Main row materials are currently weighted:
  - `10%` lambertian
  - `45%` clearcoat
  - `45%` coated plastic
- Around `15%` of slots are replaced by special-case materials such as dielectric, thin transmission, conductor metals, or GGX metal
- Includes an additional front row of large copper spheres
- Uses a custom checker floor rather than the simpler gray checkerboard from earlier scatter scenes

Best used for:

- High-quality still renders
- Video renders
- Comparing row-level material rhythm under a fixed lighting rig
- Seeing how a scene reads under camera motion and aberration

### `materials`

`materials` is a studio-style inspection scene for looking closely at individual BSDFs and surface models.

Notable traits:

- Uses the `materials_studio` lighting preset
- Has a more controlled camera than the scatter scenes
- Tuned for material readability rather than scene scale

Current preset defaults:

- `1280 x 720`
- `64 spp`
- `max_depth 8`
- `exposure 1.3`

Best used for:

- Close material study
- Comparing roughness/coat/transmission behavior
- Debugging whether a material itself is wrong versus the scene layout or lighting

### `spheres`

`spheres` is the classic small test scene. It is a general-purpose sanity-check scene and one of the fastest ways to confirm that the renderer is still producing reasonable images.

Best used for:

- Smoke tests
- Denoiser checks
- Quick render-speed checks
- Early debugging when a more complex scene would hide the problem

### `metalglass`

`metalglass` focuses on reflective and transmissive behavior. It is useful when the question is specifically about metal-vs-glass appearance rather than broad scene composition.

Best used for:

- Reflection and refraction checks
- Fresnel sanity checks
- Comparing conductor and dielectric response

### `cornell`

`cornell` is the controlled-box reference scene. It is useful when you want to remove most environmental complexity and look at direct/indirect lighting behavior in a familiar setup.

Notable traits:

- Uses the `cornell_ceiling` lighting preset by default
- Camera and exposure are more conservative than the broader test scenes

Best used for:

- Integrator debugging
- Light transport comparisons
- Regressions where you want stable, known composition

### `benchmark`

`benchmark` is the lightweight performance scene. It is intentionally configured for speed rather than beauty.

Current preset defaults:

- `1280 x 720`
- `8 spp`
- `max_depth 4`
- `benchmark_softbox` lighting

Best used for:

- Measuring relative performance changes
- Quick performance regressions
- CI/perf sanity checks

### `volumes`

`volumes` is the dedicated participating-media scene. It is where fog, isotropic media, and related effects are easiest to judge.

Notable traits:

- Uses the `volumes_backlit` lighting preset
- Matches the current global-style render defaults closely
- Intended to make volumetric structure and backlighting easier to see

Current preset defaults:

- `1920 x 1080`
- `128 spp`
- `max_depth 5`
- adaptive sampling on

Best used for:

- Fog and atmospheric testing
- Isotropic volume material tests
- Seeing whether denoising and AOVs hold up on participating media

### `sponza`

`sponza` is the large model-import scene for real environment testing.

Important note:

- The public repo does not bundle Sponza assets.
- If you want to use this scene, you need to provide the external scene files locally.
- That means this scene is optional rather than guaranteed to work out of the box for repo downloads.

Best used for:

- Real environment stress tests
- Model-loading validation
- Larger-scene BVH and material assignment tests

## Which Scene Should I Use?

If you are not sure where to start, this is a good rule of thumb:

- Use `row_scatter` for the best general-purpose showcase scene
- Use `materials` for close material inspection
- Use `scatter` for quick sphere-only comparisons
- Use `big_scatter` when you want more shape variety or larger spatial scale
- Use `cornell` for lighting/integrator debugging
- Use `benchmark` for speed checks
- Use `volumes` for fog and participating media
- Use `spheres` for the fastest basic smoke test

## Variants and Materials

Where a scene family exposes variants, the variant usually forces every slot in that family to use a single material class instead of the mixed setup. That makes the variants especially useful for side-by-side comparison renders.

Examples:

- `scatter_clearcoat`
- `scatter_dielectric`
- `big_scatter_conductor_copper`
- `big_scatter_subsurface`

The mixed variants remain the best choice when you want a more visually rich scene instead of a controlled material study.
