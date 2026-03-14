add_library(xenon_git STATIC
    git_manager.cpp
)

target_link_libraries(xenon_git 
    PUBLIC 
    Qt6::Core 
    PkgConfig::LIBGIT2
)
