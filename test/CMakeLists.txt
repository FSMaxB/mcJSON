cmake_minimum_required (VERSION 2.6)

set(tests tests test_utils)

foreach(test ${tests})
    add_executable(${test} ${test})
    target_link_libraries(${test} mcjson mcjson-utils)
    add_test(NAME ${test}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMAND "${CMAKE_CURRENT_BINARY_DIR}/${test}")
    if(NOT ("${MEMORYCHECK_COMMAND}" MATCHES "MEMORYCHECK_COMMAND-NOTFOUND"))
        add_test(NAME "${test}-valgrind"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            COMMAND "${MEMORYCHECK_COMMAND}" ${MEMORYCHECK_COMMAND_OPTIONS} "${CMAKE_CURRENT_BINARY_DIR}/${test}")
    endif(NOT ("${MEMORYCHECK_COMMAND}" MATCHES "MEMORYCHECK_COMMAND-NOTFOUND"))
endforeach(test)
