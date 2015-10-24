cmake_minimum_required (VERSION 2.6)

#needs to be add_subdirectory instead of subdirs
#because this ensures that it get's processed first
#and the variables are actually set.
add_subdirectory(test-data)

#generic tests
set(tests test-objects test_utils)

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

#file tests
add_executable(test-file test-file common)
target_link_libraries(test-file mcjson)
foreach(json-file ${json-files})
    add_test(NAME "test-file-${json-file}"
        COMMAND "${CMAKE_CURRENT_BINARY_DIR}/test-file" "test-data/${json-file}" "test-data/${json-file}.out")
    if(NOT ("${MEMORYCHECK_COMMAND}" MATCHES "MEMORYCHECK_COMMAND-NOTFOUND"))
        add_test(NAME "test-file-${json-file}-valgrind"
            COMMAND "${MEMORYCHECK_COMMAND}" ${MEMORYCHECK_COMMAND_OPTIONS} "${CMAKE_CURRENT_BINARY_DIR}/test-file" "test-data/${json-file}" "test-data/${json-file}.out")
    endif(NOT ("${MEMORYCHECK_COMMAND}" MATCHES "MEMORYCHECK_COMMAND-NOTFOUND"))
endforeach(json-file ${json-files})
