cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
project(konkurentno_template C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out/${CMAKE_BUILD_TYPE})

find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

add_executable(konkurentno_template src/main.c)
target_include_directories(konkurentno_template PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(konkurentno_template PRIVATE _POSIX_C_SOURCE=200809L)

file(GLOB EXTRA_SOURCES "src/*.c" "src/*/*.c")
list(REMOVE_ITEM EXTRA_SOURCES "${CMAKE_SOURCE_DIR}/src/main.c")
target_sources(konkurentno_template PRIVATE ${EXTRA_SOURCES})

target_link_libraries(konkurentno_template PRIVATE Threads::Threads CURL::libcurl)

install(TARGETS konkurentno_template
        RUNTIME DESTINATION bin)

install(DIRECTORY include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.h")

# --- Testovi (opciono, samo ako je Criterion dostupan) ---
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(CRITERION QUIET criterion)
endif()

if(CRITERION_FOUND)
    file(GLOB TEST_SOURCES "tests/*.c")
    add_executable(konkurentno_template_tests ${TEST_SOURCES} ${EXTRA_SOURCES})
    target_include_directories(konkurentno_template_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_compile_definitions(konkurentno_template_tests PRIVATE _POSIX_C_SOURCE=200809L)
    target_link_libraries(konkurentno_template_tests PRIVATE
        Threads::Threads CURL::libcurl ${CRITERION_LIBRARIES})
    target_include_directories(konkurentno_template_tests PRIVATE ${CRITERION_INCLUDE_DIRS})
    target_link_directories(konkurentno_template_tests PRIVATE ${CRITERION_LIBRARY_DIRS})

    enable_testing()
    add_test(NAME konkurentno_template_tests COMMAND konkurentno_template_tests)
else()
    message(STATUS "Criterion nije pronadjen - testovi se ne build-uju (opciono).")
endif()
