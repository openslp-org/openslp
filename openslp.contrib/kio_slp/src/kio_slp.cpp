#include "kio_slp.h"

using namespace KIO;

//===========================================================================
SLPBoolean myFindSrvTypesCallback(SLPHandle hslp,
				  const char* srvtypes,
				  SLPError errcode,
				  void* cookie)
{
   char*            slider1;
   char*            slider2;
   char*            cpy;

   SLP_KioProtocol* slave = (SLP_KioProtocol*)cookie;
     
   if(errcode == SLP_OK && *srvtypes)
   {
      kdDebug(7111) << "srvtypes = " << srvtypes << endl;
      cpy = strdup(srvtypes);
      if(cpy)
      {
	 slider1 = slider2 = cpy;
	 while(slider1 = strchr(slider2,','))
	 {
	    *slider1 = 0;
	    if(strncasecmp(slider2,"service:",8) == 0)
            {
	       slave->handleListEntry(slider2 + 8,false);
            }
            else
            {
	       slave->handleListEntry(slider2,false);
            }       
	    
	    slider1 ++;
	    slider2 = slider1;
	 }
	     
	 if(strncasecmp(slider2,"service:",8) == 0)
         {
	    slave->handleListEntry(slider2 + 8,false);
         }
         else
         {
	    slave->handleListEntry(slider2,false);
         }       

	 free(cpy);
      }
      
   }
   else if(errcode == SLP_LAST_CALL)
   {
//      slave->handleListEntry(0,true);
      // result will be SLP_FALSE
   }
   else
   {
      // display errorcode
      // result will be SLP_FALSE
   }
      
   return SLP_TRUE;      
}


//===========================================================================
SLPBoolean myFindSrvsCallback(SLPHandle hslp,
			      const char* srvurl,
			      unsigned short lifetime,
			      SLPError errcode,
			      void* cookie )
{
   const char* pos;
   SLP_KioProtocol* slave  = (SLP_KioProtocol*)cookie;
      
   if(errcode == SLP_OK)
   {
      if(strncasecmp(srvurl,"service:",8) == 0)
      {
	 pos = srvurl + 8;
      }
      else
      {
	 pos = srvurl;
      }
	 
      pos = strstr(pos,":/");
      if(pos)
      {
	 pos += 2;
	 if(*pos == '/') pos ++;	 
      }
      
      slave->handleListEntry(pos,false);
      
   }
   else if(errcode == SLP_LAST_CALL)
   {
      slave->handleListEntry(0,true);
      // result will be SLP_FALSE
   }
   else
   {
      // display error
      // result will be SLP_FALSE
   }   
   
   return SLP_TRUE;
}


//---------------------------------------------------------------------------
void SLP_KioProtocol::handleBadUrl(const QString& srvurl)
{
    kdDebug(7111) << "START: handleBadUrl" << endl;
    kdDebug(7111) << "    srvurl = " << srvurl << endl;
    kdDebug(7111) << "END: handleBadUrl" << endl;
}



//---------------------------------------------------------------------------
void SLP_KioProtocol::handleError(const char* message)
{
   
}


//---------------------------------------------------------------------------
void SLP_KioProtocol::handleFindSrvs(const char* srvtype)
{
    kdDebug(7111) << "START: handleFindSrvs" << endl;
    kdDebug(7111) << "    srvtype = " << srvtype << endl;
    
    SLPError result;
   
    result = SLPFindSrvs(m_hslp,
			 srvtype,
			 "",
			 "",
			 myFindSrvsCallback,
			 this);
    if(result != SLP_OK)
    {
       handleError("SLPFindSrvs() error"); 
    }
			 
    kdDebug(7111) << "END: handleFindSrvs" << endl;
}


//---------------------------------------------------------------------------
void SLP_KioProtocol::handleFindSrvTypes()
{
    kdDebug(7111) << "START: handleFindSrvTypes" << endl;
     
    SLPError result;
   
    result = SLPFindSrvTypes(m_hslp,
			     "*",
			     "",
			     myFindSrvTypesCallback,
			     this);
    if(result == SLP_OK)
    {
        handleListEntry(0,true);
    }
    else
    {
       handleError("SLPFindSrvType() error");
    } 
    kdDebug(7111) << "END: handleFindSrvTypes" << endl;
}


//---------------------------------------------------------------------------
void SLP_KioProtocol::handleSrvUrl(const char* srvurl)
{
    kdDebug(7111) << "START: handleSrvUrl" << endl;
    kdDebug(7111) << "    srvurl = " << srvurl << endl;
    
    KURL url;
    QCString str(srvurl);
   
    url.setProtocol(str.left(str.find('/')));
    url.setHost(str.mid(str.find('/')+1));

    redirection(url);
       
    kdDebug(7111) << "END: handleSrvUrl" << endl;
}


