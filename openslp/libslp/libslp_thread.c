/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slplib_reg.c                                               */
/*                                                                         */
/* Abstract:    Implementation for functions register and deregister       */
/*              services -- SLPReg() call.                                 */
/*                                                                         */
/* Author(s):   Matthew Peterson                                           */
/*                                                                         */
/***************************************************************************/

#include "slp.h"
#include "libslp.h"

/*=========================================================================*/ 
SLPError ThreadCreate(ThreadStartProc startproc, void *arg)
/* Creates a thread                                                        */
/*                                                                         */
/* startproc    (IN) Address of the thread start procedure.                */
/*                                                                         */
/* arg          (IN) An argument for the thread start procedure.           */
/*                                                                         */
/* Returns      SLPError code                                              */
/*=========================================================================*/
{
    return SLP_NOT_IMPLEMENTED;
}

