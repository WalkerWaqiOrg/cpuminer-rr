#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <alloca.h>

#include "util.h"

int util::get_num_processors() {
    int result = 1;
#if defined(WIN32)
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	result = sysinfo.dwNumberOfProcessors;
#elif defined(_SC_NPROCESSORS_CONF)
	result = sysconf(_SC_NPROCESSORS_CONF);
#elif defined(CTL_HW) && defined(HW_NCPU)
	int req[] = {CTL_HW, HW_NCPU};
	size_t len = sizeof(result);
	sysctl(req, 2, &result, &len, NULL, 0);
#else
	result = 1;
#endif
    return result;
}

char* util::bin2hex(const unsigned char *p, size_t len) {
	int i;
	char *s = (char*)malloc((len * 2) + 1); // todo: malloc, release where?
	if (!s)
		return NULL;

	for (i = 0; i < len; i++)
		sprintf(s + (i * 2), "%02x", (unsigned int) p[i]);

	return s;
}

bool util::hex2bin(unsigned char *p, const char *hexstr, size_t len) {
	char hex_byte[3];
	char *ep;

	hex_byte[2] = '\0';

	while (*hexstr && len) {
		if (!hexstr[1]) {
			printf("hex2bin str truncated");
	        fflush(stdout);
			return false;
		}
		hex_byte[0] = hexstr[0];
		hex_byte[1] = hexstr[1];
		*p = (unsigned char) strtol(hex_byte, &ep, 16);
		if (*ep) {
			printf("hex2bin failed on '%s'", hex_byte);
	        fflush(stdout);
			return false;
		}
		p++;
		hexstr += 2;
		len--;
	}

	return (len == 0 && *hexstr == 0) ? true : false;
}

#if ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3))
#define WANT_BUILTIN_BSWAP
#else
#define bswap_32(x) ((((x) << 24) & 0xff000000u) | (((x) << 8) & 0x00ff0000u) \
                   | (((x) >> 8) & 0x0000ff00u) | (((x) >> 24) & 0x000000ffu))
#endif

uint32_t util::swab32(uint32_t v) {
#ifdef WANT_BUILTIN_BSWAP
	return __builtin_bswap32(v);
#else
	return bswap_32(v);
#endif
}

void applog(const char * fmt, ...) {
	va_list ap;

	va_start(ap, fmt);

		char *f;
		int len;
		time_t now;
		struct tm tm, *tm_p;

		time(&now);

		tm_p = localtime(&now);
		memcpy(&tm, tm_p, sizeof(tm));

		len = 40 + strlen(fmt) + 2;
		f = (char *)alloca(len);
		sprintf(f, "[%d-%02d-%02d %02d:%02d:%02d] %s\n",
			tm.tm_year + 1900,
			tm.tm_mon + 1,
			tm.tm_mday,
			tm.tm_hour,
			tm.tm_min,
			tm.tm_sec,
			fmt);
		//vfprintf(stderr, f, ap);	/* atomic write to stderr */
		//fflush(stderr);

	//va_end(ap);
}

/* Subtract the `struct timeval' values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0.  */
int util::timeval_subtract(struct timeval *result, struct timeval *x,
	struct timeval *y)
{
	/* Perform the carry for the later subtraction by updating Y. */
	if (x->tv_usec < y->tv_usec) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) {
		int nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}

	/* Compute the time remaining to wait.
	 * `tv_usec' is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}
