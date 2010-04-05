
file(GLOB libtheta_sources *.cpp)

if(crlibm)
   add_library(libtheta SHARED ${libtheta_sources} ../crlibm/log.c)
else()
   add_library(libtheta SHARED ${libtheta_sources})
endif()

target_link_libraries(libtheta m dl config sqlite3 ${Boost_LIBRARIES})
set_property(TARGET libtheta PROPERTY OUTPUT_NAME theta)
