all: slp.so

slp.so: slp.pyx
	python2.2 setup.py build_ext --inplace

clean:
	rm -rf build slp.so slp.c *~

.PHONY: all clean
