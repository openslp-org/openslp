/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_win32.c                                               */
/*                                                                         */
/* Abstract:    Win32 specific part, to make SLPD run as a "service"       */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/*     Please submit patches to http://www.openslp.org                     */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/* Copyright (C) 2000 Caldera Systems, Inc                                 */
/* All rights reserved.                                                    */
/*                                                                         */
/* Redistribution and use in source and binary forms, with or without      */
/* modification, are permitted provided that the following conditions are  */
/* met:                                                                    */ 
/*                                                                         */
/*      Redistributions of source code must retain the above copyright     */
/*      notice, this list of conditions and the following disclaimer.      */
/*                                                                         */
/*      Redistributions in binary form must reproduce the above copyright  */
/*      notice, this list of conditions and the following disclaimer in    */
/*      the documentation and/or other materials provided with the         */
/*      distribution.                                                      */
/*                                                                         */
/*      Neither the name of Caldera Systems nor the names of its           */
/*      contributors may be used to endorse or promote products derived    */
/*      from this software without specific prior written permission.      */
/*                                                                         */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS     */
/* `AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT      */
/* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR   */
/* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE CALDERA      */
/* SYSTEMS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, */
/* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT        */
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON       */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT */
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE   */
/* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    */
/*                                                                         */
/***************************************************************************/


#include "slpd.h"

/*=========================================================================*/
/* slpd includes                                                           */
/*=========================================================================*/
#include "slpd_cmdline.h"
#include "slpd_log.h"
#include "slpd_property.h"
#include "slpd_database.h"
#include "slpd_socket.h"
#include "slpd_incoming.h"
#include "slpd_outgoing.h"
#include "slpd_knownda.h"


/*=========================================================================*/
/* common code includes                                                    */
/*=========================================================================*/
#include "slp_linkedlist.h"
#include "slp_xid.h"

SERVICE_STATUS          ssStatus;       /* current status of the service  */
SERVICE_STATUS_HANDLE   sshStatusHandle; 
BOOL                    bDebug = FALSE; 
TCHAR                   szErr[256];

/*-------------------------------------------------------------------------*/
extern int G_SIGTERM;
/* see slpd_main.c                                                         */
/*-------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------*/
void LoadFdSets(SLPList* socklist, 
                int* highfd, 
                fd_set* readfds, 
                fd_set* writefds);
/* see slpd_main.c                                                         */
/*-------------------------------------------------------------------------*/


/*------------------------------------------------------------------------*/
void HandleSigTerm();
/* see slpd_main.c                                                        */
/*------------------------------------------------------------------------*/


/*------------------------------------------------------------------------*/
void HandleSigAlrm();
/* see slpd_main.c                                                        */
/*------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
BOOL ReportStatusToSCMgr(DWORD dwCurrentState, 
                         DWORD dwWin32ExitCode, 
                         DWORD dwWaitHint) 
/*                                                                          */
/*  PURPOSE: Sets the current status of the service and                     */
/*           reports it to the Service Control Manager                      */
/*                                                                          */
/*  PARAMETERS:                                                             */
/*    dwCurrentState - the state of the service                             */
/*    dwWin32ExitCode - error code to report                                */
/*    dwWaitHint - worst case estimate to next checkpoint                   */
/*                                                                          */
/*  RETURN VALUE:                                                           */
/*    TRUE  - success                                                       */
/*    FALSE - failure                                                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/
{
    static DWORD dwCheckPoint = 1; 
    BOOL fResult = TRUE; 

    /* when debugging we don't report to the SCM    */
    if(G_SlpdCommandLine.action != SLPD_DEBUG)
    {
        if(dwCurrentState == SERVICE_START_PENDING)
        {
            ssStatus.dwControlsAccepted = 0;
        }
        else
        {
            ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP; 
        }

        ssStatus.dwCurrentState = dwCurrentState; 
        ssStatus.dwWin32ExitCode = dwWin32ExitCode; 
        ssStatus.dwWaitHint = dwWaitHint; 

        if(( dwCurrentState == SERVICE_RUNNING ) || 
           ( dwCurrentState == SERVICE_STOPPED ))
        {
            ssStatus.dwCheckPoint = 0;
        }
        else
        {
            ssStatus.dwCheckPoint = dwCheckPoint++; 
        }   

        /* Report the status of the service to the service control manager.*/

        if(!(fResult = SetServiceStatus( sshStatusHandle, &ssStatus)))
        {
            SLPDLog("SetServiceStatus failed"); 
        }
    }
    return fResult; 
} 


