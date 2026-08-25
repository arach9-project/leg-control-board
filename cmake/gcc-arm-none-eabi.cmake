# ==============================================================================
# GCC Arm Embedded Toolchain for STM32
# ==============================================================================

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Prevent CMake from trying to execute target binaries during configuration.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES STM32_TOOLCHAIN_BIN)

if(NOT DEFINED STM32_TOOLCHAIN_BIN)
    message(FATAL_ERROR
        "Set STM32_TOOLCHAIN_BIN to the STM32CubeCLT GNU Arm toolchain bin directory"
    )
endif()

message(STATUS "Using STM32 GCC toolchain: ${STM32_TOOLCHAIN_BIN}")


# ------------------------------------------------------------------------------
# Toolchain binaries
# ------------------------------------------------------------------------------

set(ARM_GCC     "${STM32_TOOLCHAIN_BIN}/arm-none-eabi-gcc")
set(ARM_GXX     "${STM32_TOOLCHAIN_BIN}/arm-none-eabi-g++")
set(ARM_OBJCOPY "${STM32_TOOLCHAIN_BIN}/arm-none-eabi-objcopy")
set(ARM_OBJDUMP "${STM32_TOOLCHAIN_BIN}/arm-none-eabi-objdump")
set(ARM_SIZE    "${STM32_TOOLCHAIN_BIN}/arm-none-eabi-size")
set(ARM_GDB     "${STM32_TOOLCHAIN_BIN}/arm-none-eabi-gdb")


foreach(tool
    ARM_GCC
    ARM_GXX
    ARM_OBJCOPY
    ARM_OBJDUMP
    ARM_SIZE
)
    if(NOT EXISTS "${${tool}}")
        message(FATAL_ERROR
            "${tool} does not exist: ${${tool}}"
        )
    endif()
endforeach()


# ------------------------------------------------------------------------------
# Compilers
# ------------------------------------------------------------------------------

set(CMAKE_C_COMPILER   "${ARM_GCC}")
set(CMAKE_CXX_COMPILER "${ARM_GXX}")
set(CMAKE_ASM_COMPILER "${ARM_GCC}")


# ------------------------------------------------------------------------------
# Binary utilities
# ------------------------------------------------------------------------------

set(CMAKE_OBJCOPY "${ARM_OBJCOPY}")
set(CMAKE_OBJDUMP "${ARM_OBJDUMP}")
set(CMAKE_SIZE    "${ARM_SIZE}")

if(EXISTS "${ARM_GDB}")
    set(CMAKE_GDB "${ARM_GDB}")
endif()


# ------------------------------------------------------------------------------
# Cross compilation search rules
# ------------------------------------------------------------------------------

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)


# ------------------------------------------------------------------------------
# STM32G431 Cortex-M4 configuration
# ------------------------------------------------------------------------------

set(MCU_FLAGS
    "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard"
)

set(COMMON_FLAGS
    "${MCU_FLAGS} -ffunction-sections -fdata-sections -fstack-usage -fno-common"
)

set(WARNING_FLAGS
    "-Wall -Wextra -Wshadow -Wdouble-promotion -Wformat=2"
)


# ------------------------------------------------------------------------------
# Compiler flags
# ------------------------------------------------------------------------------

set(CMAKE_C_FLAGS_INIT
    "${COMMON_FLAGS} ${WARNING_FLAGS}"
)

set(CMAKE_CXX_FLAGS_INIT
    "${COMMON_FLAGS} ${WARNING_FLAGS} -fno-exceptions -fno-rtti -fno-threadsafe-statics"
)

set(CMAKE_ASM_FLAGS_INIT
    "${MCU_FLAGS} -x assembler-with-cpp -MP"
)


# ------------------------------------------------------------------------------
# Build type flags
# ------------------------------------------------------------------------------

set(CMAKE_C_FLAGS_DEBUG_INIT
    "-Og -g3 -gdwarf-4"
)

set(CMAKE_CXX_FLAGS_DEBUG_INIT
    "-Og -g3 -gdwarf-4"
)

set(CMAKE_ASM_FLAGS_DEBUG_INIT
    "-g3 -gdwarf-4"
)


# Keep symbols for debugging tools such as STM32CubeMonitor.
set(CMAKE_C_FLAGS_RELEASE_INIT
    "-Os -g2"
)

set(CMAKE_CXX_FLAGS_RELEASE_INIT
    "-Os -g2"
)


# ------------------------------------------------------------------------------
# Linker flags
# ------------------------------------------------------------------------------

set(CMAKE_EXE_LINKER_FLAGS_INIT
    "${MCU_FLAGS} -Wl,--gc-sections -Wl,-Map=${CMAKE_PROJECT_NAME}.map -Wl,--print-memory-usage"
)
