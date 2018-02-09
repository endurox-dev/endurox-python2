#!/usr/bin/python
import sys
import os
import string
import time

from endurox.atmi import *

class server:
    def RING(self, arg):
        try:
                tplog(log_debug, "Returning STRING_2 from STRING value")
                tplog(log_error, str(arg))
                # So here UBF buffer is dictionary and occurrances may be a list
                # in dict field
                arg["T_STRING_2_FLD"]=arg["T_STRING_FLD"]
                arg['T_STRING_3_FLD']="OK"
                # Create list
                arg['T_STRING_4_FLD']=[]
                arg['T_STRING_4_FLD'].append("HELLO from Mars")
                arg['T_STRING_4_FLD'].append("HELLO from Venus")

                # Curently will not work as types does not match string
                # so basically this is Bchg, not CB...
                #arg['T_STRING_5_FLD']=1
                tplog(log_error, str(arg))
                return arg
        except Exception as e:
                print str(e)
                tplog(log_error, str(e))
        return TPFAIL
    
    def init(self, arguments):
        tplog(log_error, "About to advertise")
        try:
                tpadvertise("RING", "RING")
        except Exception as e:
                print str(e)

    def cleanup(self):
        userlog("cleanup in recv_py called!")

srv = server()

def exithandler():
    print "Ring service terminating..."
sys.exitfunc = exithandler

if __name__ == '__main__': 
    mainloop(sys.argv, srv, None)

# end
