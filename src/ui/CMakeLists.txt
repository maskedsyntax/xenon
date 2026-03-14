add_library(xenon_ui STATIC
    main_window.cpp
    file_explorer.cpp
    editor_widget.cpp
    terminal_widget.cpp
    syntax_highlighter.cpp
    command_palette.cpp
    style_manager.cpp
    find_replace_widget.cpp
)

target_link_libraries(xenon_ui 
    PUBLIC 
    Qt6::Widgets 
    Qt6::Gui 
    Qt6::Core
    xenon_core
    xenon_services
    xenon_git
    xenon_lsp
)
