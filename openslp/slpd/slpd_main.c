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

/** Main entry point for the slpd process.
 *
 * @file       slpd_main.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    SlpdCode
 */

#include "slp_types.h"

#include "slpd_log.h"
#include "slpd_socket.h"
#include "slpd_incoming.h"
#include "slpd_outgoing.h"
#include "slpd_database.h"
#include "slpd_cmdline.h"
#include "slpd_knownda.h"
#include "slpd_property.h"
#include "slpd.h"

#ifdef ENABLE_SLPv2_SECURITY
# include "slpd_spi.h"
#endif

#include "slp_xmalloc.h"
#include "slp_xid.h"
#include "slp_net.h"

int G_SIGALRM;
int G_SIGTERM;
int G_SIGHUP;                                                                                                 
#ifdef DEBUG
int G_SIGINT;     /* Signal being used for dumping registrations */
#endif 

/** Configures fd_set objects with sockets.
 *
 * @param[in] socklist - The list of sockets that is being currently 
 *    monitored by OpenSLP components.
 * @param[out] highfd - The address of storage for returning the value 
 *    of the highest file descriptor (number) in use.
 * @param[out] readfds - The fd_set to fill with read descriptors.
 * @param[out] writefds - The fd_set to fill with write descriptors.
 */
void LoadFdSets(SLPList * socklist, sockfd_t * highfd, fd_set * readfds, 
      fd_set * writefds)
{
   SLPDSocket * sock = 0;
   SLPDSocket * del = 0;

   sock = (SLPDSocket *)socklist->head;
   while (sock)
   {
      if (sock->fd > *highfd)
         *highfd = sock->fd;

      switch(sock->state)
      {
         case DATAGRAM_UNICAST:
         case DATAGRAM_MULTICAST:
         case DATAGRAM_BROADCAST:
            FD_SET(sock->fd,readfds);
            break;

         case SOCKET_LISTEN:
            if (socklist->count < SLPD_MAX_SOCKETS)
               FD_SET(sock->fd,readfds);
            break;

         case STREAM_READ:
         case STREAM_READ_FIRST:
            FD_SET(sock->fd,readfds);
            break;

         case STREAM_WRITE:
         case STREAM_WRITE_FIRST:
         case STREAM_CONNECT_BLOCK:
            FD_SET(sock->fd,writefds);
            break;

         case SOCKET_CLOSE:
            del = sock;
            break;

         default:
            break;
      }
      sock = (SLPDSocket*)sock->listitem.next;
      if(del)
      {
         SLPDSocketFree((SLPDSocket *)SLPListUnlink(socklist,
               (SLPListItem*)del));
         del = 0;
      }
   }
}

/** Handles a SIG_TERM signal from the system.
 */
void HandleSigTerm(void)
{
   struct timeval timeout;
   fd_set readfds;
   fd_set writefds;
   sockfd_t highfd = 0;
   int fdcount = 0;

   SLPDLog("****************************************\n");
   SLPDLogTime();
   SLPDLog("SLPD daemon shutting down\n");
   SLPDLog("****************************************\n");

   /* close all incoming sockets */
   SLPDIncomingDeinit();

   /* unregister with all DAs */
   SLPDKnownDADeinit();

   timeout.tv_sec  = 5;
   timeout.tv_usec = 0; 

   /* Do a dead DA passive advert to tell everyone we're goin' down */
   SLPDKnownDAPassiveDAAdvert(0, 1);

   /* if possible wait until all outgoing socket are done and closed */
   while (SLPDOutgoingDeinit(1))
   {
      FD_ZERO(&writefds);
      FD_ZERO(&readfds);
      LoadFdSets(&G_OutgoingSocketList, &highfd, &readfds, &writefds);
      fdcount = select((int)(highfd + 1), &readfds, &writefds, 0, &timeout);
      if (fdcount == 0)
         break;
      SLPDOutgoingHandler(&fdcount, &readfds, &writefds);
   }

   SLPDOutgoingDeinit(0);

   SLPDLog("****************************************\n");
   SLPDLogTime();
   SLPDLog("SLPD daemon shut down\n");
   SLPDLog("****************************************\n");

#ifdef DEBUG
# ifdef ENABLE_SLPv2_SECURITY
   SLPDSpiDeinit();
# endif
   SLPDDatabaseDeinit();
   SLPDPropertyDeinit();
   SLPDLogFileClose();
   xmalloc_deinit();    
#endif
}

