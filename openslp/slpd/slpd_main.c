/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_main.c                                                */
/*                                                                         */
/* Abstract:    Main daemon loop                                           */
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
#include "slpd_log.h"
#include "slpd_socket.h"
#include "slpd_incoming.h"
#include "slpd_outgoing.h"
#include "slpd_database.h"
#include "slpd_cmdline.h"
#include "slpd_knownda.h"
#include "slpd_property.h"
#ifdef ENABLE_AUTHENTICATION
#include "slpd_spi.h"
#endif

/*=========================================================================*/
/* common code includes                                                    */
/*=========================================================================*/
#include "../common/slp_xmalloc.h"
#include "../common/slp_xid.c"


/*==========================================================================*/
int G_SIGALRM;
int G_SIGTERM;
int G_SIGHUP;                                                                                                 
#ifdef DEBUG
int G_SIGINT;		/* Signal being used for dumping registrations */
#endif 
/*==========================================================================*/


/*-------------------------------------------------------------------------*/
void LoadFdSets(SLPList* socklist, 
                int* highfd, 
                fd_set* readfds, 
                fd_set* writefds)
/*-------------------------------------------------------------------------*/
{
    SLPDSocket* sock = 0;
    SLPDSocket* del = 0;

    sock = (SLPDSocket*)socklist->head;
    while(sock)
    {
        if(sock->fd > *highfd)
        {
            *highfd = sock->fd;
        }

        switch(sock->state)
        {
        case DATAGRAM_UNICAST:
        case DATAGRAM_MULTICAST:
        case DATAGRAM_BROADCAST:
            FD_SET(sock->fd,readfds);
            break;

        case SOCKET_LISTEN:
            if(socklist->count < SLPD_MAX_SOCKETS)
            {
                FD_SET(sock->fd,readfds);
            }
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
            SLPDSocketFree((SLPDSocket*)SLPListUnlink(socklist,(SLPListItem*)del));     
            del = 0;
        }
    }
}


/*------------------------------------------------------------------------*/
void HandleSigTerm()
/*------------------------------------------------------------------------*/
{
    struct timeval  timeout;
    fd_set          readfds;
    fd_set          writefds;
    int             highfd;
    int             fdcount         = 0;

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
    while(SLPDOutgoingDeinit(1))
    {
        FD_ZERO(&writefds);
        FD_ZERO(&readfds);
        LoadFdSets(&G_OutgoingSocketList, &highfd, &readfds,&writefds);
        fdcount = select(highfd+1,&readfds,&writefds,0,&timeout);
        if(fdcount == 0)
        {
            break;
        }

        SLPDOutgoingHandler(&fdcount,&readfds,&writefds);
    }

    SLPDOutgoingDeinit(0);

    SLPDLog("****************************************\n");
    SLPDLogTime();
    SLPDLog("SLPD daemon shut down\n");
    SLPDLog("****************************************\n");

#ifdef DEBUG
    #ifdef ENABLE_AUTHENTICATION
    SLPDSpiDeinit();
    #endif
    SLPDDatabaseDeinit();
    SLPDPropertyDeinit();
    xmalloc_deinit();    
#endif

}

/*------------------------------------------------------------------------*/
void HandleSigHup()
/*------------------------------------------------------------------------*/
{
    /* Reinitialize */
    SLPDLog("****************************************\n");
    SLPDLogTime();
    SLPDLog("SLPD daemon reset by SIGHUP\n");
    SLPDLog("****************************************\n\n");

    /* unregister with all DAs */
    SLPDKnownDADeinit();

    /* re-read properties */
    SLPDPropertyInit(G_SlpdCommandLine.cfgfile);

    /* Re-read the static registration file (slp.reg)*/
    SLPDDatabaseReInit(G_SlpdCommandLine.regfile);

    /* Rebuild Known DA database */
    SLPDKnownDAInit();

    SLPDLog("****************************************\n");
    SLPDLogTime();
    SLPDLog("SLPD daemon reset finished\n");
    SLPDLog("****************************************\n\n");
}

