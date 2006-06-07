/*-------------------------------------------------------------------------
 * Copyright (C) 2000 Caldera Systems, Inc
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *    Neither the name of Caldera Systems nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * `AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE CALDERA
 * SYSTEMS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *-------------------------------------------------------------------------*/

/** Win32 service code.
 *
 * @file       slpd_win32.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    SlpdCode
 */

#include "slp_types.h"

#include "slpd_cmdline.h"
#include "slpd_log.h"
#include "slpd_property.h"
#include "slpd_database.h"
#include "slpd_socket.h"
#include "slpd_incoming.h"
#include "slpd_outgoing.h"
#include "slpd_knownda.h"
#include "slpd.h"

#include "slp_linkedlist.h"
#include "slp_xid.h"

/*  internal and display names of the service  */
#define G_SERVICENAME         "slpd"    
#define G_SERVICEDISPLAYNAME  "Service Location Protocol"

static SERVICE_STATUS ssStatus;       /* current status of the service  */
static SERVICE_STATUS_HANDLE sshStatusHandle; 
static BOOL bDebug = FALSE; 
static TCHAR szErr[256];

/* externals (variables) from slpd_main.c */
extern int G_SIGALRM;
extern int G_SIGTERM;
extern int G_SIGHUP;
extern int G_SIGINT;

/* externals (functions) from slpd_main.c */
void LoadFdSets(SLPList * socklist, int * highfd, 
      fd_set * readfds, fd_set * writefds);
void HandleSigTerm(void);
void HandleSigAlrm(void);

/** Reports the current status of the service to the SCM.
 *
 * @param[in] dwCurrentState - The state of the service.
 * @param[in] dwWin32ExitCode - The error code to report.
 * @param[in] dwWaitHint - Worst case estimate to next checkpoint.
 *
 * @return A boolean value; TRUE on success, FALSE on failure.
 *
 * @internal
 */
static BOOL ReportStatusToSCMgr(DWORD dwCurrentState, 
      DWORD dwWin32ExitCode, DWORD dwWaitHint) 
{
   static DWORD dwCheckPoint = 1; 
   BOOL fResult = TRUE; 

   /* when debugging we don't report to the SCM */
   if (G_SlpdCommandLine.action != SLPD_DEBUG)
   {
      if (dwCurrentState == SERVICE_START_PENDING)
         ssStatus.dwControlsAccepted = 0;
      else
         ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP; 

      ssStatus.dwCurrentState = dwCurrentState; 
      ssStatus.dwWin32ExitCode = dwWin32ExitCode; 
      ssStatus.dwWaitHint = dwWaitHint; 

      if (dwCurrentState == SERVICE_RUNNING 
            || dwCurrentState == SERVICE_STOPPED)
         ssStatus.dwCheckPoint = 0;
      else
         ssStatus.dwCheckPoint = dwCheckPoint++; 

      /* report the status of the service to the service control manager.*/
      if ((fResult = SetServiceStatus( sshStatusHandle, &ssStatus)) == 0)
         SLPDLog("SetServiceStatus failed"); 
   }
   return fResult; 
} 

/** Copies error message text to a string.
 *
 * @param[out] lpszBuf - A destination buffer.
 * @param[in] dwSize - The size of @p lpszBuf in bytes.
 *
 * @return A pointer to the destination buffer (for convenience). 
 *
 * @internal
 */
static LPTSTR GetLastErrorText(LPTSTR lpszBuf, DWORD dwSize) 
{
   DWORD dwRet; 
   LPTSTR lpszTemp = 0;

   dwRet = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER 
         | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY, 
         0, GetLastError(), LANG_NEUTRAL, (LPTSTR)&lpszTemp, 0, 0); 

   /* supplied buffer is not long enough */
   if (!dwRet || (long)dwSize < (long)dwRet + 14)
      lpszBuf[0] = 0;
   else
   {
      lpszTemp[lstrlen(lpszTemp)-2] = 0;
      sprintf(lpszBuf, "%s (0x%x)", lpszTemp, GetLastError()); 
   } 

   if (lpszTemp)
      LocalFree((HLOCAL)lpszTemp );

   return lpszBuf; 
} 

