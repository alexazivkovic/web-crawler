cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
project(web-crawler C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out/${CMAKE_BUILD_TYPE})

find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

add_executable(web-crawler src/main.c)
target_include_directories(web-crawler PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(web-crawler PRIVATE _POSIX_C_SOURCE=200809L)

file(GLOB EXTRA_SOURCES "src/*.c" "src/*/*.c")
list(REMOVE_ITEM EXTRA_SOURCES "${CMAKE_SOURCE_DIR}/src/main.c")
target_sources(web-crawler PRIVATE ${EXTRA_SOURCES})

target_link_libraries(web-crawler PRIVATE Threads::Threads CURL::libcurl)

install(TARGETS web-crawler
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
    add_executable(web-crawler-tests ${TEST_SOURCES} ${EXTRA_SOURCES})
    target_include_directories(web-crawler-tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_compile_definitions(web-crawler-tests PRIVATE _POSIX_C_SOURCE=200809L)
    target_link_libraries(web-crawler-tests PRIVATE
        Threads::Threads CURL::libcurl ${CRITERION_LIBRARIES})
    target_include_directories(web-crawler-tests PRIVATE ${CRITERION_INCLUDE_DIRS})
    target_link_directories(web-crawler-tests PRIVATE ${CRITERION_LIBRARY_DIRS})

    enable_testing()
    add_test(NAME web-crawler-tests COMMAND web-crawler-tests)
else()
    message(STATUS "Criterion nije pronadjen - testovi se ne build-uju (opciono).")
endif()