/** Handles a SIG_HUP signal from the system.
 *
 * @internal
 */
static void HandleSigHup(void)
{
   /* Reinitialize */
   SLPDLog("****************************************\n");
   SLPDLogTime();
   SLPDLog("SLPD daemon reset by SIGHUP\n");
   SLPDLog("****************************************\n\n");

   /* unregister with all DAs */
   SLPDKnownDADeinit();

   /* re-read properties */
   SLPDPropertyReinit();

#ifdef ENABLE_SLPv2_SECURITY
   /* Re-initialize SPI stuff*/
   SLPDSpiInit(G_SlpdCommandLine.spifile);
#endif

   /* Re-read the static registration file (slp.reg)*/
   SLPDDatabaseReInit(G_SlpdCommandLine.regfile);

   /* Rebuild Known DA database */
   SLPDKnownDAInit();

   SLPDLog("****************************************\n");
   SLPDLogTime();
   SLPDLog("SLPD daemon reset finished\n");
   SLPDLog("****************************************\n\n");
}

/** Handles a SIG_ALRM signal from the system.
 */
void HandleSigAlrm(void)
{
   SLPDIncomingAge(SLPD_AGE_INTERVAL);
   SLPDOutgoingAge(SLPD_AGE_INTERVAL);
   SLPDKnownDAImmortalRefresh(SLPD_AGE_INTERVAL);
   SLPDKnownDAPassiveDAAdvert(SLPD_AGE_INTERVAL, 0);
   SLPDKnownDAActiveDiscovery(SLPD_AGE_INTERVAL);
   SLPDDatabaseAge(SLPD_AGE_INTERVAL, G_SlpdProperty.isDA);
}

#ifdef DEBUG
/** Handles a SIG_INT signal from the system.
 *
 * @internal
 */
static void HandleSigInt(void)
{
   SLPDIncomingSocketDump();
   SLPDOutgoingSocketDump();
   SLPDKnownDADump();
   SLPDDatabaseDump();
}
#endif

#ifndef _WIN32
/** Check a pid file to see if slpd is already running.
 *
 * The pid file contains the PID of the process that is already running.
 *
 * @param[in] pidfile - The name of a file to read.
 *
 * @return Zero on success or a non-zero value on failure.
 *
 * @internal
 */
static int CheckPid(const char * pidfile)
{
   pid_t pid;
   FILE * fd;
   char pidstr[14];

   /* make sure that we're not running already
      read the pid from the file 
    */
   fd = fopen(pidfile, "r");
   if (fd)
   {
      memset(pidstr,0,14);
      fread(pidstr,13,1,fd);
      pid = atoi(pidstr);
      if (pid && kill(pid, 0) == 0)
         return -1;  /* we are already running */
      fclose(fd);
   }
   return 0;
}

/** Write the pid file.
 *
 * @param[in] pidfile - The name of the file to write.
 * @param[in] pid - The PID value to write to @p pidfile.
 *
 * @return Zero on success, or a non-zero value on failure.
 *
 * @internal
 */
/* static */ int WritePid(const char* pidfile, pid_t pid)
{
   FILE * fd;
   char pidstr[14];

   /* write my pid to the pidfile */
   fd = fopen(pidfile, "w");
   if(fd)
   {
      sprintf(pidstr, "%i", (int)pid);
      fwrite(pidstr, strlen(pidstr), 1, fd);
      fclose(fd);
   }
   return 0;
}

/** Daemonize the calling process.
 *
 * Turn the calling process into a daemon (detach from tty setuid(), etc.
 *
 * @param[in] pidfile - The name of a file to which the process id should
 *    be written.
 *
 * @return Zero on success, or a non-zero value if slpd could not daemonize 
 *    (or if slpd is already running).
 *
 * @internal
 */
