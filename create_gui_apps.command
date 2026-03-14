#!/bin/zsh
set -euo pipefail

cd "$(dirname "$0")"
project_dir="$PWD"
tmp_root="$(mktemp -d /tmp/sr_rt_app_build.XXXXXX)"
trap 'rm -rf "$tmp_root"' EXIT

generate_icon_icns() {
  local out_icns="$1"
  local kind="$2" # "image" or "video"
  local iconset_dir="$tmp_root/${kind}.iconset"
  local master_png="$tmp_root/${kind}_1024.png"

  mkdir -p "$iconset_dir"

  swift - "$master_png" "$kind" <<'SWIFT'
import AppKit

let outPath = CommandLine.arguments[1]
let kind = CommandLine.arguments[2]
let size: CGFloat = 1024.0

let image = NSImage(size: NSSize(width: size, height: size))
image.lockFocus()

let rect = NSRect(x: 0, y: 0, width: size, height: size)
let bgPath = NSBezierPath(roundedRect: rect.insetBy(dx: 36, dy: 36), xRadius: 220, yRadius: 220)

let gradientColors: [NSColor]
if kind == "video" {
    gradientColors = [
        NSColor(calibratedRed: 0.10, green: 0.14, blue: 0.29, alpha: 1.0),
        NSColor(calibratedRed: 0.15, green: 0.37, blue: 0.63, alpha: 1.0)
    ]
} else {
    gradientColors = [
        NSColor(calibratedRed: 0.17, green: 0.13, blue: 0.09, alpha: 1.0),
        NSColor(calibratedRed: 0.47, green: 0.34, blue: 0.18, alpha: 1.0)
    ]
}

let gradient = NSGradient(colors: gradientColors)!
gradient.draw(in: bgPath, angle: -45)

NSColor(calibratedWhite: 1.0, alpha: 0.22).setStroke()
bgPath.lineWidth = 20
bgPath.stroke()

let glyphRect = rect.insetBy(dx: 160, dy: 160)
let glyphPath = NSBezierPath(roundedRect: glyphRect, xRadius: 100, yRadius: 100)
NSColor(calibratedWhite: 0.98, alpha: 0.16).setFill()
glyphPath.fill()

let text = (kind == "video") ? "VID" : "IMG"
let paragraph = NSMutableParagraphStyle()
paragraph.alignment = .center
let attrs: [NSAttributedString.Key: Any] = [
    .font: NSFont.boldSystemFont(ofSize: 300),
    .foregroundColor: NSColor(calibratedWhite: 1.0, alpha: 0.95),
    .paragraphStyle: paragraph
]
let textSize = text.size(withAttributes: attrs)
let textPoint = NSPoint(
    x: (size - textSize.width) * 0.5,
    y: (size - textSize.height) * 0.5 - 20
)
text.draw(at: textPoint, withAttributes: attrs)

image.unlockFocus()

guard
    let tiff = image.tiffRepresentation,
    let rep = NSBitmapImageRep(data: tiff),
    let png = rep.representation(using: .png, properties: [:])
else {
    fputs("Failed to produce PNG icon data.\n", stderr)
    exit(1)
}

do {
    try png.write(to: URL(fileURLWithPath: outPath))
} catch {
    fputs("Failed to write icon PNG: \(error)\n", stderr)
    exit(1)
}
SWIFT

  sips -s format png -z 16 16 "$master_png" --out "$iconset_dir/icon_16x16.png" >/dev/null
  sips -s format png -z 32 32 "$master_png" --out "$iconset_dir/icon_16x16@2x.png" >/dev/null
  sips -s format png -z 32 32 "$master_png" --out "$iconset_dir/icon_32x32.png" >/dev/null
  sips -s format png -z 64 64 "$master_png" --out "$iconset_dir/icon_32x32@2x.png" >/dev/null
  sips -s format png -z 128 128 "$master_png" --out "$iconset_dir/icon_128x128.png" >/dev/null
  sips -s format png -z 256 256 "$master_png" --out "$iconset_dir/icon_128x128@2x.png" >/dev/null
  sips -s format png -z 256 256 "$master_png" --out "$iconset_dir/icon_256x256.png" >/dev/null
  sips -s format png -z 512 512 "$master_png" --out "$iconset_dir/icon_256x256@2x.png" >/dev/null
  sips -s format png -z 512 512 "$master_png" --out "$iconset_dir/icon_512x512.png" >/dev/null
  cp "$master_png" "$iconset_dir/icon_512x512@2x.png"

  iconutil -c icns "$iconset_dir" -o "$out_icns"
}

