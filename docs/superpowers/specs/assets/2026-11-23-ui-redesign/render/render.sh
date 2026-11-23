#!/usr/bin/env bash
# Renders the accepted boards (../*.dc.html) to PNGs without the design canvas, using the stand-in
# runtime support.js (first frame, no events; <dc-import> supported). Windows + Git Bash + Edge.
#   render.sh [scale] [out-dir] [query]     defaults: 2  ../png  beat=0
#   query: openPage=<page> | beat=<n> | message=<text> | editTempo=<n>   (StageRevised)
# Example, the board at the app's scale for a 1920 x 991 window: render.sh 1.5364 /tmp/out beat=0
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
boards="$(dirname "$here")"
scale="${1:-2}"
out="${2:-$boards/png}"
query="${3:-beat=0}"
edge="/c/Program Files (x86)/Microsoft/Edge/Application/msedge.exe"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
mkdir -p "$work/fonts" "$out"
cp "$boards"/*.dc.html "$here/support.js" "$work/"
cp "$boards"/../../../../../Resources/Fonts/*.ttf "$work/fonts/"
for board in "$boards"/*.dc.html; do
    name="$(basename "$board" .dc.html)"
    size="$(grep -o '"\$preview":{"width":[0-9]*,"height":[0-9]*}' "$board" | head -1 | grep -o '[0-9]\+' | paste -sd,)"
    "$edge" --headless=new --disable-gpu --allow-file-access-from-files --user-data-dir="$(cygpath -w "$work/profile")" \
        --window-size="$size" --force-device-scale-factor="$scale" --hide-scrollbars --virtual-time-budget=3000 \
        --screenshot="$(cygpath -w "$out/$name.png")" "file:///$(cygpath -m "$work/$name.dc.html")?$query" > /dev/null 2>&1
    echo "$name ($size x $scale)"
done
