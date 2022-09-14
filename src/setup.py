from distutils.core import setup, Extension
setup(name='fastmmapmq', version='1.0',  \
      ext_modules=[Extension('fastmmapmq', ['fastmmapmq-python2.c'])])
