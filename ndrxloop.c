
/* 
   This file is an adapted version of the "main" file from the ENDUROX
   command "buildserver -k ...". The main() function has been removed and replaced
   by the ndrx_mainloop() function in the ndrxmodule.c file 
   
   (c) 1999, 2000 Ralf Henschkowski (ralfh@gmx.ch)

*/


#include <stdio.h>
#include <xa.h>
#include <atmi.h>

#include <Python.h>

#if defined(__cplusplus)
extern "C" {
#endif
extern int _tmrunserver _((int));

#if defined(__cplusplus)
}
#endif

extern void endurox_dispatch(TPSVCINFO * rqst);



static struct tmdsptchtbl_t _tmdsptchtbl[] = {
    { "", "dispatch", (void (*) _((TPSVCINFO *))) endurox_dispatch, 0, 0 },  
    { NULL, NULL, NULL, 0, 0 }
};



#ifndef _TMDLLIMPORT
#define _TMDLLIMPORT
#endif

_TMDLLIMPORT extern struct xa_switch_t tmnull_switch;

static struct xa_switch_t* xa_switch = &tmnull_switch;

static struct tmsvrargs_t tmsvrargs = {
	NULL,
	&_tmdsptchtbl[0],
	0,
	tpsvrinit,
	tpsvrdone,
	_tmrunserver,	/* PRIVATE  */
	NULL,			/* RESERVED */
	NULL,			/* RESERVED */
	NULL,			/* RESERVED */
	NULL 			/* RESERVED */
};

struct tmsvrargs_t *
#ifdef _TMPROTOTYPES
_tmgetsvrargs(void)
#else
_tmgetsvrargs()
#endif
{
    tmsvrargs.xa_switch = xa_switch;
    return(&tmsvrargs);
}


void _set_XA_switch(struct xa_switch_t* new_xa_switch) 
{
    xa_switch = new_xa_switch;
}
    
