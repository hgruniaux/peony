cmake_minimum_required (VERSION 3.8)

project("peony")

add_executable(peony
    "src/hedley.h"

    "src/bump_allocator.c"
    "src/bump_allocator.h"
    "src/identifier_table.c"
    "src/identifier_table.h"
    "src/lexer.h"
    "src/lexer_re2c.c"
    "src/main.c"
    "src/token.c"
    "src/token.h"
    "src/token_kind.c"
    "src/token_kind.h"
    "src/token_kinds.def"
    "src/lexer.c"
    "src/ast.h"
    "src/ast.c"
    "src/parser.c"
 "src/parser.h" "src/type.h" "src/type.c" "src/scope.h"   "src/scope.c" "src/detail/hash_table_common.h" "src/detail/hash_table_common.c" "src/codegen_llvm.h" "src/codegen_llvm.c" "src/utils/dynamic_array.h" "src/utils/dynamic_array.c" "src/utils/diag.h" "src/utils/diag.c" "src/sema.h" "src/sema.c")

if(MSCV)
    target_compile_options(peony /W4 /WX)
endif()

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
endif()

# LLVM:
find_package(LLVM CONFIG REQUIRED)

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(HandleLLVMOptions)
add_definitions(${LLVM_DEFINITIONS})

target_include_directories(peony PRIVATE ${LLVM_INCLUDE_DIRS})

# Find the libraries that correspond to the LLVM components that we wish to use
llvm_map_components_to_libnames(llvm_libs support core irreader x86codegen passes )

# Link against LLVM libraries
target_link_libraries(peony PRIVATE ${llvm_libs})
