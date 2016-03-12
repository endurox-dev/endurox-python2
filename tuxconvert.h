/* 
   This file decalres some functions to convert TUXEDO data types (STRING,
   FML32) to Python data types (string, dictionary) and vice versa 

   (c) 1999 Ralf Henschkowski (ralf@henschkowski.com)


*/


#ifndef TUXCONVERT_H
#define TUXCONVERT_H



#include <fml32.h>     /* TUXEDO Header File */
#include <fml1632.h>   /* TUXEDO Header File */


#define TUXBUFSIZE  16384*2

extern PyObject* fml_to_dict(FBFR32* fml);
extern FBFR32* dict_to_fml(PyObject* dict);
extern char* pystring_to_string(PyObject* pystring);
extern PyObject* string_to_pystring(char* string);



#endif /* TUXCONVERT_H */

