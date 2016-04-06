/* 
   This file decalres some functions to convert ENDUROX data types (STRING,
   UBF) to Python data types (string, dictionary) and vice versa 

   (c) 1999 Ralf Henschkowski (ralf@henschkowski.com)


*/


#ifndef NDRXCONVERT_H
#define NDRXCONVERT_H



#include <ubf.h>     /* ENDUROX Header File */


#define NDRXBUFSIZE  16384*2

extern PyObject* ubf_to_dict(UBFH* ubf);
extern UBFH* dict_to_ubf(PyObject* dict);
extern char* pystring_to_string(PyObject* pystring);
extern PyObject* string_to_pystring(char* string);



#endif /* NDRXCONVERT_H */

