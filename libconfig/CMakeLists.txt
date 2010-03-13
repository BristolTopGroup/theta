set(config_sources grammar.c  libconfig.c  libconfigcpp.cpp  scanctx.c  scanner.c  strbuf.c)
add_library(config SHARED ${config_sources})
#install(TARGETS config DESTINATION ${install_dir})

