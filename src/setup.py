from distutils.core import setup, Extension
setup(name='fastmmap', version='1.0',  \
      ext_modules=[Extension('fastmmap', ['fastmmapmq-python2.c'])])
