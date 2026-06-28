cmake_minimum_required(VERSION 3.16)
project(RandomPicker LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W3 /utf-8)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN _WINSOCKAPI_)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

option(PICKER_ENABLE_XLSX "Enable XLSX support via libzip" ON)

set(PICKER_SOURCES
    src/main.cpp
    src/reader.cpp
    src/picker.cpp
    src/cli.cpp
    src/ai.cpp
    src/http.cpp
    src/logger.cpp
    src/gui.cpp
)

set(PICKER_HEADERS
    include/common.h
    include/reader.h
    include/picker.h
    include/cli.h
    include/ai.h
    include/http.h
    include/logger.h
    include/gui.h
)

add_executable(random_picker ${PICKER_SOURCES} ${PICKER_HEADERS})

target_include_directories(random_picker PRIVATE include)

if(PICKER_ENABLE_XLSX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBZIP REQUIRED IMPORTED_TARGET libzip)
    target_compile_definitions(random_picker PRIVATE PICKER_WITH_XLSX=1)
    target_link_libraries(random_picker PRIVATE PkgConfig::LIBZIP)
endif()

if(WIN32)
    target_link_libraries(random_picker PRIVATE
        comctl32
        shell32
        ole32
        user32
        gdi32
        winhttp
        advapi32
    )
    if(NOT MSVC)
        target_link_options(random_picker PRIVATE
            -static-libgcc
            -static-libstdc++
        )
    endif()
endif()

set_target_properties(random_picker PROPERTIES
    WIN32_EXECUTABLE OFF
    MACOSX_BUNDLE OFF
)
