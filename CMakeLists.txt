cmake_minimum_required(VERSION 3.16)
project(tamagotchi-p1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ---------------------------------------------------------------------------
# Compiler (C-subset -> E0C6S46 ROM)
# ---------------------------------------------------------------------------
add_executable(tama_compiler
  compiler/src/main.cpp
  compiler/src/parser.cpp
  compiler/src/lexer.cpp
  compiler/src/font.cpp
  compiler/src/sprites_p1.cpp
  compiler/src/codegen.cpp
  compiler/src/assembler.cpp
  compiler/src/c90.cpp
)
target_include_directories(tama_compiler PRIVATE compiler/src)

# ---------------------------------------------------------------------------
# Emulator ohne Display (Shell/Konsole, portabel)
# ---------------------------------------------------------------------------
add_executable(tamagotchi-headless
  emulator/src/cpu.cpp
  emulator/src/os.cpp
  emulator/src/headless_main.cpp
)
target_include_directories(tamagotchi-headless PRIVATE emulator/src)

# ---------------------------------------------------------------------------
# Emulator mit SDL-Fenster (optional: wird nur gebaut, wenn SDL3 gefunden)
# ---------------------------------------------------------------------------
find_package(SDL3 QUIET)
if(SDL3_FOUND)
  add_executable(tamagotchi
    emulator/src/cpu.cpp
    emulator/src/display.cpp
    emulator/src/feed.cpp
    emulator/src/os.cpp
    emulator/src/main.cpp
  )
  target_include_directories(tamagotchi PRIVATE emulator/src)
  target_link_libraries(tamagotchi PRIVATE SDL3::SDL3)
endif()

if(WIN32)
  if(TARGET tamagotchi)
    target_link_libraries(tamagotchi PRIVATE ws2_32)
  endif()
endif()

# ---------------------------------------------------------------------------
# Beispiel-ROM (c_ext_demo) mit dem Compiler bauen
# ---------------------------------------------------------------------------
set(EXAMPLE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/examples/c_ext_demo/main.c)
set(EXAMPLE_ROM ${CMAKE_CURRENT_BINARY_DIR}/c_ext_demo.bin)
add_custom_command(
  OUTPUT ${EXAMPLE_ROM}
  COMMAND $<TARGET_FILE:tama_compiler> ${EXAMPLE_SRC} -o ${EXAMPLE_ROM}
  DEPENDS tama_compiler ${EXAMPLE_SRC}
  COMMENT "Building example ROM (C-subset -> E0C6S46)"
)
add_custom_target(example-rom ALL DEPENDS ${EXAMPLE_ROM})
