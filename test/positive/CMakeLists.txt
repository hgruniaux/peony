function(add_positive_test INPUT)
    set(options FAIL)
    set(oneValueArgs)
    set(multiValueArgs)
    cmake_parse_arguments(ADD_POSITIVE_TEST "${options}"
            "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_test(NAME positive.${INPUT}
            COMMAND ${CMAKE_COMMAND}
            -DPEONY_EXE=$<TARGET_FILE:peony>
            -DINPUT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/${INPUT}.peony
            -DOUTPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/${INPUT}.exe
            -DEXPECT_FAIL=${ADD_POSITIVE_TEST_FAIL}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/runtest.cmake
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endfunction()

add_positive_test(assert_fail FAIL)
add_positive_test(assert_success)
add_positive_test(barebone)
add_positive_test(even_odd)
add_positive_test(fib)
add_positive_test(function_call)
add_positive_test(if_then)
add_positive_test(if_else)
add_positive_test(lazy_and)
add_positive_test(lazy_or)
add_positive_test(bool_operators)
add_positive_test(variables)
add_positive_test(mutable_variables)
add_positive_test(return)
add_positive_test(i32_arithmetic)
add_positive_test(break_in_loop)
add_positive_test(continue_in_loop)
add_positive_test(while_loop)
