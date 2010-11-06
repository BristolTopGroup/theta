
libsources := $(wildcard *.cpp)
libobjects := $(patsubst %.cpp,.bin/%.o,$(libsources))
deps := $(patsubst %.cpp,.bin/%.d,$(libsources))

include ../Makefile.rules

all: ../lib/libtheta.so

../lib/libtheta.so: $(libobjects)
	@$(LINK_SO) -o $@ $+

clean-subdir:
	@rm -f ../lib/libtheta.so
