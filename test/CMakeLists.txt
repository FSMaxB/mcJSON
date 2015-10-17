cmake_minimum_required (VERSION 2.6)

set(tests tests test_utils)

foreach(test ${tests})
    add_executable(${test} ${test})
    target_link_libraries(${test} cjson cjson-utils)
    add_test(${test} "./${test}")
    if(NOT ("${MEMORYCHECK_COMMAND}" MATCHES "MEMORYCHECK_COMMAND-NOTFOUND"))
        add_test("${test}-valgrind" ${MEMORYCHECK_COMMAND} ${MEMORYCHECK_COMMAND_OPTIONS} "./${test}")
    endif(NOT ("${MEMORYCHECK_COMMAND}" MATCHES "MEMORYCHECK_COMMAND-NOTFOUND"))
endforeach(test)
