#!/bin/zsh
set -euo pipefail

cd "$(dirname "$0")"
project_dir="$PWD"
dist_dir="$project_dir/dist_apps"
build_dir="$project_dir/build/pyinstaller"

find_python() {
  local python_bin=""
  for cand in \
    "$(command -v python3 2>/dev/null || true)" \
    "$HOME/.rye/shims/python3" \
    "/opt/homebrew/bin/python3" \
    "/usr/local/bin/python3" \
    "/usr/bin/python3"; do
    if [[ -x "$cand" ]]; then
      python_bin="$cand"
      break
    fi
  done
  if [[ -z "$python_bin" ]]; then
    echo "No python3 interpreter found."
    exit 127
  fi
  echo "$python_bin"
}

find_ffmpeg() {
  local ffmpeg_bin=""
  for cand in \
    "$(command -v ffmpeg 2>/dev/null || true)" \
    "/opt/homebrew/bin/ffmpeg" \
    "/usr/local/bin/ffmpeg"; do
    if [[ -n "$cand" && -x "$cand" ]]; then
      ffmpeg_bin="$cand"
      break
    fi
  done
  echo "$ffmpeg_bin"
}

find_oidn_lib_dir() {
  local rpaths
  rpaths="$(otool -l "$project_dir/build/raytracer" 2>/dev/null | awk '
    $1 == "cmd" && $2 == "LC_RPATH" { in_rpath=1; next }
    in_rpath && $1 == "path" { print $2; in_rpath=0 }
  ')"
  while IFS= read -r path; do
    if [[ -n "$path" && -d "$path" && -f "$path/libOpenImageDenoise.2.dylib" ]]; then
      echo "$path"
      return
    fi
  done <<< "$rpaths"

  for cand in \
    "$HOME/deps/oidn-2.4.1.arm64.macos/lib" \
    "/opt/homebrew/lib" \
    "/usr/local/lib"; do
    if [[ -d "$cand" && -f "$cand/libOpenImageDenoise.2.dylib" ]]; then
      echo "$cand"
      return
    fi
  done
}

find_oidn_bin_dir() {
  local lib_dir
  lib_dir="$(find_oidn_lib_dir || true)"
  if [[ -n "$lib_dir" ]]; then
    local parent
    parent="$(cd "$lib_dir/.." && pwd)"
    if [[ -x "$parent/bin/oidnDenoise" ]]; then
      echo "$parent/bin"
      return
    fi
  fi

  for cand in \
    "$HOME/deps/oidn-2.4.1.arm64.macos/bin" \
    "/opt/homebrew/bin" \
    "/usr/local/bin"; do
    if [[ -x "$cand/oidnDenoise" ]]; then
      echo "$cand"
      return
    fi
  done
}

add_binary_if_exists() {
  local array_name="$1"
  local source_path="$2"
  local dest_dir="$3"
  if [[ -f "$source_path" ]]; then
    eval "$array_name+=(--add-binary \"$source_path:$dest_dir\")"
  fi
}

patch_bundled_raytracer_rpaths() {
  local app_dir="$1"
  local bin_dir
  bin_dir="$(find "$app_dir" -path '*/bin' -type d | head -n 1)"
  if [[ -z "$bin_dir" ]]; then
    return
  fi

  while IFS= read -r exe; do
    [[ -n "$exe" ]] || continue
    install_name_tool -add_rpath "@loader_path" "$exe" >/dev/null 2>&1 || true
  done < <(find "$bin_dir" -maxdepth 1 \( -name 'raytracer' -o -name 'oidnDenoise' \) -type f | sort)

  while IFS= read -r dylib; do
    [[ -n "$dylib" ]] || continue
    install_name_tool -add_rpath "@loader_path" "$dylib" >/dev/null 2>&1 || true
  done < <(find "$bin_dir" -maxdepth 1 \( -name 'libOpenImageDenoise*.dylib' -o -name 'libtbb*.dylib' \) -type f | sort)
}