/** Signal the service to stop, and then report it.
 */
static void ServiceStop(void) 
{
   G_SIGTERM = 1;
   ReportStatusToSCMgr(SERVICE_STOPPED, NO_ERROR, 3000);
} 

/** Start the service and report it.
 *
 * @param[in] argc - The number of arguments in @p argv.
 * @param[in] argv - An array of argument string pointers.
 *
 * @internal
 */
static void ServiceStart(int argc, char ** argv)
{
   fd_set readfds;
   fd_set writefds;
   int highfd;
   int fdcount = 0;
   time_t curtime;
   time_t alarmtime;
   struct timeval timeout;
   WSADATA wsaData; 
   WORD wVersionRequested = MAKEWORD(1, 1); 

   /* service initialization */
   if (!ReportStatusToSCMgr(SERVICE_START_PENDING, NO_ERROR, 3000))
      return;

   if (WSAStartup(wVersionRequested, &wsaData) != 0)
   {
      ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, 0); 
      return;
   }

   /* parse the command line */
   if (SLPDParseCommandLine(argc, argv))
   {
      ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, 0);
      goto cleanup_winsock;
   }

   if (!ReportStatusToSCMgr(SERVICE_START_PENDING, NO_ERROR, 3000))
      goto cleanup_winsock;

   /* initialize the log file */
   if (SLPDLogFileOpen(G_SlpdCommandLine.logfile, 1))
   {
      SLPDLog("Could not open logfile %s\n",G_SlpdCommandLine.logfile);
      goto cleanup_winsock;
   }

   /* seed the XID generator */
   SLPXidSeed();

   /* log startup message */
   SLPDLog("****************************************\n");
   SLPDLogTime();
   SLPDLog("SLPD daemon started\n");
   SLPDLog("****************************************\n");
   SLPDLog("Command line = %s\n",argv[0]);
   SLPDLog("Using configuration file = %s\n",G_SlpdCommandLine.cfgfile);
   SLPDLog("Using registration file = %s\n",G_SlpdCommandLine.regfile);
   if (!ReportStatusToSCMgr(SERVICE_START_PENDING, NO_ERROR, 3000))
      goto cleanup_winsock;

   /* initialize for the first time */
   if (SLPDPropertyInit(G_SlpdCommandLine.cfgfile) 
         || SLPDDatabaseInit(G_SlpdCommandLine.regfile) 
         || SLPDIncomingInit() 
         || SLPDOutgoingInit() 
         || SLPDKnownDAInit())
   {
      SLPDLog("slpd initialization failed\n");
      goto cleanup_winsock;
   }
   SLPDLog("Agent Interfaces = %s\n", G_SlpdProperty.interfaces);

   /* service is now running, perform work until shutdown    */
   if (!ReportStatusToSCMgr(SERVICE_RUNNING, NO_ERROR, 0))
      goto cleanup_winsock;

   /* main loop */
   SLPDLog("Startup complete entering main run loop ...\n\n");
   G_SIGTERM   = 0;
   curtime = time(&alarmtime);
   alarmtime = curtime + 2;  /*Start with a shorter time so SAs register with us quickly on startup*/
   while (G_SIGTERM == 0)
   {
      /* load the fdsets up with all valid sockets in the list  */
      highfd = 0;
      FD_ZERO(&readfds);
      FD_ZERO(&writefds);
      LoadFdSets(&G_IncomingSocketList, &highfd, &readfds, &writefds);
      LoadFdSets(&G_OutgoingSocketList, &highfd, &readfds, &writefds);

      /* Before select(), check to see if we got a signal */
      if (G_SIGALRM)
         goto HANDLE_SIGNAL;

      /* main select */
      timeout.tv_sec = SLPD_AGE_INTERVAL;
      timeout.tv_usec = 0;
      fdcount = select(highfd + 1, &readfds, &writefds, 0, &timeout);
      if (fdcount > 0) /* fdcount will be < 0 when timed out */
      {
         SLPDIncomingHandler(&fdcount, &readfds, &writefds);
         SLPDOutgoingHandler(&fdcount, &readfds, &writefds);
      }

      /* handle signals */
      HANDLE_SIGNAL:
      curtime = time(&curtime);
      if (curtime >= alarmtime)
      {
         HandleSigAlrm();
         alarmtime = curtime + SLPD_AGE_INTERVAL;
      }
   }

   /* Got SIGTERM */
   HandleSigTerm();

