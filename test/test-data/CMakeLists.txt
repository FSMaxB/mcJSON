cmake_minimum_required (VERSION 2.6)

set(json-files test1.json test2.json test3.json test4.json test5.json)

foreach(json-file ${json-files})
    execute_process(COMMAND cmake -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${json-file}" "${CMAKE_CURRENT_BINARY_DIR}/${json-file}")
endforeach(json-file ${json-files})
