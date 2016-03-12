#!/home/szhh5e/bin/python

"""
Distutils installer for Tuxmodule / modified setup from m2crypto module

Copyright (c) 1999-2003, Ng Pheng Siong. All rights reserved.
Copyright (c) 2003-2007, Ralf Henschkowski. All rights reserved.

"""

_RCS_id = '$Id:$'

import os, shutil
import sys
from distutils.core import setup, Extension
from distutils.command import build_ext, clean
import commands, os.path


my_inc = os.path.join(os.getcwd(), '.')
try:
    tuxedo_dir = os.environ["TUXDIR"]
except KeyError:
    print "*** ERROR ***: Please set your environment. TUXDIR not set."
    sys.exit(1)


# set to your desired Tuxedo major version number: 6 or 7/8
# (you can also access this later  from the module as tuxedo.atmi.TUXVERSION)
tuxversion = 0  

# auto-detect Tuxedo version (to link the correct "new" or "old" (pre-7.1) style libs)
tux8 = os.access(os.path.join(tuxedo_dir, "udataobj", "System.rdp"), os.F_OK)
status, _ = commands.getstatusoutput("nm %s | grep tpappthr" % (os.path.join(tuxedo_dir, "lib", "libtux.a")))
tux10 = not status
if tuxversion == 0 and tux10 == True:
    print "*** Building for Tuxedo Version > 10  ... ***"
    tuxversion = 8
elif tuxversion == 0 and tux8 == True:
    print "*** Building for Tuxedo Version > 8  ... ***"
    tuxversion = 10
else:
    print "*** Building for Tuxedo 6.x ... ***"
    tuxversion = 6



extra_compile_args = [ ]
extra_link_args = []

if sys.platform[:3] == 'aix':
   extra_link_args = ['-berok']

if os.name == 'nt':
    print "*** ERROR *** Windows not yet supported"
    sys.exit(1)

elif os.name == 'posix':
    include_dirs = [my_inc, tuxedo_dir + '/include',  '/usr/include']
    library_dirs = [tuxedo_dir + '/lib', '/usr/lib']

    if tuxversion < 7:
	libraries = ['tux', 'tmib', 'qm', 'buft', 'tux2', 'fml', 'fml32', 'gp', '/usr/lib/libcrypt.a']
	libraries_ws = ['wsc', 'buft', 'wsc', 'nws', 'nwi', 'nws', 'fml', 'fml32', 'gp', 'nsl', 'socket', '/usr/lib/libcrypt.a']
    else:
	libraries = ['tux', 'tmib', 'buft', 'fml', 'fml32', '/usr/lib/libcrypt.a']
	libraries_ws = ['wsc', 'tmib', 'buft', 'fml', 'fml32', 'gpnet', 'engine', 'dl', 'pthread', '/usr/lib/libcrypt.a']
	


# For debug purposes only, set if you experience core dumps
#extra_compile_args.append("-DDEBUG")
#extra_compile_args.append("-g")


# build the atmi and atmi/WS modules
tuxedo_ext = Extension(name = 'tuxedo.atmi',
		     define_macros = [("TUXVERSION", tuxversion)], 
		     undef_macros = ["TUXWS"], 
                     sources = ['tuxconvert.c', 'tuxmodule.c', 'tuxloop.c' ],
                     include_dirs = include_dirs,
                     library_dirs = library_dirs,
                     libraries = libraries,
                     extra_compile_args = extra_compile_args,
                     extra_link_args = extra_link_args
                     )

tuxedo_ext_ws = Extension(name = 'tuxedo.atmiws',
		     define_macros = [("TUXWS", 1), 
				      ("TUXVERSION", tuxversion)
				      ], 
                     sources = ['tuxconvert.c', 'tuxmodule.c' ],
                     include_dirs = include_dirs,
                     library_dirs = library_dirs,
                     libraries = libraries_ws,
                     extra_compile_args = extra_compile_args,
                     extra_link_args = extra_link_args
                     )

for ver in [('tuxedo', tuxedo_ext, 'IPC flavour'),('tuxedo_ws', tuxedo_ext_ws, 'Network /WS flavour')]:
    setup(name = ver[0],
          version = '1.1',
          description = 'Tuxmodule: A Python client and server library for use with the Tuxedo transaction monitor, %s' 
                           %(ver[2],),
          author = 'Ralf Henschkowski',
          author_email = 'ralf.henschkowski@gmail.com',
          url = 'http://code.google.com/p/tuxmodule',
          packages = ["tuxedo"],
          ext_modules = [ver[1]]
          )