/*------------------------------------------------------------------------*/
void HandleSigAlrm()
/*------------------------------------------------------------------------*/
{
    SLPDIncomingAge(SLPD_AGE_INTERVAL);
    SLPDOutgoingAge(SLPD_AGE_INTERVAL);
    SLPDKnownDAImmortalRefresh(SLPD_AGE_INTERVAL);
    SLPDKnownDAPassiveDAAdvert(SLPD_AGE_INTERVAL,0);
    SLPDKnownDAActiveDiscovery(SLPD_AGE_INTERVAL);
    SLPDDatabaseAge(SLPD_AGE_INTERVAL,G_SlpdProperty.isDA);
}


#ifdef DEBUG
/*--------------------------------------------------------------------------*/
void HandleSigInt()
/*--------------------------------------------------------------------------*/
{
    SLPDDatabaseDump();
}
#endif


#ifndef WIN32
/*-------------------------------------------------------------------------*/
int CheckPid(const char* pidfile)
/* Check a pid file to see if slpd is already running                      */
/*                                                                         */
/* Returns: 0 on success.  non-zero on failure                             */
/*-------------------------------------------------------------------------*/
{
    pid_t   pid;
    FILE*   fd;
    char    pidstr[14];

    /*------------------------------------------*/
    /* make sure that we're not running already */
    /*------------------------------------------*/
    /* read the pid from the file */
    fd = fopen(pidfile,"r");
    if(fd)
    {
        memset(pidstr,0,14);
        fread(pidstr,13,1,fd);
        pid = atoi(pidstr);
        if(pid)
        {
            if(kill(pid,0) == 0)
            {
                /* we are already running */
                return -1;
            }
        }

        fclose(fd);
    }

    return 0;
}


/*-------------------------------------------------------------------------*/
int WritePid(const char* pidfile, pid_t pid)
/* Write the pid file                                                      */
/*                                                                         */
/* Returns: 0 on success.  non-zero on failure                             */
/*-------------------------------------------------------------------------*/
{
    FILE*   fd;
    char    pidstr[14];

    /* write my pid to the pidfile */
    fd = fopen(pidfile,"w");
    if(fd)
    {
        sprintf(pidstr,"%i",(int)pid);
        fwrite(pidstr,strlen(pidstr),1,fd);
        fclose(fd);
    }

    return 0;
}


/*-------------------------------------------------------------------------*/
int Daemonize(const char* pidfile)
/* Turn the calling process into a daemon (detach from tty setuid(), etc   */
/*                                                                         */      
/* Returns: zero on success non-zero if slpd could not daemonize (or if    */
/*          slpd is already running                             .          */
/*-------------------------------------------------------------------------*/
{
    FILE*   fd;
    struct  passwd* pwent;
    pid_t   pid;
    char    pidstr[14];

    /* fork() if we should detach */
    if(G_SlpdCommandLine.detach)
    {
        pid = fork();
    }
    else
    {
        pid = getpid();
    }

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
        if(fd)
        {
            sprintf(pidstr,"%i",(int)pid);
            fwrite(pidstr,strlen(pidstr),1,fd);
            fclose(fd);
        }
        if(G_SlpdCommandLine.detach)
        {
            exit(0);
        }
        break;
    }

    close(0); 
    close(1); 
    close(2); 
    setsid(); /* will only fail if we are already the process group leader */

    /*----------------*/
    /* suid to daemon */
    /*----------------*/
    /* TODO: why do the following lines mess up my signal handlers? */
    pwent = getpwnam("daemon"); 
    if(pwent)
    {
        if(setgroups(1, &pwent->pw_gid) < 0 ||
           setgid(pwent->pw_gid) < 0 ||
           setuid(pwent->pw_uid) < 0)
        {
            /* TODO: should we log here and return fail */
        }
    }

    return 0;
}


/*--------------------------------------------------------------------------*/
void SignalHandler(int signum)
/*--------------------------------------------------------------------------*/
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


