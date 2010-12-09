include ../Makefile.options

libsources := $(wildcard *.cpp)
libobjects := $(patsubst %.cpp,.bin/%.o,$(libsources))
deps := $(patsubst %.cpp,.bin/%.d,$(libsources))

ifeq ($(OPTIONS_USE_CRLIBM),yes)
   libobjects += .bin/log.o
   deps += .bin/log.d
endif

include ../Makefile.rules

all: ../lib/libtheta.so

ifeq ($(OPTIONS_BUILD_STATIC),yes)
all: ../lib/libconfig.a
endif

../lib/libtheta.so: $(libobjects)
	@$(LINK_SO) -o $@ $+

../lib/libtheta.a: $(objects)
	@$(CREATE_A) $@ $+

.bin/%.o: ../crlibm/%.c
	@$(COMPILE) -x c -o $@ $<

clean-subdir:
	@rm -f ../lib/libtheta.so
