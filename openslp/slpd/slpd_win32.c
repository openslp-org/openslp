/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_win32.c                                             */
/*                                                                         */
/* Abstract:    Win32 specific part, to make SLPD run as a "service"       */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/* Copyright (c) 1995, 1999  Caldera Systems, Inc.                         */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify it */
/* under the terms of the GNU Lesser General Public License as published   */
/* by the Free Software Foundation; either version 2.1 of the License, or  */
/* (at your option) any later version.                                     */
/*                                                                         */
/*     This program is distributed in the hope that it will be useful,     */
/*     but WITHOUT ANY WARRANTY; without even the implied warranty of      */
/*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       */
/*     GNU Lesser General Public License for more details.                 */
/*                                                                         */
/*     You should have received a copy of the GNU Lesser General Public    */
/*     License along with this program; see the file COPYING.  If not,     */
/*     please obtain a copy from http://www.gnu.org/copyleft/lesser.html   */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/*     Please submit patches to http://www.openslp.org                     */
/*                                                                         */
/***************************************************************************/

#include "slpd.h"

SERVICE_STATUS          ssStatus;       /* current status of the service  */
SERVICE_STATUS_HANDLE   sshStatusHandle; 
BOOL                    bDebug = FALSE; 
TCHAR                   szErr[256];


void LoadFdSets(SLPList* list, 
                int* highfd, 
                fd_set* readfds, 
                fd_set* writefds);





/* That function is called when the "Database Age" timer wakes up          */
/*-------------------------------------------------------------------------*/
VOID CALLBACK TimerHandler( UINT Timer,      
                            UINT Reserved,     
                            DWORD Arg,  
                            DWORD DontUse1,     
                            DWORD DontUse2      
                            )
/*-------------------------------------------------------------------------*/
{
  G_SIGALRM = 1;
}

 


/*                                                                          */
/*  FUNCTION: ReportStatusToSCMgr()                                         */
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
BOOL ReportStatusToSCMgr(DWORD dwCurrentState, 
                         DWORD dwWin32ExitCode, 
                         DWORD dwWaitHint) 
/*--------------------------------------------------------------------------*/
{ 
  static DWORD dwCheckPoint = 1; 
  BOOL fResult = TRUE; 
 
 
  /* when debugging we don't report to the SCM    */
  if ( G_SlpdCommandLine.action != SLPD_DEBUG ) 
  { 
    if (dwCurrentState == SERVICE_START_PENDING) 
      ssStatus.dwControlsAccepted = 0; 
    else 
      ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP; 
 
    ssStatus.dwCurrentState = dwCurrentState; 
    ssStatus.dwWin32ExitCode = dwWin32ExitCode; 
    ssStatus.dwWaitHint = dwWaitHint; 
 
    if ( ( dwCurrentState == SERVICE_RUNNING ) || 
         ( dwCurrentState == SERVICE_STOPPED ) ) 
      ssStatus.dwCheckPoint = 0; 
    else 
      ssStatus.dwCheckPoint = dwCheckPoint++; 
 
 
    /* Report the status of the service to the service control manager.    */
        
    if (!(fResult = SetServiceStatus( sshStatusHandle, &ssStatus))) { 
      SLPLog("SetServiceStatus failed"); 
    } 
  } 
  return fResult; 
} 
 
/*                                                                          */
/*  FUNCTION: GetLastErrorText                                              */
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
LPTSTR GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize ) 
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
  if ( !dwRet || ( (long)dwSize < (long)dwRet+14 ) ) 
    lpszBuf[0] = TEXT('\0'); 
  else 
  { 
    lpszTemp[lstrlen(lpszTemp)-2] = TEXT('\0');  
    sprintf( lpszBuf, "%s (0x%x)", lpszTemp, GetLastError() ); 
  } 
 
  if ( lpszTemp ) 
    LocalFree((HLOCAL) lpszTemp ); 
 
  return lpszBuf; 
} 


