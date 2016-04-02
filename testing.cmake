
enable_testing()
add_custom_target(test_all
    COMMENT "Building all tests"
)

add_custom_target(run_tests
    COMMAND ctest -V
    DEPENDS test_all
    COMMENT "Running all tests"
)

set(TRACEFILE "${CMAKE_BINARY_DIR}/coverage.info")
set(COVERAGE_DIR "${CMAKE_BINARY_DIR}/lcov")
add_custom_target(coverage
    COMMAND cd ${CMAKE_BINARY_DIR}
    COMMAND lcov -c -d "${CMAKE_CURRENT_BINARY_DIR}" -b "${CMAKE_SOURCE_DIR}" -o ${TRACEFILE} --no-external
    COMMAND genhtml ${TRACEFILE} -o ${COVERAGE_DIR} -p "${CMAKE_SOURCE_DIR}" --demangle-cpp
    DEPENDS run_tests
)

add_custom_target(coverage_view
    COMMAND xdg-open "${COVERAGE_DIR}/index.html"
)
