add_library(nesbrasa SHARED
    src/cores.cpp
    src/controle.cpp
    src/cpu.cpp
    src/instrucao.cpp
    src/memoria.cpp
    src/nesbrasa.cpp
    src/ppu.cpp
    src/util.cpp
    src/mapeadores/cartucho.cpp
    src/mapeadores/nrom.cpp
)

target_sources(nesbrasa
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES src/nesbrasa.core.cppm
)

set_target_properties(nesbrasa PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
)

target_include_directories(nesbrasa
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nesbrasa>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/nesbrasa>
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/mapeadores
)

install(TARGETS nesbrasa
    EXPORT nesbrasaTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    FILE_SET cxx_modules DESTINATION ${CMAKE_INSTALL_LIBDIR}/cxx/nesbrasa
)

install(DIRECTORY include/nesbrasa/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nesbrasa)
install(DIRECTORY src/mapeadores/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nesbrasa/mapeadores
    FILES_MATCHING PATTERN "*.hpp"
)

if(BUILD_TESTING)
    add_executable(jmp_bug tests/jmp_bug.cpp)
    target_link_libraries(jmp_bug PRIVATE nesbrasa)
    add_test(NAME "Testar bug do JMP indireto" COMMAND jmp_bug)

    add_executable(api tests/api.cpp)
    target_link_libraries(api PRIVATE nesbrasa)
    add_test(NAME "Testar API do frontend" COMMAND api)
endif()
