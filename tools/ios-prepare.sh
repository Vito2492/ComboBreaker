#!/bin/sh
# Draws the iOS app's icons with the desktop program (built first) into
# ios/icons, at the sizes iOS asks for. Needs macOS (sips).
#   tools/ios-prepare.sh [PROGRAM]
set -e
prog=${1:-./ultracombo}
here=$(cd "$(dirname "$0")/.." && pwd)
dir=$here/ios/icons
mkdir -p "$dir"
"$prog" --write-icon "$dir/icon256.png"
sips -z 120 120 "$dir/icon256.png" --out "$dir/AppIcon60x60@2x.png" >/dev/null
sips -z 180 180 "$dir/icon256.png" --out "$dir/AppIcon60x60@3x.png" >/dev/null
sips -z 152 152 "$dir/icon256.png" --out "$dir/AppIcon76x76@2x~ipad.png" >/dev/null
rm "$dir/icon256.png"
echo "ios: ready to build"