ensure_oidn_symlinks() {
  local app_dir="$1"
  local bin_dir
  bin_dir="$(find "$app_dir" -path '*/bin' -type d | head -n 1)"
  if [[ -z "$bin_dir" ]]; then
    return
  fi

  (
    cd "$bin_dir"
    [[ -e libOpenImageDenoise.2.dylib ]] || ln -sf libOpenImageDenoise.2.4.1.dylib libOpenImageDenoise.2.dylib
    [[ -e libOpenImageDenoise.dylib ]] || ln -sf libOpenImageDenoise.2.dylib libOpenImageDenoise.dylib
    [[ -e libtbb.12.dylib ]] || ln -sf libtbb.12.17.dylib libtbb.12.dylib
    [[ -e libtbb.dylib ]] || ln -sf libtbb.12.dylib libtbb.dylib
  )
}

sign_app_bundle() {
  local app_dir="$1"
  codesign --force --deep --sign - "$app_dir"
}

python_bin="$(find_python)"
oidn_lib_dir="$(find_oidn_lib_dir || true)"
oidn_bin_dir="$(find_oidn_bin_dir || true)"

if [[ ! -x "$project_dir/build/raytracer" ]]; then
  echo "Missing build/raytracer. Build the renderer first."
  echo "Try: cmake --build build -j"
  exit 1
fi

if ! "$python_bin" -c "import PyInstaller" >/dev/null 2>&1; then
  echo "PyInstaller is not installed for $python_bin"
  echo "Install GUI packaging deps with:"
  echo "  $python_bin -m pip install -r requirements-gui.txt"
  exit 1
fi

if ! "$python_bin" -c "import PySide6" >/dev/null 2>&1; then
  echo "PySide6 is not installed for $python_bin"
  echo "Install GUI packaging deps with:"
  echo "  $python_bin -m pip install -r requirements-gui.txt"
  exit 1
fi

mkdir -p "$dist_dir" "$build_dir"
rm -rf \
  "$dist_dir/.DS_Store" \
  "$dist_dir/Image Renderer.app" \
  "$dist_dir/Video Renderer.app" \
  "$dist_dir/Image Renderer" \
  "$dist_dir/Video Renderer"

image_icon="$project_dir/Image Renderer.app/Contents/Resources/image_renderer.icns"
video_icon="$project_dir/Video Renderer.app/Contents/Resources/video_renderer.icns"
ffmpeg_bin="$(find_ffmpeg)"

image_args=(
  -m PyInstaller
  --noconfirm
  --clean
  --windowed
  --onedir
  --name "Image Renderer"
  --distpath "$dist_dir"
  --workpath "$build_dir"
  --specpath "$build_dir"
  --paths "$project_dir/tools/gui"
  --add-binary "$project_dir/build/raytracer:bin"
)

if [[ -n "$oidn_lib_dir" ]]; then
  add_binary_if_exists image_args "$oidn_lib_dir/libOpenImageDenoise.2.4.1.dylib" "bin"
  add_binary_if_exists image_args "$oidn_lib_dir/libOpenImageDenoise.2.dylib" "bin"
  add_binary_if_exists image_args "$oidn_lib_dir/libOpenImageDenoise.dylib" "bin"
  add_binary_if_exists image_args "$oidn_lib_dir/libOpenImageDenoise_core.2.4.1.dylib" "bin"
  add_binary_if_exists image_args "$oidn_lib_dir/libOpenImageDenoise_device_cpu.2.4.1.dylib" "bin"
  add_binary_if_exists image_args "$oidn_lib_dir/libOpenImageDenoise_device_metal.2.4.1.dylib" "bin"
  add_binary_if_exists image_args "$oidn_lib_dir/libtbb.12.17.dylib" "bin"
  add_binary_if_exists image_args "$oidn_lib_dir/libtbb.12.dylib" "bin"
  add_binary_if_exists image_args "$oidn_lib_dir/libtbb.dylib" "bin"
fi
if [[ -n "$oidn_bin_dir" ]]; then
  add_binary_if_exists image_args "$oidn_bin_dir/oidnDenoise" "bin"
fi

