add_library(xenon_features STATIC
    search_engine.cpp
)

target_link_libraries(xenon_features PUBLIC Qt6::Core)
