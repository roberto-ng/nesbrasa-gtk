#!/usr/bin/env sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$script_dir"
if [ -z "${EM_CACHE:-}" ]; then
  EM_CACHE="$script_dir/.emscripten-cache"
  export EM_CACHE
fi
mkdir -p "$EM_CACHE"

em++ \
  -std=c++17 \
  -O2 \
  -I../../core/include/nesbrasa \
  -I../../core/src \
  -I../../core/src/mapeadores \
  ../../core/src/cores.cpp \
  ../../core/src/controle.cpp \
  ../../core/src/cpu.cpp \
  ../../core/src/instrucao.cpp \
  ../../core/src/memoria.cpp \
  ../../core/src/nesbrasa.cpp \
  ../../core/src/ppu.cpp \
  ../../core/src/util.cpp \
  ../../core/src/mapeadores/cartucho.cpp \
  ../../core/src/mapeadores/nrom.cpp \
  wasm/nesbrasa_web.cpp \
  -s MODULARIZE=1 \
  -s EXPORT_ES6=1 \
  -s ENVIRONMENT=web \
  -s ALLOW_MEMORY_GROWTH=1 \
  --bind \
  -s EXPORTED_RUNTIME_METHODS="['HEAPU32']" \
  -o public/nesbrasa.js
