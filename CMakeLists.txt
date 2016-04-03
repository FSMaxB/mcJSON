cmake_minimum_required (VERSION 2.6)

project (mcJSON C)

subdirs(test buffer)

enable_testing()

add_library(mcjson mcJSON)
target_link_libraries(mcjson m molch-buffer)

add_library(mcjson-utils mcJSON_Utils)
target_link_libraries(mcjson-utils mcjson)

#check if running debug build
if ("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
    if("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
        set(SECURITY_C_FLAGS "-O0")
    else()
        set(SECURITY_C_FLAGS "-Og")
    endif()
else ("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
    #more secure linker and compiler flags
    if(APPLE)
        set(SECURITY_C_FLAGS "-O1 -Wformat -Wformat-security")
        #TODO: Find secure Linker flags that work on Mac OS X
    else(APPLE)
        set(SECURITY_C_FLAGS "-fstack-protector-strong -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -O1 -Wformat -Wformat-security")
        set(SECURITY_LINK_FLAGS "-z relro,now,noexecstack,noexecheap -pie")
    endif(APPLE)
endif ("${CMAKE_BUILD_TYPE}" MATCHES "Debug")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -pedantic -Wall -Wextra -Werror -fPIC ${SECURITY_C_FLAGS}")
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