static int Daemonize(const char * pidfile)
{
   FILE * fd;
   struct passwd * pwent;
   pid_t pid;
   char pidstr[14];

   /* fork() if we should detach */
   if (G_SlpdCommandLine.detach)
      pid = fork();
   else
      pid = getpid();

   /* parent or child? */
   switch(pid)
   {
      case -1:
         return -1;

      case 0:
         /* child lives */
         break;

      default:
         /* parent writes pid (or child) pid file and dies */
         fd = fopen(pidfile,"w");
         if (fd)
         {
            sprintf(pidstr,"%i",(int)pid);
            fwrite(pidstr,strlen(pidstr),1,fd);
            fclose(fd);
         }
         if (G_SlpdCommandLine.detach)
            exit(0);
         break;
   }

   close(0); 
   close(1); 
   close(2); 
   setsid(); /* will only fail if we are already the process group leader */

   /* suid to daemon */
   /* TODO: why do the following lines mess up my signal handlers? */
   pwent = getpwnam("daemon"); 
   if (pwent)
   {
      if (setgroups(1, &pwent->pw_gid) < 0 ||   setgid(pwent->pw_gid) < 0 
            || setuid(pwent->pw_uid) < 0)
      {
         /* TODO: should we log here and return fail */
      }
   }

   /* Set cwd to / (root)*/
   chdir("/");

   return 0;
}

/** Handles all registered signals from the system.
 *
 * @param[in] signum - The signal number to handle.
 *
 * @internal
 */
static void SignalHandler(int signum)
{
   switch(signum)
   {
      case SIGALRM:
         G_SIGALRM = 1;
         break;

      case SIGTERM:
         G_SIGTERM = 1;
         break;

      case SIGHUP:
         G_SIGHUP = 1;
         break;

#ifdef DEBUG
      case SIGINT:
         G_SIGINT = 1;
         break;
#endif

      case SIGPIPE:
      default:
         break;
   }
}

/** Configures signal handlers for the process.
 *
 * Configures the process to receive SIGALRM, SIGTERM, SIGHUP, SIGPIPE,
 * and SIGINT (Debug only).
 *
 * @return Zero on success, or a non-zero value on failure.
 *
 * @internal
 */
static int SetUpSignalHandlers(void)
{
   int result;
   struct sigaction sa;

   sa.sa_handler = SignalHandler;
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = 0; /* SA_ONESHOT; */

#ifdef HAVE_SA_RESTORER
   sa.sa_restorer = 0;
#endif

   result = sigaction(SIGALRM, &sa, 0);
   result |= sigaction(SIGTERM, &sa, 0);
   result |= sigaction(SIGPIPE, &sa, 0);

#ifdef DEBUG
   result |= sigaction(SIGINT, &sa, 0);
#endif

   signal(SIGHUP, SignalHandler);
   /* result |= sigaction(SIGHUP,&sa,0); */

   return result;
}

/** Process main entry point.
 *
 * @param[in] argc - The number of command line arguments passed in @p argv.
 * @param[in] argv - An array of pointers to command line arguments.
 *
 * @return Zero on success, or a non-zero shell error code.
 *
 * @remarks This routine contains the main server loop.
 */
