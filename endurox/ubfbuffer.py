
from UserDict import *
from UserList import *
import types



class EasyList(UserList):
    def __setitem__(self, i, item):
        listlen  = self.__len__()
        if listlen <= i:
            missing_elements = (i - listlen)+1
            for n in range(0, missing_elements):
                self.data.append(None)
        self.data[i] = item




class UbfBuffer(UserDict):
	def __setitem__(self, key, item):
            if not self.has_key(key):
                self.data[key] = EasyList()
            self.data[key].append(item)
			

        def __getitem__(self, key):
            if not self.has_key(key):
                self.data[key] = EasyList()
            return self.data[key]

        def as_dictionary(self):
            a={}
            for key in self.data.keys():
                a[key] = []
                for idx in range(0, len(self.data[key])):
                    a[key].append(self.data[key][idx])
            return a



def test():
    buf = UbfBuffer()
    buf['huhu'][0] = "gaga"
    buf['huhu'][1] = "gugu"
    buf['huhu'][5] = "gigi"

    buf['ralf'][3] = "Henschkowski"
    buf['ralf'].append("Andreas")
    print buf
    

    a = buf.as_dictionary()
    print " a is of type "
    print type(a)
    print a

    
    del buf['ralf']
    print buf

    del buf['huhu'][5]
    print buf


    print 

    print type(buf)


# Call test() when this file is run as a script (not imported as a module)
if __name__ == '__main__': 
    test()

