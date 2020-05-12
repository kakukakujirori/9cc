#include "9cc.h"

char *strndup(const char *s, size_t n) {
    char* dst = (char*)malloc(n + 1);
	memcpy(dst, s, n);
    dst[n] = '\0';
    return dst;
}