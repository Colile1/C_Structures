add_executable(tests
  test_main.cpp  # Create this later
)
add_dependencies(tests C_Structures)
target_link_libraries(tests PRIVATE
  GTest::GTest
)
add_test(NAME tests COMMAND tests)