if [[ -f "$image_icon" ]]; then
  image_args+=(--icon "$image_icon")
fi

image_args+=("$project_dir/tools/gui/image_app_entry.py")

video_args=(
  -m PyInstaller
  --noconfirm
  --clean
  --windowed
  --onedir
  --name "Video Renderer"
  --distpath "$dist_dir"
  --workpath "$build_dir"
  --specpath "$build_dir"
  --paths "$project_dir/tools/gui"
  --add-binary "$project_dir/build/raytracer:bin"
)

if [[ -n "$oidn_lib_dir" ]]; then
  add_binary_if_exists video_args "$oidn_lib_dir/libOpenImageDenoise.2.4.1.dylib" "bin"
  add_binary_if_exists video_args "$oidn_lib_dir/libOpenImageDenoise.2.dylib" "bin"
  add_binary_if_exists video_args "$oidn_lib_dir/libOpenImageDenoise.dylib" "bin"
  add_binary_if_exists video_args "$oidn_lib_dir/libOpenImageDenoise_core.2.4.1.dylib" "bin"
  add_binary_if_exists video_args "$oidn_lib_dir/libOpenImageDenoise_device_cpu.2.4.1.dylib" "bin"
  add_binary_if_exists video_args "$oidn_lib_dir/libOpenImageDenoise_device_metal.2.4.1.dylib" "bin"
  add_binary_if_exists video_args "$oidn_lib_dir/libtbb.12.17.dylib" "bin"
  add_binary_if_exists video_args "$oidn_lib_dir/libtbb.12.dylib" "bin"
  add_binary_if_exists video_args "$oidn_lib_dir/libtbb.dylib" "bin"
else
  echo "Warning: OIDN runtime libraries not found. Denoising may not work on other Macs."
fi
if [[ -n "$oidn_bin_dir" ]]; then
  add_binary_if_exists video_args "$oidn_bin_dir/oidnDenoise" "bin"
else
  echo "Warning: oidnDenoise executable not found. Packaged denoise fallback will be unavailable."
fi

if [[ -n "$ffmpeg_bin" ]]; then
  video_args+=(--add-binary "$ffmpeg_bin:bin")
else
  echo "Warning: ffmpeg not found. Video Renderer.app will still require ffmpeg on the user's machine."
fi

if [[ -f "$video_icon" ]]; then
  video_args+=(--icon "$video_icon")
fi

video_args+=("$project_dir/tools/gui/video_app_entry.py")

"$python_bin" "${image_args[@]}"
"$python_bin" "${video_args[@]}"

ensure_oidn_symlinks "$dist_dir/Image Renderer.app"
ensure_oidn_symlinks "$dist_dir/Video Renderer.app"
patch_bundled_raytracer_rpaths "$dist_dir/Image Renderer.app"
patch_bundled_raytracer_rpaths "$dist_dir/Video Renderer.app"
sign_app_bundle "$dist_dir/Image Renderer.app"
sign_app_bundle "$dist_dir/Video Renderer.app"

# PyInstaller leaves sibling non-.app output directories in onedir mode on macOS.
# The .app bundles are the only artifacts we want to maintain in dist_apps.
rm -rf \
  "$dist_dir/.DS_Store" \
  "$dist_dir/Image Renderer" \
  "$dist_dir/Video Renderer"

echo
echo "Standalone apps created in:"
echo "  $dist_dir/Image Renderer.app"
echo "  $dist_dir/Video Renderer.app"
echo
echo "Notes:"
echo "  - The bundled raytracer binary is included."
if [[ -n "$ffmpeg_bin" ]]; then
  echo "  - ffmpeg was bundled into Video Renderer.app."
else
  echo "  - ffmpeg was not bundled."
fi
if [[ -n "$oidn_lib_dir" ]]; then
  echo "  - OIDN runtime libraries were bundled into both apps."
else
  echo "  - OIDN runtime libraries were not bundled."
fi
if [[ -n "$oidn_bin_dir" ]]; then
  echo "  - oidnDenoise was bundled into both apps."
else
  echo "  - oidnDenoise was not bundled."
fi
