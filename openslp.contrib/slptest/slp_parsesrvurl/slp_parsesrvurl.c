#include <stdio.h>
#include <slp.h>

#define MAX_URL_SIZE 4048

int main(int argc, char* argv[])
{
   SLPSrvURL* parsedurl;
   char url[MAX_URL_SIZE];
   SLPError error;
   
   while(fgets(url, MAX_URL_SIZE, stdin))
   {
      url[strlen(url)-1] = 0; /* get rid of /n */
      error = SLPParseSrvURL(url,&parsedurl);
      if(error == SLP_OK)
      {
	 printf("URL: %s\n"
		"   s_pcSrvType   = %s\n"
		"   s_pcHost      = %s\n"
		"   s_iPort       = %i\n"
		"   s_pcNetFamily = %s\n"
		"   s_pcSrvPart   = %s\n\n",
		url,
		parsedurl->s_pcSrvType,
		parsedurl->s_pcHost,
		parsedurl->s_iPort,
		parsedurl->s_pcNetFamily,
		parsedurl->s_pcSrvPart);
      }
      else
      {
	 printf("URL: %s\n"
		"   SLPError      = %i\n\n",
		url,
		error);
      }
   }
   
   return 0;
}
