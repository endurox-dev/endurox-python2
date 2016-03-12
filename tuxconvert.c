/* 
   This file implements some functions to convert TUXEDO data types (STRING,
   FML32) to Python data types (string, dictionary) and vice versa 

   (c) 1999 Ralf Henschkowski (ralfh@gmx.ch)

*/

#include <stdio.h>

#include <atmi.h>     /* TUXEDO Header File */
#include <fml32.h>    /* TUXEDO Header File */
#include <fml1632.h>  /* TUXEDO Header File */
#include <userlog.h>  /* TUXEDO Header File */

#include <Python.h>

#include "tuxconvert.h"

PyObject* fml_to_dict(FBFR32* fml) {
    PyObject* result = NULL;
    int res ;
    char * name;
    FLDLEN32 len;
    FLDOCC32 oc;
    FLDID32 id;
    PyObject* dict, *list;
    char* type;
    
    dict = PyDict_New();

#ifdef DEBUG
    Fprint32(fml);
#endif
    id = FIRSTFLDID;
    while (1) {

	len = TUXBUFSIZE;
#ifdef DEBUG
	printf("[run %d] vor fnext() (id = %d, oc = %d, len = %d) \n", 
	       run++, id, oc, len);
#endif
	/* get next field id and occurence */
	res = Fnext32(fml, &id, &oc, NULL, NULL);
	if (res <= 0) break;
	if ((name = Fname32(id)) == NULL) {
	    fprintf(stderr, "Fname(%lu): %s", (long)id, Fstrerror(Ferror));
	    result = NULL;
	    goto leave_func;
	}
	
	if ((list = PyDict_GetItemString (dict, name)) == NULL) {
	    /* key doesn't exist -> insert new list into dict */
	    list = PyList_New(0);
	    PyDict_SetItemString(dict, name, list);
	    Py_DECREF(list);  /* reference now owned by dictionary */
	}     
	
	type = Ftype32(id);
	
	if (strcmp(type, "long") == 0) {
	    long longval = 0;
	    FLDLEN32 longlen  = sizeof (FLDLEN32);
	    PyObject* pyval;
	    
	    Fget32(fml, id, oc, (char*)&longval, &longlen);
	    pyval = Py_BuildValue("i", longval);
	    PyList_Append(list, pyval);
	    Py_DECREF(pyval);  /* reference now owned by list */
	}  else if (strcmp(type, "double") == 0) {
	    double doubleval = 0.0;
	    FLDLEN32 doublelen  = sizeof (FLDLEN32);
	    PyObject* pyval;

	    Fget32(fml, id, oc, (char*)&doubleval, &doublelen);

	    pyval = Py_BuildValue("d", doubleval);
	    PyList_Append(list, pyval);
	    Py_DECREF(pyval);  
	} else if (strcmp(type, "string") == 0) {
	    PyObject* pyval;
	    FLDLEN32 stringlen  = TUXBUFSIZE;
	    char stringval[TUXBUFSIZE] = "";
	    Fget32(fml, id, oc, (char*)&stringval, &stringlen);
	    pyval = Py_BuildValue("s", stringval);
	    PyList_Append(list, pyval);
	    Py_DECREF(pyval);  
	} else {
	    char msg[100];
	    sprintf(msg, "unsupported FML type <%s>", type);
	    PyErr_SetString(PyExc_RuntimeError, msg);
	    fprintf(stderr, "Ftype32(): %s", Fstrerror(Ferror));

	    result =  NULL;
	    goto leave_func;
	}
	
    }
    if (res < 0) {
	PyErr_SetString(PyExc_RuntimeError, "Problems with Fnext()");
	fprintf(stderr, "Fnext32(): %s", Fstrerror(Ferror));

	result =  NULL;
	goto leave_func;
    }
    result = dict;

 leave_func:    
#ifdef DEBUG
    PyObject_Print(result, stdout, 0);
    printf("\n");
#endif
    return result;
}





