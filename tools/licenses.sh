#!/bin/sh
# The licence texts a package must carry: ComboBreaker/UltraCombo (MIT), the
# menu font (OFL), libchdr and its decompressors (compiled in), and, when
# given, SDL2 and TinyCC (with where its source is).
#   tools/licenses.sh OUT_DIR [SDL_SOURCE_DIR] [TINYCC_SOURCE_DIR]
set -e
out=$1; sdl=$2; tcc=$3
here=$(cd "$(dirname "$0")/.." && pwd)
mkdir -p "$out"
cp "$here/LICENSE" "$out/UltraCombo-ComboBreaker-LICENSE.txt"
cp "$here/tools/fonts/OFL-PressStart2P.txt" "$out/"
chdr=$here/src/third_party/libchdr
{
    cat "$chdr/SOURCE.txt"
    printf '\n==== libchdr ====\n\n';     cat "$chdr/LICENSE.txt"
    printf '\n==== zstd ====\n\n';        cat "$chdr/deps/zstd-1.5.7/LICENSE"
    printf '\n==== LZMA SDK ====\n\n';    cat "$chdr/deps/lzma-26.02/LICENSE"
    printf '\n==== miniz ====\n\n';       sed -n '/Copyright/,/^ \*\*\*\*/p' "$chdr/deps/miniz-3.1.2/miniz.c" | head -40
} > "$out/libchdr-and-decompressors.txt"
if [ -n "$sdl" ]; then cp "$sdl/LICENSE.txt" "$out/SDL2-LICENSE.txt"; fi
if [ -n "$tcc" ]; then
    cp "$tcc/COPYING" "$out/TinyCC-COPYING.txt"
    echo "TinyCC (libtcc.so), LGPL 2.1, built unmodified from https://github.com/TinyCC/tinycc commit $(cd "$tcc" && git rev-parse HEAD)" > "$out/TinyCC-SOURCE.txt"
fi
