
#ifndef SLP_KIOPROTOCOL_H
#define SLP_KIOPROTOCOL_H

#include <iostream.h>
#include <kdebug.h>
#include <kinstance.h>
#include <kio/global.h>
#include <kio/slavebase.h>
#include <kurl.h>
#include <qdict.h>
#include <qregexp.h>
#include <qstringlist.h>
#include <slp.h>
#include <stdlib.h>
#include <sys/stat.h>  

// possible types of URLs
#define URL_FINDSRVTYPE 0
#define URL_FINDSRV     1
#define URL_SRVURL      2
#define URL_BADURL      3

using namespace KIO; 

//===========================================================================
extern "C" 
{ 
   int kdemain( int argc, char **argv ); 
}


//===========================================================================
class SLP_KioProtocol: public KIO::SlaveBase
{
   
private:
    SLPHandle  m_hslp;
   
   
protected:
    void handleBadUrl(const QString& srvurl);

    void handleError(const char* message);
    
    void handleFindSrvs(const char* srvtype);
    
    void handleFindSrvTypes();
    
    void handleSrvUrl(const char* srvurl);
    
    int parseURL(const KURL& url, QCString& result);
    
    void setUDSEntry(const QString& srvurl, UDSEntry& entry);


 public:
    SLP_KioProtocol(const QCString& pool, const QCString& app);

    virtual ~SLP_KioProtocol();
   
    void handleListEntry(const char* srvurl, bool status);
   
    virtual void listDir(const KURL& url);
    
    virtual void stat(const KURL& url);
};

#endif
