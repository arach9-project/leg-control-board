
file(GLOB PLATFORM_SOURCES
    CONFIGURE_DEPENDS
    Src/*.c
    Src/*.cpp
)

target_sources(${PROJECT_NAME}
    PRIVATE
        ${PLATFORM_SOURCES}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        Inc
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Inc
)