/*-------------------------------------------------------------------------*/
int SetUpSignalHandlers()
/*-------------------------------------------------------------------------*/
{
    int result;
    struct sigaction sa;

    sa.sa_handler    = SignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags      = 0;//SA_ONESHOT;
#if defined(HAVE_SA_RESTORER)
    sa.sa_restorer   = 0;
#endif

    result = sigaction(SIGALRM,&sa,0);
    result |= sigaction(SIGTERM,&sa,0);
    result |= sigaction(SIGPIPE,&sa,0);

#ifdef DEBUG
    result |= sigaction(SIGINT,&sa,0);
#endif

    signal(SIGHUP,SignalHandler);
    //result |= sigaction(SIGHUP,&sa,0);

    return result;
}

/*=========================================================================*/
int main(int argc, char* argv[])
/*=========================================================================*/
{
    fd_set          readfds;
    fd_set          writefds;
    int             highfd;
    int             fdcount         = 0;

#ifdef DEBUG
    xmalloc_init("/var/log/slpd_xmalloc.log",0);
#endif

    /*------------------------*/
    /* Parse the command line */
    /*------------------------*/
    if(SLPDParseCommandLine(argc,argv))
    {
        SLPDFatal("Invalid command line\n");
    }

    /*------------------------------*/
    /* Make sure we are root        */
    /*------------------------------*/
    if(getuid() != 0)
    {
        SLPDFatal("slpd must be started by root\n");
    }

    /*--------------------------------------*/
    /* Make sure we are not already running */
    /*--------------------------------------*/
    if(CheckPid(G_SlpdCommandLine.pidfile))
    {
        SLPDFatal("slpd is already running. Check %s\n",
                 G_SlpdCommandLine.pidfile);
    }

    /*------------------------------*/
    /* Initialize the log file      */
    /*------------------------------*/
    if(SLPDLogFileOpen(G_SlpdCommandLine.logfile, 1))
    {
        SLPDFatal("Could not open logfile %s\n",G_SlpdCommandLine.logfile);
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

    /*--------------------------------------------------*/
    /* Initialize for the first time                    */
    /*--------------------------------------------------*/
    if(SLPDPropertyInit(G_SlpdCommandLine.cfgfile) ||
       SLPDDatabaseInit(G_SlpdCommandLine.regfile) ||
#ifdef ENABLE_AUTHENTICATION
       SLPDSpiInit(G_SlpdCommandLine.spifile) ||
#endif
       SLPDIncomingInit() ||
       SLPDOutgoingInit() ||
       SLPDKnownDAInit())
    {
        SLPDFatal("slpd initialization failed\n");
    }
    SLPDLog("Agent Interfaces = %s\n",G_SlpdProperty.interfaces);
    SLPDLog("Agent URL = %s\n",G_SlpdProperty.myUrl);

    /*---------------------------*/
    /* make slpd run as a daemon */
    /*---------------------------*/
    if(Daemonize(G_SlpdCommandLine.pidfile))
    {
        SLPDFatal("Could not daemonize\n");
    }

    /*-----------------------*/
    /* Setup signal handlers */
    /*-----------------------*/
    if(SetUpSignalHandlers())
    {
        SLPDFatal("Error setting up signal handlers.\n");
    }

    /*------------------------------*/
    /* Set up alarm to age database */
    /*------------------------------*/
    alarm(SLPD_AGE_INTERVAL);

    /*-----------*/
    /* Main loop */
    /*-----------*/
    SLPDLog("Startup complete entering main run loop ...\n\n");
    G_SIGALRM   = 0;
    G_SIGTERM   = 0;
    G_SIGHUP    = 0;    
#ifdef DEBUG
    G_SIGINT    = 0;
#endif

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
        if(G_SIGALRM || G_SIGHUP)
        {
            goto HANDLE_SIGNAL;
        }

        /*-------------*/
        /* Main select */
        /*-------------*/
        fdcount = select(highfd+1,&readfds,&writefds,0,0);
        if(fdcount > 0) /* fdcount will be < 0 when interrupted by a signal */
        {
            SLPDIncomingHandler(&fdcount,&readfds,&writefds);
            SLPDOutgoingHandler(&fdcount,&readfds,&writefds);
        }

        /*----------------*/
        /* Handle signals */
        /*----------------*/
        HANDLE_SIGNAL:
        if(G_SIGHUP)
        {
            HandleSigHup();
            G_SIGHUP = 0;
        }
        if(G_SIGALRM)
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
#endif /*ifndef WIN32 */



