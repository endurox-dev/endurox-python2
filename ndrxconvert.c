/* 
   This file implements some functions to convert ENDUROX data types (STRING,
   UBF) to Python data types (string, dictionary) and vice versa 

   (c) 1999 Ralf Henschkowski (ralfh@gmx.ch)

*/

#include <stdio.h>

#include <atmi.h>     /* ENDUROX Header File */
#include <ubf.h>    /* ENDUROX Header File */
#include <userlog.h>  /* ENDUROX Header File */

#include <Python.h>

#include "ndrxconvert.h"

PyObject* ubf_to_dict(UBFH* ubf) {
    PyObject* result = NULL;
    int res ;
    char * name;
    BFLDLEN len;
    BFLDOCC oc;
    BFLDID id;
    PyObject* dict, *list;
    char* type;
    
    dict = PyDict_New();

#ifdef DEBUG
    Bprint(ubf);
#endif
    id = BFIRSTFLDID;
    while (1) {

	len = NDRXBUFSIZE;
#ifdef DEBUG
	printf("[run %d] vor bnext() (id = %d, oc = %d, len = %d) \n", 
	       run++, id, oc, len);
#endif
	/* get next field id and occurence */
	res = Bnext(ubf, &id, &oc, NULL, NULL);
	if (res <= 0) break;
	if ((name = Bbfname(id)) == NULL) {
	    bprintf(stderr, "Bfname(%lu): %s", (long)id, Bstrerror(Berror));
	    result = NULL;
	    goto leave_func;
	}
	
	if ((list = PyDict_GetItemString (dict, name)) == NULL) {
	    /* key doesn't exist -> insert new list into dict */
	    list = PyList_New(0);
	    PyDict_SetItemString(dict, name, list);
	    Py_DECREF(list);  /* reference now owned by dictionary */
	}     
	
	type = Btype(id);
	
	if (strcmp(type, "long") == 0) {
	    long longval = 0;
	    BFLDLEN longlen  = sizeof (BFLDLEN);
	    PyObject* pyval;
	    
	    Bget(ubf, id, oc, (char*)&longval, &longlen);
	    pyval = Py_BuildValue("i", longval);
	    PyList_Append(list, pyval);
	    Py_DECREF(pyval);  /* reference now owned by list */
	}  else if (strcmp(type, "double") == 0) {
	    double doubleval = 0.0;
	    BFLDLEN doublelen  = sizeof (BFLDLEN);
	    PyObject* pyval;

	    Bget(ubf, id, oc, (char*)&doubleval, &doublelen);

	    pyval = Py_BuildValue("d", doubleval);
	    PyList_Append(list, pyval);
	    Py_DECREF(pyval);  
	} else if (strcmp(type, "string") == 0) {
	    PyObject* pyval;
	    BFLDLEN stringlen  = NDRXBUFSIZE;
	    char stringval[NDRXBUFSIZE] = "";
	    Bget(ubf, id, oc, (char*)&stringval, &stringlen);
	    pyval = Py_BuildValue("s", stringval);
	    PyList_Append(list, pyval);
	    Py_DECREF(pyval);  
	} else {
	    char msg[100];
	    sprintf(msg, "unsupported UBF type <%s>", type);
	    PyErr_SetString(PyExc_RuntimeError, msg);
	    bprintf(stderr, "Btype(): %s", Bstrerror(Berror));

	    result =  NULL;
	    goto leave_func;
	}
	
    }
    if (res < 0) {
	PyErr_SetString(PyExc_RuntimeError, "Problems with Bnext()");
	bprintf(stderr, "Bnext(): %s", Bstrerror(Berror));

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





UBFH* dict_to_ubf(PyObject* dict) {
    UBFH*        result = NULL;
    int            idx, oc, bfldtype;
    BFLDID        id;
    UBFH*        ubf;

    PyObject* keylist;

    keylist = PyDict_Keys(dict);
#ifdef DEBUG
    PyObject_Print(dict, stdout, 0);
    printf("\n");
#endif
    if ((ubf = (UBFH*)tpalloc("UBF", NULL, NDRXBUFSIZE)) == NULL) {
	bprintf(stderr, "tpalloc(): %s\n", tpstrerror(tperrno));
	goto leave_func;
    }
    
    if (Binit(ubf, NDRXBUFSIZE) < 0) {
	bprintf(stderr, "Binit(): %s\n", Bstrerror(Berror));
	goto leave_func;
    }

    for (idx = 0; idx < PyList_Size(keylist); idx++) {
	PyObject*    vallist = NULL;
	PyObject*    key = NULL;
	char*        key_cstring = NULL;

	key = PyList_GetItem(keylist, idx);  /* borrowed reference */

	if (!key) {
	    bprintf(stderr, "PyList_GetItem(keys, %d) returned NULL\n", idx);
	    goto leave_func;
	}
	
	key_cstring = PyString_AsString(key);
	id = Bfldid(key_cstring);
	if (id == BBADFLDID) {
	    char tmp[1024] = "";
	    sprintf(tmp, "Bfldid(): %d - %s:", Berror, Bstrerror(Berror));
	    PyErr_SetString(PyExc_RuntimeError, tmp);
	    goto leave_func;
	}

	vallist = PyDict_GetItemString(dict, key_cstring);  /* borrowed reference */

	if (PyString_Check(vallist)) {
	    char* cval = NULL;

	    cval = PyString_AsString(vallist);
	    if (cval == NULL) {
		bprintf(stderr, "error in PyString_AsString()\n");
		goto leave_func;
	    }
	    if (Bchgs(ubf, id, 0, cval) < 0) {
		if (Berror == BNOSPACE) {
		    /* realloc buffer */
		}
		bprintf(stderr, "error in Bchgs() : %s\n", Bstrerror(Berror));
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

		bfldtype = Bbfldtype(id);
		
		switch (bfldtype) {
		case BFLD_LONG:
		    /* convert input to BFLD_LONG type */
		
		    if  (PyLong_Check(pyvalue)) {
			long cval = 0;
			BFLDLEN len = sizeof(cval);
			
			cval = PyLong_AsLong(pyvalue);
			
			if (Bchg(ubf, id, oc,(char*) &cval, len) < 0) {
			    bprintf(stderr, "error in Bchg(): %s\n", Bstrerror(Berror));
			    goto leave_func;
			}		    
			break;
		    }
		    
		    if (PyInt_Check(pyvalue)) {
			long cval = 0;
			BFLDLEN len = sizeof(cval);
			
			cval = PyInt_AsLong(pyvalue);
			
			if (Bchg(ubf, id, oc,(char*) &cval, len) < 0) {
			    bprintf(stderr, "error in Bchg(): %s\n", Bstrerror(Berror));
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
			if (Bchg(ubf, id, oc,(char*) &lval, (BFLDLEN)sizeof(long)) < 0) {
			    bprintf(stderr, "error in Bchg(): %s\n", Bstrerror(Berror));
			    goto leave_func;
			}		    
			break;
		    }
		    bprintf(stderr, 
			    "could not convert value for key %s to UBF type BFLD_LONG\n",
			    key_cstring);
		    goto leave_func;
		    break;
		case BFLD_STRING:

		    /* convert input to BFLD_STRING type */

		    if (PyFloat_Check(pyvalue)) {
			double cval = 0.0;
			char string[255] = "";
			
			cval = PyFloat_AsDouble(pyvalue);
			sprintf(string, "%f", cval);

			if (Bchgs(ubf, id, oc, string) < 0) {
			    bprintf(stderr, "error in Bchgs(): %s\n", Bstrerror(Berror));
			    goto leave_func;
			}		    
			break;
		    }
		    
		    if  (PyLong_Check(pyvalue)) {
			long cval = 0;
			char string[255] = "";
			
			cval = PyLong_AsLong(pyvalue);
			sprintf(string, "%ld", cval);
			
			if (Bchgs(ubf, id, oc, string) < 0) {
			    bprintf(stderr, "error in Bchgs(): %s\n", Bstrerror(Berror));
			    goto leave_func;
			}		    
			break;
		    }
		    
		    if (PyInt_Check(pyvalue)) {
			long cval = 0;
			char string[255] = "";

			cval = PyInt_AsLong(pyvalue);			
			sprintf(string, "%ld", cval);
			
			if (Bchgs(ubf, id, oc, string) < 0) {
			    bprintf(stderr, "error in Bchgs(): %s\n", Bstrerror(Berror));
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

			if (Bchgs(ubf, id, oc, cval) < 0) {
			    bprintf(stderr, "error in Bchgs(): %s\n", Bstrerror(Berror));
			    goto leave_func;
			}		    
			break;
		    }

		    bprintf(stderr, 
			    "could not convert value for key %s to UBF type BFLD_STRING\n",
			    key_cstring);
		    goto leave_func;
		    
		    break;
		default:
		    bprintf(stderr, "unsupported UBF type %d\n", bfldtype);
		    goto leave_func;
		}
		/* Not recognized types do not cause an error and are simply discarded */
	    }
	}
    }
    
    

    result = ubf;
 leave_func:
    if (keylist) {
	Py_DECREF(keylist);
    }
    if (!result) {
	tpfree((char*)ubf);
    }

#ifdef DEBUG
    Bprint(result);
#endif

    return result;
}

char* pystring_to_string(PyObject* pystring) {
    char*        result = NULL;
    char*        string = NULL;

    long len = 0;

    len = strlen(PyString_AsString(pystring));

    if ((string = (char*)tpalloc("STRING", NULL, len+1)) == NULL) {
	bprintf(stderr, "tpalloc(): %s\n", tpstrerror(tperrno));
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
	bprintf(stderr, "Py_BuildValue(): %s", string);
	goto leave_func;
    }
    
    result = pystring;
 leave_func:
    return result;
}





