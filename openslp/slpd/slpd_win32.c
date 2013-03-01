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
void LoadFdSets(SLPList * socklist, SLPD_fdset * fdset);
void HandleSigTerm(void);
void HandleSigAlrm(void);
void HandleSigHup(void);

/* Callback parameter type redefined here as some versions of the Windows SDK
   do not specify a calling convention for the callback type; the compiler
   will typically default to assuming CDECL, but the function must really be
   STDCALL.  That way lies crashes. */
typedef VOID (WINAPI *PSLP_IPINTERFACE_CHANGE_CALLBACK)(IN PVOID CallerContext,
                                                        IN PMIB_IPINTERFACE_ROW Row OPTIONAL,
                                                        IN MIB_NOTIFICATION_TYPE NotificationType);

typedef NETIO_STATUS (WINAPI *PNotifyIpInterfaceChange)(IN ADDRESS_FAMILY Family,
                                                        IN PSLP_IPINTERFACE_CHANGE_CALLBACK Callback,
                                                        IN PVOID CallerContext,
                                                        IN BOOLEAN InitialNotification,
                                                        IN OUT HANDLE *NotificationHandle);
typedef NETIO_STATUS (WINAPI *PCancelMibChangeNotify2)(IN HANDLE NotificationHandle);

/* interface address monitoring data (encapsulated to provide a bit of abstraction) */
struct interface_monitor
{
   HANDLE hAddrChange;
   /* WinXP data (IPv4 only) */
   OVERLAPPED addrChange;
   /* WinVista data (IPv4+IPv6) */
   HMODULE hNetIoLib;
   PNotifyIpInterfaceChange pNotifyIpInterfaceChange;
   PCancelMibChangeNotify2 pCancelMibChangeNotify2;
   BOOL addrChanged;
};

/** Callback for Vista+ IPv4/IPv6 interface change monitoring.
 *
 * @internal
 */
static VOID CALLBACK InterfaceMonitorCallback(IN PVOID CallerContext, IN PMIB_IPINTERFACE_ROW Row OPTIONAL, IN MIB_NOTIFICATION_TYPE NotificationType)
{
   struct interface_monitor *self = (struct interface_monitor *) CallerContext;
   Row;  /* unused */
   NotificationType;  /* unused */

   self->addrChanged = TRUE;
}

/** Initializes the interface monitoring.
 *
 * @param[in] self - An uninitialized struct interface_monitor.
 *
 * @internal
 */
static void InterfaceMonitorInit(struct interface_monitor *self)
{
   self->hAddrChange = NULL;
   memset(&self->addrChange, 0, sizeof(self->addrChange));

   /* try to load the Vista+ IPv6 functions */
   self->hNetIoLib = LoadLibraryA("iphlpapi");
   if (self->hNetIoLib)
   {
      self->pNotifyIpInterfaceChange = (PNotifyIpInterfaceChange) GetProcAddress(self->hNetIoLib, "NotifyIpInterfaceChange");
      self->pCancelMibChangeNotify2 = (PCancelMibChangeNotify2) GetProcAddress(self->hNetIoLib, "CancelMibChangeNotify2");
      if (!(self->pNotifyIpInterfaceChange && self->pCancelMibChangeNotify2))
      {
         FreeLibrary(self->hNetIoLib);
         self->hNetIoLib = NULL;
      }
      self->addrChanged = FALSE;
   }

   /* register for IP change notifications */
   if (self->hNetIoLib)
   {
      /* Note that the cast is required here as some versions of the Windows SDK do not specify a calling
         convention for the callback type; the compiler will typically default to assuming CDECL, but the
         function must really be STDCALL.  Without the cast we get a compiler error when that happens. */
      if (self->pNotifyIpInterfaceChange(AF_UNSPEC, &InterfaceMonitorCallback, self, FALSE, &self->hAddrChange))
      {
         SLPDLog("Error watching for IPv4/IPv6 interface changes; continuing anyway...\n");
      }
   }
   else
   {
      if (NotifyAddrChange(&self->hAddrChange, &self->addrChange) != ERROR_IO_PENDING)
      {
         SLPDLog("Error watching for IPv4 interface changes, continuing anyway...\n");
      }
   }
}

/** Deinitializes the interface monitoring.
 *
 * @param[in] self - A struct interface_monitor.
 *
 * @internal
 */
static void InterfaceMonitorDeinit(struct interface_monitor *self)
{
   if (self->hNetIoLib)
   {
      /* Vista+ */
      if (self->hAddrChange)
      {
         self->pCancelMibChangeNotify2(self->hAddrChange);
         self->hAddrChange = NULL;
         FreeLibrary(self->hNetIoLib);
         self->hNetIoLib = NULL;
      }
   }
   else
   {
      /* XP */
      if (self->hAddrChange)
      {
         CancelIPChangeNotify(&self->addrChange);
         self->hAddrChange = NULL;
      }
   }
}

