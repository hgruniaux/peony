cmake_minimum_required (VERSION 3.8)

project("peony")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(pstd)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_library(peony_lib STATIC
    "thirdparty/hedley/hedley.h"

        "src/utils/bump_allocator.hxx"
    "src/utils/diag.hxx"
    "src/utils/diag.cxx"
    "src/utils/diag_formatter.hxx"
    "src/utils/diag_formatter.cxx"
    "src/utils/diag_formatter_extra.cxx"
        "src/utils/hash_table_common.hxx"
    "src/utils/hash_table_common.cxx"
    "src/utils/line_map.hxx"
    "src/utils/line_map.cxx"
    "src/utils/source_location.hxx"
    "src/utils/source_file.hxx"
    "src/utils/source_file.cxx"
    "src/utils/source_location.cxx"

        "src/ast/ast.hxx"
        "src/identifier_table.cxx"
    "src/identifier_table.hxx"
    "src/lexer.hxx"
    "src/lexer.cxx"
    "src/lexer_re2c.cxx"
    "src/parser.cxx"
    "src/parser.hxx"
    "src/scope.hxx"
    "src/scope.cxx"
    "src/sema.hxx"
    "src/sema.cxx"
    "src/token.hxx"
    "src/token_kind.cxx"
    "src/token_kind.hxx"
    "src/token_kind.def"
    "src/type.hxx"
    "src/type.cxx"
    "src/options.hxx"
    "src/options.cxx"
    "src/literal_parser.hxx"
    "src/literal_parser.cxx"
        src/context.hxx src/context.cxx src/ast/ast_visitor.hxx src/ast/ast_printer.cxx src/ast/ast_printer.hxx src/codegen_llvm.cxx src/codegen_llvm.hxx src/ast/ast_expr.hxx src/ast/ast_stmt.hxx src/ast/ast_decl.hxx src/ast/ast_expr.cxx src/ast/ast_decl.cxx src/ast/ast_stmt.cxx src/utils/array_view.hxx src/interpreter/value.hxx src/interpreter/value.cxx src/interpreter/interpreter.cxx src/interpreter/interpreter.hxx)

target_compile_options(peony_lib PUBLIC -fsanitize=address,undefined)
target_link_options(peony_lib PUBLIC -fsanitize=address,undefined)

target_compile_definitions(peony_lib PUBLIC "-DP_DEBUG")
target_include_directories(peony_lib PUBLIC "thirdparty/hedley")
target_include_directories(peony_lib PUBLIC "src")

add_executable(peony "src/driver/main.cxx" "src/driver/cmdline_parser.cxx")
target_link_libraries(peony PRIVATE peony_lib)

if(MSCV)
    target_compile_options(peony /W4 /WX)
elif(CLANG)
    target_compile_options(peony -Wall)
endif()

# Generate lexer code
find_program(RE2C_PROGRAM re2c)
if(RE2C_PROGRAM)
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/src/lexer_re2c.cxx"
        COMMAND ${RE2C_PROGRAM}
        ARGS "${CMAKE_CURRENT_SOURCE_DIR}/src/lexer_re2c.re" -o "${CMAKE_CURRENT_SOURCE_DIR}/src/lexer_re2c.cxx"
        ARGS -W -Werror
        ARGS --no-version --no-generation-date --no-debug-info
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/lexer_re2c.re"
    )

    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/diag_formatter.cxx"
        COMMAND ${RE2C_PROGRAM}
        ARGS "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/diag_formatter.re" -o "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/diag_formatter.cxx"
        ARGS -W -Werror
        ARGS --no-version --no-generation-date --no-debug-info
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/diag_formatter.re"
    )
endif()

# LLVM:
find_package(LLVM 15.0 CONFIG REQUIRED)
target_include_directories(peony_lib PUBLIC ${LLVM_INCLUDE_DIRS})

target_include_directories(peony_lib PRIVATE ${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
target_compile_definitions(peony_lib PRIVATE ${LLVM_DEFINITIONS_LIST})

llvm_map_components_to_libnames(llvm_libs support core irreader passes x86asmparser x86codegen x86desc x86disassembler x86info)
target_link_libraries(peony_lib PUBLIC ${llvm_libs})

# Testing:
enable_testing()
include(InstallGoogleTest)

add_executable(peony_test

        "src/utils/line_map_test.cxx"
    "src/utils/diag_formatter_test.cxx"
        "src/identifier_table_test.cxx"
    "src/type_test.cxx"
    "src/lexer_test.cxx"
 "src/literal_parser_test.cxx" src/interpreter/interpreter_test.cxx)

target_link_libraries(peony_test PRIVATE peony_lib)
target_link_libraries(peony_test PRIVATE gtest gtest_main)
gtest_discover_tests(peony_test)
