set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES ARM_TOOLCHAIN_BIN)

if(NOT DEFINED ARM_TOOLCHAIN_BIN)
    message(FATAL_ERROR
        "Set ARM_TOOLCHAIN_BIN to the GNU Arm toolchain bin directory"
    )
endif()

set(ARM_GCC     "${ARM_TOOLCHAIN_BIN}/arm-none-eabi-gcc")
set(ARM_GXX     "${ARM_TOOLCHAIN_BIN}/arm-none-eabi-g++")
set(ARM_OBJCOPY "${ARM_TOOLCHAIN_BIN}/arm-none-eabi-objcopy")
set(ARM_OBJDUMP "${ARM_TOOLCHAIN_BIN}/arm-none-eabi-objdump")
set(ARM_SIZE    "${ARM_TOOLCHAIN_BIN}/arm-none-eabi-size")
set(ARM_GDB     "${ARM_TOOLCHAIN_BIN}/arm-none-eabi-gdb")

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

set(CMAKE_C_COMPILER   "${ARM_GCC}")
set(CMAKE_CXX_COMPILER "${ARM_GXX}")
set(CMAKE_ASM_COMPILER "${ARM_GCC}")

set(CMAKE_OBJCOPY "${ARM_OBJCOPY}")
set(CMAKE_OBJDUMP "${ARM_OBJDUMP}")
set(CMAKE_SIZE    "${ARM_SIZE}")

if(EXISTS "${ARM_GDB}")
    set(CMAKE_GDB "${ARM_GDB}")
endif()

set(MCU_FLAGS
    "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard"
)

set(CMAKE_C_FLAGS_INIT
    "${MCU_FLAGS} -ffunction-sections -fdata-sections -fstack-usage"
)

set(CMAKE_CXX_FLAGS_INIT
    "${MCU_FLAGS} -ffunction-sections -fdata-sections -fstack-usage -fno-exceptions -fno-rtti -fno-threadsafe-statics"
)

set(CMAKE_ASM_FLAGS_INIT
    "${MCU_FLAGS} -x assembler-with-cpp"
)

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
    "-Os -g0"
)

set(CMAKE_CXX_FLAGS_RELEASE_INIT
    "-Os -g0"
)

set(CMAKE_EXE_LINKER_FLAGS_INIT
    "${MCU_FLAGS} -Wl,--gc-sections -Wl,--print-memory-usage"
)

set(CMAKE_EXECUTABLE_SUFFIX ".elf")