/*--------------------------------------------------------------------------*/
LPTSTR GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize ) 
/*                                                                          */
/*  PURPOSE: copies error message text to string                            */
/*                                                                          */
/*  PARAMETERS:                                                             */
/*    lpszBuf - destination buffer                                          */
/*    dwSize - size of buffer                                               */
/*                                                                          */
/*  RETURN VALUE:                                                           */
/*    destination buffer                                                    */
/*                                                                          */
/*--------------------------------------------------------------------------*/
{
    DWORD dwRet; 
    LPTSTR lpszTemp = NULL; 

    dwRet = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                           FORMAT_MESSAGE_FROM_SYSTEM |
                           FORMAT_MESSAGE_ARGUMENT_ARRAY, 
                           NULL, 
                           GetLastError(), 
                           LANG_NEUTRAL, 
                           (LPTSTR)&lpszTemp, 
                           0, 
                           NULL ); 

    /* supplied buffer is not long enough    */
    if(!dwRet || ( (long)dwSize < (long)dwRet+14 ))
    {
        lpszBuf[0] = TEXT('\0');
    }
    else
    {
        lpszTemp[lstrlen(lpszTemp)-2] = TEXT('\0');  
        sprintf( lpszBuf, "%s (0x%x)", lpszTemp, GetLastError() ); 
    } 

    if(lpszTemp)
    {
        LocalFree((HLOCAL) lpszTemp );
    }

    return lpszBuf; 
} 


/*--------------------------------------------------------------------------*/
void ServiceStop() 
/*--------------------------------------------------------------------------*/
{
    G_SIGTERM = 1;
    ReportStatusToSCMgr(SERVICE_STOPPED,       /* service state */
                        NO_ERROR,              /* exit code    */
                        3000);                 /* wait hint    */
} 


