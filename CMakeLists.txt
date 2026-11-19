cmake_minimum_required(VERSION 3.20)
project(retrocast C)

# Release unless asked otherwise: tests, timings and playable speed assume it.
# (Multi-config generators such as Visual Studio: cmake --build build --config Release;
# without it, cmake --build always asks MSBuild for Debug.)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Programs in build/ with every generator (the $<> stops Visual Studio adding
# Release/ or Debug/): NAME.misses is written next to the program and read
# from here, and the tests look for build/NAME.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_BINARY_DIR}>)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -Wextra)
endif()

set(GEN_DIR ${CMAKE_BINARY_DIR}/gen)
set(NES_DIR ${CMAKE_SOURCE_DIR}/nes)

# Shared by every system: input recordings and the pad bits (core/).
add_library(core STATIC core/input_log.c)
target_include_directories(core PUBLIC core)
# Instruction templates for the recompilers (build-time tools).
add_library(recomp_template STATIC core/recomp/template.c)
target_include_directories(recomp_template PUBLIC core/recomp)

# Options were named NES_* before: a build folder configured then keeps its
# values under the new names (unless given anew).
foreach(old NES_GAMES NES_EMBED_ROM NES_AUTO_HINTS NES_AUTO_HINTS_FRAMES NES_FRONTEND NES_SDL3_PREFIX)
    if(DEFINED CACHE{${old}})
        string(REPLACE "NES_GAMES" "ROMS" new "${old}")
        string(REPLACE "NES_" "" new "${new}")
        if(NOT DEFINED CACHE{${new}})
            set(${new} "${${old}}" CACHE STRING "")
        endif()
        unset(${old} CACHE)
        message(STATUS "${old} is now ${new}")
    endif()
endforeach()

# SDL3 frontend (zlib license), linked statically; turn off with
# -DFRONTEND=OFF for headless builds (CI, tests). A prebuilt SDL3 in
# SDL3_PREFIX (the release kit ships one in sdl3/) is used when present;
# otherwise SDL3 is fetched and built (cmake/sdl3_options.cmake).
option(FRONTEND "Build the SDL3 frontend" ON)
set(SDL3_PREFIX "${CMAKE_SOURCE_DIR}/sdl3" CACHE PATH "Prebuilt static SDL3 (install prefix)")
if(FRONTEND)
    # only this prefix: a system SDL3 (often shared-only) is never picked up
    find_package(SDL3 CONFIG QUIET COMPONENTS SDL3-static PATHS "${SDL3_PREFIX}" NO_DEFAULT_PATH)
    if(NOT SDL3_FOUND)
        include(FetchContent)
        include(cmake/sdl3_options.cmake)
        FetchContent_Declare(SDL3
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG release-3.4.20
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(SDL3)
    endif()
    # A game window (no console window on Windows) running the system in
    # `system_src` (core/frontend/system.h); settings are read from
    # TARGET.ini next to the program, created on first run.
    function(frontend target system_src)
        add_executable(${target} WIN32 ${CMAKE_SOURCE_DIR}/core/frontend/sdl3_main.c
            ${CMAKE_SOURCE_DIR}/core/frontend/settings.c ${CMAKE_SOURCE_DIR}/core/frontend/tv_frame.c ${system_src})
        target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/core/frontend)
        target_compile_definitions(${target} PRIVATE APP_NAME="${target}")
        target_link_libraries(${target} PRIVATE core SDL3::SDL3-static)
    endfunction()
endif()

# ---- Recompiled programs ----
# Options for every system's recompile function (nes/CMakeLists.txt:
# nes_recompile): AUTO_HINTS traces each game in the interpreter first,
# EMBED_ROM puts the ROM in the program.
option(AUTO_HINTS "Trace each game in the interpreter before recompiling it" ON)
option(EMBED_ROM "Embed the ROM in recompiled programs" ON)
set(AUTO_HINTS_FRAMES 6000 CACHE STRING "Frames of scripted play traced by AUTO_HINTS")