/*--------------------------------------------------------------------------*/
void ServiceStop() 
/*--------------------------------------------------------------------------*/
{ 
  G_SIGTERM = 1;
  ReportStatusToSCMgr( 
    SERVICE_STOPPED,       /* service state    */
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
  UINT            AgeTimer;
  WSADATA wsaData; 
  WORD    wVersionRequested = MAKEWORD(1,1); 
  
  /* Service initialization    */
  
 
  /* report the status to the service control manager.    */
  
  if (!ReportStatusToSCMgr(SERVICE_START_PENDING, /* service state    */
                           NO_ERROR,              /* exit code    */
                           3000))                 /* wait hint    */
    goto cleanup; 
 
 
 
  /*------------------------*/
  /* Parse the command line */
  /*------------------------*/
  if(SLPDParseCommandLine(argc,argv))
  {
    ReportStatusToSCMgr(SERVICE_STOP_PENDING, /* service state    */
                        NO_ERROR,              /* exit code    */
                        0);                 /* wait hint    */
  goto cleanup;
  }

  if (WSAStartup(wVersionRequested, &wsaData) != 0)
  {
    (void)ReportStatusToSCMgr(SERVICE_STOP_PENDING, 
                              NO_ERROR, 
                              0); 
    goto cleanup;
  }

  /* report the status to the service control manager.    */
  
  if (!ReportStatusToSCMgr( 
    SERVICE_START_PENDING, /* service state    */
    NO_ERROR,              /* exit code    */
    3000))                 /* wait hint    */
    goto cleanup_winsock; 

  
  /*------------------------------*/
  /* Initialize the log file      */
  /*------------------------------*/
  SLPLogFileOpen(G_SlpdCommandLine.logfile, 0);

  SLPLog("****************************************\n");
  SLPLog("*** SLPD daemon started              ***\n");
  SLPLog("****************************************\n");
  SLPLog("command line = %s\n",argv[0]);

  /* report the status to the service control manager.    */
  
  if (!ReportStatusToSCMgr( 
    SERVICE_START_PENDING, /* service state    */
    NO_ERROR,              /* exit code    */
    3000))                 /* wait hint    */
    goto cleanup_winsock; 
    
  /*--------------------------------------------------*/
  /* Initialize for the first timestamp                    */
  /*--------------------------------------------------*/
  SLPDPropertyInit(G_SlpdCommandLine.cfgfile);
  SLPDDatabaseInit(G_SlpdCommandLine.regfile);
  SLPDIncomingInit();
  SLPDOutgoingInit();
  SLPDKnownDAInit();

  timeBeginPeriod(SLPD_AGE_INTERVAL * 1000);
  AgeTimer = timeSetEvent(SLPD_AGE_INTERVAL * 1000,
                          100, /* accuracy of 100 miilliseconds is enough */
                          TimerHandler, /* callback */
                          0, /* arg of the callback */
                          TIME_PERIODIC);
  
 

  
  /* End of initialization    */ 
  
  
  /* Service is now running, perform work until shutdown    */
  
    
  /* report the status to the service control manager.    */
  
    
  if (!ReportStatusToSCMgr( 
    SERVICE_RUNNING,       /* service state    */
    NO_ERROR,              /* exit code    */
    0))                    /* wait hint    */
    goto cleanup_winsock; 
 
  /*-----------*/
  /* Main loop */
  /*-----------*/
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
        
    /*-------------*/
    /* Main select */
    /*-------------*/
    fdcount = select(highfd+1,&readfds,&writefds,0,0);
    if(fdcount > 0) /* fdcount will be < 0 when interrupted by a signal */
    {
      SLPDIncomingHandler(&fdcount,&readfds,&writefds);
      SLPDOutgoingHandler(&fdcount,&readfds,&writefds);
    }

    /*----------------------------------------------------------*/
    /* Before select(), check to see if we should reinitialize  */
    /*----------------------------------------------------------*/
    if(G_SIGHUP)
    {
      /* Reinitialize */
      SLPLog("****************************************\n");
      SLPLog("*** SLPD daemon restarted            ***\n");
      SLPLog("****************************************\n");
      SLPLog("Got SIGHUP reinitializing... \n");
        
      SLPDPropertyInit(G_SlpdCommandLine.cfgfile);
      SLPDDatabaseInit(G_SlpdCommandLine.regfile);
      SLPDOutgoingInit(); 
      SLPDKnownDAInit();
      G_SIGHUP = 0;
            
      /* continue to top of loop so that fd_sets are loaded again */
      continue; 
    }

    /*--------------------------------------------------------------*/
    /* Before select(), check to see if we should age the database  */
    /*--------------------------------------------------------------*/
    if(G_SIGALRM)
    {
      /* TODO: add call to do passive DAAdvert */
      SLPDIncomingAge(SLPD_AGE_INTERVAL);
      SLPDOutgoingAge(SLPD_AGE_INTERVAL);
      SLPDDatabaseAge(SLPD_AGE_INTERVAL);
      SLPDKnownDAActiveDiscovery();
      G_SIGALRM = 0;
      /* continue to top of loop so that fd_sets are loaded again */
      continue;
    }
        


  }

  timeEndPeriod(SLPD_AGE_INTERVAL * 1000);
  timeKillEvent(AgeTimer);

  SLPLog("Going down\n");

  cleanup_winsock:
  
  WSACleanup();

  cleanup: 
  ;
  
  
} 
 
