/*//////////////////////////////////////////////////////////////////////////*
/*
/* Project:     OpenSLP - OpenSource implementation of Service Location
/*              Protocol
/*          
/* File:        slpd.c
/*
/* Abstract:    Main implementation file for the OpenSlp SA/DA daemon (slpd)
/*
/* Author(s):   Matthew Peterson
/*
/*//////////////////////////////////////////////////////////////////////////*

#include <string.h>

/* SLPD includes */
#include <slpd_cmdline.h>
#include <slpd_main.h>

/* Common includes */
#include <slp_logfile.h>

/*=========================================================================*/
int main(int argc, char* argv[])
/*=========================================================================*/
{
    SLPDCommandLine     cmdline;
    SLPSocketList       socketlist;
   
    
    /*------------------------------*/
    /* Make sure we are root        */
    /*------------------------------*/
    if(getuid() != 0)
    {
        SLPFatal("slpd must be run as root\n");
    }
    

    /*------------------------*/
    /* Parse the command line */
    /*------------------------*/
    if(SLPDParseCommandLine(argc,argv) == 0)
    {
        SLPFatal("Could not initialize necessary data structures\n");
    }

    
    /*-------------------------*/
    /* Initialize properties   */
    /*-------------------------*/
    if(SLPDPropertyInit(G_SlpdCommandLine.cfgfile) != 0)
    {
        SLPFatal("Could not initialize necessary data structures\n");
    }
    
    /*------------------------------*/
    /* Initialize the log file      */
    /*------------------------------*/
    #if(defined DEBUG)
    SLPLogSetLevel(4);
    if(SLPLogFileOpen(G_SlpdCommandLine.logfile, 0) != 0)
    #else
    SLPLogSetLevel(3);
    if(SLPLogFileOpen(G_SlpdCommandLine.logfile, 1) != 0)
    #endif                                     
    {
        SLPError("SLPD: Could not open logfile: %s. Logging disabled.\n",
                 G_SlpdCommandLine.logfile);
        SLPLogSetLevel(1);
    }
    SLPLog("****************************************\n");
    SLPLog("*** SLPD daemon started              ***\n");
    SLPLog("****************************************\n");
    SLPLog("command line = %s\n",argv[0]);
    
       
    /*--------------------------------------------------*/
    /* Setup the list of sockets                        */
    /*--------------------------------------------------*/
    memset(&socketlist,0,sizeof(socketlist));
    if(SLPDSetupSockets(&socketlist) != 0)
    {
        SLPFatal("Could not initialize network sockets\n");
    }

    
    /*--------------------------------------------------*/
    /* Enter the main loop                              */
    /*--------------------------------------------------*/
    SLPDMainLoop();

    
    /*--------------------------------------------------*/
    /* Main loop exited so we have received a SIG_TERM  */
    /*--------------------------------------------------*/
    
    /* TODO: clean up!  De-register registered services on DAs, etc */

    
    return 0;
}



