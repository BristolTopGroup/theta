file(GLOB libtheta_sources *.cpp)
add_library(libtheta SHARED ${libtheta_sources})
target_link_libraries(libtheta m dl config sqlite3 boost_filesystem-mt)
set_property(TARGET libtheta PROPERTY OUTPUT_NAME theta)

