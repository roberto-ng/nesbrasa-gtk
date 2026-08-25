#!/usr/bin/env sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$script_dir"
if [ -z "${EM_CACHE:-}" ]; then
  EM_CACHE="$script_dir/.emscripten-cache"
  export EM_CACHE
fi
cmake --preset web-emscripten "$script_dir/../.."
cmake --build --preset web-emscripten
