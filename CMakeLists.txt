cmake_minimum_required(VERSION 3.16)
project(song2notes LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "song2notes использует WinAPI и собирается только под Windows "
                         "(нативно на Windows или кросс-компиляцией через MinGW-w64).")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB CORE_SOURCES ${CMAKE_SOURCE_DIR}/core/*.cpp)
file(GLOB GUI_SOURCES ${CMAKE_SOURCE_DIR}/gui/*.cpp)

add_executable(song2notes WIN32
    main.cpp
    ${CORE_SOURCES}
    ${GUI_SOURCES}
)

target_compile_definitions(song2notes PRIVATE UNICODE _UNICODE)
target_link_libraries(song2notes PRIVATE comdlg32 comctl32 gdi32 user32 kernel32)

if(MSVC)
    target_compile_options(song2notes PRIVATE /utf-8 /W4)
else()
    target_compile_options(song2notes PRIVATE -municode -Wall -Wextra)
    target_link_options(song2notes PRIVATE -municode -static -static-libgcc -static-libstdc++)
endif()
