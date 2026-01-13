# Add unit tests for the project
find_package(Catch2 QUIET)

if(Catch2_FOUND)
    # If Catch2 is available, create comprehensive tests
    add_executable(dotlin_tests
        lexer_tests.cpp
        parser_tests.cpp
        interpreter_tests.cpp
    )

    target_link_libraries(dotlin_tests PRIVATE dotlin::lib Catch2::Catch2WithMain)

    dotlin_apply_sanitizers(dotlin_tests)

    add_test(NAME dotlin_tests COMMAND dotlin_tests)
else()
    # Fallback: Create a simple test executable
    add_executable(dotlin_simple_tests simple_tests.cpp)

    target_link_libraries(dotlin_simple_tests PRIVATE dotlin::lib)

    dotlin_apply_sanitizers(dotlin_simple_tests)

    add_test(NAME dotlin_simple_tests COMMAND dotlin_simple_tests)
endif()
