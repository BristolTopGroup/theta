include ../Makefile.options

libsources := $(wildcard *.cpp)
libobjects := $(patsubst %.cpp,.bin/%.o,$(libsources))
deps := $(patsubst %.cpp,.bin/%.d,$(libsources))

libobjects += .bin/log2_dot.o .bin/atomic.o

all: ../lib/libtheta.so

.bin/log2_dot.o: log2_dot.cxx
	@$(COMPILE) $(FLAGSCPP) -o $@ $<

# switch between implementations of atomic:
ifeq ($(OPTIONS_GENERIC_ARCH),yes)

.bin/atomic.o: atomic.cxx
	@$(COMPILE) $(FLAGSCPP) -o $@ $<

else

#.bin/log2_dot.o: log2_dot.s
#	@$(COMPILE) $(FLAGSC) -o $@ $<
.bin/atomic.o: atomic.s
	@$(COMPILE) $(FLAGSC) -o $@ $<
.bin/atomic_asserts.o: atomic_asserts.cxx
	@$(COMPILE) $(FLAGSCPP) -o $@ $<

libobjects += .bin/atomic_asserts.o

endif


include ../Makefile.rules


../lib/libtheta.so: $(libobjects)
	@$(LINK_SO)



clean-subdir:
	@rm -f ../lib/libtheta.so

