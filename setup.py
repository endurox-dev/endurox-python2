#!/home/szhh5e/bin/python

"""
Distutils installer for Ndrxmodule / modified setup from m2crypto module

Copyright (c) 1999-2003, Ng Pheng Siong. All rights reserved.
Copyright (c) 2003-2007, Ralf Henschkowski. All rights reserved.
Copyright (c) 2017, Mavimax SIA

"""

_RCS_id = '$Id:$'

import os, shutil
import sys
from distutils.core import setup, Extension
from distutils.command import build_ext, clean
import commands, os.path


my_inc = os.path.join(os.getcwd(), '.')
try:
    endurox_dir = os.environ["NDRX_HOME"]
except KeyError:
    print "*** ERROR ***: Please set your environment. NDRX_HOME not set."
    sys.exit(1)


# set to your desired Endurox major version number: 6 or 7/8
# (you can also access this later  from the module as endurox.atmi.NDRXVERSION)
ndrxversion = 0  

# auto-detect Endurox version (to link the correct "new" or "old" (pre-7.1) style libs)
ndrx10 = True
ndrxversion = 10

extra_compile_args = [ ]
extra_link_args = []

if sys.platform[:3] == 'aix':
   extra_link_args = ['-berok']

if os.name == 'nt':
    print "*** ERROR *** Windows not yet supported"
    sys.exit(1)

elif os.name == 'posix':
    include_dirs = [my_inc, endurox_dir + '/include',  '/usr/include']
    library_dirs = [endurox_dir + '/lib', '/usr/lib']

    libraries = ['atmisrvnomain', 'atmi', 'ubf', 'nstd', 'pthread', 'rt', 'm', '/usr/lib/libcrypt.a']

# For debug purposes only, set if you experience core dumps
#extra_compile_args.append("-DDEBUG")
#extra_compile_args.append("-g")


# build the atmi and atmi/WS modules
endurox_ext = Extension(name = 'endurox.atmi',
		     define_macros = [("NDRXVERSION", ndrxversion)], 
		     undef_macros = ["NDRXWS"], 
                     sources = ['ndrxconvert.c', 'ndrxmodule.c', 'ndrxloop.c' ],
                     include_dirs = include_dirs,
                     library_dirs = library_dirs,
                     libraries = libraries,
                     extra_compile_args = extra_compile_args,
                     extra_link_args = extra_link_args
                     )

for ver in [('endurox', endurox_ext, 'IPC flavour')]:
    setup(name = ver[0],
          version = '1.1',
          description = 'Ndrxmodule: A Python client and server library for use with the Endurox transaction monitor, %s' 
                           %(ver[2],),
          author = 'Ralf Henschkowski',
          author_email = 'ralf.henschkowski@gmail.com',
          url = 'https://github.com/endurox-dev/endurox-python2',
          packages = ["endurox"],
          ext_modules = [ver[1]]
          )



