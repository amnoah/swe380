cmake_minimum_required(VERSION 4.0)
project(swe380)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(.)

add_executable(swe380
        main.cpp
        main.h
        main.cpp
        main.h
        evaluations/readability/readability_evaluator.hpp
        evaluations/readability/readability_evaluator.cpp
)

target_link_libraries(swe380 PRIVATE winhttp)
