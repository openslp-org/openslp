#ifndef LOCAL_ACCONFIG_H
#define LOCAL_ACCONFIG_H

/* Local acconfig.h. Add a line for every new AC_DEFINE that is
   included in configure.in  */

/* defined if struct sigaction has member sa_restorer */
#undef HAVE_SA_RESTORER

/* defined to size_t if <sys/socket.h> does not support socklen_t data type */
#undef socklen_t

/* defined if predicates are enabled */
#undef USE_PREDICATES

/* defined if SLPv1 support is enabled */
#undef ENABLE_SLPv1

/* defined if the async SLP API support is enabled */
#undef ENABLE_ASYNC_API

/* defined if the SLPv2 authenticationsupport is enabled */
#undef ENABLE_AUTHENTICATION

#endif
