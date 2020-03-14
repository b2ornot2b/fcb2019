from distutils.core import setup, Extension
# from distutils.extension import Extension

from Cython.Build import cythonize
import os

os.environ["CC"] = "g++"
os.environ["CFLAGS"] = "-DDEBUG"

ext_modules = cythonize([Extension("fendermustang",
                            ["fendermustang.pyx",
                             "mustang.cpp",
                            ],
                            libraries=["usb-1.0", "pthread"],
                            language="c++")])

setup(ext_modules=ext_modules)
