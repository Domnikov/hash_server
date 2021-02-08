###################################################################################################
##
##      HASH TCP Server test
##
###################################################################################################

cmake_minimum_required(VERSION 2.8)
project(hash_server LANGUAGES CXX)

set(SOURCE_EXE src/main.cpp)
set(PROJ_NAME hash_server)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-O2 -Wall -Werror)

add_executable (${PROJ_NAME} ${SOURCE_EXE})

option(COMPILE_TESTS "Compile the tests" OFF)

if(COMPILE_TESTS)
  add_subdirectory(test)
endif(COMPILE_TESTS)

install(TARGETS ${PROJ_NAME} DESTINATION bin)

target_link_libraries(${PROJ_NAME} pthread crypto)