/*--------------------------------------------------------------------------*/
void ServiceStart (int argc, char **argv) 
/*--------------------------------------------------------------------------*/
{
    fd_set          readfds;
    fd_set          writefds;
    int             highfd;
    int             fdcount         = 0;
    time_t          curtime;
    time_t          alarmtime;
    struct timeval  timeout;
    WSADATA         wsaData; 
    WORD            wVersionRequested = MAKEWORD(1,1); 

    /*------------------------*/
    /* Service initialization */
    /*------------------------*/
    if(!ReportStatusToSCMgr(SERVICE_START_PENDING, /* service state*/
                            NO_ERROR,              /* exit code    */
                            3000))                 /* wait hint    */
    {
        goto cleanup;
    }

    if(WSAStartup(wVersionRequested, &wsaData) != 0)
    {
        (void)ReportStatusToSCMgr(SERVICE_STOP_PENDING, 
                                  NO_ERROR, 
                                  0); 
        goto cleanup;
    }

    /*------------------------*/
    /* Parse the command line */
    /*------------------------*/
    if(SLPDParseCommandLine(argc,argv))
    {
        ReportStatusToSCMgr(SERVICE_STOP_PENDING, /* service state    */
                            NO_ERROR,             /* exit code    */
                            0);                   /* wait hint    */
        goto cleanup_winsock;
    }
    if(!ReportStatusToSCMgr(SERVICE_START_PENDING, /* service state    */
                            NO_ERROR,              /* exit code    */
                            3000))                 /* wait hint    */
    {
        goto cleanup_winsock;
    }

    /*------------------------------*/
    /* Initialize the log file      */
    /*------------------------------*/
    if(SLPDLogFileOpen(G_SlpdCommandLine.logfile, 1))
    {
        SLPDLog("Could not open logfile %s\n",G_SlpdCommandLine.logfile);
        goto cleanup_winsock;
    }

    /*------------------------*/
    /* Seed the XID generator */
    /*------------------------*/
    SLPXidSeed();

    /*---------------------*/
    /* Log startup message */
    /*---------------------*/
    SLPDLog("****************************************\n");
    SLPDLogTime();
    SLPDLog("SLPD daemon started\n");
    SLPDLog("****************************************\n");
    SLPDLog("Command line = %s\n",argv[0]);
    SLPDLog("Using configuration file = %s\n",G_SlpdCommandLine.cfgfile);
    SLPDLog("Using registration file = %s\n",G_SlpdCommandLine.regfile);
    if(!ReportStatusToSCMgr(SERVICE_START_PENDING, /* service state    */
                            NO_ERROR,              /* exit code    */
                            3000))                 /* wait hint    */
    {
        goto cleanup_winsock;
    }


    /*--------------------------------------------------*/
    /* Initialize for the first time                    */
    /*--------------------------------------------------*/
    if(SLPDPropertyInit(G_SlpdCommandLine.cfgfile) ||
       SLPDDatabaseInit(G_SlpdCommandLine.regfile) ||
       SLPDIncomingInit() ||
       SLPDOutgoingInit() ||
       SLPDKnownDAInit())
    {
        SLPDLog("slpd initialization failed\n");
        goto cleanup_winsock;
    }
    SLPDLog("Agent Interfaces = %s\n",G_SlpdProperty.interfaces);
    SLPDLog("Agent URL = %s\n",G_SlpdProperty.myUrl);

    /* Service is now running, perform work until shutdown    */

    if(!ReportStatusToSCMgr(SERVICE_RUNNING,       /* service state    */
                            NO_ERROR,              /* exit code    */
                            0))                    /* wait hint    */
    {
        goto cleanup_winsock;
    }


    /*-----------*/
    /* Main loop */
    /*-----------*/
    SLPDLog("Startup complete entering main run loop ...\n\n");
    G_SIGTERM   = 0;
    curtime = time(&alarmtime);
    alarmtime = curtime + SLPD_AGE_INTERVAL;
    while(G_SIGTERM == 0)
    {
        /*--------------------------------------------------------*/
        /* Load the fdsets up with all valid sockets in the list  */
        /*--------------------------------------------------------*/
        highfd = 0;
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        LoadFdSets(&G_IncomingSocketList, &highfd, &readfds,&writefds);
        LoadFdSets(&G_OutgoingSocketList, &highfd, &readfds,&writefds);

        /*--------------------------------------------------*/
        /* Before select(), check to see if we got a signal */
        /*--------------------------------------------------*/
        if(G_SIGALRM)
        {
            goto HANDLE_SIGNAL;
        }

        /*-------------*/
        /* Main select */
        /*-------------*/
        timeout.tv_sec = SLPD_AGE_INTERVAL;
        timeout.tv_usec = 0;
        fdcount = select(highfd+1,&readfds,&writefds,0,&timeout);
        if(fdcount > 0) /* fdcount will be < 0 when timed out */
        {
            SLPDIncomingHandler(&fdcount,&readfds,&writefds);
            SLPDOutgoingHandler(&fdcount,&readfds,&writefds);
        }

        /*----------------*/
        /* Handle signals */
        /*----------------*/
        HANDLE_SIGNAL:
        curtime = time(&curtime);
        if(curtime >= alarmtime)
        {
            HandleSigAlrm();
            alarmtime = curtime + SLPD_AGE_INTERVAL;
        }

    } /* End of main loop */

    /* Got SIGTERM */
    HandleSigTerm();

    cleanup_winsock:
    WSACleanup();

    cleanup: 
    ;
} 

/*==========================================================================*/
BOOL WINAPI ControlHandler ( DWORD dwCtrlType ) 
/*                                                                          */
/*  PURPOSE: Handled console control events                                 */
/*                                                                          */
/*  PARAMETERS:                                                             */
/*    dwCtrlType - type of control event                                    */
/*                                                                          */
/*  RETURN VALUE:                                                           */
/*    True - handled                                                        */
/*    False - unhandled                                                     */
/*                                                                          */
/*==========================================================================*/
{
    switch(dwCtrlType)
    {
    case CTRL_BREAK_EVENT:  /* use Ctrl+C or Ctrl+Break to simulate    */
    case CTRL_C_EVENT:      /* SERVICE_CONTROL_STOP in debug mode    */
        printf("Stopping %s.\n", G_SERVICEDISPLAYNAME); 
        ServiceStop(); 
        return TRUE; 
        break; 

    } 
    return FALSE; 
} 


/*==========================================================================*/
VOID WINAPI ServiceCtrl(DWORD dwCtrlCode) 
/*                                                                          */
/*  PURPOSE: This function is called by the SCM whenever                    */
/*           ControlService() is called on this service.                    */
/*                                                                          */
/*  PARAMETERS:                                                             */
/*    dwCtrlCode - type of control requested                                */
/*                                                                          */
/*  RETURN VALUE:                                                           */
/*    none                                                                  */
/*                                                                          */
/*==========================================================================*/
{
    /* Handle the requested control code.    */
    /*    */

    switch(dwCtrlCode)
    {
    /* Stop the service.    */
    case SERVICE_CONTROL_STOP: 
        ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, 0); 
        ServiceStop(); 
        return; 

        /* Update the service status.    */
    case SERVICE_CONTROL_INTERROGATE: 
        break; 

        /* invalid control code    */
    default: 
        break; 

    } 

    ReportStatusToSCMgr(ssStatus.dwCurrentState, NO_ERROR, 0); 
} 


