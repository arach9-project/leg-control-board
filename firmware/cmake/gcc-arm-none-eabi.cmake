set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Locate GNU Arm Embedded tools
find_program(ARM_GCC     arm-none-eabi-gcc     REQUIRED)
find_program(ARM_GXX     arm-none-eabi-g++     REQUIRED)
find_program(ARM_OBJCOPY arm-none-eabi-objcopy REQUIRED)
find_program(ARM_OBJDUMP arm-none-eabi-objdump REQUIRED)
find_program(ARM_SIZE    arm-none-eabi-size    REQUIRED)
find_program(ARM_GDB     arm-none-eabi-gdb)

# Compilers
set(CMAKE_C_COMPILER   "${ARM_GCC}")
set(CMAKE_CXX_COMPILER "${ARM_GXX}")
set(CMAKE_ASM_COMPILER "${ARM_GCC}")

# Binary utilities
set(CMAKE_OBJCOPY "${ARM_OBJCOPY}")
set(CMAKE_OBJDUMP "${ARM_OBJDUMP}")
set(CMAKE_SIZE    "${ARM_SIZE}")

if(ARM_GDB)
    set(CMAKE_GDB "${ARM_GDB}")
endif()

# STM32G431 / Cortex-M4F architecture
set(MCU_FLAGS
    "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard"
)

# Initial compile flags
set(CMAKE_C_FLAGS_INIT
    "${MCU_FLAGS} -ffunction-sections -fdata-sections -fstack-usage"
)

set(CMAKE_CXX_FLAGS_INIT
    "${MCU_FLAGS} -ffunction-sections -fdata-sections -fstack-usage -fno-exceptions -fno-rtti -fno-threadsafe-statics"
)

set(CMAKE_ASM_FLAGS_INIT
    "${MCU_FLAGS} -x assembler-with-cpp"
)

# Build-type-specific flags
set(CMAKE_C_FLAGS_DEBUG_INIT
    "-Og -g3"
)

set(CMAKE_CXX_FLAGS_DEBUG_INIT
    "-Og -g3"
)

set(CMAKE_ASM_FLAGS_DEBUG_INIT
    "-g3"
)

set(CMAKE_C_FLAGS_RELEASE_INIT
    "-Os -g0"
)

set(CMAKE_CXX_FLAGS_RELEASE_INIT
    "-Os -g0"
)

# Architecture flags must also be used when linking
set(CMAKE_EXE_LINKER_FLAGS_INIT
    "${MCU_FLAGS} -Wl,--gc-sections -Wl,--print-memory-usage"
)

# Bare-metal executable suffix
set(CMAKE_EXECUTABLE_SUFFIX ".elf")