# Your own games (never committed): -DROMS="a.nes;b.fds;c.gb;d.gbc" (.fds needs FDS_BIOS).
# Target name = file name without "(...)"/"[...]" tags, lower case,
# non-alphanumerics -> '_' (e.g. "Arkista's Ring (U) [!].nes" -> arkista_s_ring);
# hints are read from nes/games/<name>.toml or gb/games/<name>.toml when present.
set(ROMS "" CACHE STRING "ROM files (.nes, .fds, .gb, .gbc) to recompile (semicolon-separated)")
# Relative paths are relative to the folder cmake was run from (for the
# release kit that is the archive folder, not kit/), else to this source
# folder. CMake has no variable for the former, but a cmake -P child sees it
# as CMAKE_SOURCE_DIR. The absolute paths go back into the cache, so later
# re-configures (from cmake --build, an IDE) resolve nothing again.
set(FDS_BIOS "" CACHE FILEPATH "FDS BIOS (8 KB) to recompile into fds and fds_headless")
if(ROMS OR FDS_BIOS)
    file(WRITE ${CMAKE_BINARY_DIR}/print_cwd.cmake
        "execute_process(COMMAND \"\${CMAKE_COMMAND}\" -E echo \"\${CMAKE_SOURCE_DIR}\")\n")
    execute_process(COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/print_cwd.cmake
        OUTPUT_VARIABLE cwd OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
if(FDS_BIOS)  # the same resolution as ROMS below
    get_filename_component(from_cwd "${FDS_BIOS}" ABSOLUTE BASE_DIR "${cwd}")
    get_filename_component(from_src "${FDS_BIOS}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")
    if(EXISTS "${from_cwd}")
        set(FDS_BIOS "${from_cwd}" CACHE FILEPATH "FDS BIOS (8 KB) to recompile into fds and fds_headless" FORCE)
    elseif(EXISTS "${from_src}")
        set(FDS_BIOS "${from_src}" CACHE FILEPATH "FDS BIOS (8 KB) to recompile into fds and fds_headless" FORCE)
    else()
        message(FATAL_ERROR "FDS_BIOS: not found: ${FDS_BIOS} (looked in ${cwd} and ${CMAKE_SOURCE_DIR})")
    endif()
endif()
if(ROMS)
    set(absolute_roms "")
    foreach(rom IN LISTS ROMS)
        get_filename_component(from_cwd "${rom}" ABSOLUTE BASE_DIR "${cwd}")
        get_filename_component(from_src "${rom}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")
        if(EXISTS "${from_cwd}")
            list(APPEND absolute_roms "${from_cwd}")
        elseif(EXISTS "${from_src}")
            list(APPEND absolute_roms "${from_src}")
        else()
            message(FATAL_ERROR "ROMS: not found: ${rom} (looked in ${cwd} and ${CMAKE_SOURCE_DIR})")
        endif()
    endforeach()
    set(ROMS "${absolute_roms}" CACHE STRING "ROM files (.nes, .fds, .gb, .gbc) to recompile (semicolon-separated)" FORCE)
endif()

# Warnings as errors (CI): our code only, SDL3 above is not affected.
option(WERROR "Treat compiler warnings as errors" OFF)
if(WERROR)
    add_compile_options($<IF:$<C_COMPILER_ID:MSVC>,/WX,-Werror>)
endif()

# The systems to build (CI builds the ones a change touches): nes, gb.
set(SYSTEMS "nes;gb" CACHE STRING "Systems to build (semicolon-separated): nes, gb")
foreach(sys IN LISTS SYSTEMS)
    if(NOT sys MATCHES "^(nes|gb)$")
        message(FATAL_ERROR "SYSTEMS: unknown system ${sys} (nes, gb)")
    endif()
    add_subdirectory(${sys} ${CMAKE_BINARY_DIR}/sys_${sys})  # not build/<sys>: that is the program (no .exe off Windows)
endforeach()

foreach(rom IN LISTS ROMS)
    get_filename_component(stem "${rom}" NAME_WLE)
    string(REGEX REPLACE "[ ]*[[(][^])]*[])]" "" stem "${stem}")  # drop (U), [!], ...
    string(TOLOWER "${stem}" name)
    string(REGEX REPLACE "[^a-z0-9]+" "_" name "${name}")
    string(REGEX REPLACE "^_+|_+$" "" name "${name}")
    get_filename_component(ext "${rom}" LAST_EXT)
    string(TOLOWER "${ext}" ext)
    set(sys nes)
    if(ext STREQUAL ".gb" OR ext STREQUAL ".gbc")
        set(sys gb)
    endif()
    if(NOT sys IN_LIST SYSTEMS)
        message(FATAL_ERROR "ROMS: ${rom} is a ${sys} game, but SYSTEMS is ${SYSTEMS}")
    elseif(sys STREQUAL "gb")
        gb_recompile(${name} "${rom}" ${CMAKE_SOURCE_DIR}/gb/games/${name}.toml)
    else()
        nes_recompile(${name} "${rom}" ${NES_DIR}/games/${name}.toml)
        list(APPEND game_names ${name})
    endif()
endforeach()
# for tests/nes/test_regression.py: the games this build recompiles
list(JOIN game_names "\n" game_lines)
file(WRITE ${CMAKE_BINARY_DIR}/nes_games.txt "${game_lines}\n")
