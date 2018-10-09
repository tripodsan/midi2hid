cmake_minimum_required(VERSION 3.7.2)
project(midi2hid C)

set(CMAKE_C_STANDARD 99)

add_executable (test_gadget src/test_gadget.c)
add_executable (midi-listen src/midi-listen.c)
add_executable (midi2hid src/midi2hid.c)
add_executable (test src/test.c)

target_link_libraries (midi-listen asound)
target_link_libraries (midi2hid asound pthread)
