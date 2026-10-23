set shell := ["bash", "-cu"]

# -----------------------------------------------------------------------------
# Paths
# -----------------------------------------------------------------------------

stm32clt_root := "~/st/stm32cubeclt_1.22.0"
cube_programmer := stm32clt_root + "/STM32CubeProgrammer/bin"

gcc_bin := stm32clt_root + "/GNU-tools-for-STM32/bin"
clang_bin := stm32clt_root + "/st-arm-clang/bin"

openocd := "openocd"

gdb_server := stm32clt_root + "/STLink-gdb-server/bin/ST-LINK_gdbserver"
gdb := stm32clt_root + "/GNU-tools-for-STM32/bin/arm-none-eabi-gdb"

build_dir := "build"

# Environment shared with build scripts
export STM32CLT_ROOT := replace(stm32clt_root, "~", home_directory())

# -----------------------------------------------------------------------------
# Default
# -----------------------------------------------------------------------------

default:
    @just --list

# -----------------------------------------------------------------------------
# Build
# -----------------------------------------------------------------------------

# Build firmware with GCC (default)
build *args:
    ./scripts/build.sh gcc {{args}}

# Build firmware with GCC
gcc *args:
    ./scripts/build.sh gcc {{args}}

# Build firmware with ST LLVM Clang
clang *args:
    ./scripts/build.sh clang {{args}}

# Build optimized firmware
release *args:
    ./scripts/build.sh gcc --release {{args}}

# Build with debug symbols and optimization
relwithdebinfo *args:
    ./scripts/build.sh gcc --relwithdebinfo {{args}}

# -----------------------------------------------------------------------------
# Maintenance
# -----------------------------------------------------------------------------

# Delete build artifacts and clangd cache
clean:
    rm -rf {{build_dir}}
    rm -f compile_commands.json
    rm -rf .cache/clangd

# Clean and rebuild using GCC
reconfigure *args:
    just clean
    just gcc {{args}}

# Clean and rebuild using Clang
reconfigure-clang *args:
    just clean
    just clang {{args}}





verbose:
    ./scripts/build.sh --verbose


# -----------------------------------------------------------------------------
# Probe-rs configuration
# -----------------------------------------------------------------------------

probe_chip := "STM32G431CBUx"
probe_gdb_port := "1337"


# -----------------------------------------------------------------------------
# Probe
# -----------------------------------------------------------------------------

# List connected debug probes
probe-list:
    probe-rs list

# List supported STM32G431 variants
probe-chips:
    probe-rs chip list | grep STM32G431

# Reset microcontroller
reset:
    probe-rs reset --chip {{probe_chip}}


# -----------------------------------------------------------------------------
# Monitor
# -----------------------------------------------------------------------------

# Attach to running firmware without flashing or resetting
# SWO monitor
monitor:
  ./scripts/monitor.sh
  # probe-rs itm \
  #     --chip {{probe_chip}} \
  #     swo 30000 170000000 2000000


# -----------------------------------------------------------------------------
# Flash
# -----------------------------------------------------------------------------

# Flash firmware and reset
flash:
    #!/usr/bin/env bash
    set -euo pipefail

    elf="$(find {{build_dir}} -maxdepth 1 -name '*.elf' -print -quit)"

    if [[ -z "$elf" ]]; then
        echo "No ELF found in {{build_dir}}"
        exit 1
    fi

    echo "Flashing $elf"

    probe-rs download \
        --chip {{probe_chip}} \
        "$elf"

    probe-rs reset --chip {{probe_chip}}


# Build and flash
bf: build flash

# Build and flash
run: build flash monitor





# -----------------------------------------------------------------------------
# Debugger
# -----------------------------------------------------------------------------

# Interactive probe-rs debugger
debug:
    #!/usr/bin/env bash
    set -euo pipefail

    elf="$(find {{build_dir}} -maxdepth 1 -name '*.elf' -print -quit)"

    if [[ -z "$elf" ]]; then
        echo "No ELF found in {{build_dir}}"
        exit 1
    fi

    probe-rs debug \
        --chip {{probe_chip}} \
        "$elf"


# Build and launch interactive debugger
debug-build: build
    #!/usr/bin/env bash
    set -euo pipefail

    elf="$(find {{build_dir}} -maxdepth 1 -name '*.elf' -print -quit)"

    test -n "$elf" || {
        echo "No ELF found in {{build_dir}}"
        exit 1
    }

    probe-rs debug \
        --chip {{probe_chip}} \
        --launch \
        "$elf"


# -----------------------------------------------------------------------------
# GDB
# -----------------------------------------------------------------------------

# Start probe-rs GDB server
gdb-server:
    probe-rs gdb \
        --chip {{probe_chip}} \
        --gdb-connection-string "127.0.0.1:{{probe_gdb_port}}"


# Connect GDB to an existing probe-rs GDB server
gdb:
    #!/usr/bin/env bash
    set -euo pipefail

    elf="$(find {{build_dir}} -maxdepth 1 -name '*.elf' -print -quit)"

    test -n "$elf" || {
        echo "No ELF found"
        exit 1
    }

    {{gdb}} "$elf" \
        -ex "target extended-remote localhost:{{probe_gdb_port}}"


# Connect GDB and break at main
gdb-main:
    #!/usr/bin/env bash
    set -euo pipefail

    elf="$(find {{build_dir}} -maxdepth 1 -name '*.elf' -print -quit)"

    test -n "$elf" || {
        echo "No ELF found"
        exit 1
    }

    {{gdb}} "$elf" \
        -ex "target extended-remote localhost:{{probe_gdb_port}}" \
        -ex "monitor reset halt" \
        -ex "tbreak main" \
        -ex "continue"


# Connect GDB and break at Error_Handler
gdb-error:
    #!/usr/bin/env bash
    set -euo pipefail

    elf="$(find {{build_dir}} -maxdepth 1 -name '*.elf' -print -quit)"

    test -n "$elf" || {
        echo "No ELF found"
        exit 1
    }

    {{gdb}} "$elf" \
        -ex "target extended-remote localhost:{{probe_gdb_port}}" \
        -ex "break Error_Handler" \
        -ex "continue"


# -----------------------------------------------------------------------------
# Convenience / inspection
# -----------------------------------------------------------------------------

elf:
    @find {{build_dir}} -maxdepth 1 -name '*.elf' -print

size:
    elf="$(find {{build_dir}} -maxdepth 1 -name '*.elf' -print -quit)"
    test -n "$elf" || { echo "No ELF found in {{build_dir}}"; exit 1; }
    {{gcc_bin}}/arm-none-eabi-size "$elf"

symbols pattern:
    elf="$(find {{build_dir}} -maxdepth 1 -name '*.elf' -print -quit)"
    test -n "$elf" || { echo "No ELF found in {{build_dir}}"; exit 1; }
    {{gcc_bin}}/arm-none-eabi-nm -C "$elf" | grep -i "{{pattern}}"

debugger:
  cargo run -p board-debugger
