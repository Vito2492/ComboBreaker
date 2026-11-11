#!/bin/sh
# Puts what the Android build needs beside android/: SDL2's sources (C into
# app/jni/SDL, Java into app/src/main/java/org/libsdl/app) and the launcher
# icon, drawn by the desktop program (`ultracombo --write-icon`, built first).
#   tools/android-prepare.sh SDL_SOURCE_DIR [PROGRAM]
set -e
sdl=$1
prog=${2:-./ultracombo}
here=$(cd "$(dirname "$0")/.." && pwd)
app=$here/android/app
rm -rf "$app/jni/SDL"
mkdir -p "$app/jni/SDL"
cp -r "$sdl"/. "$app/jni/SDL/"
mkdir -p "$app/src/main/java/org/libsdl/app"
cp "$sdl"/android-project/app/src/main/java/org/libsdl/app/*.java "$app/src/main/java/org/libsdl/app/"
for d in mdpi:48 hdpi:72 xhdpi:96 xxhdpi:144 xxxhdpi:192; do
    mkdir -p "$app/src/main/res/mipmap-${d%%:*}"
done
"$prog" --write-icon "$app/src/main/res/mipmap-xxxhdpi/ic_launcher.png"
# the other densities: the same picture (Android scales it)
for d in mdpi hdpi xhdpi xxhdpi; do
    cp "$app/src/main/res/mipmap-xxxhdpi/ic_launcher.png" "$app/src/main/res/mipmap-$d/"
done
echo "android: ready to build (cd android && gradle assembleRelease)"
