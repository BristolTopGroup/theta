include ../Makefile.rules

objects = lbfgs.o

all: ../lib/liblbfgs.so

../lib/liblbfgs.so: $(objects)
	$(CC) -o $@ -shared $(objects)
