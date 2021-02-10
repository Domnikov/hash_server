find_package(GTest REQUIRED)
if (NOT GTest_FOUND)
    message(FATAL_ERROR "Cannot find Google Test Framework!")
    return()
endif()

include_directories(${GTEST_INCLUDE_DIRS} ../src)

set(TEST_SRC test_main.cpp test.cpp)

foreach(SRC ${SOURCE_EXE})
    if(NOT ${SRC} MATCHES "src/main.cpp")
        message(${SRC})
        set(TEST_SRC  ${TEST_SRC}  ../${SRC})
    endif()
endforeach()


add_executable(${PROJ_NAME}_test ${TEST_SRC})

target_link_libraries(${PROJ_NAME}_test ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES} pthread crypto)