FBFR32* dict_to_fml(PyObject* dict) {
    FBFR32*        result = NULL;
    int            idx, oc, fldtype;
    FLDID32        id;
    FBFR32*        fml;

    PyObject* keylist;

    keylist = PyDict_Keys(dict);
#ifdef DEBUG
    PyObject_Print(dict, stdout, 0);
    printf("\n");
#endif
    if ((fml = (FBFR32*)tpalloc("FML32", NULL, TUXBUFSIZE)) == NULL) {
	fprintf(stderr, "tpalloc(): %s\n", tpstrerror(tperrno));
	goto leave_func;
    }
    
    if (Finit32(fml, TUXBUFSIZE) < 0) {
	fprintf(stderr, "Finit32(): %s\n", Fstrerror(Ferror));
	goto leave_func;
    }

    for (idx = 0; idx < PyList_Size(keylist); idx++) {
	PyObject*    vallist = NULL;
	PyObject*    key = NULL;
	char*        key_cstring = NULL;

	key = PyList_GetItem(keylist, idx);  /* borrowed reference */

	if (!key) {
	    fprintf(stderr, "PyList_GetItem(keys, %d) returned NULL\n", idx);
	    goto leave_func;
	}
	
	key_cstring = PyString_AsString(key);
	id = Fldid32(key_cstring);
	if (id == BADFLDID) {
	    char tmp[1024] = "";
	    sprintf(tmp, "Fldid32(): %d - %s:", Ferror32, Fstrerror32(Ferror32));
	    PyErr_SetString(PyExc_RuntimeError, tmp);
	    goto leave_func;
	}

	vallist = PyDict_GetItemString(dict, key_cstring);  /* borrowed reference */

	if (PyString_Check(vallist)) {
	    char* cval = NULL;

	    cval = PyString_AsString(vallist);
	    if (cval == NULL) {
		fprintf(stderr, "error in PyString_AsString()\n");
		goto leave_func;
	    }
	    if (Fchgs32(fml, id, 0, cval) < 0) {
		if (Ferror == FNOSPACE) {
		    /* realloc buffer */
		}
		fprintf(stderr, "error in Fchgs() : %s\n", Fstrerror(Ferror));
		goto leave_func;
	    }		    
	} else {	    
	    
	    /* process all occurences (elements of the list) for this ID
               (Field Name) */
	    
	    for (oc = 0; oc < PyList_Size(vallist); oc++) {
		PyObject* pyvalue = NULL;
		if (PyList_Check(vallist)) {
		    pyvalue = PyList_GetItem(vallist, oc);  /* borrowed reference */
		}
		if (pyvalue == NULL) continue;

		/* !!! type given by field id, not by Python types !!! */		

		fldtype = Fldtype32(id);
		
		switch (fldtype) {
		case FLD_LONG:
		    /* convert input to FLD_LONG type */
		
		    if  (PyLong_Check(pyvalue)) {
			long cval = 0;
			FLDLEN32 len = sizeof(cval);
			
			cval = PyLong_AsLong(pyvalue);
			
			if (Fchg32(fml, id, oc,(char*) &cval, len) < 0) {
			    fprintf(stderr, "error in Fchg(): %s\n", Fstrerror(Ferror));
			    goto leave_func;
			}		    
			break;
		    }
		    
		    if (PyInt_Check(pyvalue)) {
			long cval = 0;
			FLDLEN32 len = sizeof(cval);
			
			cval = PyInt_AsLong(pyvalue);
			
			if (Fchg32(fml, id, oc,(char*) &cval, len) < 0) {
			    fprintf(stderr, "error in Fchg(): %s\n", Fstrerror(Ferror));
			    goto leave_func;
			}		    
			break;
		    }

		    if (PyString_Check(pyvalue)) {
			char * cval;
			long lval = 0;

			cval = PyString_AsString(pyvalue);
			if (cval == NULL) {
			    goto leave_func;
			}

			lval = atol(cval);
			if (Fchg32(fml, id, oc,(char*) &lval, (FLDLEN32)sizeof(long)) < 0) {
			    fprintf(stderr, "error in Fchg(): %s\n", Fstrerror(Ferror));
			    goto leave_func;
			}		    
			break;
		    }
		    fprintf(stderr, 
			    "could not convert value for key %s to FML type FLD_LONG\n",
			    key_cstring);
		    goto leave_func;
		    break;
		case FLD_STRING:

		    /* convert input to FLD_STRING type */

		    if (PyFloat_Check(pyvalue)) {
			double cval = 0.0;
			char string[255] = "";
			
			cval = PyFloat_AsDouble(pyvalue);
			sprintf(string, "%f", cval);

			if (Fchgs32(fml, id, oc, string) < 0) {
			    fprintf(stderr, "error in Fchgs(): %s\n", Fstrerror(Ferror));
			    goto leave_func;
			}		    
			break;
		    }
		    
		    if  (PyLong_Check(pyvalue)) {
			long cval = 0;
			char string[255] = "";
			
			cval = PyLong_AsLong(pyvalue);
			sprintf(string, "%ld", cval);
			
			if (Fchgs32(fml, id, oc, string) < 0) {
			    fprintf(stderr, "error in Fchgs(): %s\n", Fstrerror(Ferror));
			    goto leave_func;
			}		    
			break;
		    }
		    
		    if (PyInt_Check(pyvalue)) {
			long cval = 0;
			char string[255] = "";

			cval = PyInt_AsLong(pyvalue);			
			sprintf(string, "%ld", cval);
			
			if (Fchgs32(fml, id, oc, string) < 0) {
			    fprintf(stderr, "error in Fchgs(): %s\n", Fstrerror(Ferror));
			    goto leave_func;
			}		    
			break;
		    }

		    if (PyString_Check(pyvalue)) {
			char * cval;

			cval = PyString_AsString(pyvalue);
			if (cval == NULL) {
			    goto leave_func;
			}

			if (Fchgs32(fml, id, oc, cval) < 0) {
			    fprintf(stderr, "error in Fchgs(): %s\n", Fstrerror(Ferror));
			    goto leave_func;
			}		    
			break;
		    }

		    fprintf(stderr, 
			    "could not convert value for key %s to FML type FLD_STRING\n",
			    key_cstring);
		    goto leave_func;
		    
		    break;
		default:
		    fprintf(stderr, "unsupported FML type %d\n", fldtype);
		    goto leave_func;
		}
		/* Not recognized types do not cause an error and are simply discarded */
	    }
	}
    }
    
    

    result = fml;
 leave_func:
    if (keylist) {
	Py_DECREF(keylist);
    }
    if (!result) {
	tpfree((char*)fml);
    }

#ifdef DEBUG
    Fprint32(result);
#endif

    return result;
}

char* pystring_to_string(PyObject* pystring) {
    char*        result = NULL;
    char*        string = NULL;

    long len = 0;

    len = strlen(PyString_AsString(pystring));

    if ((string = (char*)tpalloc("STRING", NULL, len+1)) == NULL) {
	fprintf(stderr, "tpalloc(): %s\n", tpstrerror(tperrno));
	goto leave_func;
    }

    strcpy(string, PyString_AsString(pystring));

    result = string;
 leave_func:
    if (!result) {
	tpfree((char*)string);
    }
    return result;
}



PyObject* string_to_pystring(char* string) {
    PyObject*     result = NULL;
    PyObject*     pystring = NULL;

    if ((pystring = Py_BuildValue("s", string)) == NULL) {
	fprintf(stderr, "Py_BuildValue(): %s", string);
	goto leave_func;
    }
    
    result = pystring;
 leave_func:
    return result;
}





