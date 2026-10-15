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
add_octos_test(test_docker_backend unit/test_docker_backend.cpp)
add_octos_test(test_diff_engine unit/test_diff_engine.cpp)
add_octos_test(test_result_presentation unit/test_result_presentation.cpp)
add_octos_test(test_settings_and_files unit/test_settings_and_files.cpp)
add_octos_test(test_docker_integration integration/test_docker_integration.cpp)
add_octos_test(test_host_compilers integration/test_host_compilers.cpp)
add_octos_test(test_docker_flags integration/test_docker_flags.cpp)
add_octos_test(test_parser_and_workspace unit/test_parser_and_workspace.cpp)
add_octos_test(test_asm_parser unit/test_asm_parser.cpp)
add_octos_test(test_workspace unit/test_workspace.cpp)
add_octos_test(test_fuzz_properties unit/test_fuzz_properties.cpp)
target_compile_definitions(test_fuzz_properties PRIVATE OCTOS_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
add_octos_test(test_project_controller unit/test_project_controller.cpp)
add_octos_test(test_registry_and_probe unit/test_registry_and_probe.cpp)
target_link_libraries(test_registry_and_probe PRIVATE Qt6::Network)
target_compile_definitions(test_asm_parser PRIVATE OCTOS_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

# GUI tests link the same octos_ui library as the application (offscreen platform plugin)
macro(add_octos_gui_test test_name source_file)
    add_executable(${test_name} ${source_file})
    target_link_libraries(${test_name} PRIVATE octos_ui Qt6::Test)
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endmacro()

add_octos_gui_test(test_diff_and_highlighters unit/test_diff_and_highlighters.cpp)
add_octos_gui_test(test_diff_dialog unit/test_diff_dialog.cpp)
add_octos_gui_test(test_theme unit/test_theme.cpp)
add_octos_gui_test(test_main_window unit/test_main_window.cpp)
add_octos_gui_test(test_project_ui unit/test_project_ui.cpp)
target_compile_definitions(test_project_ui PRIVATE OCTOS_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

# libFuzzer targets (need a clang with the fuzzer runtime, e.g. Homebrew LLVM):
#   cmake -S . -B build-fuzz -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ -DOCTOS_BUILD_FUZZERS=ON
option(OCTOS_BUILD_FUZZERS "Build libFuzzer targets" OFF)
if(OCTOS_BUILD_FUZZERS)
    foreach(target asm_parser operand_parser diff_engine workspace_json compiler_args)
        add_executable(fuzz_${target} fuzz/fuzz_${target}.cpp)
        target_link_libraries(fuzz_${target} PRIVATE octos_core Qt6::Core)
        target_include_directories(fuzz_${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        target_compile_options(fuzz_${target} PRIVATE -fsanitize=fuzzer-no-link,address,undefined -fno-omit-frame-pointer -g)
        target_link_options(fuzz_${target} PRIVATE -fsanitize=fuzzer,address,undefined)
    endforeach()
endif()