//---------------------------------------------------------------------------
int SLP_KioProtocol::parseURL(const KURL& url,
			      QCString& result)
{
    result = url.prettyURL().local8Bit();
    
    kdDebug(7111) << "url.prettyUFL() = " << result << endl; 
   
    if(result.length() == 9 && result.left(9) == "service:/")
    {
        result = "*";
        return URL_FINDSRVTYPE; 
    }
    
    if(result.left(9) == "service:/")
    {
       result = result.mid(9);       
    }
       
    if(result.find('/') == -1)
    {
        return URL_FINDSRV;
    }   
      
    
   
   
    return URL_SRVURL;
}


//---------------------------------------------------------------------------
void SLP_KioProtocol::setUDSEntry( const QString& srvurl,
				   UDSEntry& entry )
{
    kdDebug(7111) << "createUDSEntry, name = " << srvurl << endl;
    UDSAtom atom;

    atom.m_uds = KIO::UDS_NAME;
    atom.m_str = srvurl;
        
    entry.append( atom );

    atom.m_uds = KIO::UDS_FILE_TYPE;
    atom.m_long = S_IFDIR;
    entry.append( atom );

    atom.m_uds  = KIO::UDS_ACCESS;
    atom.m_long = 0555;
    entry.append( atom );
}


//---------------------------------------------------------------------------
void SLP_KioProtocol::handleListEntry(const char* srvurl, bool status)
{
   UDSEntry         udsentry;
   
   if(srvurl)
   {
      setUDSEntry(srvurl,udsentry);
   }
   else
   {
      setUDSEntry(QString::null,udsentry);
   }
   
   SlaveBase::listEntry(udsentry, status);
}


//---------------------------------------------------------------------------
void SLP_KioProtocol::listDir( const KURL& url )
{
    kdDebug(7111) << "START: listDir" << endl;
    
    QCString  parsedurl;
       
    kdDebug(7111) << "   url = " << url.prettyURL() << endl;
       
    switch(parseURL(url,parsedurl))
    {
     case URL_FINDSRVTYPE:
         kdDebug(7111) << "   FindSrvTypes parsedurl = " << parsedurl << endl;
         handleFindSrvTypes();
     break;
       
     case URL_FINDSRV:
         kdDebug(7111) << "   FindSrvs parsedurl = " << parsedurl << endl;
         handleFindSrvs(parsedurl);
     break;
              
     case URL_SRVURL:
         kdDebug(7111) << "   SrvUrl parsedurl = " << parsedurl << endl;
         handleSrvUrl(parsedurl);
     break;
     
     default:
         kdDebug(7111) << "   BadUrl parsedurl = " << parsedurl << endl;
         handleBadUrl(parsedurl);
     break;
    }
   
    SlaveBase::finished();   
    kdDebug(7111) << "END: listDir" << endl;
}


//---------------------------------------------------------------------------
void SLP_KioProtocol::stat( const KURL& url )
{
    kdDebug(7111) << "START: stat" << endl;
    kdDebug(7111) << "   url = " << url.prettyURL() << endl;
    
    UDSEntry udsentry;
   
    setUDSEntry(url.prettyURL().local8Bit(),udsentry);

    SlaveBase::statEntry(udsentry);
    SlaveBase::finished();
    
    kdDebug(7111) << "END: stat" << endl;
}


//---------------------------------------------------------------------------
SLP_KioProtocol::SLP_KioProtocol( const QCString& pool, const QCString& app )
    : SlaveBase( "slp", pool, app )
{
    SLPError result;
    
    result = SLPOpen( NULL, SLP_FALSE, &m_hslp );
    if ( result != SLP_OK )
    {
        handleError("Could not open SLPHandle");
        exit( -1 );
    }
}


//---------------------------------------------------------------------------
SLP_KioProtocol::~SLP_KioProtocol()
{
   SLPClose(m_hslp);
}


//============================================================================
int kdemain( int argc, char** argv )
//============================================================================
{
    kdDebug(7111) << "START: kdemain for kio_slp" << endl;

    KInstance instance( "kio_slp" );
    if( argc != 4 )
    {
        kdDebug(7111) <<
            "Usage: kio_nfs protocol domain-socket1 domain-socket2" << endl;
        return -1;
    }

    SLP_KioProtocol slave( argv[2], argv[3] );
    slave.dispatchLoop();

    return 0;
}

