cmake_minimum_required(VERSION 3.25)

find_package(cmake-bare REQUIRED PATHS node_modules/cmake-bare)
find_package(cmake-fetch REQUIRED PATHS node_modules/cmake-fetch)
find_package(cmake-napi REQUIRED PATHS node_modules/cmake-napi)

project(tt_native C)

fetch_package("github:holepunchto/libtt#bfcf3b3")

add_bare_module(tt_native_bare)

target_sources(
  ${tt_native_bare}
  PRIVATE
    binding.c
)

target_link_libraries(
  ${tt_native_bare}
  PRIVATE
    $<TARGET_OBJECTS:tt>
  PUBLIC
    tt
)

add_napi_module(tt_native_node)

target_sources(
  ${tt_native_node}
  PRIVATE
    binding.c
)

target_link_libraries(
  ${tt_native_node}
  PRIVATE
    $<TARGET_OBJECTS:tt>
  PUBLIC
    tt
)
