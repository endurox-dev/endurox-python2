/* 

   This file implements a Python extension module for accessing the ATMI API of the
   ENDURO/X Middleware. It can be used to build clients or servers in the Python language.  
   
   (c) 1999,2000 Ralf Henschkowski (ralfh@gmx.ch)
   (c) 2017 Mavimax, SIA
   
*/



/* {{{ includes */

#include <stdio.h>              /* System header file */

#ifdef USE_THREADS
#include <pthread.h>            /* System header file */  
#endif /* USE_THREADS */

#include <string.h>
#include <xa.h>                 /* ENDUROX Header File */
#include <atmi.h>               /* ENDUROX Header File */
#include "ubf.h"                /* ENDUROX Header File */
#include <userlog.h>            /* ENDUROX Header File */
#include <ubf.h>              /* ENDUROX Header File */
#include <tpadm.h>

#include <Python.h>


#include <ndebug.h>
#include "ndrxconvert.h"         /* Needed for some helper functions to convert Python 
				   data types to ENDUROX data types and vice versa */


/* }}} */
/* {{{ defines & typedefs */

#define MAX_PY_SERVICES       100

#define MAX_SVC_NAME_LEN      15 + 1 
#define MAX_METHOD_NAME_LEN      1024 


typedef struct {
    char name[MAX_SVC_NAME_LEN];  /* constant from atmi.h */
    char method[MAX_METHOD_NAME_LEN];            
} service_entry;




/* }}} */
/* {{{ local function prototypes */

/* Forward declarions of local functions */

#ifndef NDRXWS
void endurox_dispatch(TPSVCINFO * rqst);
#endif /* not NDRXWS */

static PyObject * makeargvobject(int argc, char** argv);
static int find_entry(const char* name);
static int find_free_entry(const char* name);
static char* transform_py_to_ndrx(PyObject* res_py);
static PyObject* transform_ndrxpy_to_py(char* ndrxbuf);
static PyObject* ndrxpy_tppost(PyObject* self, PyObject* arg);
static PyObject* ndrxpy_tpsubscribe(PyObject* self, PyObject* arg);
static PyObject* ndrxpy_tpunsubscribe(PyObject* self, PyObject* arg);
static PyObject* ndrxpy_tpnotify(PyObject* self, PyObject* arg);
static PyObject* ndrxpy_tpbroadcast(PyObject* self, PyObject* arg);
static PyObject* ndrxpy_tpsetunsol(PyObject* self, PyObject* arg);
static PyObject* ndrxpy_tpchkunsol(PyObject* self, PyObject* arg);
static void mainloop(int argc, char** argv);
static PyObject * ndrxpy_tpcall(PyObject * self, PyObject * args);
static PyObject * ndrxpy_tpacall(PyObject * self, PyObject * args);
static PyObject * ndrxpy_tpadmcall(PyObject * self, PyObject * args);
static PyObject * ndrxpy_tpforward(PyObject * self, PyObject * args);
static PyObject * ndrx_mainloop(PyObject * self, PyObject * args);
static PyObject* ndrxpy_tpadvertise(PyObject* self, PyObject* arg);
static PyObject* ndrxpy_tpunadvertise(PyObject* self, PyObject* arg);
static PyObject * ndrxpy_tpopen(PyObject * self, PyObject * args);
static PyObject * ndrxpy_tpclose(PyObject * self, PyObject * args);
static PyObject * ndrxpy_tpgetrply(PyObject * self, PyObject * args);
static PyObject * ndrxpy_tpconnect(PyObject * self, PyObject * args);
static PyObject * ndrxpy_tpsend(PyObject * self, PyObject * args);
static PyObject * ndrxpy_tprecv(PyObject * self, PyObject * args);
static PyObject * ndrxpy_tpbegin(PyObject * self, PyObject * args);
static PyObject * ndrxpy_tpcommit(PyObject * self, PyObject * args);
static PyObject * ndrxpy_tpabort(PyObject * self, PyObject * args);
static PyObject * ndrxpy_tpsuspend(PyObject * self, PyObject * args);
static PyObject * ndrxpy_tpresume(PyObject * self, PyObject * args);
static PyObject * ndrxpy_tpgetlev(PyObject * self, PyObject * args);
static PyObject * ndrxpy_tpdiscon(PyObject * self, PyObject * args);
static PyObject * ndrxpy_userlog(PyObject * self, PyObject * args);
static PyObject * ndrxpy_tplog(PyObject * self, PyObject * args);
static void ins(PyObject *d, char *s, long x);
static PyObject * ndrxpy_tpinit(PyObject * self, PyObject * args);
static PyObject * ndrxpy_tpgetctxt(PyObject * self, PyObject * args);
static PyObject * ndrxpy_tpsetctxt(PyObject * self, PyObject * args);
static PyObject * ndrxpy_tpchkauth(PyObject * self, PyObject * args);
static PyObject * ndrxpy_tpterm(PyObject * self, PyObject * args);
static PyObject* ndrxpy_get_tpurcode(PyObject* self, PyObject * args);
static PyObject* ndrxpy_set_tpurcode(PyObject* self, PyObject * args);
static PyObject * ndrxpy_tpenqueue(PyObject * self, PyObject * args);
static PyObject * ndrxpy_tpdequeue(PyObject * self, PyObject * args);
static PyObject * ndrxpy_tpgetnodeid(PyObject * self, PyObject * args);

/* }}} */
/* {{{ local variables */

static PyMethodDef ndrxpy_methods[] = {
    {"tpinit",	         ndrxpy_tpinit,	    METH_VARARGS, "args: {usrname: '', clt=''}"},
    {"tpgetctxt",	 ndrxpy_tpgetctxt,	    METH_VARARGS, "args: {} -> context"},
    {"tpsetctxt",	 ndrxpy_tpsetctxt,	    METH_VARARGS, "args: {context}"},
    {"tpterm",	         ndrxpy_tpterm,	    METH_VARARGS},
    {"tpchkauth",	 ndrxpy_tpchkauth,	    METH_VARARGS},
    {"tpcall",	         ndrxpy_tpcall,	    METH_VARARGS, "args: ('service', {args}|'args')"},
    {"tpacall",	         ndrxpy_tpacall,	    METH_VARARGS, "args: ('service', {args}|'args') -> handle"},
    {"tpconnect",	 ndrxpy_tpconnect,	    METH_VARARGS, "args: ('service', {args}|'args') -> handle"},
    {"tpsend",           ndrxpy_tpsend,        METH_VARARGS, "args: (handle, input, [flags]) -> revent"},
    {"tpadmcall",	 ndrxpy_tpadmcall,	    METH_VARARGS, "args: ({args}|'flags')"},
    {"tpopen",           ndrxpy_tpopen,	    METH_VARARGS, ""},
    {"tpclose",          ndrxpy_tpclose,	    METH_VARARGS, ""},
    {"tpadvertise",      ndrxpy_tpadvertise,   METH_VARARGS},
    {"tpunadvertise",    ndrxpy_tpunadvertise, METH_VARARGS},
    {"mainloop",	 ndrx_mainloop,	    METH_VARARGS},
    {"tpforward",	 ndrxpy_tpforward,	    METH_VARARGS, "args: ('service', {args}|'args')"},
    {"tpcommit",         ndrxpy_tpcommit,	    METH_VARARGS, ""},
    {"tpabort",          ndrxpy_tpabort,	    METH_VARARGS, ""},
    {"tpbegin",          ndrxpy_tpbegin,	    METH_VARARGS, "args: timeout"},
    {"tpsuspend",        ndrxpy_tpsuspend,	    METH_VARARGS, ""},
    {"tpresume",         ndrxpy_tpresume,	    METH_VARARGS, ""},
    {"tpgetlev",         ndrxpy_tpgetlev,	    METH_VARARGS, ""},
    {"tprecv",           ndrxpy_tprecv,	    METH_VARARGS, ""},
    {"tpdiscon",	 ndrxpy_tpdiscon,	    METH_VARARGS, ""},
    {"tpgetrply",	 ndrxpy_tpgetrply,	    METH_VARARGS, ""},
    {"tpenqueue",	 ndrxpy_tpenqueue,	    METH_VARARGS, "args: ('qspace', 'qname', data, {qctl})"},
    {"tpdequeue",	 ndrxpy_tpdequeue,	    METH_VARARGS, "args: ('qspace', 'qname', {qctl})"},
    {"tppost",           ndrxpy_tppost,        METH_VARARGS},
    {"tpsubscribe",      ndrxpy_tpsubscribe,   METH_VARARGS, "args: ('expr', 'filter', {evctl}) -> handle"},
    {"tpunsubscribe",    ndrxpy_tpunsubscribe, METH_VARARGS, "args: (handle)"},
    {"tpnotify",         ndrxpy_tpnotify,      METH_VARARGS},
    {"tpbroadcast",      ndrxpy_tpbroadcast,   METH_VARARGS},
    {"tpsetunsol",       ndrxpy_tpsetunsol,    METH_VARARGS},
    {"tpchkunsol",       ndrxpy_tpchkunsol,    METH_VARARGS},
    {"userlog",          ndrxpy_userlog,       METH_VARARGS},
    {"tplog",            ndrxpy_tplog,         METH_VARARGS},
    {"get_tpurcode",     ndrxpy_get_tpurcode,  METH_VARARGS},
    {"set_tpurcode",     ndrxpy_set_tpurcode,  METH_VARARGS},
    {"tpgetnodeid",      ndrxpy_tpgetnodeid,   METH_VARARGS, ""},
    {NULL,		 NULL,		    0}
};

/* static variables: only used in the server */


 /* Flag indicating that a server is running (in a ENDUROX sense) */
static int _server_is_running = 0;      

/* Holds the urcode (user return code) to be returned with the next
   tpreturn */
static int _set_tpurcode = 0;            

/* Table to hold all the services currently advertised by this server */
static service_entry _registered_services[MAX_PY_SERVICES];

/* Reference to the Python object that implements the server */
static PyObject*  _server_obj = NULL;    

/* Reference to the Python function that eventually reloads a server object
   during server runtime (for debugging purposes */
static PyObject*  _reloader_function = NULL;    

/* Flag indicating that a tpforward() instead of a tpreturn() should be
   used. See ndrxpy_tpforward() for details */
static int _forward = 0;

/* Name of the service to which the request should be forwarded */
static char* _forward_service = NULL;

/* The data that should be forwarded */
static PyObject* _forward_pydata = NULL;

/* Holds the Unsolicited Message Handler function */
static PyObject * py_unsol_handler = NULL;


/* }}} */


/* **************************************** */
/*                                          */
/*     Definitions of local functions       */
/*                                          */
/* **************************************** */
/* {{{ makeargvobject */

/* This function is taken from the Python interpreter's source code. It converts the C
   argc/argv to a Python argv list */

static PyObject *
makeargvobject(int argc, char** argv)
{
    PyObject *av;
    if (argc <= 0 || argv == NULL) {
	/* Ensure at least one (empty) argument is seen */
	static char *empty_argv[1] = {""};
	argv = empty_argv;
	argc = 1;
    }
    av = PyList_New(argc);
    if (av != NULL) {
	int i;
	for (i = 0; i < argc; i++) {
	    PyObject *v = PyString_FromString(argv[i]);
	    if (v == NULL) {
		Py_DECREF(av);
		av = NULL;
		break;
	    }
	    PyList_SetItem(av, i, v);
	}
    }
    return av;
}

/* }}} */

/* {{{ find_entry() */

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  find_entry() looks up a service in the static services table. 

  int find_entry       Return: index to _registered_services array or -1 
                               if the service could not be found

  const char* name     Name of the service                                 :IN
  +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


static int find_entry(const char* name) {
    int i;
    for (i = 0; i < MAX_PY_SERVICES; i++) {
	if (!strcmp(name, _registered_services[i].name)) {
	    return i;
	}
    }
    return -1;
}

