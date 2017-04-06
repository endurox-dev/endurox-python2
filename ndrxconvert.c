/* 
   This file implements some functions to convert ENDUROX data types (STRING,
   UBF) to Python data types (string, dictionary) and vice versa 

   (c) 1999 Ralf Henschkowski (ralfh@gmx.ch)
   (c) 2017 Mavimax, SIA

*/

#include <stdio.h>

#include <atmi.h>     /* ENDUROX Header File */
#include <ubf.h>    /* ENDUROX Header File */
#include <userlog.h>  /* ENDUROX Header File */

#include <ndebug.h>
#include <ubfutil.h>
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
	int type;
	int run=0;

	dict = PyDict_New();

	ndrx_debug_dump_UBF(log_debug, "ubf_to_dict enters with buffer", ubf);
	id = BFIRSTFLDID;
	while (1)
	{
		len = ATMI_MSG_MAX_SIZE;

		NDRX_LOG(log_info, "[run %d] vor bnext() (id = %d, oc = %d, len = %d) \n", 
			run++, id, oc, len);

		/* get next field id and occurence */
		res = Bnext(ubf, &id, &oc, NULL, NULL);
		if (res <= 0) break;

		if ((name = Bfname(id)) == NULL)
		{
			NDRX_LOG(log_info, "Bfname(%lu): %s", (long)id, Bstrerror(Berror));
			result = NULL;
			goto leave_func;
		}

		if ((list = PyDict_GetItemString (dict, name)) == NULL)
		{
			/* key doesn't exist -> insert new list into dict */
			list = PyList_New(0);
			PyDict_SetItemString(dict, name, list);
			Py_DECREF(list);  /* reference now owned by dictionary */
		}     

		type = Bfldtype(id);
		switch (type)
		{
			case BFLD_LONG:
			{
				long longval = 0;
				PyObject* pyval;
				
				Bget(ubf, id, oc, (char*)&longval, 0L);
				pyval = Py_BuildValue("i", longval);
				PyList_Append(list, pyval);
				Py_DECREF(pyval);  /* reference now owned by list */
				break;
			}
			case BFLD_SHORT:
			{
				short shortval = 0.0;
				PyObject* pyval;

				/* TODO: Add error handler: */
				Bget(ubf, id, oc, (char*)&shortval, 0L);

				pyval = Py_BuildValue("h", shortval);
				PyList_Append(list, pyval);
				Py_DECREF(pyval);  
				break;
			}
			case BFLD_CHAR:
			{
				short charval = 0.0;
				PyObject* pyval;

				/* TODO: Add error handler: */
				Bget(ubf, id, oc, (char*)&charval, 0L);

				pyval = Py_BuildValue("b", charval);
				PyList_Append(list, pyval);
				Py_DECREF(pyval);  
				break;
			}
			case BFLD_DOUBLE:
			case BFLD_FLOAT:
			{
				double doubleval = 0.0;
				BFLDLEN doublelen  = sizeof (BFLDLEN);
				PyObject* pyval;

				/* TODO: Add error handler: */
				Bget(ubf, id, oc, (char*)&doubleval, &doublelen);

				pyval = Py_BuildValue("d", doubleval);
				PyList_Append(list, pyval);
				Py_DECREF(pyval);  
				break;
			}
			case BFLD_STRING:
			{
				PyObject* pyval;
				BFLDLEN stringlen  = ATMI_MSG_MAX_SIZE;
				char stringval[ATMI_MSG_MAX_SIZE] = "";
				Bget(ubf, id, oc, (char*)&stringval, &stringlen);
				pyval = Py_BuildValue("s", stringval);
				PyList_Append(list, pyval);
				Py_DECREF(pyval);  
				break;
			}
			default:
			{
				char msg[100];
				sprintf(msg, "unsupported UBF type <%d>", type);
				PyErr_SetString(PyExc_RuntimeError, msg);
				NDRX_LOG(log_info, "Btype(): %s", Bstrerror(Berror));

				result =  NULL;
				goto leave_func;
			}
		}

	}
	
	if (res < 0)
	{
		PyErr_SetString(PyExc_RuntimeError, "Problems with Bnext()");
		NDRX_LOG(log_info, "Bnext(): %s", Bstrerror(Berror));

		result =  NULL;
		goto leave_func;
	}
	result = dict;

leave_func:

	/* Print the buffer we got */
	if (G_ndrx_debug.level>=5)
	{
		PyObject_Print(result, G_ndrx_debug.dbg_f_ptr, 0);
		fprintf(G_ndrx_debug.dbg_f_ptr, "\n");
	}

	return result;
}




/*
 * Convert dictionary to UBF
 */
