set shell := ["bash", "-cu"]

# -----------------------------------------------------------------------------
# Paths
# -----------------------------------------------------------------------------

stm32clt_root := "/opt/ST/STM32CubeCLT_1.22.0"
cube_programmer := stm32clt_root + "/STM32CubeProgrammer/bin"

gcc_bin := stm32clt_root + "/GNU-tools-for-STM32/bin"

openocd := "openocd"

gdb_server := stm32clt_root + "/STLink-gdb-server/bin/ST-LINK_gdbserver"
gdb := stm32clt_root + "/GNU-tools-for-STM32/bin/arm-none-eabi-gdb"

build_dir := "build"


# -----------------------------------------------------------------------------
# Default
# -----------------------------------------------------------------------------

default:
    @just --list


# -----------------------------------------------------------------------------
# Build
# -----------------------------------------------------------------------------

build *args:
    ./scripts/build.sh {{args}}

debug:
    ./scripts/build.sh --debug

release:
    ./scripts/build.sh --release

relwithdebinfo:
    ./scripts/build.sh --relwithdebinfo

clean:
    ./scripts/build.sh --clean

reconfigure:
    ./scripts/build.sh --reconfigure

gcc:
    ./scripts/build.sh --toolchain gcc

clang:
    ./scripts/build.sh --toolchain clang

verbose:
    ./scripts/build.sh --verbose


# -----------------------------------------------------------------------------
# Monitor
# -----------------------------------------------------------------------------

monitor:
    ./scripts/monitor.sh


# -----------------------------------------------------------------------------
# Flash
# -----------------------------------------------------------------------------

flash:
    #!/usr/bin/env bash
    set -euo pipefail

    elf="$(find build -maxdepth 1 -name '*.elf' -print -quit)"

    if [[ -z "$elf" ]]; then
        echo "No ELF found in build"
        exit 1
    fi

    echo "Flashing $elf"

    {{cube_programmer}}/STM32_Programmer_CLI \
        -c port=SWD \
        -w "$elf" \
        -rst


# Build and flash
bf: build flash

# Build, flash, then monitor
run: build flash monitor



# -----------------------------------------------------------------------------
# GDB
# -----------------------------------------------------------------------------


gdb:
    elf="$$(find {{build_dir}} -maxdepth 1 -name '*.elf' -print -quit)"; \
    test -n "$$elf" || { echo "No ELF found in {{build_dir}}"; exit 1; }; \
    {{gdb}} "$$elf"

gdb-connect:
    elf="$$(find {{build_dir}} -maxdepth 1 -name '*.elf' -print -quit)"; \
    test -n "$$elf" || { echo "No ELF found in {{build_dir}}"; exit 1; }; \
    {{gdb}} \
        "$$elf" \
        -ex "target extended-remote localhost:61234"

gdb-main:
    elf="$$(find {{build_dir}} -maxdepth 1 -name '*.elf' -print -quit)"; \
    test -n "$$elf" || { echo "No ELF found in {{build_dir}}"; exit 1; }; \
    {{gdb}} \
        "$$elf" \
        -ex "target extended-remote localhost:61234" \
        -ex "monitor reset" \
        -ex "break main" \
        -ex "continue"

gdb-server:
    {{gdb_server}} \
        --swd \
        -cp {{cube_programmer}}


gdb-error:
    #!/usr/bin/env bash
    set -euo pipefail

    elf="$(find build -maxdepth 1 -name '*.elf' -print -quit)"

    if [[ -z "$elf" ]]; then
        echo "No ELF found in build"
        exit 1
    fi

    echo "Using ELF: $elf"

    {{gdb}} \
        "$elf" \
        -ex "target extended-remote localhost:61234" \
        -ex "monitor reset" \
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
