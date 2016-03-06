set(cmake_generated ${CMAKE_BINARY_DIR}/CMakeCache.txt
                    ${CMAKE_BINARY_DIR}/cmake_install.cmake  
                    ${CMAKE_BINARY_DIR}/Makefile
                    ${CMAKE_BINARY_DIR}/CMakeFiles
)

file(GLOB_RECURSE DATA_FILES
    *.dat
    *.eet
    *.db
    core
    doc/apidocs/*
)

foreach(file ${cmake_generated})
    if (EXISTS ${file})
        file(REMOVE_RECURSE ${file})
    endif()
endforeach(file)

foreach(file ${DATA_FILES})
    if (EXISTS ${file})
        file(REMOVE_RECURSE ${file})
    endif()
endforeach(file)
