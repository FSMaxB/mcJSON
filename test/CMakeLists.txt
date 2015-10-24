cmake_minimum_required (VERSION 2.6)

#needs to be add_subdirectory instead of subdirs
#because this ensures that it get's processed first
#and the variables are actually set.
add_subdirectory(test-data)

#test-objects
add_executable(test-objects test-objects common)
target_link_libraries(test-objects mcjson)
add_test(NAME test-objects
    COMMAND "${CMAKE_CURRENT_BINARY_DIR}/test-objects")
if(NOT ("${MEMORYCHECK_COMMAND}" MATCHES "MEMORYCHECK_COMMAND-NOTFOUND"))
    add_test(NAME "test-objects-valgrind"
        COMMAND "${MEMORYCHECK_COMMAND}" ${MEMORYCHECK_COMMAND_OPTIONS} "${CMAKE_CURRENT_BINARY_DIR}/test-objects")
endif(NOT ("${MEMORYCHECK_COMMAND}" MATCHES "MEMORYCHECK_COMMAND-NOTFOUND"))

#test_utils
add_executable(test_utils test_utils)
target_link_libraries(test_utils mcjson-utils)
add_test(NAME test-utils
    COMMAND "${CMAKE_CURRENT_BINARY_DIR}/test_utils" "test_utils.out")
if(NOT ("${MEMORYCHECK_COMMAND}" MATCHES "MEMORYCHECK_COMMAND-NOTFOUND"))
    add_test(NAME "test-utils-valgrind"
        COMMAND "${MEMORYCHECK_COMMAND}" ${MEMORYCHECK_COMMAND_OPTIONS} "${CMAKE_CURRENT_BINARY_DIR}/test_utils" "test_utils.out")
endif(NOT ("${MEMORYCHECK_COMMAND}" MATCHES "MEMORYCHECK_COMMAND-NOTFOUND"))
execute_process(COMMAND cmake -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/test_utils.ref" "${CMAKE_CURRENT_BINARY_DIR}/test_utils.ref")
add_test(NAME test_utils-comparison
    COMMAND cmake -E compare_files "${CMAKE_CURRENT_BINARY_DIR}/test_utils.out" "${CMAKE_CURRENT_BINARY_DIR}/test_utils.ref")

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
    add_test(NAME "test-file-${json-file}-comparison"
        COMMAND cmake -E compare_files "${CMAKE_CURRENT_BINARY_DIR}/test-data/${json-file}.out" "${CMAKE_CURRENT_BINARY_DIR}/test-data/${json-file}.ref")
endforeach(json-file ${json-files})
