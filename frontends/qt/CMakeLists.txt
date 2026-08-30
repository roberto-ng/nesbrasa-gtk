find_package(Qt6 COMPONENTS Widgets REQUIRED)

qt_add_executable(nesbrasa-qt
    src/arquivo.cpp
    src/arquivo.hpp
    src/janela_principal.cpp
    src/janela_principal.hpp
    src/main.cpp
)

target_link_libraries(nesbrasa-qt PRIVATE nesbrasa Qt6::Widgets)
target_include_directories(nesbrasa-qt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

set_target_properties(nesbrasa-qt PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS nesbrasa-qt
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
