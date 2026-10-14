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
