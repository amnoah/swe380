cmake_minimum_required(VERSION 4.0)
project(swe380)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(.)

add_executable(MySqliteApp main.cpp dependencies/sqlite3.c
        test/test.cpp
        test/impl/readability/readability_test.cpp
        test/impl/readability/readability_test.h
        test/test.h
        test/impl/reuse/reuse_test.cpp
        test/impl/reuse/reuse_test.h
)

add_executable(swe380
        main.cpp
        main.h
        main.cpp
        main.h
        evaluations/readability/readability_evaluator.h
        evaluations/readability/readability_evaluator.cpp
        evaluations/reuse/reuse_evaluator.cpp
        evaluations/reuse/reuse_evaluator.h
        test/test.cpp
        test/test.h
        test/impl/readability/readability_test.cpp
        test/impl/readability/readability_test.h
        test/impl/reuse/reuse_test.cpp
        test/impl/reuse/reuse_test.h
        dependencies/sqlite3.c
        dependencies/sqlite3.h
)

target_link_libraries(swe380 PRIVATE winhttp)