UBFH* dict_to_ubf(PyObject* dict)
{
	UBFH*        result = NULL;
	int            idx, oc/* , bfldtype*/;
	BFLDID        id;
	UBFH*        ubf;
	
	PyObject* keylist;

	keylist = PyDict_Keys(dict);

	/* This will also cause G_ndrx_debug to make init... */
	NDRX_LOG(log_info, "Into dict_to_ubf()");


	NDRX_LOG(log_debug, "Converting out: ");

	if (G_ndrx_debug.level>=5)
	{
		PyObject_Print(dict, G_ndrx_debug.dbg_f_ptr, 0);
		fprintf(G_ndrx_debug.dbg_f_ptr, "\n");
	}


	if ((ubf = (UBFH*)tpalloc("UBF", NULL, NDRXBUFSIZE)) == NULL)
	{
		NDRX_LOG(log_info, "tpalloc(): %s", tpstrerror(tperrno));
		goto leave_func;
	}

	if (Binit(ubf, NDRXBUFSIZE) < 0)
	{
		NDRX_LOG(log_info, "Binit(): %s", Bstrerror(Berror));
		goto leave_func;
	}

	for (idx = 0; idx < PyList_Size(keylist); idx++)
	{
		PyObject*    vallist = NULL;
		PyObject*    key = NULL;
		char*        key_cstring = NULL;

		key = PyList_GetItem(keylist, idx);  /* borrowed reference */

		if (!key)
		{
			NDRX_LOG(log_info, "PyList_GetItem(keys, %d) returned NULL\n", idx);
			goto leave_func;
		}

		key_cstring = PyString_AsString(key);
		id = Bfldid(key_cstring);
		if (id == BBADFLDID)
		{
			char tmp[1024] = "";
			sprintf(tmp, "Bfldid(): %d - %s:", Berror, Bstrerror(Berror));
			PyErr_SetString(PyExc_RuntimeError, tmp);
			goto leave_func;
		}

		/* borrowed reference */
		vallist = PyDict_GetItemString(dict, key_cstring);

		if (PyString_Check(vallist))
		{
			char* cval = NULL;

			cval = PyString_AsString(vallist);
			if (cval == NULL)
			{
				NDRX_LOG(log_info, "error in PyString_AsString()");
				goto leave_func;
			}
			
			if (CBchg(ubf, id, 0, cval, 0L, BFLD_STRING) < 0)
			{
				if (Berror == BNOSPACE)
				{
					/* realloc buffer */
				}
				NDRX_LOG(log_error, "error in Bchgs() : %s\n", 
					 Bstrerror(Berror));
				goto leave_func;
			}	
		} 
		else
		{
			NDRX_LOG(log_debug,  "Not a string STRING!");
			
			/* process all occurences (elements of the list) for this ID
			(Field Name) */
			
			for (oc = 0; oc < PyList_Size(vallist); oc++)
			{
				PyObject* pyvalue = NULL;
				if (PyList_Check(vallist))
				{
					/* borrowed reference */
					pyvalue = PyList_GetItem(vallist, oc);
				}
				if (pyvalue == NULL) continue;

				/* !!! type given by field id, not by Python types !!! */		

				/* bfldtype = Bfldtype(id); */

				/* convert input to BFLD_STRING type */

				if (PyFloat_Check(pyvalue))
				{
					double cval = 0.0;
					char string[255] = "";
					
					cval = PyFloat_AsDouble(pyvalue);
					sprintf(string, "%f", cval);

					if (CBchg(ubf, id, oc, string, 0L, BFLD_STRING) < 0)
					{
						NDRX_LOG(log_error, "error in Bchgs(): %s", 
							Bstrerror(Berror));
						goto leave_func;
					}		    
					continue;
				}
				
				if  (PyLong_Check(pyvalue))
				{
					long cval = 0;
					char string[255] = "";
					
					cval = PyLong_AsLong(pyvalue);
					sprintf(string, "%ld", cval);
					
					if (CBchg(ubf, id, oc, string, 0L, BFLD_STRING) < 0) 
					{
						NDRX_LOG(log_error, "error in Bchgs(): %s", 
							 Bstrerror(Berror));
						goto leave_func;
					}		    
					continue;
				}
				
				if (PyInt_Check(pyvalue))
				{
					long cval = 0;
					char string[255] = "";

					cval = PyInt_AsLong(pyvalue);			
					sprintf(string, "%ld", cval);
					
					if (CBchg(ubf, id, oc, string, 0L, BFLD_STRING) < 0)
					{
						NDRX_LOG(log_error, "error in Bchgs(): %s",
							 Bstrerror(Berror));
						goto leave_func;
					}		    
					continue;
				}

				if (PyString_Check(pyvalue) || 1)
				{
					char * cval;

					cval = PyString_AsString(pyvalue);
					if (cval == NULL)
					{
						goto leave_func;
					}

					if (Bchgs(ubf, id, oc, cval) < 0)
					{
						NDRX_LOG(log_error, "error in Bchgs(): %s", 
							 Bstrerror(Berror));
						goto leave_func;
					}		    
					continue;
				}

				NDRX_LOG(log_error,
					"could not convert value for key %s "
					"to ubf type BFLD_STRING\n",
					key_cstring);
				
				goto leave_func;
			}
		}
	}

	result = ubf;
leave_func:
	if (keylist)
	{
		Py_DECREF(keylist);
	}
	
	if (!result)
	{
		tpfree((char*)ubf);
	}

	ndrx_debug_dump_UBF(log_debug, "Result buffer", result);

	return result;
}

char* pystring_to_string(PyObject* pystring)
{
	char*        result = NULL;
	char*        string = NULL;

	long len = 0;

	len = strlen(PyString_AsString(pystring));

	if ((string = (char*)tpalloc("STRING", NULL, len+1)) == NULL)
	{
		NDRX_LOG(log_error, "tpalloc(): %s", tpstrerror(tperrno));
		goto leave_func;
	}

	strcpy(string, PyString_AsString(pystring));

	result = string;
	
leave_func:

	if (!result)
	{
		tpfree((char*)string);
	}
	
	return result;
}



PyObject* string_to_pystring(char* string)
{
	PyObject*     result = NULL;
	PyObject*     pystring = NULL;

	if ((pystring = Py_BuildValue("s", string)) == NULL)
	{
		NDRX_LOG(log_error, "Py_BuildValue(): %s", string);
		goto leave_func;
	}

	result = pystring;
leave_func:
	return result;
}





