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
        hints.ai_family = PF_INET;
        sts = getaddrinfo(src, NULL, &hints, &res);
        if (sts == 0) {
            struct addrinfo * aicheck = res;
            while (aicheck != NULL) {
                if (aicheck->ai_addr->sa_family == af) {
                    sts = 1;
                    d4Src = &(((struct sockaddr_in *)res->ai_addr)->sin_addr);
                    memcpy(&d4Dst->s_addr, &d4Src->s_addr, 4);
                    break;
                }
                else {
                    aicheck = aicheck->ai_next;
                }
            }
            /* if aicheck was NULL, sts will still be 0, if not, sts will be 1 */
        }
        else {
            sts = 0;
        }
    }
    else if (af == AF_INET6) {
        struct in6_addr *d6Dst = (struct in6_addr *) dst;
        struct in6_addr *d6Src;
        hints.ai_family = PF_INET6;
        sts = getaddrinfo(src, NULL, &hints, &res);
        if (sts == 0) {
            struct addrinfo * aicheck = res;
            while (aicheck != NULL) {
                if (aicheck->ai_addr->sa_family == af) {
                    sts = 1;
                    d6Src = &(((struct sockaddr_in6 *)res->ai_addr)->sin6_addr);
                    memcpy(&d6Dst->s6_addr, &d6Src->s6_addr, 16); 
                    break;
                }
                else {
                    aicheck = aicheck->ai_next;
                }
            }
            /* if aicheck was NULL, sts will still be 0, if not, sts will be 1 */
        }
        else {
            sts = 0;
        }
    }
    else {
        sts = -1;
    }
    return(sts);
}

const char *inet_ntop(int af, const void *src, char *dst, size_t size) {
    char tmp[25];
    DWORD i;

    if (size > 0) {
        dst[0] = '\0';
        if (af == AF_INET) {
            struct in_addr *d4 = (struct in_addr *) src;
            unsigned char *paddr = (unsigned char *)&d4->s_addr;
            itoa(paddr[0], tmp, 10);
            strncat(dst, tmp, size - (strlen(dst) + 1));
            strncat(dst, ".", size - (strlen(dst) + 1));
            itoa(paddr[1], tmp, 10);
            strncat(dst, tmp, size - (strlen(dst) + 1));
            strncat(dst, ".", size - (strlen(dst) + 1));
            itoa(paddr[2], tmp, 10);
            strncat(dst, tmp, size - (strlen(dst) + 1));
            strncat(dst, ".", size - (strlen(dst) + 1));
            itoa(paddr[3], tmp, 10);
            strncat(dst, tmp, size - (strlen(dst) + 1));
        }
        else if (af == AF_INET6) {
            struct in_addr6 *d6 = (struct in6_addr *) src;
            unsigned char *paddr = (unsigned char *)&(d6->s6_addr);
            for (i = 0; i < 7; i++) {
                sprintf(tmp, "%2.2X", paddr[2 * i]);
                strncat(dst, tmp, size - (strlen(dst) + 1));
                sprintf(tmp, "%2.2X", paddr[(2 * i) + 1]);
                strncat(dst, tmp, size - (strlen(dst) + 1));
                strncat(dst, ":", size - (strlen(dst) + 1));
            }
            // now do the last one
            sprintf(tmp, "%2.2X", paddr[14]);
            strncat(dst, tmp, size - (strlen(dst) + 1));
            sprintf(tmp, "%2.2X", paddr[15]);
            strncat(dst, tmp, size - (strlen(dst) + 1));
        }
    }
    return(dst);
}