/*==========================================================================*/
void WINAPI SLPDServiceMain(DWORD argc, LPTSTR *argv) 
/*==========================================================================*/
{

    /* register our service control handler:    */
    sshStatusHandle = RegisterServiceCtrlHandler( G_SERVICENAME, ServiceCtrl); 

    if(sshStatusHandle != 0)
    {
        /* SERVICE_STATUS members that don't change    */
        ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS; 
        ssStatus.dwServiceSpecificExitCode = 0; 


        /* report the status to the service control manager.    */
        if(ReportStatusToSCMgr(SERVICE_START_PENDING, /* service state    */
                               NO_ERROR,              /* exit code    */
                               3000))                 /* wait hint    */
        {
            ServiceStart(argc, argv); 
        }
    }


    /* try to report the stopped status to the service control manager.    */

    if(sshStatusHandle)
        (void)ReportStatusToSCMgr(SERVICE_STOPPED, 
                                  0, 
                                  0);


} 


/*--------------------------------------------------------------------------*/
void SLPDCmdInstallService() 
/*--------------------------------------------------------------------------*/
{
    SC_HANDLE   schService; 
    SC_HANDLE   schSCManager; 

    TCHAR szPath[512]; 

    if(GetModuleFileName( NULL, szPath, 512 ) == 0)
    {
        printf("Unable to install %s - %s\n", 
               G_SERVICEDISPLAYNAME, 
               GetLastErrorText(szErr, 256)); 
        return; 
    }

    schSCManager = OpenSCManager(
                                NULL,                   /* machine (NULL == local)    */
                                NULL,                   /* database (NULL == default)    */
                                SC_MANAGER_ALL_ACCESS); /* access required    */

    if(schSCManager)
    {
        schService = CreateService(
                                  schSCManager,               /* SCManager database    */
                                  G_SERVICENAME,        /* name of service    */
                                  G_SERVICEDISPLAYNAME, /* name to display    */
                                  SERVICE_ALL_ACCESS,         /* desired access    */
                                  SERVICE_WIN32_OWN_PROCESS,  /* service type    */
                                  SERVICE_DEMAND_START,       /* start type    */
                                  SERVICE_ERROR_NORMAL,       /* error control type    */
                                  szPath,                     /* service's binary    */
                                  NULL,                       /* no load ordering group    */
                                  NULL,                       /* no tag identifier    */
                                  "",       /* dependencies    */
                                  NULL,                       /* LocalSystem account    */
                                  NULL);                      /* no password    */

        if(schService)
        {
            printf("%s installed.\n", G_SERVICEDISPLAYNAME ); 
            CloseServiceHandle(schService); 
        }
        else
        {
            printf("CreateService failed - %s\n", GetLastErrorText(szErr, 256)); 
        } 

        CloseServiceHandle(schSCManager); 
    }
    else
        printf("OpenSCManager failed - %s\n", GetLastErrorText(szErr,256)); 
} 

