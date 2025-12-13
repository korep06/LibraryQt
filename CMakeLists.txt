cmake_minimum_required(VERSION 3.16)

project(LibraryQt VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets PrintSupport Test Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets PrintSupport Test Core Sql)

# Основные исходники приложения
set(PROJECT_SOURCES
    Logger.h
    Logger.cpp

    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui

    startmenu.cpp
    startmenu.h
    startmenu.ui

    BookModel.cpp
    BookModel.h

    ReaderModel.cpp
    ReaderModel.h

    Exception.cpp
    Exception.h

    img-button.qrc

    DataBaseManager.cpp
    DataBaseManager.h

    InputValid.cpp
    InputValid.h
)

#Для логгирования
include_directories(${CMAKE_SOURCE_DIR})

# --- Исполняемый файл приложения ---
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(LibraryQt
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        DataBaseManager.h
        DataBaseManager.cpp
        InputValid.h
        InputValid.cpp

    )
else()
    if(ANDROID)
        add_library(LibraryQt SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(LibraryQt
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(LibraryQt PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::PrintSupport
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql
)

# --- Тесты для BookModel ---
add_executable(BookModelTests
    tests/test_bookmodel.cpp
    BookModel.cpp
    BookModel.h
    Exception.cpp
    Exception.h
    DataBaseManager.cpp
    DataBaseManager.h
)

target_link_libraries(BookModelTests PRIVATE
    Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql
)

target_include_directories(BookModelTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# --- Тесты для ReaderModel ---
add_executable(ReaderModelTests
    tests/test_readermodel.cpp
    ReaderModel.cpp
    ReaderModel.h
    Exception.cpp
    Exception.h
    DataBaseManager.cpp
    DataBaseManager.h
)

target_link_libraries(ReaderModelTests PRIVATE
    Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql
)

target_include_directories(ReaderModelTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)




# --- Свойства приложения (как в шаблоне Qt Creator) ---
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.LibraryQt)
endif()

set_target_properties(LibraryQt PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS LibraryQt
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(LibraryQt)
endif()
