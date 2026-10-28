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
        test/impl/examples/examples_test.cpp
        test/impl/examples/examples_test.h
        test/impl/length/length_test.cpp
        test/impl/length/length_test.h
        test/impl/completeness/completeness_test.cpp
        test/impl/completeness/completeness_test.h
)

add_executable(swe380
        main.cpp
        main.h
        main.cpp
        main.h
        evaluations/completeness/completeness_evaluator.h
        evaluations/completeness/completeness_evaluator.cpp
        evaluations/examples/example_evaluator.h
        evaluations/examples/example_evaluator.cpp
        evaluations/length/length_evaluator.cpp
        evaluations/length/length_evaluator.h
        evaluations/readability/readability_evaluator.h
        evaluations/readability/readability_evaluator.cpp
        evaluations/reuse/reuse_evaluator.cpp
        evaluations/reuse/reuse_evaluator.h
        evaluations/shared/markdown_utils.cpp
        evaluations/shared/markdown_utils.h
        test/test.cpp
        test/test.h
        test/impl/completeness/completeness_test.cpp
        test/impl/completeness/completeness_test.h
        test/impl/examples/examples_test.cpp
        test/impl/examples/examples_test.h
        test/impl/length/length_test.cpp
        test/impl/length/length_test.h
        test/impl/readability/readability_test.cpp
        test/impl/readability/readability_test.h
        test/impl/reuse/reuse_test.cpp
        test/impl/reuse/reuse_test.h
        dependencies/sqlite3.c
        dependencies/sqlite3.h
)

target_link_libraries(swe380 PRIVATE winhttp)

# RQ2: Skill Length and Example Evaluation

add_library(skill_quality_features STATIC
    evaluations/shared/markdown_utils.cpp
    evaluations/length/length_evaluator.cpp
    evaluations/examples/example_evaluator.cpp
)

target_include_directories(skill_quality_features
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# Export skill measurements
add_executable(quality_export
    tools/quality_export.cpp
    dependencies/sqlite3.c
)

target_link_libraries(quality_export
    PRIVATE skill_quality_features
)

# Automated tests
include(CTest)

if(BUILD_TESTING)
    add_executable(quality_tests
        tests/quality_tests.cpp
    )

    target_link_libraries(quality_tests
        PRIVATE skill_quality_features
    )

    add_test(
        NAME quality_features
        COMMAND quality_tests
    )
endif()
