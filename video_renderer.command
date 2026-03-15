#!/bin/zsh
set -euo pipefail

cd "$(dirname "$0")"
project_dir="$PWD"
app_path="$project_dir/dist_apps/Video Renderer.app"

if [[ ! -d "$app_path" ]]; then
  echo "Video Renderer.app not found in dist_apps."
  echo "Building standalone apps first..."
  "$project_dir/package_standalone_apps.command"
fi

open "$app_path"
