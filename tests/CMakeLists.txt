# Unit tests
macro(add_octos_test test_name source_file)
    add_executable(${test_name} ${source_file})
    target_link_libraries(${test_name} PRIVATE
        octos_core
        Qt6::Test
        Qt6::Core
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endmacro()

add_octos_test(test_compiler_strategies unit/test_compiler_strategies.cpp)
add_octos_test(test_assembly_processor unit/test_assembly_processor.cpp)
add_octos_test(test_job_manager unit/test_job_manager.cpp)
add_octos_test(test_docker_security unit/test_docker_security.cpp)
add_octos_test(test_settings_and_files unit/test_settings_and_files.cpp)
add_octos_test(test_docker_integration integration/test_docker_integration.cpp)

# 0.4.0 Diff and Highlighters test
add_executable(test_diff_and_highlighters
    unit/test_diff_and_highlighters.cpp
    ../src/highlighters/RustHighlighter.cpp
    ../src/highlighters/PythonHighlighter.cpp
)
target_link_libraries(test_diff_and_highlighters PRIVATE
    octos_core
    Qt6::Test
    Qt6::Core
    Qt6::Gui
)
target_include_directories(test_diff_and_highlighters PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/highlighters
)
add_test(NAME test_diff_and_highlighters COMMAND test_diff_and_highlighters)
