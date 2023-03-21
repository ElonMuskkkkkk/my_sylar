cmake_minimum_required(VERSION 3.22)

project(sylar)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
set(CMAKE_C_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -fPIC -ggdb -std=c11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")


set(LIB_SRC 
    ./sylar/src/log.cpp
    ./sylar/src/util.cpp)

add_library(sylar SHARED ${LIB_SRC})
add_executable(log_test ./tests/log_test.cpp)
add_dependencies(log_test sylar)
target_link_libraries(log_test sylar) 

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
