#include "slp_net.h"
#include "slp_win32.h"

// return 1 if successful, 0 if fails, and -1 if af argument is unknown
int inet_pton(int af, const char *src, void *dst) {
    int sts = 0;
    struct addrinfo *res;
    struct addrinfo hints;

    memset (&hints, 0, sizeof(hints));
    if (af == AF_INET) {
        struct in_addr *d4Dst = (struct in_addr *) dst;
        struct in_addr *d4Src;
        hints.ai_protocol = AF_INET;
        sts = getaddrinfo(src, NULL, &hints, &res);
        d4Src = &res->ai_addr->sa_data[2];
        memcpy(&d4Dst->S_un, &d4Src->S_un, 4); 
    }
    else if (af == AF_INET6) {
        struct in6_addr *d6Dst = (struct in6_addr *) dst;
        struct in6_addr *d6Src;
        hints.ai_protocol = AF_INET6;
        sts = getaddrinfo(src, NULL, &hints, &res);
        d6Src = res->ai_addr;
        memcpy(&d6Dst->u, &d6Src->u, 16); 
    }
    else {
        sts = -1;
    }
    return(sts);
}

const char *inet_ntop(int af, const void *src, char *dst, size_t size) {
    char tmp[5];
    DWORD i;

    if (size > 0) {
        dst[0] = '\0';
        if (af == AF_INET) {
            struct in_addr *d4 = (struct in_addr *) src;
            itoa(d4->S_un.S_un_b.s_b1, tmp, 10);
            strncat(dst, tmp, size - (strlen(dst) + 1));
            strncat(dst, ".", size - (strlen(dst) + 1));
            itoa(d4->S_un.S_un_b.s_b2, tmp, 10);
            strncat(dst, tmp, size - (strlen(dst) + 1));
            strncat(dst, ".", size - (strlen(dst) + 1));
            itoa(d4->S_un.S_un_b.s_b3, tmp, 10);
            strncat(dst, tmp, size - (strlen(dst) + 1));
            strncat(dst, ".", size - (strlen(dst) + 1));
            itoa(d4->S_un.S_un_b.s_b4, tmp, 10);
            strncat(dst, tmp, size - (strlen(dst) + 1));
        }
        else if (af == AF_INET6) {
            struct in_addr6 *d6 = (struct in6_addr *) src;
            for (i = 0; i < 7; i++) {
                itoa(d6->u.Byte[2 * i], tmp, 16);
                strncat(dst, tmp, size - (strlen(dst) + 1));
                itoa(d6->u.Byte[(2 * i) + 1], tmp, 16);
                strncat(dst, tmp, size - (strlen(dst) + 1));
                strncat(dst, ":", size - (strlen(dst) + 1));
            }
            // now do the last one
            itoa(d6->u.Byte[14], tmp, 16);
            strncat(dst, tmp, size - (strlen(dst) + 1));
            itoa(d6->u.Byte[15], tmp, 16);
            strncat(dst, tmp, size - (strlen(dst) + 1));
        }
    }
    return(dst);
}
