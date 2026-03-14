add_library(xenon_services STATIC
    settings_manager.cpp
)

target_link_libraries(xenon_services PUBLIC Qt6::Core)
