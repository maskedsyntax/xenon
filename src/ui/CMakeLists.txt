add_library(xenon_ui STATIC
    main_window.cpp
    editor_widget.cpp
    split_pane_container.cpp
    quick_open_dialog.cpp
    search_replace_dialog.cpp
    file_explorer.cpp
    terminal_widget.cpp
    status_bar.cpp
    theme_manager.cpp
    command_palette.cpp
)

target_include_directories(xenon_ui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_compile_options(xenon_ui PRIVATE ${GTK_CFLAGS_OTHER} ${GTKMM_CFLAGS_OTHER} ${GTKSOURCEVIEW_CFLAGS_OTHER})

target_link_libraries(xenon_ui
    PUBLIC
        xenon_core
        xenon_lsp
        xenon_git
        ${GTK_LIBRARIES}
        ${GTKMM_LIBRARIES}
        ${GTKSOURCEVIEW_LIBRARIES}
        ${GTKSOURCEVIEWMM_LIBRARIES}
)

if(VTE_FOUND)
    target_compile_options(xenon_ui PRIVATE ${VTE_CFLAGS_OTHER})
    target_link_libraries(xenon_ui PUBLIC ${VTE_LIBRARIES})
endif()