/* }}} */
/* {{{ find_free_entry() */

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  find_free_entry() finds a free slot for a given service in the static
  services table. If the service is already listed in the table, that slot
  is returned

  int find_free_entry Return: index to a free slot in _registered_services
                              or -1 if no slot could not be found

  const char* name    Name of the service that should be inserted         :IN
  +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


static int find_free_entry(const char* name) {
    int i ;
    for (i = 0; i < MAX_PY_SERVICES; i++) {
	if (_registered_services[i].name[0] == '\0' || (!strcmp(_registered_services[i].name, name)) ) {
	    NDRX_LOG(log_debug, "find returning %d\n", i);
 	    return i;
	}
    }
    return -1;
}

/* }}} */

/* {{{ delete_entry() */

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Delete a given service from the static service table

  int delete_entry    Return: index to the freed slot

  const char* name    Name of the service to be deleted                   :IN
  +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

static int delete_entry(const char* name) {
    int i;
    for (i = 0; i < MAX_PY_SERVICES; i++) {
	if (!strcmp(_registered_services[i].name, name)) {
	    _registered_services[i].name[0]   = '\0';
	    _registered_services[i].method[0] = '\0';
	    return i;
	}
    }
    return i;
}

/* }}} */

/* {{{ ins() */

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Helper function to insert a value x / key s into a dictionary d
  
  PyObject *d   Dictionary                                          :IN
  
  char *s       Key                                                 :IN
  
  long x        Value                                               :IN
  +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


static void
ins(PyObject *d, char *s, long x)
{
    PyObject *v = PyLong_FromLong(x);
    if (v) {
	PyDict_SetItemString(d, s, v);
	Py_DECREF(v);
    }
}

/* }}} */

/* {{{ transform_py_to_ndrx() */

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  This function checks for the Python-type of its arguments and returns a
  corresponding ENDUROX typed buffer. Currently, only strings or
  dictionaries are allowed

  char* transform_py_to_ndrx   Return: pointer to a ENDUROX typed buffer

  PyObject* res_py            Python object                             :IN
  +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


static char* transform_py_to_ndrx(PyObject* res_py) {
    char* res_ndrx = NULL;
    if (PyDict_Check(res_py)) {
	res_ndrx = (char*)dict_to_ubf(res_py);
    } else if (PyString_Check(res_py)) {
	res_ndrx = pystring_to_string(res_py);
    } else {
	PyErr_SetString(PyExc_RuntimeError, "Only String or Dictionary arguments are allowed");
    }
    return res_ndrx;
}    

/* }}} */
/* {{{ transform_ndrxpy_to_py() */


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  This function converts a ENDUROX typed buffer to the corresponding Python
  types (string or dictionary). Currently, only STRING and UBF buffers
  are allowed.

  PyObject* transform_ndrxpy_to_py  Return: Python object 

  char* ndrxbuf                   pointer to a ENDUROX typed buffer         :IN
  +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


