#order matters
DIRS = src libconfig liblbfgs plugins root bin

#dependency on no_existent will always force re-build.

all:
	for d in $(DIRS); do make -C $$d; done

clean:
	for d in $(DIRS); do make -C $$d clean; done

.PHONY: clean all
