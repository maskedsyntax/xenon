cmake_minimum_required(VERSION 3.10)
project(Xenon VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED gtk+-3.0)
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)
pkg_check_modules(GTKSOURCEVIEW gtksourceview-3.0)
if(NOT GTKSOURCEVIEW_FOUND)
    pkg_check_modules(GTKSOURCEVIEW gtksourceview-4)
endif()
pkg_check_modules(GTKSOURCEVIEWMM gtksourceviewmm-3.0)
if(NOT GTKSOURCEVIEWMM_FOUND)
    pkg_check_modules(GTKSOURCEVIEWMM gtksourceviewmm-4)
endif()
pkg_check_modules(VTE vte-2.91)
if(NOT VTE_FOUND)
    pkg_check_modules(VTE vte-2.90)
endif()
pkg_check_modules(LIBGIT2 libgit2)

add_compile_definitions(
    $<$<BOOL:${GTKSOURCEVIEW_FOUND}>:HAVE_GTKSOURCEVIEW>
    $<$<BOOL:${GTKSOURCEVIEWMM_FOUND}>:HAVE_GTKSOURCEVIEWMM>
    $<$<BOOL:${VTE_FOUND}>:HAVE_VTE>
    $<$<BOOL:${LIBGIT2_FOUND}>:HAVE_LIBGIT2>
)

include_directories(
    ${GTK_INCLUDE_DIRS}
    ${GTKMM_INCLUDE_DIRS}
)

if(GTKSOURCEVIEW_FOUND)
    include_directories(${GTKSOURCEVIEW_INCLUDE_DIRS})
endif()
if(GTKSOURCEVIEWMM_FOUND)
    include_directories(${GTKSOURCEVIEWMM_INCLUDE_DIRS})
endif()
if(VTE_FOUND)
    include_directories(${VTE_INCLUDE_DIRS})
endif()
if(LIBGIT2_FOUND)
    include_directories(${LIBGIT2_INCLUDE_DIRS})
endif()

link_directories(
    ${GTK_LIBRARY_DIRS}
    ${GTKMM_LIBRARY_DIRS}
)

if(GTKSOURCEVIEW_FOUND)
    link_directories(${GTKSOURCEVIEW_LIBRARY_DIRS})
endif()
if(GTKSOURCEVIEWMM_FOUND)
    link_directories(${GTKSOURCEVIEWMM_LIBRARY_DIRS})
endif()
if(VTE_FOUND)
    link_directories(${VTE_LIBRARY_DIRS})
endif()
if(LIBGIT2_FOUND)
    link_directories(${LIBGIT2_LIBRARY_DIRS})
endif()

add_subdirectory(src)

enable_testing()
