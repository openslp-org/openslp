#include "slp_net.h"
#include "slp_win32.h"

// return 1 if successful, 0 if fails, and -1 if af argument is unknown
int inet_pton(int af, const char *src, void *dst) {
    int sts = 0;
    struct addrinfo *res;
    struct addrinfo hints;

    memset (&hints, 0, sizeof(hints));
    if (af == PF_INET) {
        struct in_addr *d4 = (struct in_addr *) dst;
        hints.ai_protocol = PF_INET;
        sts = getaddrinfo(src, NULL, NULL, &res);
        memcpy(&d4->S_un, res->ai_addr, min(sizeof(d4->S_un), res->ai_addrlen)); 
    }
    else if (af == PF_INET6) {
        struct in6_addr *d6 = (struct in6_addr *) dst;
        hints.ai_protocol = PF_INET6;
        sts = getaddrinfo(src, NULL, NULL, &res);
        memcpy(&d6->u, res->ai_addr, min(sizeof(d6->u), res->ai_addrlen)); 
    }
    else {
        sts = -1;
    }
    return(sts);
}

const char *inet_ntop(int af, const void *src, char *dst, size_t size) {
    return(src);
}
