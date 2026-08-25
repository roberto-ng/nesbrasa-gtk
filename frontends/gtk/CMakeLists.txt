find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-3.0)

find_program(GLIB_COMPILE_RESOURCES glib-compile-resources REQUIRED)

set(GRESOURCE_XML ${CMAKE_CURRENT_SOURCE_DIR}/src/nesbrasa-gtk.gresource.xml)
set(GRESOURCE_C ${CMAKE_CURRENT_BINARY_DIR}/nesbrasa-gtk-resources.c)

add_custom_command(
    OUTPUT ${GRESOURCE_C}
    COMMAND ${GLIB_COMPILE_RESOURCES}
        --sourcedir ${CMAKE_CURRENT_SOURCE_DIR}/src
        --target ${GRESOURCE_C}
        --generate ${GRESOURCE_XML}
    DEPENDS ${GRESOURCE_XML}
            ${CMAKE_CURRENT_SOURCE_DIR}/src/janela_principal.ui
    VERBATIM
)

add_executable(nesbrasa-gtk
    src/arquivo.cpp
    src/main.cpp
    src/aplicacao.cpp
    src/janela_principal.cpp
    ${GRESOURCE_C}
)

target_sources(nesbrasa-gtk
    PRIVATE
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/nesbrasa.gtk.cppm
)

target_link_libraries(nesbrasa-gtk PRIVATE nesbrasa PkgConfig::GTKMM)
target_include_directories(nesbrasa-gtk PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

install(TARGETS nesbrasa-gtk RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/nesbrasa.nesbrasa.emu.desktop.in
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    RENAME nesbrasa.nesbrasa.emu.desktop
)
install(FILES data/nesbrasa.nesbrasa.emu.appdata.xml.in
    DESTINATION ${CMAKE_INSTALL_DATADIR}/appdata
    RENAME nesbrasa.nesbrasa.emu.appdata.xml
)
install(FILES data/nesbrasa.nesbrasa.emu.gschema.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/glib-2.0/schemas
)
