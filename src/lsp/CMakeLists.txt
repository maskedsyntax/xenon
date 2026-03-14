add_library(xenon_lsp STATIC
    json_rpc.cpp
    lsp_client.cpp
)

target_link_libraries(xenon_lsp PUBLIC Qt6::Core Qt6::Network)
