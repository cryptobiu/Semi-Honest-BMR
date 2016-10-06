cmake_minimum_required(VERSION 2.8)
project(src)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -maes -O3 -mavx")

add_custom_target(src
        COMMAND make
        WORKING_DIRECTORY $ENV{HOME}/Semi-Honest-BMR/src)