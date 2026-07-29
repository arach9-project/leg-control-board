file(GLOB CPP_SOURCES "Src/*.cpp")
file(GLOB C_SOURCES "Src/*.c")

add_library(foc9 STATIC ${CPP_SOURCES} ${C_SOURCES})

# Add C++17 Standard
target_compile_features(foc9 PUBLIC cxx_std_17)
set_target_properties(foc9 PROPERTIES 
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_include_directories(foc9
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include
        ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/DSP/Include
        ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Core/Include
)

target_compile_definitions(foc9
    PUBLIC
        ARM_MATH_CM4
        ARM_MATH_ROUNDING
        __FPU_PRESENT=1U
        __PROGRAM_START=1
)

# --- FIND AND LINK CMSIS DSP LIBRARY ---
# Find the CMSIS-DSP static library provided in Drivers/CMSIS/DSP/Lib or Drivers/CMSIS/Lib
find_library(CMSIS_DSP_LIB
    NAMES arm_cortexM4lf_math arm_cortexM4f_math
    PATHS 
        ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/DSP/Lib/GCC
        ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Lib/GCC
)

target_link_libraries(foc9
    PUBLIC
        stm32cubemx
        ${CMSIS_DSP_LIB}  # <--- LINK CMSIS DSP HERE
)
