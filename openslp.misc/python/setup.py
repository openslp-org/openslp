import sys

from distutils.core import setup
from distutils.extension import Extension
from Pyrex.Distutils import build_ext

setup(
  name = "slp",
  version = "1.0",
  py_modules = ["slp"],
  ext_modules=[ 
    Extension("_slp", ["_slp.pyx"], libraries = ["slp"])
    ],
  cmdclass = {'build_ext': build_ext}
)

