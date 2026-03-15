#!/bin/zsh
set -euo pipefail

cd "$(dirname "$0")"
project_dir="$PWD"

echo "create_gui_apps.command now delegates to the canonical dist_apps build."
echo "Maintained app bundles live in:"
echo "  $project_dir/dist_apps"
echo

"$project_dir/package_standalone_apps.command"
