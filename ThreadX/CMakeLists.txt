# Keep board integration outside CubeMX-generated sources and upstream ThreadX.
project(threadx LANGUAGES C ASM)

add_library(threadx STATIC)

# These unmodified upstream lists deliberately exclude the example application,
# startup, vector table, and linker script shipped with the port.
set(THREADX_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../ThirdParty/ThreadX")
add_subdirectory("${THREADX_SOURCE_DIR}/common" "${CMAKE_CURRENT_BINARY_DIR}/common")
add_subdirectory("${THREADX_SOURCE_DIR}/ports/cortex_m3/gnu" "${CMAKE_CURRENT_BINARY_DIR}/port")

target_include_directories(threadx PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_compile_definitions(threadx PUBLIC
    TX_INCLUDE_USER_DEFINE_FILE
    $<$<CONFIG:Debug>:DEBUG>
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/app_threadx.c"
    "${CMAKE_CURRENT_LIST_DIR}/tx_initialize_low_level.S"
)
# Match the plain signature used by CubeMX for the firmware target.
target_link_libraries(${CMAKE_PROJECT_NAME} threadx)
