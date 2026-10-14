cmake_minimum_required(VERSION 3.16)
project(OCTOS VERSION 0.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

option(BUILD_TESTING "Build automated tests" ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network)

# Core library (business logic, strategies, docker backend, job management, settings)
set(OCTOS_CORE_SOURCES
    src/core/AppSettings.cpp
    src/strategies/StrategyUtils.cpp
    src/strategies/GccStrategy.cpp
    src/strategies/ClangStrategy.cpp
    src/strategies/RustStrategy.cpp
    src/strategies/JavaStrategy.cpp
    src/strategies/PythonStrategy.cpp
    src/strategies/CSharpStrategy.cpp
    src/strategies/AdaStrategy.cpp
    src/strategies/LanguageStrategyRegistry.cpp
    src/backend/DockerExecutionBackend.cpp
    src/backend/MockExecutionBackend.cpp
    src/backend/JobManager.cpp
    src/backend/DockerCompilerManager.cpp
    src/managers/RecentFilesManager.cpp
    src/managers/SnippetManager.cpp
    src/utils/AssemblyTextProcessor.cpp
    src/utils/AssemblyDiffProcessor.cpp
    src/utils/OpcodeReference.cpp
)

set(OCTOS_CORE_HEADERS
    src/core/CompilerEnums.h
    src/core/Diagnostic.h
    src/core/CompilationRequest.h
    src/core/CompilationResult.h
    src/core/CompilerDefinition.h
    src/core/AppSettings.h
    src/strategies/ILanguageStrategy.h
    src/strategies/StrategyUtils.h
    src/strategies/GccStrategy.h
    src/strategies/ClangStrategy.h
    src/strategies/RustStrategy.h
    src/strategies/JavaStrategy.h
    src/strategies/PythonStrategy.h
    src/strategies/CSharpStrategy.h
    src/strategies/AdaStrategy.h
    src/strategies/LanguageStrategyRegistry.h
    src/backend/DockerExecutionConfig.h
    src/backend/IExecutionBackend.h
    src/backend/DockerExecutionBackend.h
    src/backend/MockExecutionBackend.h
    src/backend/JobManager.h
    src/backend/DockerCompilerManager.h
    src/managers/RecentFilesManager.h
    src/managers/SnippetManager.h
    src/utils/AssemblyTextProcessor.h
    src/utils/AssemblyDiffProcessor.h
    src/utils/OpcodeReference.h
)

add_library(octos_core STATIC ${OCTOS_CORE_SOURCES} ${OCTOS_CORE_HEADERS})

target_link_libraries(octos_core PUBLIC
    Qt6::Core
    Qt6::Network
)

target_include_directories(octos_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/strategies
    ${CMAKE_CURRENT_SOURCE_DIR}/src/backend
    ${CMAKE_CURRENT_SOURCE_DIR}/src/managers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
)

# Main Application executable
set(OCTOS_UI_SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/ui/CompilerOutputPane.cpp
    src/ui/CompilerManagerDialog.cpp
    src/ui/DiffDialog.cpp
    src/ui/Theme.cpp
    src/highlighters/CppHighlighter.cpp
    src/highlighters/AsmHighlighter.cpp
    src/highlighters/RustHighlighter.cpp
    src/highlighters/PythonHighlighter.cpp
)

set(OCTOS_UI_HEADERS
    src/MainWindow.h
    src/ui/CompilerOutputPane.h
    src/ui/CompilerManagerDialog.h
    src/ui/DiffDialog.h
    src/ui/Theme.h
    src/highlighters/CppHighlighter.h
    src/highlighters/AsmHighlighter.h
    src/highlighters/RustHighlighter.h
    src/highlighters/PythonHighlighter.h
)

set(OCTOS_RESOURCES
    resources.qrc
)

add_executable(${PROJECT_NAME} ${OCTOS_UI_SOURCES} ${OCTOS_UI_HEADERS} ${OCTOS_RESOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    octos_core
    Qt6::Widgets
    Qt6::Gui
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/highlighters
)

# Automated Tests
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    enable_testing()
    add_subdirectory(tests)
endif()
