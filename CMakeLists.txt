cmake_minimum_required(VERSION 3.28)
project(megattt)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD_REQUIRED ON)

function(setup_target_warnings_and_sanitizers target)
  target_compile_options(${target} PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wshadow -fdiagnostics-color=always>
    $<$<C_COMPILER_ID:MSVC>:/W4>
  )
  set(SANITIZER_EXPR $<$<AND:$<C_COMPILER_ID:GNU,Clang>,$<NOT:$<PLATFORM_ID:Windows>>,$<CONFIG:Debug>>:-fsanitize=address,undefined,leak>)
  target_compile_options(${target} PRIVATE ${SANITIZER_EXPR})
  target_link_options(${target} PRIVATE ${SANITIZER_EXPR})
endfunction()

# fetch an compile raylib
include(FetchContent)

FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5
)

FetchContent_MakeAvailable(raylib)

FetchContent_Declare(
  raygui
  GIT_REPOSITORY https://github.com/raysan5/raygui.git
  GIT_TAG 4.0
)

FetchContent_MakeAvailable(raygui)

add_library(raygui INTERFACE)
target_include_directories(raygui INTERFACE ${raygui_SOURCE_DIR}/src)

add_executable(megattt
  src/client.c
  src/client_net.c
  src/raygui_impl.c
)
add_executable(megattt-server
  src/server.c
  src/buffer_control.c
)

target_link_libraries(megattt PRIVATE raylib raygui)
setup_target_warnings_and_sanitizers(megattt)

