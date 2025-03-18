cmake_minimum_required(VERSION 3.30)

set(CMAKE_CXX_STANDARD 20)

project(mc3-tools)

add_subdirectory(emulator/gui-lib)

add_executable(mc3as assembler/main.cpp)

add_executable(C_compiler C_compiler/main.cpp)

add_executable(mc3dump disassembler/main.cpp)

add_executable(mc3emu emulator/main.cpp)

target_link_libraries(mc3emu PRIVATE gui-lib sfml-graphics sfml-window sfml-system sfml-audio)

configure_file(emulator/3270NerdFontMono-Regular.ttf 3270NerdFontMono-Regular.ttf COPYONLY)
