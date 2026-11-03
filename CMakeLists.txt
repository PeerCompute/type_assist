cmake_minimum_required(VERSION 3.19)
project(type_assist LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(type_assist
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    filemanager.cpp
    filemanager.h
    Autocomplete.cpp
    Autocomplete.h
    NGramTrie.cpp
    NGramTrie.h
    wordTable.cpp
    wordTable.h
)

target_link_libraries(type_assist
    PRIVATE
        Qt::Core
        Qt::Widgets
)

# Put corpus.txt next to the executable so the app finds it.
add_custom_command(TARGET type_assist POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/corpus.txt
        $<TARGET_FILE_DIR:type_assist>/corpus.txt
)

install(TARGETS type_assist
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES corpus.txt DESTINATION ${CMAKE_INSTALL_BINDIR})

qt_generate_deploy_app_script(
    TARGET type_assist
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
