add_library(xenon_features STATIC
    search_engine.cpp
)

target_include_directories(xenon_features
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_compile_options(xenon_features PRIVATE ${GTK_CFLAGS_OTHER})
