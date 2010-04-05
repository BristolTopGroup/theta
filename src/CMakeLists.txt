file(GLOB libtheta_sources *.cpp)
add_library(libtheta SHARED ${libtheta_sources})

if(crmlib_path)
   add_library(crlibm STATIC IMPORTED)
   set_property(TARGET crlibm PROPERTY IMPORTED_LOCATION ${crmlib_path})
   target_link_libraries(libtheta crlibm)
endif()

target_link_libraries(libtheta m dl config sqlite3 ${Boost_LIBRARIES})
set_property(TARGET libtheta PROPERTY OUTPUT_NAME theta)
