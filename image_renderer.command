#!/bin/zsh
set -e
cd "$(dirname "$0")"
project_dir="$PWD"
osascript >/dev/null 2>&1 <<EOF
do shell script "cd " & quoted form of "$project_dir" & " && nohup python3 tools/gui/raytracer_gui.py >/tmp/sr_rt_image_gui.log 2>&1 < /dev/null &"
EOF
exit 0
