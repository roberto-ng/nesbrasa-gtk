set(NESBRASA_CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/core/src/cores.cpp
    ${CMAKE_SOURCE_DIR}/core/src/controle.cpp
    ${CMAKE_SOURCE_DIR}/core/src/cpu.cpp
    ${CMAKE_SOURCE_DIR}/core/src/instrucao.cpp
    ${CMAKE_SOURCE_DIR}/core/src/memoria.cpp
    ${CMAKE_SOURCE_DIR}/core/src/nesbrasa.cpp
    ${CMAKE_SOURCE_DIR}/core/src/ppu.cpp
    ${CMAKE_SOURCE_DIR}/core/src/util.cpp
    ${CMAKE_SOURCE_DIR}/core/src/mapeadores/cartucho.cpp
    ${CMAKE_SOURCE_DIR}/core/src/mapeadores/nrom.cpp
)

add_executable(nesbrasa-web
    ${NESBRASA_CORE_SOURCES}
    wasm/nesbrasa_web.cpp
)

set_target_properties(nesbrasa-web PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/public
    OUTPUT_NAME nesbrasa
)

target_include_directories(nesbrasa-web PRIVATE
    ${CMAKE_SOURCE_DIR}/core/include/nesbrasa
    ${CMAKE_SOURCE_DIR}/core/src
    ${CMAKE_SOURCE_DIR}/core/src/mapeadores
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
