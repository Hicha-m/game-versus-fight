# CMake generated Testfile for 
# Source directory: /home/user/Documents/Cours/jeux IA
# Build directory: /home/user/Documents/Cours/jeux IA/build-tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(game_tests "/home/user/Documents/Cours/jeux IA/build-tests/game_tests")
set_tests_properties(game_tests PROPERTIES  _BACKTRACE_TRIPLES "/home/user/Documents/Cours/jeux IA/CMakeLists.txt;66;add_test;/home/user/Documents/Cours/jeux IA/CMakeLists.txt;0;")
add_test(architecture_rules "bash" "/home/user/Documents/Cours/jeux IA/scripts/check_architecture.sh")
set_tests_properties(architecture_rules PROPERTIES  _BACKTRACE_TRIPLES "/home/user/Documents/Cours/jeux IA/CMakeLists.txt;67;add_test;/home/user/Documents/Cours/jeux IA/CMakeLists.txt;0;")
