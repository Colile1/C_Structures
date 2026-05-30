# CMake generated Testfile for 
# Source directory: /mnt/c/Users/Colile/Documents/claude/Projects/C_Structures/C_Structures
# Build directory: /mnt/c/Users/Colile/Documents/claude/Projects/C_Structures/C_Structures/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(ModelTests "/mnt/c/Users/Colile/Documents/claude/Projects/C_Structures/C_Structures/build/tests" "--gtest_filter=NodeTest*:BeamTest*")
set_tests_properties(ModelTests PROPERTIES  _BACKTRACE_TRIPLES "/mnt/c/Users/Colile/Documents/claude/Projects/C_Structures/C_Structures/CMakeLists.txt;94;add_test;/mnt/c/Users/Colile/Documents/claude/Projects/C_Structures/C_Structures/CMakeLists.txt;0;")
add_test(PhysicsTests "/mnt/c/Users/Colile/Documents/claude/Projects/C_Structures/C_Structures/build/tests" "--gtest_filter=PhysicsEngine*")
set_tests_properties(PhysicsTests PROPERTIES  _BACKTRACE_TRIPLES "/mnt/c/Users/Colile/Documents/claude/Projects/C_Structures/C_Structures/CMakeLists.txt;95;add_test;/mnt/c/Users/Colile/Documents/claude/Projects/C_Structures/C_Structures/CMakeLists.txt;0;")
add_test(CSVTests "/mnt/c/Users/Colile/Documents/claude/Projects/C_Structures/C_Structures/build/tests" "--gtest_filter=CSVHandler*")
set_tests_properties(CSVTests PROPERTIES  _BACKTRACE_TRIPLES "/mnt/c/Users/Colile/Documents/claude/Projects/C_Structures/C_Structures/CMakeLists.txt;96;add_test;/mnt/c/Users/Colile/Documents/claude/Projects/C_Structures/C_Structures/CMakeLists.txt;0;")
add_test(IntegrationTests "/mnt/c/Users/Colile/Documents/claude/Projects/C_Structures/C_Structures/build/tests" "--gtest_filter=Integration*")
set_tests_properties(IntegrationTests PROPERTIES  _BACKTRACE_TRIPLES "/mnt/c/Users/Colile/Documents/claude/Projects/C_Structures/C_Structures/CMakeLists.txt;97;add_test;/mnt/c/Users/Colile/Documents/claude/Projects/C_Structures/C_Structures/CMakeLists.txt;0;")
