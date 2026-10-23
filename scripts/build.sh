
#!/usr/bin/env bash

set -Eeuo pipefail

# ==============================================================================
# STM32 firmware build
#
# Usage:
#   ./scripts/build.sh gcc
#   ./scripts/build.sh clang
#   ./scripts/build.sh gcc --release
#   ./scripts/build.sh clang --jobs 8 --verbose
#
# Environment:
#   STM32CLT_ROOT    Required STM32CubeCLT installation path
# ==============================================================================

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"

BUILD_DIR="${ROOT_DIR}/build"
COMPILE_COMMANDS="${ROOT_DIR}/compile_commands.json"

BUILD_TYPE="Debug"
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

die() {
  printf '\033[1;31mError:\033[0m %s\n' "$1" >&2
  exit 1
}

on_error() {
  local exit_code=$?
  printf '\n' >&2
  printf 'Build failed at line %s (exit %s).\n' \
    "$1" "$exit_code" >&2
  exit "$exit_code"
}

trap 'on_error ${LINENO}' ERR

require_command() {
  command -v "$1" >/dev/null 2>&1 ||
    die "Required command not found: $1"
}

show_help() {
  cat <<EOF
Usage: build.sh <gcc|clang> [options]

Options:
  -d, --debug          Debug configuration (default)
      --release        Release configuration
      --relwithdebinfo RelWithDebInfo configuration
  -j, --jobs N         Parallel build jobs
  -v, --verbose        Print compiler commands
  -h, --help           Show this help message

Environment:
  STM32CLT_ROOT        Required STM32CubeCLT path
EOF
}

# ------------------------------------------------------------------------------
# Arguments
# ------------------------------------------------------------------------------

if [[ $# -eq 0 ]]; then
  show_help
  exit 2
fi

if [[ "$1" == "--help" || "$1" == "-h" ]]; then
  show_help
  exit 0
fi

TOOLCHAIN="$1"
shift

case "$TOOLCHAIN" in
  gcc | clang)
    ;;
  *)
    die "Invalid toolchain: ${TOOLCHAIN}. Expected gcc or clang."
    ;;
esac

while [[ $# -gt 0 ]]; do
  case "$1" in

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
        die "Missing value for --jobs"

      [[ "$2" =~ ^[1-9][0-9]*$ ]] ||
        die "Invalid job count: $2"

      JOBS="$2"
      shift 2
      ;;

    -v | --verbose)
      VERBOSE=true
      shift
      ;;

    *)
      die "Unknown option: $1"
      ;;
  esac
done

# ------------------------------------------------------------------------------
# Environment
# ------------------------------------------------------------------------------

: "${STM32CLT_ROOT:?STM32CLT_ROOT must be defined}"

case "$TOOLCHAIN" in

  gcc)
    TOOLCHAIN_FILE="${ROOT_DIR}/cmake/gcc-arm-none-eabi.cmake"
    TOOLCHAIN_BIN="${STM32CLT_ROOT}/GNU-tools-for-STM32/bin"
    COMPILER="${TOOLCHAIN_BIN}/arm-none-eabi-gcc"
    TOOLCHAIN_NAME="GNU Arm Embedded GCC"
    ;;

  clang)
    TOOLCHAIN_FILE="${ROOT_DIR}/cmake/starm-clang.cmake"
    TOOLCHAIN_BIN="${STM32CLT_ROOT}/st-arm-clang/bin"
    COMPILER="${TOOLCHAIN_BIN}/starm-clang"
    TOOLCHAIN_NAME="ST Arm LLVM Clang"
    ;;

esac

[[ -f "$TOOLCHAIN_FILE" ]] ||
  die "Toolchain file not found: ${TOOLCHAIN_FILE}"

[[ -x "$COMPILER" ]] ||
  die "Compiler not found: ${COMPILER}"

require_command cmake
require_command ninja

# ------------------------------------------------------------------------------
# Configure
# ------------------------------------------------------------------------------

cd "$ROOT_DIR"

mkdir -p "$BUILD_DIR"

print_info "Configuring STM32 firmware"

printf '    Toolchain:  %s\n' "$TOOLCHAIN_NAME"
printf '    Compiler:   %s\n' "$COMPILER"
printf '    Build type: %s\n' "$BUILD_TYPE"
printf '    Build dir:  %s\n' "$BUILD_DIR"
printf '    Jobs:       %s\n' "$JOBS"

# CMake cannot change toolchain in an existing build directory.
if [[ -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
  CACHED_TOOLCHAIN="$(
    grep '^CMAKE_TOOLCHAIN_FILE:' \
      "${BUILD_DIR}/CMakeCache.txt" | cut -d= -f2 || true
  )"

  if [[ -n "$CACHED_TOOLCHAIN" &&
        "$CACHED_TOOLCHAIN" != "$TOOLCHAIN_FILE" ]]; then
    die "Build directory uses another toolchain. Run just clean first."
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

# ------------------------------------------------------------------------------
# Build
# ------------------------------------------------------------------------------

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

# ------------------------------------------------------------------------------
# Clangd integration
# ------------------------------------------------------------------------------

if [[ -f "${BUILD_DIR}/compile_commands.json" ]]; then
  ln -sfn \
    "${BUILD_DIR}/compile_commands.json" \
    "$COMPILE_COMMANDS"

  print_info "Updated compile_commands.json"
fi

print_success "Build completed successfully"
