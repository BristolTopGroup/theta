include ../Makefile.rules

libsources = $(wildcard *.cpp)
libobjects = $(patsubst %.cpp,%.o,$(libsources))

all: ../lib/libtheta.so

../lib/libtheta.so: $(libobjects)
	@$(LINK_SO) -o $@ $+

clean-subdir:
	@rm -f ../lib/libtheta.so

