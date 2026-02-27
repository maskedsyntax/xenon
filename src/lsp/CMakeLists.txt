add_library(xenon_lsp STATIC
    json_rpc.cpp
    lsp_client.cpp
)

target_include_directories(xenon_lsp
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(xenon_lsp PUBLIC pthread)
