add_library(xenon_ui STATIC
    main_window.cpp
    editor_widget.cpp
)

target_include_directories(xenon_ui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_compile_options(xenon_ui PRIVATE ${GTK_CFLAGS_OTHER} ${GTKMM_CFLAGS_OTHER} ${GTKSOURCEVIEW_CFLAGS_OTHER})

target_link_libraries(xenon_ui
    PUBLIC
        xenon_core
        ${GTK_LIBRARIES}
        ${GTKMM_LIBRARIES}
        ${GTKSOURCEVIEW_LIBRARIES}
        ${GTKSOURCEVIEWMM_LIBRARIES}
)
