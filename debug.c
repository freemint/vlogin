#include <stdio.h>
#include <stdarg.h>

#include "debug.h"

FILE *fp;

void debug_print(char *s, ...) {
	char buffer[1024];

	/* construct a single string */
	va_list a;
	va_start(a, s);
	vsprintf(buffer, s, a);
	va_end(a);

#if defined(NF_DEBUG)

	{
		/* general NatFeat stuff */
		static long NF_getid = 0x73004e75L;
		static long NF_call  = 0x73014e75L;
#  define nfGetID(n)	(((long(*)(const char *))&NF_getid)n)
#  define nfCall(n)	(((long(*)(long, ...))&NF_call)n)

		nfCall((nfGetID(("NF_STDERR")),buffer));
	}
#else
	// print it whole out to the log file
	fprintf(fp,"%s", buffer);
//	fprintf(stderr, "%s", buffer);
#endif
}


void InitDebug(void) {
#if defined(DEBUG)
	fp = fopen("/c/vlogin.log", "w");
#endif
}

void ExitDebug(void) {
#if defined(DEBUG)
	fclose(fp);
#endif
}

