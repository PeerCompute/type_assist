cmake_minimum_required(VERSION 3.19)
project(type_assist LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(type_assist
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
<<<<<<< HEAD
    texthighlighter.cpp
    texthighlighter.h
    ispellchecker.h
=======
    filemanager.cpp
    filemanager.h
>>>>>>> 5fa9a59fe134625e64349c8a64d7be1d8f766a4a
)

target_link_libraries(type_assist
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS type_assist
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET type_assist
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})