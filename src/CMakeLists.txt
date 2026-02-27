add_subdirectory(core)
add_subdirectory(features)
add_subdirectory(services)
add_subdirectory(lsp)
add_subdirectory(ui)

add_executable(xenon main.cpp)

target_link_libraries(xenon
    PRIVATE
        xenon_ui
        xenon_services
        xenon_features
        xenon_core
        xenon_lsp
        ${GTK_LIBRARIES}
        ${GTKMM_LIBRARIES}
)

if(GTKSOURCEVIEW_FOUND)
    target_link_libraries(xenon PRIVATE ${GTKSOURCEVIEW_LIBRARIES})
endif()

if(GTKSOURCEVIEWMM_FOUND)
    target_link_libraries(xenon PRIVATE ${GTKSOURCEVIEWMM_LIBRARIES})
endif()

if(VTE_FOUND)
    target_link_libraries(xenon PRIVATE ${VTE_LIBRARIES})
endif()

if(LIBGIT2_FOUND)
    target_link_libraries(xenon PRIVATE ${LIBGIT2_LIBRARIES})
endif()
