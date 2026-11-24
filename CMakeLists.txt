cmake_minimum_required(VERSION 3.25)

find_package(cmake-bare REQUIRED PATHS node_modules/cmake-bare)
find_package(cmake-fetch REQUIRED PATHS node_modules/cmake-fetch)
find_package(cmake-napi REQUIRED PATHS node_modules/cmake-napi)
find_package(cmake-npm REQUIRED PATHS node_modules/cmake-npm)

project(tt_native C)

fetch_package("github:holepunchto/libtt#73c9c93")

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

target_compile_definitions(
  ${tt_native_node}
  PRIVATE
    NAPI_VERSION=9
)

resolve_node_module(bare-compat-napi compat)

target_include_directories(
  ${tt_native_node}
  PRIVATE
    "${compat}/include"
)
