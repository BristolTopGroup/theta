#order matters
DIRS = src libconfig liblbfgs plugins root bin

all:
	@for d in $(DIRS); do make -C $$d; done

clean:
	@for d in $(DIRS); do make -C $$d clean; done
	@make -C test clean

test:
	@make -C test

#ok, this is not very nice, as it hardcodes the documentation copy target, but as long as I am the only
# serious developer, it should work well:
doc:
	@doxygen
	@if [ "`hostname`" = "ekplx22" ]; then rsync -a --del doc/* /usr/users/ott/public_html/theta; fi

run-test: test
	@test/test

.PHONY: clean all test doc
