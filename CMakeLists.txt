cmake_minimum_required (VERSION 3.8)

project("peony")

add_library(peony_lib STATIC
    "src/utils/hedley.h"

    "src/utils/bump_allocator.c"
    "src/utils/bump_allocator.h"
    "src/utils/diag.h"
    "src/utils/diag.c"
    "src/utils/diag_formatter.h"
    "src/utils/diag_formatter.c"
    "src/utils/diag_formatter_extra.c"
    "src/utils/dynamic_array.h"
    "src/utils/dynamic_array.c"
    "src/utils/hash_table_common.h"
    "src/utils/hash_table_common.c"
    "src/utils/line_map.h"
    "src/utils/line_map.c"
    "src/utils/source_location.h"
    "src/utils/source_file.h"
    "src/utils/source_file.c"
    "src/utils/source_location.c"

    "src/ast.h"
    "src/ast.c"
    "src/codegen_llvm.h"
    "src/codegen_llvm.c"
    "src/identifier_table.c"
    "src/identifier_table.h"
    "src/lexer.h"
    "src/lexer.c"
    "src/lexer_re2c.c"
    "src/parser.c"
    "src/parser.h"
    "src/scope.h"
    "src/scope.c"
    "src/sema.h"
    "src/sema.c"
    "src/token.c"
    "src/token.h"
    "src/token_kind.c"
    "src/token_kind.h"
    "src/token_kinds.def"
    "src/type.h"
    "src/type.c"
)

target_compile_definitions(peony_lib PUBLIC "-DP_DEBUG")

add_executable(peony "src/main.c")
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
        OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/src/lexer_re2c.c"
        COMMAND ${RE2C_PROGRAM}
        ARGS "${CMAKE_CURRENT_SOURCE_DIR}/src/lexer_re2c.re" -o "${CMAKE_CURRENT_SOURCE_DIR}/src/lexer_re2c.c"
        ARGS -W -Werror
        ARGS --no-version --no-generation-date --no-debug-info
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/lexer_re2c.re"  
    )

    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/diag_formatter.c"
        COMMAND ${RE2C_PROGRAM}
        ARGS "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/diag_formatter.re" -o "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/diag_formatter.c"
        ARGS -W -Werror
        ARGS --no-version --no-generation-date --no-debug-info
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/diag_formatter.re"  
    )
endif()

# LLVM:
find_package(LLVM CONFIG REQUIRED)

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(HandleLLVMOptions)
add_definitions(${LLVM_DEFINITIONS})

target_include_directories(peony_lib PUBLIC ${LLVM_INCLUDE_DIRS})

# Find the libraries that correspond to the LLVM components that we wish to use
llvm_map_components_to_libnames(llvm_libs support core irreader x86codegen passes )

# Link against LLVM libraries
target_link_libraries(peony_lib PUBLIC ${llvm_libs})

# Testing:
enable_testing()
include(GoogleTest)
find_package(GTest CONFIG REQUIRED)

add_executable(peony_test
    "src/main_test.cxx"

    "src/utils/line_map_test.cxx"
    "src/utils/diag_formatter_test.cxx"
    "src/utils/dynamic_array_test.cxx"
    "src/type_test.cxx"
    "src/identifier_table_test.cxx"
)

target_link_libraries(peony_test PRIVATE peony_lib)
target_link_libraries(peony_test PRIVATE GTest::gtest)
gtest_discover_tests(peony_test)

add_subdirectory(test)

