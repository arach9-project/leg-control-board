# ==============================================================================
# ST Arm LLVM Clang Toolchain for STM32
# ==============================================================================

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES STM32_TOOLCHAIN_BIN)


# ------------------------------------------------------------------------------
# Toolchain path
# ------------------------------------------------------------------------------

if(NOT DEFINED STM32_TOOLCHAIN_BIN)
    message(FATAL_ERROR
        "Set STM32_TOOLCHAIN_BIN to the STM32CubeCLT LLVM toolchain bin directory"
    )
endif()

message(STATUS "Using STM32 Clang toolchain: ${STM32_TOOLCHAIN_BIN}")


# ------------------------------------------------------------------------------
# Toolchain binaries
# ------------------------------------------------------------------------------

set(STARM_CLANG   "${STM32_TOOLCHAIN_BIN}/starm-clang")
set(STARM_CLANGXX "${STM32_TOOLCHAIN_BIN}/starm-clang++")
set(STARM_OBJCOPY "${STM32_TOOLCHAIN_BIN}/starm-objcopy")
set(STARM_SIZE    "${STM32_TOOLCHAIN_BIN}/starm-size")
set(STARM_GDB     "${STM32_TOOLCHAIN_BIN}/starm-gdb")


foreach(tool
    STARM_CLANG
    STARM_CLANGXX
    STARM_OBJCOPY
    STARM_SIZE
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

set(CMAKE_C_COMPILER   "${STARM_CLANG}")
set(CMAKE_CXX_COMPILER "${STARM_CLANGXX}")
set(CMAKE_ASM_COMPILER "${STARM_CLANG}")


# ------------------------------------------------------------------------------
# Binary utilities
# ------------------------------------------------------------------------------

set(CMAKE_OBJCOPY "${STARM_OBJCOPY}")
set(CMAKE_SIZE    "${STARM_SIZE}")

if(EXISTS "${STARM_GDB}")
    set(CMAKE_GDB "${STARM_GDB}")
endif()


# ------------------------------------------------------------------------------
# Cross compilation search rules
# ------------------------------------------------------------------------------

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)


# ------------------------------------------------------------------------------
# Runtime library configuration
#
# Options:
#   STARM_HYBRID
#   STARM_NEWLIB
#   STARM_PICOLIBC
# ------------------------------------------------------------------------------

set(STARM_TOOLCHAIN_CONFIG "STARM_NEWLIB"
    CACHE STRING
    "ST Arm LLVM runtime configuration"
)


set(TOOLCHAIN_MULTILIBS "")

if(STARM_TOOLCHAIN_CONFIG STREQUAL "STARM_HYBRID")

    set(TOOLCHAIN_MULTILIBS
        "--multi-lib-config=${CLANG_GCC_CMSIS_COMPILER}/multilib.gnu_tools_for_stm32.yaml"
        "--gcc-toolchain=${GCC_TOOLCHAIN_ROOT}"
    )

elseif(STARM_TOOLCHAIN_CONFIG STREQUAL "STARM_NEWLIB")

    set(TOOLCHAIN_MULTILIBS
        "--config=newlib.cfg"
    )


else()

    message(FATAL_ERROR
        "Unknown STARM_TOOLCHAIN_CONFIG: ${STARM_TOOLCHAIN_CONFIG}"
    )

endif()


# ------------------------------------------------------------------------------
# STM32G431 Cortex-M4 configuration
# ------------------------------------------------------------------------------

set(MCU_FLAGS
    "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard ${TOOLCHAIN_MULTILIBS}"
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
    "${COMMON_FLAGS} ${WARNING_FLAGS} -fno-rtti -fno-exceptions -fno-threadsafe-statics"
)

set(CMAKE_ASM_FLAGS_INIT
    "${MCU_FLAGS} -x assembler-with-cpp -MP"
)


# ------------------------------------------------------------------------------
# Debug / Release flags
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


set(CMAKE_C_FLAGS_RELEASE_INIT
    "-Oz -g2"
)

set(CMAKE_CXX_FLAGS_RELEASE_INIT
    "-Oz -g2"
)


# ------------------------------------------------------------------------------
# Linker flags
# ------------------------------------------------------------------------------

set(LINKER_FLAGS
    "${MCU_FLAGS} -Wl,--gc-sections -Wl,-Map=${CMAKE_PROJECT_NAME}.map -Wl,--print-memory-usage -z noexecstack"
)

if(STARM_TOOLCHAIN_CONFIG STREQUAL "STARM_HYBRID")

    set(LINKER_FLAGS
        "${LINKER_FLAGS} --gcc-specs=nano.specs"
    )

elseif(STARM_TOOLCHAIN_CONFIG STREQUAL "STARM_NEWLIB")

    set(LINKER_FLAGS
        "${LINKER_FLAGS} -lcrt0-nosys"
    )

elseif(STARM_TOOLCHAIN_CONFIG STREQUAL "STARM_PICOLIBC")

    set(LINKER_FLAGS
        "${LINKER_FLAGS} -lcrt0-hosted -z norelro"
    )

endif()

set(CMAKE_EXE_LINKER_FLAGS_INIT "${LINKER_FLAGS}")
