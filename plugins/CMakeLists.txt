file(GLOB plugin_sources *.cpp)
add_library(plugin MODULE ${plugin_sources})

target_link_libraries(plugin sqlite3 pq)
#TODO: ...
include_directories("/usr/include/postgresql/")

add_custom_command(TARGET plugin POST_BUILD COMMAND mv ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libplugin.so ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/core-plugins.so)

#install(TARGETS plugin DESTINATION ${install_dir})
#install(SCRIPT rename_plugin.cmake)



