#!/home/szhh5e/bin/python

"""
Distutils installer for Ndrxmodule / modified setup from m2crypto module

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
    endurox_dir = os.environ["NDRX_HOME"]
except KeyError:
    print "*** ERROR ***: Please set your environment. NDRX_HOME not set."
    sys.exit(1)


# set to your desired Endurox major version number: 6 or 7/8
# (you can also access this later  from the module as endurox.atmi.NDRXVERSION)
ndrxversion = 0  

# auto-detect Endurox version (to link the correct "new" or "old" (pre-7.1) style libs)
ndrx8 = os.access(os.path.join(endurox_dir, "udataobj", "System.rdp"), os.F_OK)
status, _ = commands.getstatusoutput("nm %s | grep tpappthr" % (os.path.join(endurox_dir, "lib", "libndrx.a")))
ndrx10 = not status
if ndrxversion == 0 and ndrx10 == True:
    print "*** Building for Endurox Version > 10  ... ***"
    ndrxversion = 8
elif ndrxversion == 0 and ndrx8 == True:
    print "*** Building for Endurox Version > 8  ... ***"
    ndrxversion = 10
else:
    print "*** Building for Endurox 6.x ... ***"
    ndrxversion = 6



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

    if ndrxversion < 7:
	libraries = ['ndrx', 'tmib', 'qm', 'buft', 'ndrx2', 'ubf', 'ubf32', 'gp', '/usr/lib/libcrypt.a']
	libraries_ws = ['wsc', 'buft', 'wsc', 'nws', 'nwi', 'nws', 'ubf', 'ubf32', 'gp', 'nsl', 'socket', '/usr/lib/libcrypt.a']
    else:
	libraries = ['ndrx', 'tmib', 'buft', 'ubf', 'ubf32', '/usr/lib/libcrypt.a']
	libraries_ws = ['wsc', 'tmib', 'buft', 'ubf', 'ubf32', 'gpnet', 'engine', 'dl', 'pthread', '/usr/lib/libcrypt.a']
	


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

endurox_ext_ws = Extension(name = 'endurox.atmiws',
		     define_macros = [("NDRXWS", 1), 
				      ("NDRXVERSION", ndrxversion)
				      ], 
                     sources = ['ndrxconvert.c', 'ndrxmodule.c' ],
                     include_dirs = include_dirs,
                     library_dirs = library_dirs,
                     libraries = libraries_ws,
                     extra_compile_args = extra_compile_args,
                     extra_link_args = extra_link_args
                     )

for ver in [('endurox', endurox_ext, 'IPC flavour'),('endurox_ws', endurox_ext_ws, 'Network /WS flavour')]:
    setup(name = ver[0],
          version = '1.1',
          description = 'Ndrxmodule: A Python client and server library for use with the Endurox transaction monitor, %s' 
                           %(ver[2],),
          author = 'Ralf Henschkowski',
          author_email = 'ralf.henschkowski@gmail.com',
          url = 'http://code.google.com/p/ndrxmodule',
          packages = ["endurox"],
          ext_modules = [ver[1]]
          )



