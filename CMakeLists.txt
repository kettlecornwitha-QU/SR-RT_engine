cmake_minimum_required(VERSION 3.16)
project(raytracer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(RAYTRACER_ENABLE_OIDN "Enable OpenImageDenoise post-process integration" ON)

add_executable(raytracer
  main.cpp
  src/app/arg_parsers.cpp
  src/app/cli.cpp
  src/app/options_schema.cpp
  src/app/run_config.cpp
  src/materials/mirror.cpp
  src/materials/ggx_metal.cpp
  src/materials/anisotropic_ggx_metal.cpp
  src/materials/conductor_metal.cpp
  src/materials/dielectric.cpp
  src/materials/clearcoat.cpp
  src/materials/coated_plastic.cpp
  src/materials/thin_transmission.cpp
  src/materials/sheen.cpp
  src/materials/isotropic.cpp
  src/materials/subsurface.cpp
  src/materials/checker_lambertian.cpp
  src/materials/checker_material.cpp
  src/materials/lambertian.cpp
  src/materials/textured_lambertian.cpp
  src/materials/emissive.cpp
  src/geometry/sphere.cpp
  src/geometry/plane.cpp
  src/geometry/rectangle.cpp
  src/geometry/triangle.cpp
  src/geometry/constant_medium.cpp
  src/accel/bvh.cpp
  src/scene/scene.cpp
  src/scene/scene_presets.cpp
  src/scene/builders/big_scatter_builder.cpp
  src/scene/builders/scatter_builder.cpp
  src/scene/builders/row_scatter_builder.cpp
  src/scene/builders/row_scatter_material_policy.cpp
  src/scene/layout/scatter_layout.cpp
  src/scene/color/scatter_color.cpp
  src/scene/presets/scene_configs.cpp
  src/scene/presets/scene_params.cpp
  src/scene/lighting_presets.cpp
  src/scene/scatter_material_factory.cpp
  src/scene/polyhedra_builder.cpp
  src/scene/scene_builder.cpp
  src/scene/model_loader.cpp
  src/scene/scene_registry.cpp
  src/render/rng.cpp
  src/render/sampling.cpp
  src/render/aberration.cpp
  src/render/atmosphere.cpp
  src/render/camera_controls.cpp
  src/render/optics.cpp
  src/render/texture.cpp
  src/render/integrator.cpp
  src/render/renderer.cpp
  src/render/output_naming.cpp
  src/render/output.cpp
  src/render/output_pipeline.cpp
  src/render/quality.cpp
)
target_include_directories(raytracer PRIVATE include)
target_compile_options(raytracer PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-O2 -Wall -Wextra -pedantic>
)

if(RAYTRACER_ENABLE_OIDN)
  find_package(OpenImageDenoise CONFIG QUIET)
  if(OpenImageDenoise_FOUND)
    message(STATUS "OpenImageDenoise found: enabling --denoise backend")
    target_compile_definitions(raytracer PRIVATE ENABLE_OIDN=1)
    if(TARGET OpenImageDenoise::OpenImageDenoise)
      target_link_libraries(raytracer PRIVATE OpenImageDenoise::OpenImageDenoise)
    elseif(TARGET OpenImageDenoise)
      target_link_libraries(raytracer PRIVATE OpenImageDenoise)
    else()
      message(WARNING "OpenImageDenoise found but no known CMake target exported; --denoise may be unavailable")
    endif()
  else()
    message(STATUS "OpenImageDenoise not found; building without OIDN")
  endif()
endif()

add_executable(aberration_tests
  tests/aberration_test.cpp
  src/render/aberration.cpp
)
target_include_directories(aberration_tests PRIVATE include)
target_compile_options(aberration_tests PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-O2 -Wall -Wextra -pedantic>
)
enable_testing()
add_test(NAME aberration_tests COMMAND aberration_tests)
