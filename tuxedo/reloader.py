import os
import re
import tuxedo



class Reloader:

    """ dynamically reload a server class if the .py file is newer than .pyc file """ 

    def __init__(self, module, server):
        self.last_mtime = 0
        self.first_run = 1
        self.server = server
        self.module = module

    def reloader_func(self):
        try:
            if self.load_if_modified() == 1:
                del self.server
                self.server = self.module.server()
        except:
                tuxedo.atmi.userlog("can't reload " + `self.module`)
        s=self.server
        return s

        
    def load_if_modified(self):
        ret_val = 0
        mtime_pyc = 0
        filename_pyc = re.match(r"<.* from '(.*)'>", `self.module`).group(1)

        m = re.match(r"(.*)\.py(.*)", filename_pyc)
        filename_base = m.group(1)

        try:
            mtime_pyc = os.stat(filename_pyc)[9]
        except:
            tuxedo.atmi.userlog("load_if_modified: exception @ stat (1)")
            pass

        mtime_py = 0
        try:
            m.group(2)
            filename_py = filename_base + ".py"
            mtime_py = os.stat(filename_py)[9]
        except:
            tuxedo.atmi.userlog("load_if_modified: exception @ stat (2)")
            pass
        
        if mtime_py:
            if mtime_py > mtime_pyc:
                mtime_pyc = mtime_py
            
        if mtime_pyc > self.last_mtime:
            if not self.first_run:
                reload(self.module)
                ret_val = 1
        else:
            ret_val = 0
        self.last_mtime = mtime_pyc
        self.first_run = 0
        return ret_val
