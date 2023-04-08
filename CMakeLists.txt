cmake_minimum_required(VERSION 3.22)

project(sylar)
include (cmake/utils.cmake)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
set(CMAKE_C_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -fPIC -ggdb -std=c11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")


set(LIB_SRC 
    ./sylar/src/log.cpp
    ./sylar/src/conf.cpp
    ./sylar/src/util.cpp
    ./sylar/src/thread.cpp)


add_library(sylar SHARED ${LIB_SRC})

set(LIBS
        sylar
        yaml-cpp
        pthread)

force_redefine_file_macro_for_sources(sylar)

sylar_add_executable(log_test "./tests/log_test.cpp" sylar "${LIBS}")
sylar_add_executable(config_test "./tests/config_test.cpp" sylar "${LIBS}")
sylar_add_executable(thread_test "./tests/thread_test.cpp" sylar "${LIBS}")


set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