/*                                                                          */
/*  FUNCTION: service_ctrl                                                  */
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
/*--------------------------------------------------------------------------*/
VOID WINAPI ServiceCtrl(DWORD dwCtrlCode) 
/*--------------------------------------------------------------------------*/
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

 
/*--------------------------------------------------------------------------*/
void WINAPI SLPDServiceMain(DWORD argc, LPTSTR *argv) 
/*--------------------------------------------------------------------------*/
{ 

  /* register our service control handler:    */
  sshStatusHandle = RegisterServiceCtrlHandler( G_SERVICENAME, ServiceCtrl); 
 
  if (sshStatusHandle != 0)
  {
    /* SERVICE_STATUS members that don't change    */
    ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS; 
    ssStatus.dwServiceSpecificExitCode = 0; 
 
 
    /* report the status to the service control manager.    */
    if (ReportStatusToSCMgr(SERVICE_START_PENDING, /* service state    */
                            NO_ERROR,              /* exit code    */
                            3000))                 /* wait hint    */
    {
      ServiceStart(argc, argv); 
    }
  }

    
  /* try to report the stopped status to the service control manager.    */
  
  if (sshStatusHandle) 
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
 
  if ( GetModuleFileName( NULL, szPath, 512 ) == 0 ) 
  { 
    printf("Unable to install %s - %s\n", 
           G_SERVICEDISPLAYNAME, 
           GetLastErrorText(szErr, 256)); 
    return; 
  } 
 
  schSCManager = OpenSCManager( 
    NULL,                   /* machine (NULL == local)    */
    NULL,                   /* database (NULL == default)    */
    SC_MANAGER_ALL_ACCESS   /* access required    */
    ); 
  if ( schSCManager ) 
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
 
    if ( schService ) 
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
void SLPDCmdRemoveService() 
/*--------------------------------------------------------------------------*/
{ 
  SC_HANDLE   schService; 
  SC_HANDLE   schSCManager; 
 
  schSCManager = OpenSCManager( 
    NULL,                   /* machine (NULL == local)    */
    NULL,                   /* database (NULL == default)    */
    SC_MANAGER_ALL_ACCESS   /* access required    */
    ); 
  if ( schSCManager ) 
  { 
    schService = OpenService(schSCManager, G_SERVICENAME, SERVICE_ALL_ACCESS); 
 
    if (schService) 
    { 
      /* try to stop the service    */
      if ( ControlService( schService, SERVICE_CONTROL_STOP, &ssStatus ) ) 
      { 
        printf("Stopping %s.", G_SERVICEDISPLAYNAME); 
        Sleep( 1000 ); 
 
        while( QueryServiceStatus( schService, &ssStatus ) ) 
        { 
          if ( ssStatus.dwCurrentState == SERVICE_STOP_PENDING ) 
          { 
            printf("."); 
            Sleep( 1000 ); 
          } 
          else 
            break; 
        } 
 
        if ( ssStatus.dwCurrentState == SERVICE_STOPPED ) 
          printf("\n%s stopped.\n", G_SERVICEDISPLAYNAME ); 
        else 
          printf("\n%s failed to stop.\n", G_SERVICEDISPLAYNAME ); 
 
      } 
 
      /* now remove the service    */
      if( DeleteService(schService) ) 
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
 
 
 
 
 
/*                                                                          */
/*  FUNCTION: ControlHandler ( DWORD dwCtrlType )                           */
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
/*--------------------------------------------------------------------------*/
BOOL WINAPI ControlHandler ( DWORD dwCtrlType ) 
/*--------------------------------------------------------------------------*/
{ 
  switch( dwCtrlType ) 
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


/*--------------------------------------------------------------------------*/
void SLPDCmdDebugService(int argc, char ** argv) 
/*--------------------------------------------------------------------------*/
{ 
 
  printf("Debugging %s.\n", G_SERVICEDISPLAYNAME); 
 
  SetConsoleCtrlHandler( ControlHandler, TRUE ); 
  ServiceStart( argc, argv ); 
} 
 

 
 