cleanup_winsock:

   WSACleanup();     
} 

/** Handles console control events.
 *
 * @param[in] dwCtrlType - The type of control event.
 *
 * @return A boolean value; TRUE if the event was handled, FALSE if not.
 *
 * @internal
 */
static BOOL WINAPI ControlHandler(DWORD dwCtrlType) 
{
   switch(dwCtrlType)
   {
      case CTRL_BREAK_EVENT:  /* use Ctrl+C or Ctrl+Break to simulate */
      case CTRL_C_EVENT:      /* SERVICE_CONTROL_STOP in debug mode */
         printf("Stopping %s.\n", G_SERVICEDISPLAYNAME); 
         ServiceStop(); 
         return TRUE; 
   } 
   return FALSE; 
} 

/** Called by the SCM whenever ControlService is called on this service.
 *
 * @param[in] dwCtrlCode - The type of control requested.
 *
 * @internal
 */
static VOID WINAPI ServiceCtrl(DWORD dwCtrlCode) 
{
   switch(dwCtrlCode)
   {
      case SERVICE_CONTROL_STOP: 
         ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, 0); 
         ServiceStop(); 
         return; 

      case SERVICE_CONTROL_INTERROGATE: 
         break; 

      default: 
         break; 
   } 
   ReportStatusToSCMgr(ssStatus.dwCurrentState, NO_ERROR, 0); 
} 

/** Win32 service main entry point.
 *
 * @param[in] argc - The number of arguments in @p argv.
 *
 * @param[in] argv - An array of argument string pointers.
 *
 * @internal
 */
static void WINAPI SLPDServiceMain(DWORD argc, LPTSTR * argv) 
{
   sshStatusHandle = RegisterServiceCtrlHandler(G_SERVICENAME, ServiceCtrl);      

   if (sshStatusHandle != 0)
   {
      /* SERVICE_STATUS members that don't change */
      ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS; 
      ssStatus.dwServiceSpecificExitCode = 0; 

      /* report the status to the service control manager.    */
      if (ReportStatusToSCMgr(SERVICE_START_PENDING, NO_ERROR, 3000))
         ServiceStart(argc, argv); 
   }

   /* try to report the stopped status to the service control manager. */
   if (sshStatusHandle)
      ReportStatusToSCMgr(SERVICE_STOPPED, 0, 0);
} 

/** Install the service.
 *
 * @param[in] automatic - A flag indicating whether or not the service
 *    should be installed to start automatically at boot time.
 *
 * @internal
 */
static void SLPDCmdInstallService(int automatic) 
{
   SC_HANDLE schService; 
   SC_HANDLE schSCManager; 

   DWORD start_type;
   TCHAR szPath[512]; 

   if (GetModuleFileName(0, szPath, 512) == 0)
   {
      printf("Unable to install %s - %s\n", G_SERVICEDISPLAYNAME, 
            GetLastErrorText(szErr, 256));
      return; 
   }

   if (automatic)
      start_type = SERVICE_AUTO_START;
   else
      start_type = SERVICE_DEMAND_START;

   schSCManager = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
   if (schSCManager)
   {
      schService = CreateService(schSCManager, G_SERVICENAME, 
            G_SERVICEDISPLAYNAME, SERVICE_ALL_ACCESS, 
            SERVICE_WIN32_OWN_PROCESS, start_type, 
            SERVICE_ERROR_NORMAL, szPath, 0, 0, "", 0, 0);
      if (schService)
      {
         printf("%s installed.\n", G_SERVICEDISPLAYNAME ); 
         CloseServiceHandle(schService); 
      }
      else
         printf("CreateService failed - %s\n", GetLastErrorText(szErr, 256)); 

      CloseServiceHandle(schSCManager); 
   }
   else
      printf("OpenSCManager failed - %s\n", GetLastErrorText(szErr, 256)); 
} 

