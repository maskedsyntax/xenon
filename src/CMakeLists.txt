# Add the src directory to the include path for all subdirectories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(core)
add_subdirectory(services)
add_subdirectory(git)
add_subdirectory(lsp)
add_subdirectory(features)
add_subdirectory(ui)

add_executable(xenon
    main.cpp
)

target_link_libraries(xenon
    PRIVATE
    xenon_ui
    xenon_features
    xenon_lsp
    xenon_git
    xenon_core
    xenon_services
    Qt6::Widgets
)
