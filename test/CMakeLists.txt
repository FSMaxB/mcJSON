cmake_minimum_required (VERSION 2.6)

subdirs(test-data)

set(tests tests test-files test_utils)

foreach(test ${tests})
    add_executable(${test} ${test} common)
    target_link_libraries(${test} mcjson mcjson-utils)
    add_test(NAME ${test}
        COMMAND "${CMAKE_CURRENT_BINARY_DIR}/${test}")
    if(NOT ("${MEMORYCHECK_COMMAND}" MATCHES "MEMORYCHECK_COMMAND-NOTFOUND"))
        add_test(NAME "${test}-valgrind"
            COMMAND "${MEMORYCHECK_COMMAND}" ${MEMORYCHECK_COMMAND_OPTIONS} "${CMAKE_CURRENT_BINARY_DIR}/${test}")
    endif(NOT ("${MEMORYCHECK_COMMAND}" MATCHES "MEMORYCHECK_COMMAND-NOTFOUND"))
endforeach(test)
