#!/usr/bin/python2.2

import sys

from distutils.core import setup
from distutils.extension import Extension
from Pyrex.Distutils import build_ext

setup(
  name = "slp",
  version = "0.2",
  description = "Python Wrapper for OpenSLP",
  author = "Ganesan Rajagopal",
  author_email = "rganesan@debian.org",
  py_modules = [ "slp" ],
  ext_modules = [ 
    Extension("_slp", ["_slp.pyx"], libraries = ["slp"])
    ],
  cmdclass = { 'build_ext': build_ext }
)

