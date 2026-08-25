#!/usr/bin/env bash

set -Eeuo pipefail

# ==============================================================================
# STM32 CMake build script
#
# Usage:
#   ./scripts/compile.sh
#   ./scripts/compile.sh --toolchain gcc
#   ./scripts/compile.sh --toolchain clang
#   ./scripts/compile.sh --clean
#   ./scripts/compile.sh --reconfigure
#   ./scripts/compile.sh --release
#   ./scripts/compile.sh --jobs 8
#   ./scripts/compile.sh --verbose
#
# Options:
#   -c, --clean              Delete build directory and exit
#   -r, --reconfigure        Delete build directory before building
#   --toolchain gcc|clang    Select compiler toolchain (default: gcc)
#   -d, --debug              Debug configuration (default)
#       --release            Release configuration
#       --relwithdebinfo     RelWithDebInfo configuration
#   -j, --jobs N             Number of parallel build jobs
#   -v, --verbose            Print full compiler/linker commands
#   -h, --help               Show this help message
#
# Environment:
#   STM32CLT_ROOT            Override STM32CubeCLT installation path
# ==============================================================================

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"

BUILD_DIR="${ROOT_DIR}/build"
COMPILE_COMMANDS="${ROOT_DIR}/compile_commands.json"

GCC_TOOLCHAIN_FILE="${ROOT_DIR}/cmake/gcc-arm-none-eabi.cmake"
CLANG_TOOLCHAIN_FILE="${ROOT_DIR}/cmake/starm-clang.cmake"

STM32CLT_ROOT="${STM32CLT_ROOT:-/opt/ST/STM32CubeCLT_1.22.0}"

GCC_BIN="${STM32CLT_ROOT}/GNU-tools-for-STM32/bin"
CLANG_BIN="${STM32CLT_ROOT}/st-arm-clang/bin"

TOOLCHAIN="gcc"
BUILD_TYPE="Debug"

CLEAN_ONLY=false
RECONFIGURE=false
VERBOSE=false

if command -v sysctl >/dev/null 2>&1; then
  JOBS="$(sysctl -n hw.logicalcpu 2>/dev/null || echo 1)"
elif command -v nproc >/dev/null 2>&1; then
  JOBS="$(nproc)"
else
  JOBS=1
fi

print_info() {
  printf '\033[1;34m==>\033[0m %s\n' "$1"
}

print_success() {
  printf '\033[1;32m==>\033[0m %s\n' "$1"
}

print_error() {
  printf '\033[1;31mError:\033[0m %s\n' "$1" >&2
}

die() {
  print_error "$1"
  exit 1
}

show_help() {
  sed -n '3,28p' "$0"
}

on_error() {
  local exit_code=$?
  local line_number=$1

  printf '\n' >&2
  print_error "Build failed at line ${line_number} with exit code ${exit_code}."
  exit "$exit_code"
}

trap 'on_error ${LINENO}' ERR

require_command() {
  command -v "$1" >/dev/null 2>&1 ||
    die "Required command not found: $1"
}

clean_build_directory() {
  if [[ -d "$BUILD_DIR" ]]; then
    print_info "Removing build directory"
    rm -rf "$BUILD_DIR"
  fi

  if [[ -L "$COMPILE_COMMANDS" ]]; then
    rm -f "$COMPILE_COMMANDS"
  fi

  rm -rf "${ROOT_DIR}/.cache/clangd"
}

while [[ $# -gt 0 ]]; do
  case "$1" in

  -c | --clean)
    CLEAN_ONLY=true
    shift
    ;;

  -r | --reconfigure)
    RECONFIGURE=true
    shift
    ;;

  --toolchain)
    [[ $# -ge 2 ]] ||
      die "The $1 option requires gcc or clang."

    case "$2" in
    gcc | clang)
      TOOLCHAIN="$2"
      ;;
    *)
      die "Unsupported toolchain: $2"
      ;;
    esac

    shift 2
    ;;

  -d | --debug)
    BUILD_TYPE="Debug"
    shift
    ;;

  --release)
    BUILD_TYPE="Release"
    shift
    ;;

  --relwithdebinfo)
    BUILD_TYPE="RelWithDebInfo"
    shift
    ;;

  -j | --jobs)
    [[ $# -ge 2 ]] ||
      die "The $1 option requires a job count."

    [[ "$2" =~ ^[1-9][0-9]*$ ]] ||
      die "Invalid job count: $2"

    JOBS="$2"
    shift 2
    ;;

  -v | --verbose)
    VERBOSE=true
    shift
    ;;

  -h | --help)
    show_help
    exit 0
    ;;

  *)
    die "Unknown option: $1. Run '$0 --help' for usage."
    ;;
  esac
done

cd "$ROOT_DIR"

require_command cmake
require_command ninja

case "$TOOLCHAIN" in

gcc)
  TOOLCHAIN_FILE="$GCC_TOOLCHAIN_FILE"
  TOOLCHAIN_BIN="$GCC_BIN"
  TOOLCHAIN_NAME="GNU Arm Embedded GCC"

  [[ -x "${GCC_BIN}/arm-none-eabi-gcc" ]] ||
    die "GCC toolchain not found: ${GCC_BIN}"

  ;;

clang)
  TOOLCHAIN_FILE="$CLANG_TOOLCHAIN_FILE"
  TOOLCHAIN_BIN="$CLANG_BIN"
  TOOLCHAIN_NAME="ST Arm LLVM Clang"

  [[ -x "${CLANG_BIN}/starm-clang" ]] ||
    die "ST Arm LLVM toolchain not found: ${CLANG_BIN}"

  ;;

esac

[[ -f "$TOOLCHAIN_FILE" ]] ||
  die "Toolchain file not found: ${TOOLCHAIN_FILE}"

if "$CLEAN_ONLY"; then
  clean_build_directory
  print_success "Project cleaned"
  exit 0
fi

if "$RECONFIGURE"; then
  clean_build_directory
fi

mkdir -p "$BUILD_DIR"

print_info "Configuring STM32 firmware"

printf '    Toolchain:  %s\n' "$TOOLCHAIN_NAME"
printf '    Toolchain:  %s\n' "$TOOLCHAIN_BIN"
printf '    Build type: %s\n' "$BUILD_TYPE"
printf '    Build dir:  %s\n' "$BUILD_DIR"
printf '    Jobs:       %s\n' "$JOBS"

if [[ -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
  CACHED_TOOLCHAIN=$(grep CMAKE_TOOLCHAIN_FILE "${BUILD_DIR}/CMakeCache.txt" | cut -d= -f2)

  if [[ -n "$CACHED_TOOLCHAIN" && "$CACHED_TOOLCHAIN" != "$TOOLCHAIN_FILE" ]]; then
    die "Build directory was configured with another toolchain. Use --reconfigure."
  fi
fi

cmake \
  -S "$ROOT_DIR" \
  -B "$BUILD_DIR" \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
  -DSTM32_TOOLCHAIN_BIN="$TOOLCHAIN_BIN" \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

print_info "Building firmware"

BUILD_COMMAND=(
  cmake
  --build "$BUILD_DIR"
  --parallel "$JOBS"
)

if "$VERBOSE"; then
  BUILD_COMMAND+=(--verbose)
fi

"${BUILD_COMMAND[@]}"

if [[ -f "${BUILD_DIR}/compile_commands.json" ]]; then
  ln -sfn \
    "${BUILD_DIR}/compile_commands.json" \
    "$COMPILE_COMMANDS"

  rm -rf "${ROOT_DIR}/.cache/clangd"

  print_info "Updated compile_commands.json for clangd"
fi

print_success "Build completed successfully"
