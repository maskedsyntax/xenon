add_library(xenon_services STATIC
    settings_manager.cpp
)

target_include_directories(xenon_services
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_compile_options(xenon_services PRIVATE ${GTK_CFLAGS_OTHER})
