# threadx.cmake - builds Eclipse ThreadX from the ./threadx submodule and
# links it into the CubeMX-generated executable.
# Include this from the root CMakeLists.txt AFTER add_executable().

set(THREADX_ARCH      "cortex_m3")
set(THREADX_TOOLCHAIN "gnu")
set(TX_USER_FILE      "${CMAKE_SOURCE_DIR}/app/tx_user.h")

# ThreadX's CMakeLists predates CMake 4.x; harmless on older CMake.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

add_subdirectory(${CMAKE_SOURCE_DIR}/threadx ${CMAKE_BINARY_DIR}/threadx)

# Some ThreadX releases compile the port's example tx_initialize_low_level.S
# into the library. We supply our own (board clock + Cube linker symbols),
# so strip theirs if present to avoid duplicate symbols.
get_target_property(_tx_srcs threadx SOURCES)
list(FILTER _tx_srcs EXCLUDE REGEX "tx_initialize_low_level")
set_target_properties(threadx PROPERTIES SOURCES "${_tx_srcs}")

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/app/tx_initialize_low_level.S
    ${CMAKE_SOURCE_DIR}/app/app.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/app)

target_link_libraries(${CMAKE_PROJECT_NAME} threadx)