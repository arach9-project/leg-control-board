#!/bin/bash
set -e
ROOT=$(pwd)
rm -rf build
mkdir -p build
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/gcc-arm-none-eabi.cmake" \
  -DCMAKE_C_FLAGS="-u _printf_float" \
  -DCMAKE_CXX_FLAGS="-u _printf_float" \
  -DCMAKE_BUILD_TYPE=DEBUG

cmake --build build -j$(sysctl -n hw.logicalcpu)

# refresh clangd's view of the project
ln -sf build/compile_commands.json "$ROOT/compile_commands.json"
rm -rf "$ROOT/.cache/clangd"
