cmake_minimum_required (VERSION 2.6)

project (cJSON)

enable_testing()

add_library(cjson cJSON)
target_link_libraries(cjson m)

add_library(cjson-utils cJSON_Utils)
target_link_libraries(cjson-utils cjson)

#more secure linker and compiler flags
if(APPLE)
    set(SECURITY_C_FLAGS "-O1 -Wformat -Wformat-security -fPIE")
    #TODO: Find secure Linker flags that work on Mac OS X
else(APPLE)
    set(SECURITY_C_FLAGS "-fstack-protector-strong -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -O1 -Wformat -Wformat-security -fPIE")
    set(SECURITY_LINK_FLAGS "-z relro -z now -pie -z noexecstack -z noexecheap")
endif(APPLE)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -pedantic -Wall -Wextra -Werror ${SECURITY_C_FLAGS}")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} ${SECURITY_LINK_FLAGS}")

#check for valgrind
find_program(MEMORYCHECK_COMMAND valgrind)
set(MEMORYCHECK_COMMAND_OPTIONS --trace-children=yes --leak-check=full --error-exitcode=10)

if("${MEMORYCHECK_COMMAND}" MATCHES "MEMORYCHECK_COMMAND-NOTFOUND")
    message(WARNING "valgrind not found")
endif("${MEMORYCHECK_COMMAND}" MATCHES "MEMORYCHECK_COMMAND-NOTFOUND")

if("${DISABLE_MEMORYCHECK_COMMAND}" MATCHES "TRUE")
    set(MEMORYCHECK_COMMAND "MEMORYCHECK_COMMAND-NOTFOUND")
endif("${DISABLE_MEMORYCHECK_COMMAND}" MATCHES "TRUE")

#add normal test
add_executable(tests test)
target_link_libraries(tests cjson)
add_test(tests "./tests")
if(NOT ("${MEMORYCHECK_COMMAND}" MATCHES "MEMORYCHECK_COMMAND-NOTFOUND"))
    add_test("tests-valgrind" ${MEMORYCHECK_COMMAND} ${MEMORYCHECK_COMMAND_OPTIONS} "./tests")
endif(NOT ("${MEMORYCHECK_COMMAND}" MATCHES "MEMORYCHECK_COMMAND-NOTFOUND"))

#add util test
add_executable(test-utils test_utils)
target_link_libraries(test-utils cjson-utils)
add_test(test-utils "./test-utils")
if(NOT ("${MEMORYCHECK_COMMAND}" MATCHES "MEMORYCHECK_COMMAND-NOTFOUND"))
    add_test("test-utils-valgrind" ${MEMORYCHECK_COMMAND} ${MEMORYCHECK_COMMAND_OPTIONS} "./test-utils")
endif(NOT ("${MEMORYCHECK_COMMAND}" MATCHES "MEMORYCHECK_COMMAND-NOTFOUND"))
