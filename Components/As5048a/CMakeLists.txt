add_library(as5048a STATIC
    Src/as5048a.cpp
)

target_include_directories(as5048a
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc
)

target_link_libraries(as5048a
    stm32cubemx
)
