file(GLOB_RECURSE FOC9_CPP_SOURCES
    CONFIGURE_DEPENDS
    Src/*.cpp
)

file(GLOB_RECURSE FOC9_C_SOURCES
    CONFIGURE_DEPENDS
    Src/*.c
)

add_library(foc9 STATIC
    ${FOC9_CPP_SOURCES}
    ${FOC9_C_SOURCES}
)

target_include_directories(foc9
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc

    PRIVATE
        ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include
        ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/DSP/Include
        ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Core/Include
)

target_compile_features(foc9
    PUBLIC
        cxx_std_17
)

set_target_properties(foc9 PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_compile_definitions(foc9
    PUBLIC
        ARM_MATH_CM4
        ARM_MATH_ROUNDING
        __FPU_PRESENT=1U
        __PROGRAM_START=1
)

find_library(CMSIS_DSP_LIB
    NAMES
        arm_cortexM4lf_math
        arm_cortexM4f_math

    PATHS
        ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/DSP/Lib/GCC
        ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Lib/GCC
)

if(NOT CMSIS_DSP_LIB)
    message(FATAL_ERROR "CMSIS-DSP library not found")
endif()

target_link_libraries(foc9
    PUBLIC
        ${CMSIS_DSP_LIB}
)
