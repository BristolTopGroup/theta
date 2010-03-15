add_executable(theta theta.cpp)
target_link_libraries(theta libtheta boost_program_options-mt boost_filesystem-mt)

add_executable(merge merge.cpp)
target_link_libraries(merge libtheta boost_regex-mt boost_program_options-mt boost_filesystem-mt)

#add_custom_command(TARGET theta POST_BUILD COMMAND cp theta ${bin_dir}/)
