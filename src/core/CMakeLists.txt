add_library(xenon_core STATIC
    document.cpp
    edit_history.cpp
    file_manager.cpp
    text_position.cpp
    text_range.cpp
)

target_link_libraries(xenon_core PUBLIC Qt6::Core)