int main(int argc, char * argv[])
{
   fd_set readfds;
   fd_set writefds;
   int highfd;
   int fdcount = 0;
   time_t curtime;
   struct timeval timeout;

#ifdef DEBUG
   xmalloc_init("/var/log/slpd_xmalloc.log", 0);
#endif

   /* Parse the command line */
   if (SLPDParseCommandLine(argc,argv))
      SLPDFatal("Invalid command line\n");

   /* make sure we are root */
   if (getuid() != 0)
      SLPDFatal("slpd must be started by root\n");

   /* make sure we are not already running */
   if (CheckPid(G_SlpdCommandLine.pidfile))
      SLPDFatal("slpd is already running. Check %s\n",
            G_SlpdCommandLine.pidfile);

   /* initialize the log file */
   if (SLPDLogFileOpen(G_SlpdCommandLine.logfile, 1))
      SLPDFatal("Could not open logfile %s\n",G_SlpdCommandLine.logfile);

   /* seed the XID generator */
   SLPXidSeed();

   /* log startup message */
   SLPDLog("****************************************\n");
   SLPDLogTime();
   SLPDLog("SLPD daemon started\n");
   SLPDLog("****************************************\n");
   SLPDLog("Command line = %s\n", argv[0]);
   SLPDLog("Using configuration file = %s\n", G_SlpdCommandLine.cfgfile);
   SLPDLog("Using registration file = %s\n", G_SlpdCommandLine.regfile);
#ifdef ENABLE_SLPv2_SECURITY
   SLPDLog("Using SPI file = %s\n", G_SlpdCommandLine.spifile);
#endif

   /* initialize for the first time */
   if (SLPDPropertyInit(G_SlpdCommandLine.cfgfile)
#ifdef ENABLE_SLPv2_SECURITY
         || SLPDSpiInit(G_SlpdCommandLine.spifile)
#endif     
         || SLPDDatabaseInit(G_SlpdCommandLine.regfile)
         || SLPDIncomingInit() 
         || SLPDOutgoingInit() 
         || SLPDKnownDAInit())
      SLPDFatal("slpd initialization failed\n");
   SLPDLog("Agent Interfaces = %s\n", G_SlpdProperty.interfaces);

   /* make slpd run as a daemon */
   if (Daemonize(G_SlpdCommandLine.pidfile))
      SLPDFatal("Could not daemonize\n");

   /* Setup signal handlers */
   if (SetUpSignalHandlers())
      SLPDFatal("Error setting up signal handlers.\n");

   /* Set up alarm to age database -- a shorter start, so SAs register with us quickly on our startup */
   alarm(2);

   /* Main loop */
   SLPDLog("Startup complete entering main run loop ...\n\n");
   G_SIGALRM   = 0;
   G_SIGTERM   = 0;
   G_SIGHUP    = 0;    
#ifdef DEBUG
   G_SIGINT    = 0;
#endif

   while (G_SIGTERM == 0)
   {
      /* load the fdsets up with all valid sockets in the list  */
      highfd = 0;
      FD_ZERO(&readfds);
      FD_ZERO(&writefds);
      LoadFdSets(&G_IncomingSocketList, &highfd, &readfds, &writefds);
      LoadFdSets(&G_OutgoingSocketList, &highfd, &readfds, &writefds);

      /* before select(), check to see if we got a signal */
      if (G_SIGALRM || G_SIGHUP)
         goto HANDLE_SIGNAL;

      /* main select -- we time out every second so the outgoing retries can occur*/
      time(&curtime);  
      timeout.tv_sec = 1;
      timeout.tv_usec = 0;
      fdcount = select(highfd + 1, &readfds, &writefds, 0, &timeout);
      if (fdcount > 0) /* fdcount will be < 0 when interrupted by a signal */
      {
         SLPDIncomingHandler(&fdcount, &readfds, &writefds);
         SLPDOutgoingHandler(&fdcount, &readfds, &writefds);
         SLPDOutgoingRetry(time(0) - curtime);
      }
      else if (fdcount == 0)
         SLPDOutgoingRetry(time(0) - curtime);


HANDLE_SIGNAL:

      if (G_SIGHUP)
      {
         HandleSigHup();
         G_SIGHUP = 0;
      }
      if (G_SIGALRM)
      {
         HandleSigAlrm();
         G_SIGALRM = 0;
         alarm(SLPD_AGE_INTERVAL);
      }

#ifdef DEBUG
      if (G_SIGINT)
      {
         HandleSigInt();
         G_SIGINT = 0;
      }        
#endif

   } /* End of main loop */

   /* Got SIGTERM */
   HandleSigTerm();

   return 0;
}
#endif   /* _WIN32 */

/*=========================================================================*/
