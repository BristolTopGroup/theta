
file(GLOB test_sources *.cpp)
add_executable(test-exe ${test_sources})

add_library(libtest MODULE test-ex2.cxx  test-ex3.cxx  test-ex.cxx)

target_link_libraries(test-exe libtheta)

add_custom_command(TARGET test-exe POST_BUILD COMMAND mv ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-exe ${test_dir}/test)
add_custom_command(TARGET libtest POST_BUILD COMMAND cp ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/liblibtest.so ${test_dir}/test-ex.so)

#add_test(unittests /bin/sh -e "cd ${test_dir}/..; ${test_dir}/test")

