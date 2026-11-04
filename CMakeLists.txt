cmake_minimum_required(VERSION 3.19)
project(type_assist LANGUAGES CXX)

find_package(Qt6 6.11.2 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(type_assist
    WIN32 MACOSX_BUNDLE

    # GUI files
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

    # Dummy implementations (unused now)
    # dummyspellchecker.h
    # dummyautocompleteengine.h

    # Autocomplete adapter
    AutocompleteEngineAdapter.cpp
    AutocompleteEngineAdapter.h

    # Autocomplete worklet (N-gram model)
    Autocomplete.cpp
    Autocomplete.h
    NGramTrie.cpp
    NGramTrie.h
    wordTable.cpp
    wordTable.h

    # Spell checker worklet
    Spell_Checker/SpellChecker.h
    Spell_Checker/Suggestions.h
    Spell_Checker/NgramInvertedIndex.h
    Spell_Checker/levenshtein+kbrd.h
)

# corpus.txt (autocomplete) and dictionary.txt (spell checker) are read at
# start-up: keep a copy next to the executable so they are found when the
# app runs from the build folder.
foreach(data_file corpus.txt dictionary.txt)
    add_custom_command(TARGET type_assist POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_CURRENT_SOURCE_DIR}/${data_file}
                $<TARGET_FILE_DIR:type_assist>/${data_file}
    )
endforeach()

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

install(FILES corpus.txt dictionary.txt DESTINATION ${CMAKE_INSTALL_BINDIR})
install(SCRIPT ${deploy_script})
