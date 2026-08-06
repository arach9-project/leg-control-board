cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#
#

# Setup compiler settings
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME leg_control_board)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME}
    LANGUAGES C CXX ASM
)

enable_language(C CXX ASM)

message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

set(LINKER_SCRIPT
    "${CMAKE_SOURCE_DIR}/STM32G431xx_FLASH.ld"
)

if(NOT EXISTS "${LINKER_SCRIPT}")
    message(FATAL_ERROR "Linker script not found: ${LINKER_SCRIPT}")
endif()

target_link_options(leg_control_board PRIVATE
    "-T${LINKER_SCRIPT}"
    "-Wl,-Map=${CMAKE_PROJECT_NAME}.map"
    "-Wl,--gc-sections"
    "-Wl,--print-memory-usage"
    "--specs=nano.specs"
    "--specs=nosys.specs"
    "-u_printf_float"
)

set_target_properties(leg_control_board PROPERTIES
    SUFFIX ".elf"
)

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)
add_subdirectory(Components/DRV8316)
add_subdirectory(Components/AS5048A)
add_subdirectory(Components/FOC9)
add_subdirectory(Platform)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
    # FOC9
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE 
  Core/Src/swo.c
  Core/Src/current_sense.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    # Add user defined libraries
    drv8316
    as5048a
    foc9

)



