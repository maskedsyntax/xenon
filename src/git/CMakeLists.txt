add_library(xenon_git STATIC
    git_manager.cpp
)

target_include_directories(xenon_git
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

if(LIBGIT2_FOUND)
    target_compile_options(xenon_git PRIVATE ${LIBGIT2_CFLAGS_OTHER})
    target_link_libraries(xenon_git PUBLIC ${LIBGIT2_LIBRARIES})
endif()
