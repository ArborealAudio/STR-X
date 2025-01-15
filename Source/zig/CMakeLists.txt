cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(LIB_ARTIFACT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/zig-out/lib")

if (UNIX)
  set(LIB_ARTIFACT_NAME "libProcessor.a")
else()
  set(LIB_ARTIFACT_NAME "Processor.lib")
endif()

set(LIB_ARTIFACT "${LIB_ARTIFACT_PATH}/${LIB_ARTIFACT_NAME}")

if (UNIX AND NOT APPLE)
  set(LIB_TARGET "native-linux-musl")
else()
  if (APPLE)
    set (LIB_TARGET "native-macos.11.0.0")
  else ()
    set(LIB_TARGET "native")
  endif()
endif()

add_custom_command(OUTPUT ${LIB_ARTIFACT}
  VERBATIM
  COMMAND zig ARGS build -Doptimize=Debug -Dtarget=${LIB_TARGET}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS processor.zig util.zig build.zig Str_X.zig Str_Y.zig Str_Z.zig
)

add_custom_target(processor_build DEPENDS ${LIB_ARTIFACT})

add_library(processor STATIC IMPORTED GLOBAL)
add_dependencies(processor processor_build)
set_target_properties(processor PROPERTIES
  IMPORTED_LOCATION ${LIB_ARTIFACT}
)
