all: _slp.so

_slp.so: _slp.pyx
	python2.2 setup.py build_ext --inplace

clean:
	rm -rf build _slp.so _slp.c *~

.PHONY: all clean
