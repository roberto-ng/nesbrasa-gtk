add_library(nesbrasa SHARED
    src/nesbrasa.cpp
    src/cores.cpp
    src/util.cpp
    src/controle.cpp
    src/cpu.cpp
    src/instrucao.cpp
    src/memoria.cpp
    src/ppu.cpp
    src/mapeadores/cartucho.cpp
    src/mapeadores/nrom.cpp
)

set_target_properties(nesbrasa PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
)

target_include_directories(nesbrasa PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/nesbrasa>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(TARGETS nesbrasa
    EXPORT nesbrasaTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(BUILD_TESTING)
    add_executable(jmp_bug tests/jmp_bug.cpp)
    target_link_libraries(jmp_bug PRIVATE nesbrasa)
    add_test(NAME "Testar bug do JMP indireto" COMMAND jmp_bug)

    add_executable(api tests/api.cpp)
    target_link_libraries(api PRIVATE nesbrasa)
    add_test(NAME "Testar API do frontend" COMMAND api)
endif()
