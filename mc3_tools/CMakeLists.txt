cmake_minimum_required(VERSION 3.30)

set(CMAKE_CXX_STANDARD 20)

project(mc3-tools)

add_subdirectory(emulator/gui-lib)

add_subdirectory(C_compiler/c-compiler-lib)

add_executable(mc3as assembler/main.cpp)

add_executable(mc3cc C_compiler/main.cpp)

add_executable(mc3dump disassembler/main.cpp)

add_executable(mc3emu emulator/main.cpp)

target_link_libraries(mc3cc PRIVATE c-compiler-lib)

target_link_libraries(mc3emu PRIVATE gui-lib sfml-graphics sfml-window sfml-system sfml-audio)

configure_file(emulator/PublicPixel.ttf PublicPixel.ttf COPYONLY)
