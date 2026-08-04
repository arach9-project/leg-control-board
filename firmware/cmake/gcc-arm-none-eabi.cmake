set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

find_program(ARM_GCC arm-none-eabi-gcc REQUIRED)
find_program(ARM_GXX arm-none-eabi-g++ REQUIRED)
find_program(ARM_AS arm-none-eabi-gcc REQUIRED)
find_program(ARM_OBJCOPY arm-none-eabi-objcopy REQUIRED)
find_program(ARM_SIZE arm-none-eabi-size REQUIRED)

set(CMAKE_C_COMPILER ${ARM_GCC})
set(CMAKE_CXX_COMPILER ${ARM_GXX})
set(CMAKE_ASM_COMPILER ${ARM_AS})

set(CMAKE_OBJCOPY ${ARM_OBJCOPY})
set(CMAKE_SIZE ${ARM_SIZE})

set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)