/*--------------------------------------------------------------------------*/
static void SLPDHlpStopService(SC_HANDLE schService)
/*--------------------------------------------------------------------------*/
{
	/* try to stop the service    */
	if(ControlService(schService, SERVICE_CONTROL_STOP, &ssStatus))
	{
		printf("Stopping %s.", G_SERVICEDISPLAYNAME);
		Sleep(1000);
      
		while(QueryServiceStatus(schService, &ssStatus))
		{
			if(ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
			{
				printf(".");
				Sleep(1000);
			}
			else
				break; 
		} 
      
		if(ssStatus.dwCurrentState == SERVICE_STOPPED)
			printf("\n%s stopped.\n", G_SERVICEDISPLAYNAME);
		else
			printf("\n%s failed to stop.\n", G_SERVICEDISPLAYNAME); 
	}
}
 
/*--------------------------------------------------------------------------*/
void SLPDCmdRemoveService() 
/*--------------------------------------------------------------------------*/
{
    SC_HANDLE schService; 
    SC_HANDLE schSCManager; 

    schSCManager = OpenSCManager(
                                NULL,                   /* machine (NULL == local)    */
                                NULL,                   /* database (NULL == default)    */
                                SC_MANAGER_ALL_ACCESS); /* access required    */
    if(schSCManager)
    {
        schService = OpenService(schSCManager, G_SERVICENAME, SERVICE_ALL_ACCESS); 

        if(schService)
        {
				SLPDHlpStopService(schService);

				/* now remove the service    */
            if(DeleteService(schService))
                printf("%s removed.\n", G_SERVICEDISPLAYNAME );
            else
                printf("DeleteService failed - %s\n", GetLastErrorText(szErr,256)); 


            CloseServiceHandle(schService); 
        }
        else
            printf("OpenService failed - %s\n", GetLastErrorText(szErr,256)); 

        CloseServiceHandle(schSCManager); 
    }
    else
        printf("OpenSCManager failed - %s\n", GetLastErrorText(szErr,256)); 
} 

/*--------------------------------------------------------------------------*/
void SLPDCmdStartService(void)
/*--------------------------------------------------------------------------*/
{
	 SC_HANDLE schService; 
	 SC_HANDLE schSCManager; 

	 schSCManager = OpenSCManager(
	 									  NULL,                   /* machine (NULL == local)    */
	 									  NULL,                   /* database (NULL == default) */
	 									  SC_MANAGER_ALL_ACCESS); /* access required    		  */
	 if(schSCManager)
	 {
	 	  schService = OpenService(schSCManager, G_SERVICENAME, SERVICE_ALL_ACCESS); 

	 	  if(schService)
	 	  {
	 			if( !StartService(schService, 0, NULL))
	 			{
	 				 printf("OpenService failed - %s\n", GetLastErrorText(szErr,256)); 
	 			}
	 			CloseServiceHandle(schService); 
	 	  }
	 	  else
	 	  {
	 	  		printf("OpenService failed - %s\n", GetLastErrorText(szErr,256)); 
	 	  }
	 	  CloseServiceHandle(schSCManager); 
	 }
	 else
	 {
	 	  printf("OpenSCManager failed - %s\n", GetLastErrorText(szErr,256)); 
	 }
}

/*--------------------------------------------------------------------------*/
void SLPDCmdStopService(void)
/*--------------------------------------------------------------------------*/
{
	 SC_HANDLE schService; 
	 SC_HANDLE schSCManager; 

	 schSCManager = OpenSCManager(
	 									  NULL,                   /* machine (NULL == local)    */
	 									  NULL,                   /* database (NULL == default) */
	 									  SC_MANAGER_ALL_ACCESS); /* access required    		  */
	 if(schSCManager)
	 {
	 	  schService = OpenService(schSCManager, G_SERVICENAME, SERVICE_ALL_ACCESS); 

	 	  if(schService)
	 	  {
	 			SLPDHlpStopService(schService);
	 			CloseServiceHandle(schService); 
	 	  }
	 	  else
	 	  {
	 			printf("OpenService failed - %s\n", GetLastErrorText(szErr,256)); 
	 	  }
	 	  CloseServiceHandle(schSCManager); 
	 }
	 else
	 {
	 	  printf("OpenSCManager failed - %s\n", GetLastErrorText(szErr,256)); 
	 }
}

/*--------------------------------------------------------------------------*/
void SLPDCmdDebugService(int argc, char ** argv) 
/*--------------------------------------------------------------------------*/
{
    printf("Debugging %s.\n", G_SERVICEDISPLAYNAME); 

    SetConsoleCtrlHandler( ControlHandler, TRUE ); 
    ServiceStart( argc, argv ); 
}

/*==========================================================================*/
void __cdecl main(int argc, char **argv) 
/*==========================================================================*/
{
    SERVICE_TABLE_ENTRY dispatchTable[] = 
    { 
        { G_SERVICENAME, (LPSERVICE_MAIN_FUNCTION)SLPDServiceMain}, 
        { NULL, NULL} 
    }; 

    /*------------------------*/
    /* Parse the command line */
    /*------------------------*/
    if(SLPDParseCommandLine(argc,argv))
    {
        SLPDFatal("Invalid command line\n");
    }

    switch(G_SlpdCommandLine.action)
    {
    case SLPD_DEBUG:
        SLPDCmdDebugService(argc, argv);
        break;
    case SLPD_INSTALL:
        SLPDCmdInstallService();
        break;
    case SLPD_REMOVE:
        SLPDCmdRemoveService();
        break;
    case SLPD_START:
        SLPDCmdStartService();
		  break;
    case SLPD_STOP:
        SLPDCmdStopService();
        break;
    default:
        SLPDPrintUsage();
        StartServiceCtrlDispatcher(dispatchTable);

        break;
    } 
} 




