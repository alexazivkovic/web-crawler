cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_STANDARD 99)
project(konkurentno_template C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out/${CMAKE_BUILD_TYPE})

add_executable(konkurentno_template src/main.c)
target_include_directories(konkurentno_template PRIVATE ${CMAKE_SOURCE_DIR}/include)

file(GLOB EXTRA_SOURCES "src/*.c" "src/*/*.c")
list(REMOVE_ITEM EXTRA_SOURCES "${CMAKE_SOURCE_DIR}/src/main.c")
target_sources(konkurentno_template PRIVATE ${EXTRA_SOURCES})

install(TARGETS konkurentno_template 
        RUNTIME DESTINATION bin)

install(DIRECTORY include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.h")
