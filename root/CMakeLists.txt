#set(root_sources root_histogram.cpp  root_minuit.cpp)
#add_library(root MODULE ${root_sources})

#add_custom_command(TARGET root POST_BUILD COMMAND mv ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libroot.so ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/root.so)

#set(root_includes )
#set_direcory_properties(EXCLUDE_FROM_ALL ON INCLUDE_DIRECTORIES "${root_includes}")