/** Stop a service by service handle.
 *
 * @param[in] schService - A handle to the service to be stopped.
 *
 * @internal
 */
static void SLPDHlpStopService(SC_HANDLE schService)
{
   /* try to stop the service */
   if (ControlService(schService, SERVICE_CONTROL_STOP, &ssStatus))
   {
      printf("Stopping %s.", G_SERVICEDISPLAYNAME);
      Sleep(1000);

      while (QueryServiceStatus(schService, &ssStatus))
      {
         if (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
         {
            printf(".");
            Sleep(1000);
         }
         else
            break; 
      } 
      if (ssStatus.dwCurrentState == SERVICE_STOPPED)
         printf("\n%s stopped.\n", G_SERVICEDISPLAYNAME);
      else
         printf("\n%s failed to stop.\n", G_SERVICEDISPLAYNAME); 
   }
}
 
/** Uninstall the service.
 *
 * @internal
 */
static void SLPDCmdRemoveService(void)
{
   SC_HANDLE schService; 
   SC_HANDLE schSCManager; 

   schSCManager = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
   if (schSCManager)
   {
      schService = OpenService(schSCManager, G_SERVICENAME, SERVICE_ALL_ACCESS); 
      if (schService)
      {
         SLPDHlpStopService(schService);

         /* now remove the service    */
         if (DeleteService(schService))
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

/** Start the service.
 *
 * @internal
 */
void SLPDCmdStartService(void)
{
   SC_HANDLE schService; 
   SC_HANDLE schSCManager; 

   schSCManager = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
   if (schSCManager)
   {
      schService = OpenService(schSCManager, G_SERVICENAME, SERVICE_ALL_ACCESS); 
      if (schService)
      {
         if (!StartService(schService, 0, 0))
            printf("OpenService failed - %s\n", GetLastErrorText(szErr,256)); 
         CloseServiceHandle(schService); 
      }
      else
         printf("OpenService failed - %s\n", GetLastErrorText(szErr,256)); 
      CloseServiceHandle(schSCManager); 
   }
   else
      printf("OpenSCManager failed - %s\n", GetLastErrorText(szErr,256)); 
}

/** Stop the service.
 *
 * @internal
 */
static void SLPDCmdStopService(void)
{
   SC_HANDLE schService; 
   SC_HANDLE schSCManager; 

   schSCManager = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
   if (schSCManager)
   {
      schService = OpenService(schSCManager, G_SERVICENAME, SERVICE_ALL_ACCESS); 
      if (schService)
      {
         SLPDHlpStopService(schService);
         CloseServiceHandle(schService); 
      }
      else
         printf("OpenService failed - %s\n", GetLastErrorText(szErr,256)); 
      CloseServiceHandle(schSCManager); 
   }
   else
      printf("OpenSCManager failed - %s\n", GetLastErrorText(szErr,256)); 
}

/** Debug the service.
 *
 * @param[in] argc - The number of arguments in @p argv.
 *
 * @param[in] argv - An array of argument string pointers.
 *
 * @internal
 */
static void SLPDCmdDebugService(int argc, char ** argv) 
{
   printf("Debugging %s.\n", G_SERVICEDISPLAYNAME); 

   SetConsoleCtrlHandler(ControlHandler, TRUE); 
   ServiceStart(argc, argv); 
}

/** The main program entry point (when not executed as a service). 
 *
 * @param[in] argc - The number of arguments in @p argv.
 *
 * @param[in] argv - An array of argument string pointers.
 */
void __cdecl main(int argc, char ** argv) 
{
   SERVICE_TABLE_ENTRY dispatchTable[] = 
   { 
      {G_SERVICENAME, (LPSERVICE_MAIN_FUNCTION)SLPDServiceMain}, 
      {0, 0} 
   }; 

   /* parse the command line */
   if (SLPDParseCommandLine(argc, argv))
      SLPDFatal("Invalid command line\n");

   switch (G_SlpdCommandLine.action)
   {
      case SLPD_DEBUG:
         SLPDCmdDebugService(argc, argv);
         break;
      case SLPD_INSTALL:
         SLPDCmdInstallService(G_SlpdCommandLine.autostart);
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

/*=========================================================================*/
