add_library(nesbrasa SHARED
)

set(NESBRASA_MODULES
    src/modules/types.cppm
    src/modules/ports.cppm
    src/modules/memory.cppm
    src/modules/instruction.cppm
    src/modules/cpu.cppm
    src/modules/ppu.cppm
    src/modules/controller.cppm
    src/modules/cartridge.cppm
    src/modules/mapper.nrom.cppm
    src/modules/cores.cppm
    src/modules/util.cppm
    src/modules/nes.cppm
    src/modules/nesbrasa.cppm
)

target_sources(nesbrasa
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src/modules
        FILES ${NESBRASA_MODULES}
)

set_target_properties(nesbrasa PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
)

install(TARGETS nesbrasa
    EXPORT nesbrasaTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    FILE_SET cxx_modules DESTINATION ${CMAKE_INSTALL_LIBDIR}/cxx/nesbrasa
)

if(BUILD_TESTING)
    add_executable(jmp_bug tests/jmp_bug.cpp)
    target_link_libraries(jmp_bug PRIVATE nesbrasa)
    add_test(NAME "Testar bug do JMP indireto" COMMAND jmp_bug)

    add_executable(api tests/api.cpp)
    target_link_libraries(api PRIVATE nesbrasa)
    add_test(NAME "Testar API do frontend" COMMAND api)
endif()
