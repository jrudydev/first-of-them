cmake_minimum_required(VERSION 3.10)
project(FirstOfThem VERSION 2.0.0 LANGUAGES C)

# C99 standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find SDL2
find_package(SDL2 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

# Source files
set(SOURCES
    src/main.c
    src/game.c
    src/renderer.c
)

# Executable
add_executable(FirstOfThem ${SOURCES})

# Include directories
target_include_directories(FirstOfThem PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(FirstOfThem PRIVATE
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    m
)

# AGINUZ Seed (from environment or cmake variable)
# This changes the memory layout for anti-cheat protection
if(DEFINED ENV{AGINUZ_SEED})
    target_compile_definitions(FirstOfThem PRIVATE AGINUZ_SEED=$ENV{AGINUZ_SEED})
    message(STATUS "AGINUZ Seed: $ENV{AGINUZ_SEED} (PROTECTED BUILD)")
elseif(DEFINED AGINUZ_SEED)
    target_compile_definitions(FirstOfThem PRIVATE AGINUZ_SEED=${AGINUZ_SEED})
    message(STATUS "AGINUZ Seed: ${AGINUZ_SEED} (PROTECTED BUILD)")
else()
    message(STATUS "AGINUZ Seed: 0 (UNPROTECTED BUILD - for CheatEngine demo)")
endif()

# Debug output
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(FirstOfThem PRIVATE DEBUG=1)
endif()

# Copy resources to build directory
add_custom_command(TARGET FirstOfThem POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/../resources
    ${CMAKE_BINARY_DIR}/../resources
    COMMENT "Copying game resources..."
)

# Print build info
message(STATUS "")
message(STATUS "First of Them - SDL2 Port")
message(STATUS "Original: YC Hacks 2014 (Pebble Watch)")
message(STATUS "Port: 2026 (SDL2 + AGINUZ Protection)")
message(STATUS "")
