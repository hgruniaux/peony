find_package(Python REQUIRED COMPONENTS Interpreter)

file(GLOB_RECURSE TESTSUITE_SOURCES "*.p")
foreach(TESTSUITE_FILE ${TESTSUITE_SOURCES})
    file(RELATIVE_PATH TEST_NAME ${CMAKE_CURRENT_SOURCE_DIR} ${TESTSUITE_FILE})
    string(REPLACE ".p" "" TEST_NAME ${TEST_NAME})
    string(REPLACE "/" "." TEST_NAME ${TEST_NAME})
    string(CONCAT TEST_NAME "testsuite." ${TEST_NAME})
    add_test(NAME ${TEST_NAME}
         COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_runner.py --compiler-exe "$<TARGET_FILE:peony>" --name "${TEST_NAME}" --file "${TESTSUITE_FILE}"
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()
