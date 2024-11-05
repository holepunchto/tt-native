cmake_minimum_required(VERSION 3.25)

find_package(cmake-bare REQUIRED PATHS node_modules/cmake-bare)
find_package(cmake-napi REQUIRED PATHS node_modules/cmake-napi)

project(tt_native C)

if(NOT TARGET tt)
  add_subdirectory(vendor/libtt EXCLUDE_FROM_ALL)
endif()

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
