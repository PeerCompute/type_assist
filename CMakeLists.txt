cmake_minimum_required(VERSION 3.19)
project(btech_project LANGUAGES CXX)

find_package(Qt6 6.11.2 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(btech_project
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    texthighlighter.cpp
    texthighlighter.h
    ispellchecker.h
    filemanager.cpp
    filemanager.h
    suggestionpopup.h
    suggestionpopup.cpp
    textedit.h
    textedit.cpp
    iAutocompleteEngine.h
    #dummyspellchecker.h
    #dummyautocompleteengine.h
    SpellChecker/SpellChecker.h
    SpellChecker/Suggestions.h
    SpellChecker/NgramInvertedIndex.h
    SpellChecker/levenshtein+kbrd.h
)

target_link_libraries(btech_project
    PRIVATE
        Qt::Core
        Qt::Widgets
)

target_include_directories(btech_project
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

install(TARGETS btech_project
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET btech_project
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)

install(SCRIPT ${deploy_script})
