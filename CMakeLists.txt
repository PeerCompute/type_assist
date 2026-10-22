cmake_minimum_required(VERSION 3.19)
project(type_assist LANGUAGES CXX)

find_package(Qt6 6.11.2 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(type_assist
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

target_link_libraries(type_assist
    PRIVATE
        Qt::Core
        Qt::Widgets
)

target_include_directories(type_assist
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

install(TARGETS type_assist
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET type_assist
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)

install(SCRIPT ${deploy_script})
