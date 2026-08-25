#!/usr/bin/env sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project_dir="$script_dir/../.."
cd "$project_dir"
if [ -z "${EM_CACHE:-}" ]; then
  EM_CACHE="$script_dir/.emscripten-cache"
  export EM_CACHE
fi
cmake --preset web-emscripten
cmake --build --preset web-emscripten

library=$(find "$project_dir/build/web-emscripten/core" -maxdepth 1 -type f -name 'libnesbrasa.so.*' -print -quit)
if [ -z "$library" ]; then
    echo "Could not find the nesbrasa WebAssembly side module" >&2
    exit 1
fi

cp "$library" "$script_dir/public/$(basename "$library")"
