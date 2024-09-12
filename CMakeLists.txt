cmake_minimum_required(VERSION 3.25)

project(tt_native C)

include(bare)
include(napi)

if(NOT TARGET tt)
  add_subdirectory(vendor/libtt EXCLUDE_FROM_ALL)
endif()

add_bare_module(tt_native_bare)

target_sources(
  ${tt_native_bare}
  PRIVATE
    binding.c
  PUBLIC
    $<TARGET_OBJECTS:tt>
)

target_link_libraries(
  ${tt_native_bare}
  PUBLIC
    tt
)

add_napi_module(tt_native_node)

target_sources(
  ${tt_native_node}
  PRIVATE
    binding.c
  PUBLIC
    $<TARGET_OBJECTS:tt>
)

target_link_libraries(
  ${tt_native_node}
  PUBLIC
    tt
)
