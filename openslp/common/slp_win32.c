#include "slp_net.h"
#include "slp_win32.h"

int inet_pton(int af, const char *src, void *dst) {
    return(0);
}

const char *inet_ntop(int af, const void *src, char *dst, size_t size) {
    return(src);
}