/** Reports whether interfaces have been changed.
 *
 * @param[in] self - An initialized struct interface_monitor.
 *
 * @return A boolean value; TRUE if interfaces have changed, FALSE otherwise.
 *
 * @internal
 */
static BOOL InterfacesChanged(struct interface_monitor *self)
{
   if (self->hNetIoLib)
   {
      /* Vista+ */
      if (self->addrChanged)
      {
         self->addrChanged = FALSE;
         return TRUE;
      }
      return FALSE;
   }
   else
   {
      /* XP */
      if (self->hAddrChange)
      {
         DWORD dwResult;
         if (GetOverlappedResult(self->hAddrChange, &self->addrChange, &dwResult, FALSE))
         {
            /* start watching again for further interface changes */
            NotifyAddrChange(&self->hAddrChange, &self->addrChange);
            return TRUE;
         }
      }
   }
   return FALSE;
}

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
         ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP
                                     | SERVICE_ACCEPT_PARAMCHANGE;

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
         SLPDLog("SetServiceStatus failed\n");
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
      sprintf(lpszBuf, "%s (0x%lx)", lpszTemp, GetLastError());
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
   ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, 3000);
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
   SLPD_fdset fdset;
   int fdcount = 0;
   time_t curtime;
   time_t alarmtime;
#if !HAVE_POLL
   struct timeval timeout;
#endif
   WSADATA wsaData;
   WORD wVersionRequested = MAKEWORD(1, 1);
   struct interface_monitor monitor;

   SLPD_fdset_init(&fdset);

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

   /* Initialize the preferences so we know if the log file is to be
      overwritten or appended.*/
   if (SLPDPropertyInit(G_SlpdCommandLine.cfgfile))
   {
      fprintf(stderr, "slpd initialization failed during property load\n");
      goto cleanup_winsock;
   }

   /* initialize the log file */
   if (SLPDLogFileOpen(G_SlpdCommandLine.logfile, G_SlpdProperty.appendLog))
   {
      fprintf(stderr, "Could not open logfile %s\n",G_SlpdCommandLine.logfile);
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

   /* start watching for address changes *before* we initialise, to minimise races */
   InterfaceMonitorInit(&monitor);

   /* initialize for the first time */
   SLPDPropertyReinit();  /*So we get any property-related log messages*/
   if (SLPDDatabaseInit(G_SlpdCommandLine.regfile)
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
   G_SIGHUP = 0;
   time(&curtime);
   alarmtime = curtime + 2;  /*Start with a shorter time so SAs register with us quickly on startup*/
   while (G_SIGTERM == 0)
   {
      /* load the fdsets up with all valid sockets in the list  */
      SLPD_fdset_reset(&fdset);
      LoadFdSets(&G_IncomingSocketList, &fdset);
      LoadFdSets(&G_OutgoingSocketList, &fdset);

      /* Before select(), check to see if we got a signal */
      if (G_SIGALRM)
         goto HANDLE_SIGNAL;

      /* main select -- we time out every second so the outgoing retries can occur*/
      time(&curtime);
#if HAVE_POLL
      fdcount = poll(fdset.fds, fdset.used, 1000);
#else
      timeout.tv_sec = 1;
      timeout.tv_usec = 0;
      fdcount = select((int)fdset.highfd + 1, &fdset.readfds, &fdset.writefds, 0, &timeout);
#endif
      if (fdcount > 0)
      {
         SLPDIncomingHandler(&fdcount, &fdset);
         SLPDOutgoingHandler(&fdcount, &fdset);
         SLPDOutgoingRetry(time(0) - curtime);
      }
      else if (fdcount == 0)
         SLPDOutgoingRetry(time(0) - curtime);

      /* handle signals */
HANDLE_SIGNAL:
      time(&curtime);
      if (curtime >= alarmtime)
      {
         HandleSigAlrm();
         alarmtime = curtime + SLPD_AGE_INTERVAL;
      }

      /* check for interface changes */
      if (InterfacesChanged(&monitor) || G_SIGHUP)
      {
         G_SIGHUP = 0;
         HandleSigHup();
      }
   }

   /* Got SIGTERM */
   HandleSigTerm();

cleanup_winsock:

   InterfaceMonitorDeinit(&monitor);
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
         ServiceStop();
         return;

      case SERVICE_CONTROL_INTERROGATE:
         break;

      case SERVICE_CONTROL_PARAMCHANGE:
         G_SIGHUP = 1;
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
      ReportStatusToSCMgr(SERVICE_STOPPED, G_SIGTERM ? NO_ERROR : 1, 0);
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
int __cdecl main(int argc, char ** argv)
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
   return 0;
}

/*=========================================================================*/
