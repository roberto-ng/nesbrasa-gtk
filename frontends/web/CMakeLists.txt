add_executable(nesbrasa-web
    wasm/nesbrasa_web.cpp
)

target_link_libraries(nesbrasa-web PRIVATE nesbrasa)

set_target_properties(nesbrasa-web PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/public
    OUTPUT_NAME nesbrasa
)

target_compile_options(nesbrasa-web PRIVATE -O2)
target_link_options(nesbrasa-web PRIVATE
    -sMODULARIZE=1
    -sEXPORT_ES6=1
    -sENVIRONMENT=web
    -sALLOW_MEMORY_GROWTH=1
    --bind
    -sEXPORTED_RUNTIME_METHODS=HEAPU32
)
