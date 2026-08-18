cmake_minimum_required(VERSION 3.16)
project(OCTOS VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/CompilerDriver.cpp
    src/CompilerDetector.cpp
    src/backend/DockerCompilerManager.cpp
    src/highlighters/AsmHighlighter.cpp
    src/highlighters/CppHighlighter.cpp
    src/ui/CompilerSelectDialog.cpp
    src/ui/CompilerManagerDialog.cpp
    src/ui/CompilerOutputPane.cpp
    src/ui/Theme.cpp
    src/ui/ManageCompilersDialog.cpp
    src/ui/PythonInstallWizard.cpp
    src/ui/CompilerStatusWidget.cpp
    src/ui/AsyncCompiler.cpp
    src/strategies/CppStrategy.cpp
    src/strategies/JavaStrategy.cpp
    src/strategies/CSharpStrategy.cpp
    src/strategies/PythonStrategy.cpp
    src/strategies/RustStrategy.cpp
    src/managers/PackageManager.cpp
    src/managers/RecentFilesManager.cpp
    src/managers/SnippetManager.cpp
    src/utils/AssemblyTextProcessor.cpp
)

set(HEADERS
    src/MainWindow.h
    src/CompilerDriver.h
    src/CompilerDetector.h
    src/backend/DockerCompilerManager.h
    src/highlighters/AsmHighlighter.h
    src/highlighters/CppHighlighter.h
    src/ui/CompilerSelectDialog.h
    src/ui/CompilerManagerDialog.h
    src/ui/CompilerOutputPane.h
    src/ui/Theme.h
    src/ui/ManageCompilersDialog.h
    src/ui/PythonInstallWizard.h
    src/ui/CompilerStatusWidget.h
    src/ui/AsyncCompiler.h
    src/strategies/ILanguageStrategy.h
    src/strategies/CppStrategy.h
    src/strategies/JavaStrategy.h
    src/strategies/CSharpStrategy.h
    src/strategies/PythonStrategy.h
    src/strategies/RustStrategy.h
    src/managers/PackageManager.h
    src/managers/RecentFilesManager.h
    src/managers/SnippetManager.h
    src/utils/AssemblyTextProcessor.h
    src/ui/AsyncCompiler.h
)

set(RESOURCES
    resources.qrc
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Network)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/backend
    ${CMAKE_CURRENT_SOURCE_DIR}/src/strategies
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/managers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/highlighters
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
)
