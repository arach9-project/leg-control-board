add_library(drv8316 STATIC
    Src/drv8316.cpp
)

target_include_directories(drv8316
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc
)

target_link_libraries(drv8316
    stm32cubemx
)
