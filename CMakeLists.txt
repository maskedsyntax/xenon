cmake_minimum_required(VERSION 3.16)
project(Xenon VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt 6 Setup
find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core Network Concurrent)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(APPLE)
    set(MACOSX_BUNDLE ON)
endif()

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Find LibGit2
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBGIT2 REQUIRED libgit2 IMPORTED_TARGET)

add_compile_definitions(
    $<$<BOOL:${LIBGIT2_FOUND}>:HAVE_LIBGIT2>
)

add_subdirectory(src)

enable_testing()
