add_library(foc STATIC
    Src/current_sense.cpp
    Src/debug.cpp
    Src/encoder.cpp
    Src/foc_driver.cpp
    Src/foc_utils.cpp
    Src/focmotor.cpp
    Src/lowpass_filter.cpp
    Src/pid.cpp
    Src/time_utils.cpp
)

target_include_directories(foc
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc
)

target_link_libraries(foc
    stm32cubemx
)