create_app() {
  local app_name="$1"
  local bundle_id="$2"
  local launch_py="$3"
  local log_path="$4"
  local icon_file_name="$5"

  local app_dir="$app_name.app"
  rm -rf "$app_dir"
  mkdir -p "$app_dir/Contents/MacOS" "$app_dir/Contents/Resources" "$app_dir/Contents/Resources/gui" "$app_dir/Contents/Resources/bin"

  cat > "$app_dir/Contents/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleDisplayName</key>
  <string>$app_name</string>
  <key>CFBundleExecutable</key>
  <string>launch</string>
  <key>CFBundleIdentifier</key>
  <string>$bundle_id</string>
  <key>CFBundleIconFile</key>
  <string>$icon_file_name</string>
  <key>CFBundleName</key>
  <string>$app_name</string>
  <key>CFBundlePackageType</key>
  <string>APPL</string>
  <key>CFBundleShortVersionString</key>
  <string>1.0</string>
  <key>CFBundleVersion</key>
  <string>1</string>
  <key>LSMinimumSystemVersion</key>
  <string>11.0</string>
</dict>
</plist>
EOF

  cp tools/gui/*.py "$app_dir/Contents/Resources/gui/"
  if [[ -x "$project_dir/build/raytracer" ]]; then
    cp "$project_dir/build/raytracer" "$app_dir/Contents/Resources/bin/raytracer"
    chmod +x "$app_dir/Contents/Resources/bin/raytracer"
  else
    echo "Warning: build/raytracer not found; bundle will be created without a renderer binary."
  fi

  cat > "$app_dir/Contents/MacOS/launch" <<EOF
#!/bin/zsh
set -euo pipefail
bundle_root="\$(cd "\$(dirname "\$0")/.." && pwd)"
resources_dir="\$bundle_root/Resources"
gui_dir="\$resources_dir/gui"
bin_dir="\$resources_dir/bin"
mkdir -p "\$HOME/Pictures/SR-RT Engine/outputs"
export SR_RT_APP_ROOT="\$resources_dir"
export SR_RT_OUTPUT_DIR="\$HOME/Pictures/SR-RT Engine/outputs"
if [[ -x "\$bin_dir/raytracer" ]]; then
  export SR_RT_RAYTRACER_BIN="\$bin_dir/raytracer"
fi
python_bin=""
for cand in \
  "\$HOME/.rye/shims/python3" \
  "/opt/homebrew/bin/python3" \
  "/usr/local/bin/python3" \
  "/usr/bin/python3"; do
  if [[ -x "\$cand" ]]; then
    python_bin="\$cand"
    break
  fi
done
if [[ -z "\$python_bin" ]]; then
  echo "No python3 interpreter found." >>$log_path
  exit 127
fi
exec "\$python_bin" "\$gui_dir/$launch_py" >>$log_path 2>&1
EOF
  chmod +x "$app_dir/Contents/MacOS/launch"
  cp "$tmp_root/$icon_file_name" "$app_dir/Contents/Resources/$icon_file_name"
}

generate_icon_icns "$tmp_root/image_renderer.icns" "image"
generate_icon_icns "$tmp_root/video_renderer.icns" "video"

create_app "Image Renderer" "dev.srrt.image-renderer" "tools/gui/raytracer_gui.py" "/tmp/sr_rt_image_gui.log" "image_renderer.icns"
create_app "Video Renderer" "dev.srrt.video-renderer" "tools/gui/raytracer_animation_gui.py" "/tmp/sr_rt_video_gui.log" "video_renderer.icns"

echo "Created:"
echo "  $project_dir/Image Renderer.app"
echo "  $project_dir/Video Renderer.app"
