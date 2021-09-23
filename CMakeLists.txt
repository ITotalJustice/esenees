cmake_minimum_required(VERSION 3.18.0)


list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)


project(snes
    VERSION 0.0.1
    DESCRIPTION "snes emulator written by TotalJustice and Kivan117"
    HOMEPAGE_URL "https://github.com/ITotalJustice/esenees"
    LANGUAGES C
)


message("COMPILER: using ${CMAKE_C_COMPILER_ID}")

# SEE: https://cmake.org/cmake/help/latest/manual/cmake-compile-features.7.html#supported-compilers
list(APPEND cmake_supported_compiler_features
    AppleClang
    Clang
    GNU
    MSVC
    SunPro
    Intel
)

if (${CMAKE_C_COMPILER_ID} IN_LIST cmake_supported_compiler_features)
    set(COMPILER_FEATURES ON)
    message("compiler features are enabled for this compiler")
else()
    set(COMPILER_FEATURES OFF)
    message("compiler features are NOT enabled for this compiler")
endif()


option(SNES_DEBUG "enable debug" OFF)
option(SNES_DEV "enables debug and sanitizers" OFF)

if (SNES_DEV)
    set(SNES_DEBUG ON)

    find_program(CLANGD_INSTALLED clangd)
    if (CLANGD_INSTALLED)
        set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
        message("found clangd: generating compile_commands.json to ${CMAKE_BINARY_DIR}/compile_commands.json")
    else()
        message("cant find clangd: not generating compile_commands.json")
    endif()
endif()

add_subdirectory(src)

# this main file is just for testing (load a rom)
add_executable(snes main.c)
target_link_libraries(snes LINK_PRIVATE libsnes)
