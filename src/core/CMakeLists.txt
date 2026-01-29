add_library(xenon_core STATIC
    document.cpp
    edit_history.cpp
    text_position.cpp
    text_range.cpp
    file_manager.cpp
)

target_include_directories(xenon_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_compile_options(xenon_core PRIVATE ${GTK_CFLAGS_OTHER})
