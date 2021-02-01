###################################################################################################
##
##      HASH TCP Server test
##
###################################################################################################

cmake_minimum_required(VERSION 2.8)
project(hash_server LANGUAGES CXX)

set(SOURCE_EXE src/main.cpp src/hash_server.cpp)
set(PROJ_NAME hash_server)

add_executable (${PROJ_NAME} ${SOURCE_EXE})

add_compile_options(-std=c++17 -Wall -Werror)

if(NOT CMAKE_CXX_EXTENSIONS)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

option(COMPILE_TESTS "Compile the tests" OFF)

if(COMPILE_TESTS)
  add_subdirectory(test)
endif(COMPILE_TESTS)

install(TARGETS ${PROJ_NAME} DESTINATION bin)

target_link_libraries(${PROJ_NAME} pthread crypto)