static PyObject* transform_ndrxpy_to_py(char* ndrxbuf) {
    char buffer_type[100] = "";
    char buffer_subtype[100] = "";
    PyObject* obj = NULL;

    if (tptypes(ndrxbuf, buffer_type, buffer_subtype) < 0) {
	char tmp[200] = "";
	sprintf(tmp, "tptypes() : %d - %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }


    if (!strcmp(buffer_type, "UBF")) {
	if ((obj = ubf_to_dict((UBFH*)ndrxbuf)) == NULL) {

	    NDRX_LOG(log_debug, "no ubf buffer");

	    goto leave_func;
	}	
    } else if (!strcmp(buffer_type, "STRING")) {
	if ((obj = string_to_pystring((char*)ndrxbuf)) == NULL) {

	    NDRX_LOG(log_debug, "no string buffer");

	    goto leave_func;
	}	
    }	

 leave_func:

	if (G_ndrx_debug.level>=5)
	{
		PyObject_Print(obj, G_ndrx_debug.dbg_f_ptr, 0);
		fprintf(G_ndrx_debug.dbg_f_ptr, "");
	}

    return obj;
}

/* }}} */

#ifndef NDRXWS

/* {{{ mainloop() */

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  This function is taken from the ENDUROX generated main. When called, the
  server is under ENDUROX control and can process requests. This function
  returns only when the server is shut down.

  int argc       # of command line arguments                            :IN
  
  char** argv    command line arguments                                 :IN
  +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/



static void
mainloop(int argc, char** argv) {

#ifdef TMMAINEXIT
#include "mainexit.h"
#endif
    
    ndrx_main(argc, argv);
}

/* }}} */


#endif /* NDRXWS */


/* {{{ ndrxpy_tpcall() */

static PyObject * 
ndrxpy_tpcall(PyObject * self, PyObject * args)
{
    PyObject * result = NULL;
    PyObject * input_py = NULL;
    PyObject * flags_py = NULL;
    PyObject * service_py = NULL;

    char* service_name;
    char* ndrxbuf = NULL;
    
    long outlen = 0;
    long flags = 0;

    if (PyArg_ParseTuple(args, "OO|O", &service_py, &input_py, &flags_py) < 0) {
	goto leave_func;
    }	
    if (!service_py || ((service_name = PyString_AsString(service_py)) == NULL)) {
	PyErr_SetString(PyExc_RuntimeError, "tpcall(): No service name and/or arguments given");
	goto leave_func;
    }

    if (strlen(service_name) > MAX_SVC_NAME_LEN) {
	PyErr_SetString(PyExc_RuntimeError, "tpcall(): Service name length too long");
	goto leave_func;
    }

    if (flags_py) {
	if ((flags = PyLong_AsLong(flags_py)) < 0) {
	    PyErr_SetString(PyExc_RuntimeError, "tpcall(): Bad flags given");
	    goto leave_func;
	}
    }

    if ((ndrxbuf = transform_py_to_ndrx(input_py)) == NULL) {
	goto leave_func;
    }

    {
	char bubfname[200] = "";
	tptypes(ndrxbuf, bubfname, NULL);
	NDRX_LOG(log_debug, "calling tpcall(%s, [%s]...)", service_name, bubfname);
    }

    
    if (tpcall(service_name, ndrxbuf, 0, &ndrxbuf, &outlen, flags ) < 0) {
	char tmp[200] = "";
	sprintf(tmp, "tpcall(): %d - %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }
    
    if ((result = transform_ndrxpy_to_py(ndrxbuf)) == NULL) {
	goto leave_func;
    }

 leave_func:
    if (ndrxbuf) tpfree(ndrxbuf);
    return result;
}

/* }}} */

#ifndef NDRXWS
/* {{{ ndrxpy_tpadmcall() */

static PyObject * 
ndrxpy_tpadmcall(PyObject * self, PyObject * args)
{
    PyObject * result = NULL;
    PyObject * input_py = NULL;
    PyObject * flags_py = NULL;

    char bubfname[200] = "";
    char* ndrxbuf = NULL;
    
    long flags = 0;

    if (PyArg_ParseTuple(args, "O|O", &input_py, &flags_py) < 0) {
	goto leave_func;
    }	

    if (flags_py) {
	if ((flags = PyLong_AsLong(flags_py)) < 0) {
	    PyErr_SetString(PyExc_RuntimeError, "tpadmcall(): Bad flags given");
	    goto leave_func;
	}
    }

    if ((ndrxbuf = transform_py_to_ndrx(input_py)) == NULL) {
	goto leave_func;
    }

    tptypes(ndrxbuf, bubfname, NULL);
    
    if (strcmp(bubfname, "UBF") != 0) {
      PyErr_SetString(PyExc_RuntimeError, "tpadmcall(): Must pass dictionary as the input buffer");
      goto leave_func;
    }
      
    {
	char bubfname[200] = "";
	tptypes(ndrxbuf, bubfname, NULL);
	NDRX_LOG(log_debug, "calling tpadmcall([%s]...)", bubfname);
    }
    
    if (tpadmcall((UBFH*)ndrxbuf, (UBFH**)&ndrxbuf, flags ) < 0) {
	char tmp[200] = "";
	sprintf(tmp, "tpadmcall(): %d - %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }
    

    
    if ((result = transform_ndrxpy_to_py(ndrxbuf)) == NULL) {
	goto leave_func;
    }

 leave_func:
    if (ndrxbuf) tpfree(ndrxbuf);
    return result;
}

/* }}} */

#endif /* NDRXWS */

/* {{{ ndrxpy_tpacall() */


static PyObject * 
ndrxpy_tpacall(PyObject * self, PyObject * args)
{
    PyObject * result = NULL;
    PyObject * input_py = NULL;
    PyObject * service_py = NULL;
    PyObject * flags_py = NULL;
    

    char* service_name;
    char* ndrxbuf = NULL;
    
    
    long flags = 0;
    int handle = -1;
    
    if (PyArg_ParseTuple(args, "OO|O", &service_py, &input_py, &flags_py) < 0) {
	goto leave_func;
    }	
    if (!service_py || ((service_name = PyString_AsString(service_py)) == NULL)) {
	PyErr_SetString(PyExc_RuntimeError, "tpacall(): No service name and/or arguments given");
	goto leave_func;
    }

    if (strlen(service_name ) > MAX_SVC_NAME_LEN) {
	PyErr_SetString(PyExc_RuntimeError, "tpacall(): Service name length too long");
	goto leave_func;
    }
	
    if (flags_py) {
	if ((flags = PyLong_AsLong(flags_py)) < 0) {
	    PyErr_SetString(PyExc_RuntimeError, "tpacall(): Bad flags given");
	    goto leave_func;
	}
    }

    if ((ndrxbuf = transform_py_to_ndrx(input_py)) == NULL) {
	goto leave_func;
    }

    if ((handle = tpacall(service_name, ndrxbuf, 0, flags)) < 0) {
      char tmp[200] = "";
      sprintf(tmp, "tpacall(): %d - %s", tperrno, tpstrerror(tperrno));
      PyErr_SetString(PyExc_RuntimeError, tmp);
      goto leave_func;
    }
    
    if (! handle) {
      Py_INCREF(Py_None);
      result = Py_None;
    } else {
      result = Py_BuildValue("l", (long)handle);
    }

 leave_func:
    if (ndrxbuf) tpfree(ndrxbuf);
    return result;
}

/* }}} */
/* {{{ ndrxpy_tpgetrply() */

static PyObject * 
ndrxpy_tpgetrply(PyObject * self, PyObject * args)
{
    PyObject * result    = NULL;
    PyObject * flags_py  = NULL;

    char* ndrxbuf       = NULL;

    int handle         = -1;
    long outlen        = 0;
    long flags         = 0;

    if (PyArg_ParseTuple(args, "i|O", &handle, &flags_py) < 0) {
	goto leave_func;
    }	

    /* Buffer type will be changed by tpgetrply() if necessary */
    if ((ndrxbuf = tpalloc("UBF", NULL, NDRXBUFSIZE)) == NULL) {
	NDRX_LOG(log_debug, "tpalloc(): %d - %s\n", tperrno, tpstrerror(tperrno));
	goto leave_func;
    }

    if (flags_py) {
	if ((flags = PyLong_AsLong(flags_py)) < 0) {
	    PyErr_SetString(PyExc_RuntimeError, "tpgetrply(): Bad flags given");
	    goto leave_func;
	}
    }
    
    if (Binit((UBFH*)ndrxbuf, NDRXBUFSIZE) < 0) {
	NDRX_LOG(log_debug, "tpgetrply(): Binit(): %s\n", Bstrerror(Berror));
	goto leave_func;
    }

    if (handle == 0) {
	flags |= TPGETANY;
    }

    if (tpgetrply(&handle, &ndrxbuf, &outlen, flags) < 0) {
	char tmp[200] = "";
	sprintf(tmp, "tgetrply(): %d -  %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }
    
    if ((result = transform_ndrxpy_to_py(ndrxbuf)) == NULL) {
	goto leave_func;
    }
    
 leave_func:
    if (ndrxbuf) tpfree(ndrxbuf);
    return result;
}

/* }}} */

#ifndef NDRXWS
/* {{{ ndrxpy_tpforward() */

static PyObject *
ndrxpy_tpforward(PyObject * self, PyObject * args)
{
    NDRX_LOG(log_debug, "call ndrxpy_tpforward()");
    PyObject_Print(args, stdout, 0);

    if (PyArg_ParseTuple(args, "sO", &_forward_service, &_forward_pydata)  < 0) {
	return NULL;
    }

    NDRX_LOG(log_debug, "forward to %s", _forward_service);

    Py_INCREF(_forward_pydata); 
    _forward++;

    Py_INCREF(Py_None);
    return Py_None;
}    

/* }}} */
#endif /* NDRXWS */
/* {{{ ndrxpy_tpconnect() */

static PyObject * 
ndrxpy_tpconnect(PyObject * self, PyObject * args)
{
    PyObject * result = NULL;
    PyObject * input = NULL;
    PyObject * service = NULL;
    PyObject * flags_py = NULL;

    char* service_name;
    char* ndrxbuf = NULL;
    
    int handle = -1;
    long flags = 0;

    if (PyArg_ParseTuple(args, "OO|O", &service, &input, &flags_py) < 0) {
	goto leave_func;
    }	

    if (flags_py) {
	if ((flags = PyLong_AsLong(flags_py)) < 0) {
	    PyErr_SetString(PyExc_RuntimeError, "tpconnect(): Bad flags given");
	    goto leave_func;
	}
    }

    if (!service || ((service_name = PyString_AsString(service)) == NULL)) {
	PyErr_SetString(PyExc_RuntimeError, "tpconnect(): No service name and/or arguments given");
	goto leave_func;
    }

    if (strlen(service_name) > MAX_SVC_NAME_LEN) {
	PyErr_SetString(PyExc_RuntimeError, "tpconnect(): Service name length too long");
	goto leave_func;
    }

    if ((ndrxbuf = transform_py_to_ndrx(input)) == NULL) {
	goto leave_func;
    }
       
    /* int tpconnect(char *svc, char *data, long len, long flags) */

    if ((handle = tpconnect(service_name, ndrxbuf, 0, flags)) < 0) {
	char tmp[200] = "";
	sprintf(tmp, "tpconnect(): %d - %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }
    result = Py_BuildValue("l", (long)handle);

 leave_func:
    if (ndrxbuf) tpfree(ndrxbuf);
    return result;
}

/* }}} */
/* {{{ ndrxpy_tpdiscon() */

static PyObject * 
ndrxpy_tpdiscon(PyObject * self, PyObject * args)
{
    PyObject * result = NULL;
    PyObject * handle_py = NULL;

    long handle = -1;
    
    if (PyArg_ParseTuple(args, "O", &handle_py) < 0) {
	goto leave_func;
    }	

    if (!handle_py || ((handle = PyInt_AsLong(handle_py)) < 0)) {
	PyErr_SetString(PyExc_RuntimeError, "tpdiscon(): No handle given");
	goto leave_func;
    }
    
    /* int tpdiscon(int cd) */

    if ((handle = tpdiscon(handle)) < 0) {
	char tmp[200] = "";
	sprintf(tmp, "tpdiscon(%lu): %d - %s", handle, tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }
    
    result = Py_BuildValue("l", (long)handle);

 leave_func:
    return result;
}

/* }}} */
/* {{{ ndrxpy_tpsend() */

static PyObject * 
ndrxpy_tpsend(PyObject * self, PyObject * args)
{
    PyObject * result = NULL;
    PyObject * input = NULL;
    PyObject * handle_py = NULL;
    PyObject * flags_py = NULL;

    char* ndrxbuf = NULL;
    long revent = 0;
    int handle = -1;
    long flags = 0;
    int ret = -1;

    if (PyArg_ParseTuple(args, "OO|O", &handle_py, &input, &flags_py) < 0) {
	goto leave_func;
    }	

    if (!handle_py || ((handle = PyInt_AsLong(handle_py)) < 0)) {
	PyErr_SetString(PyExc_RuntimeError, "tpsend(): No handle given");
	goto leave_func;
    }

    if (flags_py) {
	if ((flags = PyLong_AsLong(flags_py)) < 0) {
	    PyErr_SetString(PyExc_RuntimeError, "tpsend(): Bad flags given");
	    goto leave_func;
	}
    }

    if ((ndrxbuf = transform_py_to_ndrx(input)) == NULL) {
	goto leave_func;
    }
 
    /* int tpsend(int cd, char *data, long len, long flags, long *revent) */
    if ((ret = tpsend(handle, ndrxbuf, 0, flags, &revent)) < 0) {
	if (tperrno != TPEEVENT) {
	    char tmp[200] = "";
	    sprintf(tmp, "tpsend(): %d - %s, revent = %lu", tperrno, tpstrerror(tperrno), revent);
	    PyErr_SetString(PyExc_RuntimeError, tmp);
	    goto leave_func;
	}
    }
    
    result = Py_BuildValue("l", (long)revent);

 leave_func:
    if (ndrxbuf) tpfree(ndrxbuf);
    return result;
}

/* }}} */
/* {{{ ndrxpy_tprecv() */

static PyObject * 
ndrxpy_tprecv(PyObject * self, PyObject * args)
{
    PyObject * result = NULL;
    PyObject * res_tuple = NULL;
    PyObject * handle_py = NULL;
    PyObject * flags_py = NULL;

    char* ndrxbuf = NULL;
    
    int handle = -1;
    long flags = 0;
    long len = 0;
    long revent = 0;
    int ret = 0;

    if (PyArg_ParseTuple(args, "O|O", &handle_py, &flags_py) < 0) {
	goto leave_func;
    }	

    if (!handle_py || ((handle = PyInt_AsLong(handle_py)) < 0)) {
	PyErr_SetString(PyExc_RuntimeError, "tprecv(): No handle given");
	goto leave_func;
    }

    if (flags_py) {
	if ((flags = PyLong_AsLong(flags_py)) < 0) {
	    PyErr_SetString(PyExc_RuntimeError, "tprecv(): Bad flags given");
	    goto leave_func;
	}
    }

    /* Buffer type will be changed by tprecv() if necessary */
    if ((ndrxbuf = tpalloc("UBF", NULL, NDRXBUFSIZE)) == NULL) {
	NDRX_LOG(log_error, "tpalloc(): %d - %s", tperrno, tpstrerror(tperrno));
	goto leave_func;
    }
    
    if (Binit((UBFH*)ndrxbuf, NDRXBUFSIZE) < 0) {
	NDRX_LOG(log_error, "Binit(): %s", Bstrerror(Berror));
	goto leave_func;
    }

    /*    int tprecv(int cd, char **data, long *len, long flags, long *revent) */
    if ((ret = tprecv(handle, &ndrxbuf, &len, flags, &revent)) < 0) {
	char tmp[200] = "";
	if (tperrno != TPEEVENT) {
	    sprintf(tmp, "tprecv(): %d - %s", tperrno, tpstrerror(tperrno));
	    PyErr_SetString(PyExc_RuntimeError, tmp);
	    goto leave_func;
	}
    }



    /* If an event exists for the descriptor, cd, then tprecv() will return setting tperrno to TPEEVENT. The
       event type is returned in revent. Data can be received along with the TPEV_SVCSUCC,
       TPEV_SVCFAIL, and TPEV_SENDONLY events. Valid events for tprecv() are as follows. */

    if ((revent & ( TPEV_SVCSUCC | TPEV_SVCFAIL | TPEV_SENDONLY))) {
	if ((len > 0) && (result = transform_ndrxpy_to_py(ndrxbuf)) == NULL) {
	    goto leave_func;
	}
    }
    res_tuple = PyTuple_New(2);
    PyTuple_SetItem(res_tuple, 0, PyLong_FromLong(revent));
    if (result)
	PyTuple_SetItem(res_tuple, 1, result);
    else
	PyTuple_SetItem(res_tuple, 1, PyLong_FromLong(revent));

 leave_func:
    if (ndrxbuf) tpfree(ndrxbuf);
    return res_tuple;
}

/* }}} */
#ifndef NDRXWS
/* {{{ ndrxpy_tpopen() */

static PyObject* ndrxpy_tpopen(PyObject* self, PyObject* arg) {

    PyObject * result         = NULL;

    int ret      = -1;
    
    ret = tpopen();
    if (ret == -1) {
	char tmp[200] = "";
	sprintf(tmp, "tpopen(): %d - %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }

   result = PyLong_FromLong((long)ret);

 leave_func:
    return result; 
}

/* }}} */
/* {{{ ndrxpy_tpclose() */

static PyObject* ndrxpy_tpclose(PyObject* self, PyObject* arg) {

    PyObject * result         = NULL;
    int ret      = -1;

    ret = tpclose();
    if (ret < 0) {
	char tmp[200] = "";
	sprintf(tmp, "tpclose(): %d -  %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }

   result = PyLong_FromLong((long)ret);

 leave_func:
    return result; 
}

/* }}} */
#endif /* NDRXWS */
/* {{{ ndrxpy_tpbegin() */

static PyObject* ndrxpy_tpbegin(PyObject* self, PyObject* arg) {

    PyObject * result         = NULL;
    PyObject * timeout_py     = NULL;
    PyObject * flags_py       = NULL;
    
    long timeout = 0;
    int ret      = -1;
    long flags   = 0;

    char tmp[200] = "";


    if (PyArg_ParseTuple(arg, "O|O", &timeout_py, &flags_py) < 0) {
	sprintf(tmp, "tpbegin(): wrong argument");
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }

    if (! timeout_py) {
	sprintf(tmp, "tpbegin(): timeout value not given");
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }
    
    if (flags_py) {
	if ((flags = PyLong_AsLong(flags_py)) < 0) {
	    PyErr_SetString(PyExc_RuntimeError, "tpbegin(): Bad flags given");
	    goto leave_func;
	}
    }

    if (PyInt_Check(timeout_py)) {
	timeout = PyInt_AsLong(timeout_py);
    } else if (PyLong_Check(timeout_py)) {
	timeout = PyLong_AsLong(timeout_py);
    } else {
	PyErr_SetString(PyExc_RuntimeError, "tpbegin(): timeout must be INT or LONG");
	goto leave_func;
    }
    
    ret = tpbegin(timeout, flags);
    if (ret < 0) {
	sprintf(tmp, "tpbegin(%lu): %d - %s", timeout, tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }

   result = PyLong_FromLong((long)ret);

 leave_func:
    return result; 
}

/* }}} */
/* {{{ ndrxpy_tpcommit() */

static PyObject* ndrxpy_tpcommit(PyObject* self, PyObject* arg) {

    PyObject * result         = NULL;
    PyObject * flags_py       = NULL;

    int ret     = -1;
    long flags  = 0;

    if (PyArg_ParseTuple(arg, "|O", &flags_py) < 0) {
	char tmp[200] = "";
	sprintf(tmp, "tpcommit(): wrong argument");
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }

    if (flags_py) {
	if ((flags = PyLong_AsLong(flags_py)) < 0) {
	    char tmp[200] = "";
	    sprintf(tmp, "tpcommit(): Bad flags given (%d)", (int)flags);
	    PyErr_SetString(PyExc_RuntimeError, tmp); 
	    goto leave_func;
	}
    }

    ret = tpcommit(flags);
    if (ret < 0) {
	char tmp[200] = "";
	sprintf(tmp, "tpcommit(): %d: %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }
    
    result = PyLong_FromLong((long)ret);
 leave_func:
    return result; 
}

/* }}} */
/* {{{ ndrxpy_tpabort() */

static PyObject* ndrxpy_tpabort(PyObject* self, PyObject* arg) {

    PyObject * result         = NULL;
    PyObject * flags_py       = NULL;

    int ret = -1;
    long flags = 0;

    if (PyArg_ParseTuple(arg, "|O", &flags_py) < 0) {
	char tmp[200] = "";
	sprintf(tmp, "tpabort(): wrong argument");
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }

    if (flags_py) {
	if ((flags = PyLong_AsLong(flags_py)) < 0) {
	    PyErr_SetString(PyExc_RuntimeError, "tpabort(): Bad flags given");
	    goto leave_func;
	}
    }

    ret = tpabort(flags);
    if (ret < 0) {
	char tmp[200] = "";
	sprintf(tmp, "tpabort(): %d - %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }
    
    result = PyLong_FromLong((long)ret);
 leave_func:
    return result; 
}

/* }}} */
/* {{{ ndrxpy_tpsuspend() */

static PyObject* ndrxpy_tpsuspend(PyObject* self, PyObject* arg) {

    PyObject * result         = NULL; 
    PyObject * flags_py       = NULL;
    
    int ret    = -1;
    long flags = 0;

    char tranid_strrep[TPCONVMAXSTR+1] = "";
    TPTRANID tranid_binrep;

    if (PyArg_ParseTuple(arg, "|O", &flags_py) < 0) {
	char tmp[200] = "";
	sprintf(tmp, "tpsuspend(): wrong argument");
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }

    if (flags_py) {
	if ((flags = PyLong_AsLong(flags_py)) < 0) {
	    PyErr_SetString(PyExc_RuntimeError, "tpsuspend(): Bad flags given");
	    goto leave_func;
	}
    }

    ret = tpsuspend(&tranid_binrep, flags);
    if (ret < 0) {
	char tmp[200] = "";
	sprintf(tmp, "tpsuspend(): %d - %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }

    /* convert the binary (long[6]) transaction ID to a string
       representation that can be returned to the caller */
    
    if (tpconvert(tranid_strrep, (char*)&tranid_binrep, TPCONVTRANID | TPTOSTRING) < 0) {
	char tmp[200] = "";
	sprintf(tmp, "tpconvert(TRANID)(): %d - %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }
    
    result = PyString_FromString(tranid_strrep);

 leave_func:
    return result; 
}

/* }}} */
/* {{{ ndrxpy_tpresume() */

static PyObject* ndrxpy_tpresume(PyObject* self, PyObject* arg) {

    int ret    = -1;
    long flags = 0;
    PyObject * result         = NULL;
    PyObject * flags_py       = NULL;

    char*  tranid_strrep = NULL;
    
    TPTRANID tranid_binrep;

    if (PyArg_ParseTuple(arg, "s|O", &tranid_strrep, &flags_py) < 0) {
	goto leave_func;
    }	
    

    ret = tpconvert(tranid_strrep, (char*)&tranid_binrep, TPCONVTRANID);
    if (ret < 0) {
	char tmp[200] = "";
	sprintf(tmp, "tpconvert(TRANID): %d - %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }
    
    
    ret = tpresume(&tranid_binrep, flags);
    if (ret < 0) {
	char tmp[200] = "";
	sprintf(tmp, "tpresume(): %d -  %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }
    
    result = PyLong_FromLong((long)ret);
 leave_func:
    return result; 
}

/* }}} */
/* {{{ ndrxpy_tpgetlev() */

static PyObject* ndrxpy_tpgetlev(PyObject* self, PyObject* arg) {

    int ret    = -1;
    PyObject * result         = NULL;

    ret = tpgetlev();
    if (ret < 0) {
	char tmp[200] = "";
	sprintf(tmp, "tpgetlev(): %d - %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }
    
    result = PyLong_FromLong((long)ret);
 leave_func:
    return result; 
}

/* }}} */

/* {{{ ndrxpy_tpinit() */

static PyObject * 
ndrxpy_tpinit(PyObject * self, PyObject * args)
{
    PyObject * result = NULL;
    PyObject * input = NULL;
    TPINIT* ndrxbuf = NULL;
    int idx = 0;

    NDRX_LOG(log_debug, "entering ndrxpy_tpinit(args= %x0x) ...\n", args);

    if (args) {
	if (PyArg_ParseTuple(args, "O|O", &input) < 0) {

	    NDRX_LOG(log_debug, "parseTuple (ndrxpy_tpinit)");
	    goto leave_func;
	}
    }

    if (input && PyDict_Check(input)) {
	PyObject* keylist;
	
#define TPINITDATASIZE 4096

	if ((ndrxbuf = (TPINIT*)tpalloc("TPINIT", NULL, TPINITNEED(TPINITDATASIZE))) == NULL) {
	    char tmp[200] = "";
	    sprintf(tmp, "tpalloc(TPINIT): %d - %s", tperrno, tpstrerror(tperrno));
	    PyErr_SetString(PyExc_RuntimeError, tmp);
	    goto leave_func;
	}

	keylist = PyDict_Keys(input);
	if (!keylist) {
	    NDRX_LOG(log_error, "tpinit(): PyDict_Keys(keys, %d) returned NULL", idx);
	    goto leave_func;

	}	    

	for (idx = 0; idx < PyList_Size(keylist); idx++) {
	    PyObject*    key;
	    char*        val_cstring = NULL;
	    char*        key_cstring = NULL;

	    key = PyList_GetItem(keylist, idx);  /* borrowed reference */
	    if (!key) {
		NDRX_LOG(log_error, "tpinit(): PyList_GetItem(keys, %d) returned NULL", idx);
		goto leave_func;
	    }
	    key_cstring = PyString_AsString(key);

	    /*
	      char      usrname[MAXTIDENT+2]; 
	      char      cltname[MAXTIDENT+2];
	      char      passwd[MAXTIDENT+2];
	      char      grpname[MAXTIDENT+2];
	      long      datalen;
	      long      data; 
	      long      flags;
	    */


	    if (!strcmp(key_cstring, "usrname")) {
		val_cstring = PyString_AsString(PyDict_GetItemString(input, key_cstring));  /* borrowed ref. */
		strcpy(ndrxbuf->usrname, val_cstring);
	    } else if (!strcmp(key_cstring, "cltname")) {
		val_cstring = PyString_AsString(PyDict_GetItemString(input, key_cstring));  /* borrowed ref. */
		strcpy(ndrxbuf->cltname, val_cstring);
	    } else if (!strcmp(key_cstring, "passwd")) {
		val_cstring = PyString_AsString(PyDict_GetItemString(input, key_cstring));  /* borrowed ref. */
		strcpy(ndrxbuf->passwd, val_cstring);
	    } else if (!strcmp(key_cstring, "grpname")) {
		val_cstring = PyString_AsString(PyDict_GetItemString(input, key_cstring));  /* borrowed ref. */
		strcpy(ndrxbuf->grpname, val_cstring);
	    } else if (!strcmp(key_cstring, "data")) {
		val_cstring = PyString_AsString(PyDict_GetItemString(input, key_cstring));  /* borrowed ref. */
		strncpy((char*)&(ndrxbuf->data), val_cstring, TPINITDATASIZE-1);
		ndrxbuf->datalen = (long)strlen(val_cstring) + 1;
	    } else if (!strcmp(key_cstring, "flags")) {
		long val_flags = PyLong_AsLong(PyDict_GetItemString(input, key_cstring));  /* borrowed ref. */
		ndrxbuf->flags = val_flags;
	    }
	    
	    else {
		char tmp[200] = "";
		sprintf(tmp, "tpinit(): unknown tpinit structure member '%s'!", val_cstring);
		PyErr_SetString(PyExc_RuntimeError, tmp);
		goto leave_func;
	    }
	}
    }

	NDRX_LOG(log_debug, "calling tpinit()");
	if (ndrxbuf)
	    NDRX_LOG(log_debug, "usrname = >%s<, cltname = >%s<\n", 
		    ndrxbuf->usrname, ndrxbuf->cltname);

	
    if (tpinit(ndrxbuf) < 0) {
	char tmp[200] = "";
	sprintf(tmp, "tpinit(): %d - %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }

    if ((result = Py_BuildValue("i", 1)) == NULL) {
	goto leave_func;
    }
    
 leave_func:
    if (ndrxbuf) tpfree((char*)ndrxbuf);
    return result;
}

/* }}} */
/* {{{ ndrxpy_tpgetctxt() */
static PyObject * 
ndrxpy_tpgetctxt(PyObject * self, PyObject * args)
{
    PyObject * result = NULL;
    PyObject * flags_py = NULL;

    TPCONTEXT_T context = 0;
    long flags = 0;
    int ret = -1;

    if (PyArg_ParseTuple(args, "|O", &flags_py) < 0) {
	goto leave_func;
    }	

    if (flags_py) {
      if ((flags = PyLong_AsLong(flags_py)) < 0) {
	PyErr_SetString(PyExc_RuntimeError, "tpgetctxt(): Bad flags given");
	goto leave_func;
      }
    }

    /* int tpgetctxt(TPCONTEXT_T* context, long flags) */
    if ((ret = tpgetctxt(&context, flags)) < 0) {
      char tmp[200] = "";
      sprintf(tmp, "tpgetctxt(): %d - %s", tperrno, tpstrerror(tperrno));
      PyErr_SetString(PyExc_RuntimeError, tmp);
      goto leave_func;
    }
    
    result = Py_BuildValue("l", context);
    
 leave_func:
    return result;
}

/* }}} */
/* {{{ ndrxpy_tpsetctxt() */

static PyObject * 
ndrxpy_tpsetctxt(PyObject * self, PyObject * args)
{
    PyObject * result = NULL;
    PyObject * context_py = NULL;
    PyObject * flags_py = NULL;

    TPCONTEXT_T context = 0;
    long flags = 0;
    int ret = -1;

    if (PyArg_ParseTuple(args, "O|O", &context_py, &flags_py) < 0) {
	goto leave_func;
    }	

    if (!context_py || ((context = (TPCONTEXT_T) PyLong_AsLong(context_py)) < 0)) {
	PyErr_SetString(PyExc_RuntimeError, "tpsetctxt(): No context given");
	goto leave_func;
    }

    if (flags_py) {
      if ((flags = PyLong_AsLong(flags_py)) < 0) {
	PyErr_SetString(PyExc_RuntimeError, "tpsetctxt(): Bad flags given");
	goto leave_func;
      }
    }

    /* int tpsetctxt(TPCONTEXT_T context, long flags) */
    if ((ret = tpsetctxt((TPCONTEXT_T)context, flags)) < 0) {
      char tmp[200] = "";
      sprintf(tmp, "tpsetctxt(): %d - %s", tperrno, tpstrerror(tperrno));
      PyErr_SetString(PyExc_RuntimeError, tmp);
      goto leave_func;
    }

    result = Py_BuildValue("l", ret);
    
 leave_func:
    return result;
}

/* }}} */
/* {{{ ndrxpy_tpchkauth() */

static PyObject* ndrxpy_tpchkauth(PyObject* self, PyObject* arg) {

    PyObject * result         = NULL;
    int ret = -1;

    ret = tpchkauth() ;
    if (ret < 0) {
	char tmp[200] = "";
	sprintf(tmp, "tpchkauth(): %d - %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }
    
    result = PyLong_FromLong((long)ret);
 leave_func:
    return result; 
}

/* }}} */
/* {{{ ndrxpy_tpterm() */

static PyObject * 
ndrxpy_tpterm(PyObject * self, PyObject * args)
{
    PyObject * result = NULL;
    
    if (tpterm() < 0) {
	char tmp[200] = "";
	sprintf(tmp, "tpterm(): %d - %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }

    if ((result = Py_BuildValue("i", 1)) == NULL) {
	goto leave_func;
    }
    
 leave_func:
    return result;
}

/* }}} */
#ifndef NDRXWS
/* {{{ ndrxpy_tpadvertise() */

static PyObject* ndrxpy_tpadvertise(PyObject* self, PyObject* arg) {
    int idx = 0;
    char * service_name   = NULL;
    char * method_name    = NULL;
    PyObject * result     = NULL;


    if (!_server_is_running) {
	PyErr_SetString(PyExc_RuntimeError, "tpadvertise(): Don't call me before mainloop()!");
	goto leave_func;
    }

    if (!PyArg_ParseTuple(arg, "s|s", &service_name, &method_name)) {
	goto leave_func;
    }

    if (strlen(service_name) >= MAX_SVC_NAME_LEN) {
	PyErr_SetString(PyExc_RuntimeError, "tpadvertise(): Service name length too long");
	goto leave_func;
    }

    if ((idx = find_free_entry(service_name)) < 0) {
	PyErr_SetString(PyExc_RuntimeError, 
			"tpadvertise(): Number of services > 100 for this server or internal data corrupted");
	goto leave_func;
    }
    
    if (tpadvertise(service_name, endurox_dispatch) < 0) {
	char tmp[200] = "";
	sprintf(tmp, "tpadvertise(): %d - %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }

    /* save the method_name and service_name  */
    if (method_name) {
	if (strlen(method_name) >= MAX_METHOD_NAME_LEN) {
	    PyErr_SetString(PyExc_RuntimeError, "tpadvertise(): Method name length too long");
	    goto leave_func;
	}
	strncpy(_registered_services[idx].method, method_name, MAX_METHOD_NAME_LEN);
    } else {
	/* method name is advertised name */
	strncpy(_registered_services[idx].method, service_name, MAX_SVC_NAME_LEN);
    }
    strcpy(_registered_services[idx].name, service_name);

    result = PyInt_FromLong((long)tpurcode);
 leave_func:
    return result; 
}

/* }}} */
/* {{{ ndrxpy_tpunadvertise() */

static PyObject* ndrxpy_tpunadvertise(PyObject* self, PyObject* arg) {
    char * svc_name = NULL;
    PyObject * result = NULL;

    if (!_server_is_running) {
	PyErr_SetString(PyExc_RuntimeError, "tpunadvertise(): Don't call me before mainloop()!");
	goto leave_func;
    }

    if (!PyArg_ParseTuple(arg, "s", &svc_name)) {
	goto leave_func;
    }

    if (strlen(svc_name) > MAX_SVC_NAME_LEN) {
	PyErr_SetString(PyExc_RuntimeError, "tpunadvertise(): Service name length too long");
	goto leave_func;
    }
	
    if (tpunadvertise(svc_name) < 0) {
	char tmp[200] = "";
	sprintf(tmp, "tpunadvertise(): %d - %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }

    if (delete_entry(svc_name) < 0) {
	PyErr_SetString(PyExc_RuntimeError, "tpunadvertise(): internal data corrupted");
	NDRX_LOG(log_debug, "Not found: %s", svc_name);
	goto leave_func;
    }

    result = PyInt_FromLong(1L);
 leave_func:
    return result; 
}

/* }}} */
#endif /* NDRXWS */
/* {{{ ndrxpy_tpenqueue() */

static PyObject * 
ndrxpy_tpenqueue(PyObject * self, PyObject * args)
{
    PyObject * result   = NULL;
    PyObject * flags_py = NULL;
    PyObject * data     = NULL;
    PyObject * qctl_obj = NULL;
    
    char* queue_space = NULL;
    char* queue_name  = NULL;
    char* ndrxbuf      = NULL;
    
    long flags = 0;

    TPQCTL qctl;
    
    memset(&qctl, '\0', sizeof (qctl));

    if (PyArg_ParseTuple(args, "ssOO|O", 
			 &queue_space, 
			 &queue_name, 
			 &data, 
			 &qctl_obj, 
			 &flags_py) 
	< 0) {
	goto leave_func;
    }	

    if (flags_py) {
	if ((flags = PyLong_AsLong(flags_py)) < 0) {
	    PyErr_SetString(PyExc_RuntimeError, "tpenqueue(): Bad flags given");
	    goto leave_func;
	}
    }

    NDRX_LOG(log_debug, "qspace = %s", queue_space);
    NDRX_LOG(log_debug, "qname  = %s", queue_name);

    if (!queue_space || !queue_name) {
	PyErr_SetString(PyExc_RuntimeError, "tpenqueue(): No queue name and/or queue space given");
	goto leave_func;
    }

    /*
      struct tpqctl_t {		   control parameters to queue primitives 
        long flags;		   indicates which of the values are set 
	long deq_time;		   absolute/relative  time for dequeuing 
	long priority;		   enqueue priority 
	long diagnostic;	   indicates reason for failure 
	char msgid[TMMSGIDLEN];	   id of message before which to queue 
	char corrid[TMCORRIDLEN];  correlation id used to identify message 
	char replyqueue[TMQNAMELEN+1];	 queue name for reply message 
	char failurequeue[TMQNAMELEN+1]; queue name for failure message 
	CLIENTID cltid;		   client identifier for originating client 
	long urcode;		   application user-return code 
	long appkey;		   application authentication client key 
      };
      typedef struct tpqctl_t TPQCTL;
    */


    if (PyDict_Check(qctl_obj)) {
	PyObject * item = NULL;
	qctl.flags = TPNOFLAGS;

	/* Convert dictionary to TPQCTL structure */
	if ((item = PyDict_GetItemString (qctl_obj, "deq_time")) != NULL) {
	    qctl.deq_time = PyLong_AsLong(item);

	    NDRX_LOG(log_debug, "deq_time = %d\n", qctl.deq_time);

	    if (!(qctl.flags & TPQTIME_REL) || !(qctl.flags & TPQTIME_ABS)) {
		qctl.flags |= TPQTIME_ABS;
	    }
	} 
	if ((item = PyDict_GetItemString (qctl_obj, "priority")) != NULL) {
	    /* requires flag set in "flags" field */
	    qctl.priority = PyLong_AsLong(item);

	    NDRX_LOG(log_debug, "priority = %d\n", qctl.priority);

	    qctl.flags |= TPQPRIORITY;
	}
	if ((item = PyDict_GetItemString (qctl_obj, "urcode")) != NULL) {
	    qctl.urcode = PyLong_AsLong(item);

	    NDRX_LOG(log_debug, "urcode = %d\n", qctl.urcode);

	}
	if ((item = PyDict_GetItemString (qctl_obj, "msgid")) != NULL) {
	    strncpy(qctl.msgid, PyString_AsString(item), TMMSGIDLEN);

	    NDRX_LOG(log_debug, "msgid = %s\n", qctl.msgid);

	    qctl.flags |= TPQBEFOREMSGID;
	}
	if ((item = PyDict_GetItemString (qctl_obj, "corrid")) != NULL) {
	    strncpy(qctl.corrid, PyString_AsString(item), TMCORRIDLEN);

	    NDRX_LOG(log_debug, "corrid = %s\n", qctl.corrid);

	    qctl.flags |= TPQCORRID;
	}
	if ((item = PyDict_GetItemString (qctl_obj, "replyqueue")) != NULL) {
	    strncpy(qctl.replyqueue, PyString_AsString(item), TMQNAMELEN+1);

	    NDRX_LOG(log_debug, "replyqueue = %s\n", qctl.replyqueue);

	    qctl.flags |= TPQREPLYQ;
	}
	if ((item = PyDict_GetItemString (qctl_obj, "failurequeue")) != NULL) {
	    strncpy(qctl.failurequeue, PyString_AsString(item), TMQNAMELEN+1);

	    NDRX_LOG(log_debug, "failurequeue = %s\n", qctl.failurequeue);

	    qctl.flags |= TPQFAILUREQ;
	}
	if ((item = PyDict_GetItemString (qctl_obj, "flags")) != NULL) {
	    qctl.flags |= PyLong_AsLong(item);

	    NDRX_LOG(log_debug, "flags = %d\n", qctl.flags);

	}
    } 

    if ((ndrxbuf = transform_py_to_ndrx(data)) == NULL) {
	goto leave_func;
    }

    if (tpenqueue(queue_space, queue_name, &qctl, ndrxbuf, 0, flags) < 0) {
	char tmp[200] = "";

	/* 
	   "
	   If the call to tpenqueue() failed and tperrno is set to TPEDIAGNOSTIC, a value indicating the
	   reason for failure is returned in ctl->diagnostic. The possible values are defined below in the
	   DIAGNOSTICS section.
	   " 
	*/
	if (tperrno == TPEDIAGNOSTIC) {
	    sprintf(tmp, "tpenqueue() error: (Q diagnostics = %d)", (int)qctl.diagnostic);
	} else {
	    sprintf(tmp, "tpenqueue(): %d - %s", tperrno, tpstrerror(tperrno));
	}
	    
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }


    if (qctl.flags & TPQMSGID) {
	PyObject * item  = NULL;

	NDRX_LOG(log_debug, "message id = %s\n", qctl.msgid);	

        if ((item = Py_BuildValue("s", qctl.msgid)) == NULL) {
	    goto leave_func;
	}
	PyDict_SetItemString(qctl_obj, "msgid", item);
	Py_DECREF(item);
    }

    result = Py_BuildValue("l", 1);
 leave_func:
    if (ndrxbuf) tpfree(ndrxbuf);
    return result;
}

/* }}} */
/* {{{ ndrxpy_tpdequeue() */

static PyObject * 
ndrxpy_tpdequeue(PyObject * self, PyObject * args)
{
    PyObject * result   = NULL;
    PyObject * flags_py = NULL;
    PyObject * qctl_obj = NULL;
    PyObject * item     = NULL;

    char* queue_space = NULL;
    char* queue_name  = NULL;

    char* ndrxbuf = NULL;

    long flags  = 0;
    long outlen = 0;

    TPQCTL qctl;
    
    memset(&qctl, '\0', sizeof (qctl));

    if (PyArg_ParseTuple(args, "ssO|O", &queue_space, &queue_name, &qctl_obj, &flags_py) < 0) {
	goto leave_func;
    }	


    NDRX_LOG(log_debug, "qspace = %s", queue_space);
    NDRX_LOG(log_debug, "qname  = %s", queue_name);


    if (!queue_space || !queue_name) {
	PyErr_SetString(PyExc_RuntimeError, "tpdequeue(): No queue name and/or queue space given");
	goto leave_func;
    }


    if (PyDict_Check(qctl_obj)) {
	PyObject * item = NULL;

	qctl.flags = TPNOFLAGS;
	
	/* Convert to TPQCTL structure */
	if ((item = PyDict_GetItemString (qctl_obj, "msgid")) != NULL) { /* borrowed reference */
	    strncpy(qctl.msgid, PyString_AsString(item), TMMSGIDLEN);

	    NDRX_LOG(log_debug, "msgid = %s", qctl.msgid);

	    qctl.flags |= TPQGETBYMSGID;
	}
	if ((item = PyDict_GetItemString (qctl_obj, "corrid")) != NULL) {
	    strncpy(qctl.corrid, PyString_AsString(item), TMCORRIDLEN);

	    NDRX_LOG(log_debug, "corrid = %s", qctl.corrid);

	    qctl.flags |= TPQGETBYCORRID;
	}
	if ((item = PyDict_GetItemString (qctl_obj, "flags")) != NULL) {
	    qctl.flags = PyLong_AsLong(item);

	    NDRX_LOG(log_debug, "flags = %d", qctl.flags);

	}
    } 

    if ((ndrxbuf = tpalloc("UBF", NULL, NDRXBUFSIZE) ) == NULL) {

	    NDRX_LOG(log_debug, "%d : tpalloc failed\n", __LINE__);

	goto leave_func;
    }


	    NDRX_LOG(log_debug, "%d : before tpdequeue", __LINE__);

    if (tpdequeue(queue_space, queue_name, &qctl, &ndrxbuf, &outlen, flags) < 0) {
	char tmp[200] = "";

	if (tperrno == TPEDIAGNOSTIC) {
	    if (qctl.diagnostic == QMENOMSG) {
		Py_INCREF(Py_None);
		result = Py_None;
	    } else {
		sprintf(tmp, "tpdequeue() error: (Q diagnostics = %d)", (int)qctl.diagnostic);
		PyErr_SetString(PyExc_RuntimeError, tmp);
		goto leave_func;
	    }
	} else {
	    sprintf(tmp, "tpdequeue(): tperrno = %d (%s)", tperrno, tpstrerror(tperrno));
	    PyErr_SetString(PyExc_RuntimeError, tmp);
	    goto leave_func;
	}
	    
    }

	    NDRX_LOG(log_debug, "%d : after tpdequeue", __LINE__);


    if ((result = transform_ndrxpy_to_py(ndrxbuf)) == NULL) {

	    NDRX_LOG(log_debug, "%d : transform_ndrxpy_to_py failed ", __LINE__);

      
	goto leave_func;
    }
    
    /* disassemble QCTL structure */
    item  = NULL;
    
    if (qctl.flags & TPQPRIORITY) {
        if ((item = Py_BuildValue("l", qctl.priority)) == NULL) {

	    NDRX_LOG(log_debug, "%d : qctl.priority failed", __LINE__);

	    goto leave_func;
	}
	PyDict_SetItemString(qctl_obj, "priority", item);
    }
    if (qctl.flags & TPQMSGID) {
        if ((item = Py_BuildValue("s", qctl.msgid)) == NULL) {

	    NDRX_LOG(log_debug, "%d : qctl.msgid failed", __LINE__);

	    goto leave_func;
	}
	PyDict_SetItemString(qctl_obj, "msgid", item);
    }
    if (qctl.flags & TPQCORRID) {
        if ((item = Py_BuildValue("s", qctl.corrid)) == NULL) {

	    NDRX_LOG(log_debug, "%d : qctl.corrid failed", __LINE__);

	    goto leave_func;
	}
	PyDict_SetItemString(qctl_obj, "corrid", item);
    }
    if (qctl.flags & TPQREPLYQ) {
        if ((item = Py_BuildValue("s", qctl.replyqueue)) == NULL) {

	    NDRX_LOG(log_debug, "%d : qctl.replyqueue failed", __LINE__);

	    goto leave_func;
	}
	PyDict_SetItemString(qctl_obj, "replyqueue", item);
    }
    if (qctl.flags & TPQFAILUREQ) {
        if ((item = Py_BuildValue("s", qctl.failurequeue)) == NULL) {

	    NDRX_LOG(log_debug, "%d :  qctl.failurequeue failed", __LINE__);

	    goto leave_func;
	}
	PyDict_SetItemString(qctl_obj, "failurequeue", item);
    }
    
     
 leave_func:
    if (item) { Py_DECREF(item); }
    if (ndrxbuf) { tpfree(ndrxbuf); }
    return result;
}

/* }}} */

/* {{{ ndrxpy_tppost() */

static PyObject* ndrxpy_tppost(PyObject* self, PyObject* arg) {

    PyObject * result   = NULL;
    PyObject * flags_py = NULL;
    PyObject * evt_name = NULL;
    PyObject * evdata   = NULL;

    char * event_name = NULL;
    char* ndrxbuf = NULL;

    long flags = 0;


    if (!PyArg_ParseTuple(arg, "OO|O", &evt_name, &evdata, &flags_py)) {
	goto leave_func;
    }
    if (flags_py) {
	if ((flags = PyLong_AsLong(flags_py)) < 0) {
	    PyErr_SetString(PyExc_RuntimeError, "tppost(): Bad flags given");
	    goto leave_func;
	}
    }

    if ((event_name = PyString_AsString(evt_name)) == NULL) {
	PyErr_SetString(PyExc_RuntimeError, "tppost(): No event name given");
	goto leave_func;
    }
    if ((ndrxbuf = transform_py_to_ndrx(evdata)) == NULL) {
	goto leave_func;
    }

    if (tppost(event_name, ndrxbuf, 0, flags) < 0) {
	char tmp[200] = "";
	sprintf(tmp, "tppost(): %d - %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }

    result = PyInt_FromLong((long)tpurcode);

 leave_func:
    if (ndrxbuf) tpfree(ndrxbuf);

    return result; 
}

/* }}} */
/* {{{ ndrxpy_tpsubscribe() */

static PyObject* ndrxpy_tpsubscribe(PyObject* self, PyObject* arg) {

    PyObject * result       = NULL;
    PyObject * flags_py     = NULL;
    PyObject * ctl_obj      = NULL;
    
    long handle       = 0;
    long flags        = 0;

    char * evt_expr   = NULL;
    char * evt_filter = NULL;
    TPEVCTL ctl;
    
    memset(&ctl, 0, sizeof ctl);

    if (!PyArg_ParseTuple(arg, "ssO|O", &evt_expr, &evt_filter, &ctl_obj, &flags_py)) {
	goto leave_func;
    }
    /* Convert to TPEVCTL structure */
    
    if (PyDict_Check(ctl_obj)) {
	PyObject * item = NULL;
	
	if ((item = PyDict_GetItemString (ctl_obj, "name1")) != NULL) { /* borrowed reference */
	    strncpy(ctl.name1, PyString_AsString(item), 32);

	    NDRX_LOG(log_debug, "name1 = %s", ctl.name1);

	}
	if ((item = PyDict_GetItemString (ctl_obj, "name2")) != NULL) { /* borrowed reference */
	    strncpy(ctl.name2, PyString_AsString(item), 32);

	    NDRX_LOG(log_debug, "name2 = %s", ctl.name2);

	}
	if ((item = PyDict_GetItemString (ctl_obj, "flags")) != NULL) {
	    ctl.flags = PyLong_AsLong(item);

	    NDRX_LOG(log_debug, "flags = %d", ctl.flags);

	}
    }


    NDRX_LOG(log_debug, "calling tpsubscribe(%s, %s, ctl, %d)\n", evt_expr, evt_filter, flags);
    
    if ((handle = tpsubscribe(evt_expr, evt_filter, &ctl, flags)) < 0) {
	char tmp[200] = "";
	sprintf(tmp, "tpsubscribe(): %d - %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }
    
    result = PyLong_FromLong((long)handle);
 leave_func:
    return result; 
}


/* }}} */



/* {{{ ndrxpy_tpunsubscribe() */

static PyObject* ndrxpy_tpunsubscribe(PyObject* self, PyObject* arg) {

    PyObject * result   = NULL;
    PyObject * flags_py = NULL;

    long handle        = 0;
    long flags         = 0;
    

    if (!PyArg_ParseTuple(arg, "l|O", &handle, &flags_py)) {
	goto leave_func;
    }

    if (flags_py) {
	if ((flags = PyLong_AsLong(flags_py)) < 0) {
	    PyErr_SetString(PyExc_RuntimeError, "tpunsubscribe(): Bad flags given");
	    goto leave_func;
	}
    }

    if (tpunsubscribe(handle, flags) < 0) {
	char tmp[200] = "";
	sprintf(tmp, "tpunsubscribe(): %d - %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }
    
    result = PyLong_FromLong((long)tpurcode);
 leave_func:
    return result; 
}

/* }}} */

/* {{{ ndrxpy_tpnotify() */

static PyObject* ndrxpy_tpnotify(PyObject* self, PyObject* arg) {

    PyObject * result      = NULL;
    PyObject * clientid_py = NULL;
    PyObject * data_py     = NULL;
    PyObject * flags_py     = NULL;

    char * clientid_string = NULL;
    char * ndrxbuf          = NULL;
    long flags             = 0;

    CLIENTID clientid;

    if (!PyArg_ParseTuple(arg, "OO|O", &clientid_py, &data_py, &flags_py)) {
	goto leave_func;
    }


    if (flags_py) {
	if ((flags = PyLong_AsLong(flags_py)) < 0) {
	    PyErr_SetString(PyExc_RuntimeError, "tpnotify(): Bad flags given");
	    goto leave_func;
	}
    }

    if ((clientid_string = PyString_AsString(clientid_py)) == NULL) {

	NDRX_LOG(log_debug, "tpnotify(): No client name given");

	goto leave_func;
    }
    
    if ((ndrxbuf = transform_py_to_ndrx(data_py)) == NULL) {
	goto leave_func;
    }

    if (tpconvert((char*)clientid_string, (char*)&clientid, TPCONVCLTID) == -1) {
	char tmp[200] = "";
	sprintf(tmp, "tpconvert(string_clientid -> bin_clientid): %d - %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }
    
    if(tpnotify(&clientid, ndrxbuf, 0, flags) == -1) {
	char tmp[200] = "";
	sprintf(tmp, "tpnotify(): %d - %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }
    
    result = PyInt_FromLong((long)tpurcode);

 leave_func:
    if (ndrxbuf) tpfree(ndrxbuf);
    return result; 
}

/* }}} */
/* {{{ ndrxpy_tpbroadcast() */

static PyObject* ndrxpy_tpbroadcast(PyObject* self, PyObject* arg) {

    PyObject * result      = NULL;
    PyObject * data_py     = NULL;
    PyObject * lmid_py     = NULL;
    PyObject * usrname_py  = NULL;
    PyObject * cltname_py  = NULL;
    PyObject * flags_py    = NULL;

    char * lmid    = NULL;
    char * usrname = NULL;
    char * cltname = NULL;
    char * ndrxbuf  = NULL;

    long flags     = 0;

    if (!PyArg_ParseTuple(arg, "OOOO|O", &lmid_py, &usrname_py, &cltname_py, &data_py, &flags_py)) {
	goto leave_func;
    }

    if (flags_py) {
	if ((flags = PyLong_AsLong(flags_py)) < 0) {
	    PyErr_SetString(PyExc_RuntimeError, "tpbroadcast(): Bad flags given");
	    goto leave_func;
	}
    }
    
    if (lmid_py == Py_None) {
	lmid = NULL;
    } else {
	if ((lmid = PyString_AsString(lmid_py)) == NULL) {
	    char tmp[200] = "";
	    sprintf(tmp, "tpbroadcast(): no valid LMID");
	    PyErr_SetString(PyExc_RuntimeError, tmp);
	    goto leave_func;
	}
    }
	
    if (usrname_py == Py_None) {
	usrname = NULL;
    } else {
	if ((usrname = PyString_AsString(usrname_py)) == NULL) {
	    char tmp[200] = "";
	    sprintf(tmp, "tpbroadcast(): no valid usrname");
	    PyErr_SetString(PyExc_RuntimeError, tmp);
	    goto leave_func;
	}
    }

    if (cltname_py == Py_None) {
	cltname = NULL;
    } else {
	if ((cltname = PyString_AsString(cltname_py)) == NULL) {
	    char tmp[200] = "";
	    sprintf(tmp, "tpbroadcast(): no valid cltname");
	    PyErr_SetString(PyExc_RuntimeError, tmp);
	    goto leave_func;
	}
    }
	
    if ((ndrxbuf = transform_py_to_ndrx(data_py)) == NULL) {
	goto leave_func;
    }

/* EnduroX - not supported.
    if (tpbroadcast(lmid, usrname, cltname, tuxbuf,  0, flags) == -1) {
	char tmp[200] = "";
	sprintf(tmp, "tpbroadcast(): %d - %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }
*/
    
    result = PyInt_FromLong((long)tpurcode);

 leave_func:
    if (ndrxbuf) tpfree(ndrxbuf);
    return result; 
}

/* }}} */

/* {{{ ndrxpy_tpsetunsol() */

/* This function is the message handler that is called from the ENDUROX
   libraries. It calls the user-specified Python callable object */

static void unsol_handler(char* ndrxbuf, long len, long flags) {

    PyObject* data_py = NULL;

    /* Transform the ENDUROX buffer to a Python type (len is not needed
       (only STRING/UBF is supported), flags is not supported by ENDUROX */

    if ((data_py = transform_ndrxpy_to_py(ndrxbuf)) == NULL) {
	NDRX_LOG(log_debug, "transform_ndrxpy_to_py failed");
	goto leave_func;
    }

    /* Obtain the Global Interpreter Lock */
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();
    PyObject_CallFunction (py_unsol_handler, "O", data_py); 
    /* Release the thread. No Python API allowed beyond this point. */
    PyGILState_Release(gstate);    

 leave_func:
    
    return;
}


static PyObject* ndrxpy_tpsetunsol(PyObject* self, PyObject* arg) {
    char tmp[200] = "";
    PyObject * result = NULL;
    PyObject * old_py_unsol_handler;

    /*
      tpsetunsol(None) -> disable unsolicited message handler 
      tpsetunsol(mthd) -> enable unsolicited message handler
    */

    old_py_unsol_handler = py_unsol_handler;
    /* freed by caller (???) */

    if (!PyArg_ParseTuple(arg, "O", &py_unsol_handler)) {
	goto leave_func;
    }


    if (py_unsol_handler == Py_None) {
	if (tpsetunsol(NULL) == TPUNSOLERR) {
	    sprintf(tmp, "tpsetunsol(NULL): %d - %s", tperrno, tpstrerror(tperrno));
	    PyErr_SetString(PyExc_RuntimeError, tmp);
	    goto leave_func;
	}
    } else if (PyCallable_Check(py_unsol_handler)){
	if (tpsetunsol(unsol_handler) == TPUNSOLERR) {
	    sprintf(tmp, "tpsetunsol(func): %d - %s", tperrno, tpstrerror(tperrno));
	    PyErr_SetString(PyExc_RuntimeError, tmp);
	    goto leave_func;
	}
	/* store the python function, it will be called by the unsol_handler */
	Py_INCREF(py_unsol_handler); /* needed */
	
    } else {
	sprintf(tmp, "tpsetunsol(): No callable object given");
	PyObject_Print(py_unsol_handler, stdout, 0);
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }
    /* return the old function */

    if (old_py_unsol_handler) {
	result = old_py_unsol_handler;
    } else {
	/* No function was registered before */
	result = Py_None;
	Py_INCREF(Py_None);
    }
 leave_func:
    return result; 
}
/* }}} */
/* {{{ ndrxpy_tpchkunsol() */

static PyObject* ndrxpy_tpchkunsol(PyObject* self, PyObject* arg) {
    PyObject * result = NULL;
    long num_evts = 0;

    if ((num_evts = tpchkunsol()) == -1) {
	char tmp[200] = "";
	sprintf(tmp, "tpchkunsol(): %d - %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }
    
    result = PyInt_FromLong((long)num_evts);

 leave_func:
    return result; 
}

/* }}} */

/* {{{ ndrxpy_userlog() */

static PyObject * 
ndrxpy_userlog(PyObject * self, PyObject * arg)
{
    PyObject * result = NULL;
    char * log_string  = NULL;

    if (!PyArg_ParseTuple(arg, "s", &log_string)) {
	PyErr_SetString(PyExc_RuntimeError, "userlog(): expects a string to log");
	goto leave_func;
    }

    (void)userlog(log_string); 

    if ((result = Py_BuildValue("i", 1)) == NULL) {
	goto leave_func;
    }
    
 leave_func:
    return result;
}

/* }}} */

/* {{{ ndrxpy_tplog() */

static PyObject * 
ndrxpy_tplog(PyObject * self, PyObject * arg)
{
    PyObject * result = NULL;
    
    char * log_string  = NULL;
    int log_level = log_debug;

    if (!PyArg_ParseTuple(arg, "i|s", &log_level, &log_string)) {
	PyErr_SetString(PyExc_RuntimeError, "userlog(): expects a string to log");
	goto leave_func;
    }

    (void)tplog(log_level, log_string); 

    if ((result = Py_BuildValue("i", 1)) == NULL) {
	goto leave_func;
    }
    
 leave_func:
    return result;
}

/* }}} */


/* {{{ ndrxpy_get_tpurcode() */

static PyObject* ndrxpy_get_tpurcode(PyObject* self, PyObject* arg) {
    PyObject * result = NULL;
    result = PyInt_FromLong((long)tpurcode); /* tpurcode is thread-specific */
    return result; 
}

/* }}} */
/* {{{ ndrxpy_set_tpurcode() */

static PyObject* ndrxpy_set_tpurcode(PyObject* self, PyObject* arg) {
    PyObject * result = NULL;
    if (!PyArg_ParseTuple(arg, "l", &_set_tpurcode)) {
	result = NULL;
    }
    return result; 
}

/* }}} */
#ifndef NDRXWS
/* {{{ ndrx_mainloop() */

static PyObject * 
ndrx_mainloop(PyObject * self, PyObject * args)
{
    int i;
    int argc;
    char* argv[30];

    PyObject*  argv_obj  = NULL;    
    PyObject*  xa_switch = NULL;    
    
    /* 1st arg: argv, 2nd arg: server object, 3rd arg
       (optional): XA function switch */

    if (!PyArg_ParseTuple(args, "OOO|O", 
			  &argv_obj, &_server_obj, &_reloader_function, &xa_switch)) {

	NDRX_LOG(log_debug, "parseTuple 2");

	return NULL;
    }

    Py_INCREF(_server_obj);
    if (_reloader_function != Py_None) {
      Py_INCREF(_reloader_function);
    }
    
    for (i = 0; i < (argc = PyList_Size(argv_obj)); i++) {
	PyObject* tmp;
	tmp = PyList_GetItem(argv_obj, i);

	if (PyTuple_Check(tmp)) {
	    NDRX_LOG(log_debug, "PyTuple detected");
	}

	if (PyString_Check(tmp)) {
	    if (!(argv[i] = PyString_AsString(tmp))) {

		NDRX_LOG(log_debug, "argv[%d]: PyString_asString()", i);

		return NULL;
	    }

		NDRX_LOG(log_debug, "argv[%d] = %s \n", i, argv[i]);

	}
    } 

    mainloop(argc, argv);
    
    Py_INCREF(Py_None);
    return Py_None;
}

/* }}} */
#endif /* NDRXWS */

/* **************************************** */
/*                                          */
/*      Debinitions of global functions     */
/*                                          */
/* **************************************** */

/* {{{ initatmi() */
PyMODINIT_FUNC 
#ifndef NDRXWS
    initatmi(void)
#else
    initatmiws(void)
#endif /* NDRXWS */
{
    PyObject *m = NULL;
    PyObject *d = NULL;


    /* set proc_name (ATMI global variable) to display the client's name in
       the ULOG (if this is called by a client */
    
    /*proc_name = */

    /* Create the module and add the functions */
#ifndef NDRXWS    
    m = Py_InitModule("atmi", ndrxpy_methods);
#else
    m = Py_InitModule("atmiws", ndrxpy_methods);
#endif /* NDRXWS */

    /* Add some symbolic constants to the module */
    d = PyModule_GetDict(m);

    /* Exit codes */

    ins(d, "TPSUCCESS",	TPSUCCESS);
    ins(d, "TPFAIL",	TPFAIL);
    ins(d, "TPEXIT",	TPEXIT);


    /* Flags */

    ins(d, "TPNOBLOCK", TPNOBLOCK);	/* non-blocking send/rcv */


    ins(d, "TPSIGRSTRT", TPSIGRSTRT);	/* restart rcv on interrupt */
    ins(d, "TPNOREPLY", TPNOREPLY);	/* no reply expected */
    ins(d, "TPNOTRAN", TPNOTRAN);	/* not sent in transaction mode */
    ins(d, "TPTRAN", TPTRAN);	/* sent in transaction mode */
    ins(d, "TPNOTIME", TPNOTIME);	/* no timeout */
    ins(d, "TPABSOLUTE", TPABSOLUTE);	/* absolute value on tmsetprio */
    ins(d, "TPGETANY", TPGETANY);	/* get any valid reply */
    ins(d, "TPNOCHANGE", TPNOCHANGE);	/* force incoming buffer to match */
    ins(d, "RESERVED_BIT1", RESERVED_BIT1);	/* reserved for future use */
    ins(d, "TPCONV", TPCONV);	/* conversational service */
    ins(d, "TPSENDONLY", TPSENDONLY);	/* send-only mode */
    ins(d, "TPRECVONLY", TPRECVONLY);	/* recv-only mode */
    ins(d, "TPACK", TPACK);	/* */


    /* Flags to tpscmt() - Valid TP_COMMIT_CONTROL characteristic values */
    ins(d, "TP_CMT_LOGGED", TP_CMT_LOGGED);  /* return after commit decision is logged */
    ins(d, "TP_CMT_COMPLETE", TP_CMT_COMPLETE);	/* return after commit has completed */


    /* Flags to tpinit() */
    ins(d, "TPU_MASK", TPU_MASK); /* unsolicited notification mask */
    ins(d, "TPU_SIG", TPU_SIG);	/* signal based notification */
    ins(d, "TPU_DIP", TPU_DIP);	/* dip-in based notification */
    ins(d, "TPU_IGN", TPU_IGN);	/* ignore unsolicited messages */

    ins(d, "TPSA_FASTPATH", TPSA_FASTPATH);	/* System access == fastpath */
    ins(d, "TPSA_PROTECTED", TPSA_PROTECTED);	/* System access == protected */

    ins(d, "TPMULTICONTEXTS", TPMULTICONTEXTS);	/* Enable MULTI context */

    /* Flags to tpconvert() */
    ins(d, "TPTOSTRING", TPTOSTRING);	/* Convert structure to string */
    ins(d, "TPCONVCLTID", TPCONVCLTID);	/* Convert CLIENTID */
    ins(d, "TPCONVTRANID", TPCONVTRANID);	/* Convert TRANID */
    ins(d, "TPCONVXID", TPCONVXID);	/* Convert XID */

    ins(d, "TPCONVMAXSTR", TPCONVMAXSTR);		/* Maximum string size */

    /* Return values to tpchkauth() */
    ins(d, "TPNOAUTH", TPNOAUTH); /*authentication */
    ins(d, "TPSYSAUTH", TPSYSAUTH); /*authentication */
    ins(d, "TPAPPAUTH", TPAPPAUTH); /* and application authentication */

    ins(d, "MAXTIDENT", MAXTIDENT); /*len of a /T identifier */


    ins(d, "TPNOFLAGS",	TPNOFLAGS);                     /* set/get correlation id */            
    ins(d, "TPQFAILUREQ", TPQFAILUREQ);			/* set/get failure queue */             
    ins(d, "TPQBEFOREMSGID", TPQBEFOREMSGID);		/* enqueue before message id */         
    ins(d, "TPQGETBYMSGID", TPQGETBYMSGID);		/* dequeue by msgid */                  
    ins(d, "TPQMSGID", TPQMSGID);			/* get msgid of enq/deq message */      
    ins(d, "TPQCORRID", TPQCORRID);			/* get corrid of enq/deq message */      
    ins(d, "TPQPRIORITY", TPQPRIORITY);			/* set/get message priority */          
    ins(d, "TPQTOP", TPQTOP);				/* enqueue at queue top */              
    ins(d, "TPQWAIT", TPQWAIT);				/* wait for dequeuing */                
    ins(d, "TPQREPLYQ", TPQREPLYQ);			/* set/get reply queue */               
    ins(d, "TPQTIME_ABS", TPQTIME_ABS);			/* set absolute time */                 
    ins(d, "TPQTIME_REL", TPQTIME_REL);		        /* set absolute time */                 
    ins(d, "TPQGETBYCORRID", TPQGETBYCORRID);		/* dequeue by corrid */                 
    ins(d, "TPQPEEK", TPQPEEK);


    /* Event Subscription related constants */

    ins(d, "TPEVSERVICE", TPEVSERVICE);
    ins(d, "TPEVQUEUE", TPEVQUEUE  );
    ins(d, "TPEVTRAN", TPEVTRAN   );
    ins(d, "TPEVPERSIST", TPEVPERSIST);

    
    /* Conversation related constants */

    ins(d, "TPCONV", TPCONV);                           /* conversational service */
    ins(d, "TPSENDONLY", TPSENDONLY);                   /* send-only mode */
    ins(d, "TPRECVONLY", TPRECVONLY);                   /* recv-only mode */

    ins(d, "TPEV_DISCONIMM", TPEV_DISCONIMM);
    ins(d, "TPEV_SVCERR", TPEV_SVCERR);
    ins(d, "TPEV_SVCSUCC", TPEV_SVCSUCC);
    ins(d, "TPEV_SENDONLY", TPEV_SENDONLY);


    /* Error constants */
    
    ins(d, "TPMINVAL", TPMINVAL);
    ins(d, "TPEABORT", TPEABORT);
    ins(d, "TPEBADDESC", TPEBADDESC);
    ins(d, "TPEBLOCK", TPEBLOCK);
    ins(d, "TPEINVAL", TPEINVAL);
    ins(d, "TPELIMIT", TPELIMIT);
    ins(d, "TPENOENT", TPENOENT);
    ins(d, "TPEOS", TPEOS);
    ins(d, "TPEPERM", TPEPERM);
    ins(d, "TPEPROTO", TPEPROTO);
    ins(d, "TPESVCERR", TPESVCERR);
    ins(d, "TPESVCFAIL", TPESVCFAIL);
    ins(d, "TPESYSTEM", TPESYSTEM);
    ins(d, "TPETIME", TPETIME);
    ins(d, "TPETRAN", TPETRAN);
    ins(d, "TPGOTSIG", TPGOTSIG);
    ins(d, "TPERMERR", TPERMERR);
    ins(d, "TPEITYPE", TPEITYPE);
    ins(d, "TPEOTYPE", TPEOTYPE);
    ins(d, "TPERELEASE", TPERELEASE);
    ins(d, "TPEHAZARD", TPEHAZARD);
    ins(d, "TPEHEURISTIC", TPEHEURISTIC);
    ins(d, "TPEEVENT", TPEEVENT);
    ins(d, "TPEMATCH", TPEMATCH);
    ins(d, "TPEDIAGNOSTIC", TPEDIAGNOSTIC);
    ins(d, "TPEMIB", TPEMIB);
    ins(d, "TPMAXVAL", TPMAXVAL);
    ins(d, "TPNULLCONTEXT", TPNULLCONTEXT);
    ins(d, "tperrno", tperrno);


    /* Endurox Version as defined by user at compile time */
    
    ins(d, "NDRXVERSION", NDRXVERSION);
    
    /* tplog constants: */
    
    ins(d, "log_always", log_always);
    ins(d, "log_error", log_error);
    ins(d, "log_warn", log_warn);
    ins(d, "log_info", log_info);
    ins(d, "log_debug", log_debug);
    ins(d, "log_dump", log_dump);

    /* Check for errors */
    if (PyErr_Occurred())
	Py_FatalError("can't initialize module atmi");
}

/* }}} */

#ifndef NDRXWS

/* {{{ tpsvrinit() */

int
tpsvrinit(int argc, char *argv[])
{
    PyObject * argv_py = NULL;
    /* build a list from char* argv[] */
    
    argv_py = makeargvobject(argc, argv);

    _server_is_running++;

    (void)PyObject_CallMethod(_server_obj, "init", "O", argv_py);
    Py_DECREF(argv_py);

    return(0);
}

/* }}} */
/* {{{ tpsvrdone() */

void
tpsvrdone(void)
{
    _server_is_running--;
    (void)PyObject_CallMethod(_server_obj, "cleanup", NULL);
    return;
}

/* }}} */


/* {{{ ndrxpy_tpgetnodeid() */

static PyObject* ndrxpy_tpgetnodeid(PyObject* self, PyObject* arg) {

    PyObject * result         = NULL;
    int ret      = -1;

    ret = tpgetnodeid();
    if (ret < 0) {
	char tmp[200] = "";
	sprintf(tmp, "tpgetnodeid(): %d -  %s", tperrno, tpstrerror(tperrno));
	PyErr_SetString(PyExc_RuntimeError, tmp);
	goto leave_func;
    }

   result = PyLong_FromLong((long)ret);

 leave_func:
    return result; 
}

/* }}} */

/* {{{ endurox_dispatch() */

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  This is the service routine that is called by the Ndrx runtime
  environment.  The TPSVCINFO structure is transformed to instance
  variables of the server object. The data portion is transformed to a
  Python string or dictionary. The appropriate (rqst->name) service routine
  is called, the returned value is transformed back to a Ndrx data type.

  TPSVCINFO * rqst         TPSVCINFO structure from the Ndrx runtime environment
  +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

void endurox_dispatch(TPSVCINFO * rqst) {
    int idx = 0;
    PyObject* new_server_obj = NULL;
    PyObject* obj, *pydata;
    PyObject *py_name = NULL;
    PyObject *py_cltid = NULL;
    PyObject *py_cd = NULL;
    PyObject *py_flags = NULL;
    PyObject *py_appkey = NULL;
    char cltid_string[TPCONVMAXSTR+1] = "";
    char* res_ndrx = NULL;
    long tp_returncode = TPSUCCESS;

    /* reset user return code */
    _set_tpurcode = 0;
    
    /* reset flag advising us to do a tpforward() instead of a tpreturn() */
    _forward = 0;


    /* store values from TPSVCINFO into instance variables */

    /* TPSVCINFO = 
       
       char        name[32];
       char        *data;
       long        len;  
       long        flags;  
       int         cd;  
       long        appkey;  
       CLIENTID    cltid;  (long[4])
	   
    */

    /* call user-spupplied pre-dispatch routine */

    /*    PyObject* PyObject_CallFunction (PyObject *callable_object, char *format, ...) */
    

    if (_reloader_function != Py_None) {
	new_server_obj = PyObject_CallFunction(_reloader_function, NULL);
	if (new_server_obj != NULL) {
	    Py_DECREF(_server_obj);
	    _server_obj = new_server_obj;
	    Py_INCREF(_server_obj);
	}
    } 

    py_name = PyString_FromString(rqst->name);
    if (py_name) {
	if (PyObject_SetAttrString(_server_obj, "name", py_name) < 0) {
	    NDRX_LOG(log_debug, "PyObject_SetAttrString( name ) error");
	}
	Py_DECREF(py_name); 
	py_name = NULL;
    }
    
    /* connection descriptor makes sense only if this is a conversational 
       service */
    py_cd = PyInt_FromLong(rqst->cd);
    if (py_cd && (rqst->flags & TPCONV)) {
	if (PyObject_SetAttrString( _server_obj, "cd", py_cd) < 0) {
	    NDRX_LOG(log_debug, "PyObject_SetAttrString( cd ) error");
	}
    }
    if (py_cd) { 
	Py_DECREF(py_cd); py_cd = NULL; 
    }

    py_flags = PyLong_FromLong(rqst->flags);
    if (py_flags) {
	if (PyObject_SetAttrString(_server_obj, "flags", py_flags) < 0) {
	    NDRX_LOG(log_debug, "PyObject_SetAttrString( flags ) error");
	}
	Py_DECREF(py_flags);
	py_flags = NULL;
    }
    
    py_appkey = PyLong_FromLong(rqst->appkey);
    if (py_appkey) {
	if (PyObject_SetAttrString(_server_obj, "appkey", py_appkey) < 0) {
	    NDRX_LOG(log_debug, "PyObject_SetAttrString( appkey ) error");
	}
	Py_DECREF(py_appkey);
	py_appkey = NULL;
    }
    
    /* convert CLIENTID to string */
    
    if (tpconvert(cltid_string, (char*)(rqst->cltid).clientdata, TPTOSTRING | TPCONVCLTID) == -1) {
	NDRX_LOG(log_debug, "tpconvert(bin_clientid -> string_clientid): %d - %s", tperrno, tpstrerror(tperrno));
	tpreturn(TPFAIL, _set_tpurcode, 0, 0L, 0);
    }

    py_cltid = PyString_FromString(cltid_string);
    if (py_cltid) {
	if (PyObject_SetAttrString(_server_obj, "cltid", py_cltid) < 0) {
	    NDRX_LOG(log_debug, "PyObject_SetAttrString( cltid ) error");
	}
	Py_DECREF(py_cltid); 
	py_cltid = NULL;
    }

    if ((idx=find_entry(rqst->name)) < 0) {
	NDRX_LOG(log_debug, "unknown servicename");
	tpreturn(TPFAIL, _set_tpurcode, 0, 0L, 0);
    }

    NDRX_LOG(log_debug, "transforming buffer ...");

    if ((obj = transform_ndrxpy_to_py(rqst->data)) == NULL) {
	NDRX_LOG(log_debug, "Cannot convert input buffer to a Python type");
	tpreturn(TPFAIL, _set_tpurcode, 0, 0L, 0);
    }
    
    NDRX_LOG(log_debug, "calling %s/%s ... (_server_obj=%p)", 
            _registered_services[idx].name, _registered_services[idx].method,
            _server_obj);
    
    if (!(pydata = PyObject_CallMethod(_server_obj, _registered_services[idx].method, "O", obj))) {
	Py_XDECREF(obj);
	NDRX_LOG(log_debug, "Error calling method %s ...", _registered_services[idx].method);
	tpreturn(TPFAIL, _set_tpurcode, 0, 0L, 0);
    }

    /* _forward was maybe set by the server's method by calling
       tpforward(). In this case, the data that should be returned to the
       caller has been stored in _forward_pydata.  */

    if (_forward) {

	NDRX_LOG(log_debug, "endurox_dispatch: forward: %s ->", _forward_service);
	PyObject_Print(_forward_pydata, stdout, 0);
	NDRX_LOG(log_debug, "");

	Py_XDECREF(pydata); /* don't need the data returned from function call (should be NULL) */
	pydata = _forward_pydata; /* reference count was incremented by ndrxpy_tpforward() */
    }


    NDRX_LOG(log_debug, "returning ...");


    /* If the method call returned an integer, transform it to a Ndrx return
       code. If a string or dictionary was returned, transform it to the
       corresponding Ndrx data type. In this case, a return code of
       TPSUCCESS is returned */
    
    if (PyInt_Check(pydata) || PyLong_Check(pydata)) {
	switch (PyInt_AsLong(pydata)) {
	case TPFAIL: 
	    tp_returncode = TPFAIL; 
	    break;
	case TPEXIT: 
	    tp_returncode = TPEXIT; 
	    break;
	case TPSUCCESS:
	    tp_returncode = TPSUCCESS; 
	    break;
	default:
	    NDRX_LOG(log_debug, "Unknown integer return code (assuming TPEXIT)");
	    PyErr_SetString(PyExc_RuntimeError, "Unknown integer return code (assuming TPEXIT)");
	    tp_returncode = TPEXIT;
	}
    } else {
	/* transform..() takes String or Dict */
	if ((res_ndrx = transform_py_to_ndrx(pydata)) == NULL) {
	    Py_XDECREF(obj);
	    Py_XDECREF(pydata);
	    tpreturn(TPFAIL, _set_tpurcode, 0, 0L, 0);
	}
    }


    Py_XDECREF(obj);
    Py_XDECREF(pydata);
    
    if (_forward) {

	NDRX_LOG(log_debug, "call tpforward(%s, ...)\n", _forward_service);

	tpforward(_forward_service, (char*)res_ndrx, 0L, 0);
    } else {
	NDRX_LOG(log_debug, "call tpreturn(TPSUCCESS, ...)");
	tpreturn(tp_returncode, _set_tpurcode, (char*)res_ndrx, 0L, 0);
    }
}

/* }}} */

#endif /* NDRXWS */
