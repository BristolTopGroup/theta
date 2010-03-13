add_executable(true_q true_q.cpp)
add_custom_command(TARGET true_q POST_BUILD COMMAND mv ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/true_q ${test_dir}/test-stat/)
