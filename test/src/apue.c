/*************************************************************************
    > File Name: apue.c
    > Author: ma6174
    > Mail: ma6174@163.com 
    > Created Time: Fri 24 Apr 2015 11:39:20 PM PDT
 ************************************************************************/

#include<stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>


#include "apue.h"

void err_sys(const char *fmt, ...)
{
	const size_t tmpStrLen = 1024;
	va_list ap;
	va_start(ap, fmt);
	char buf[tmpStrLen];
	vsnprintf(buf, tmpStrLen-1, fmt, ap);
	snprintf(buf+strlen(buf), tmpStrLen-strlen(buf)-1, " : %s", strerror(errno));
	strcat(buf, "\n");
	fflush(stdout);
	fputs(buf, stderr);
	fflush(NULL);
	va_end(ap);
	exit(1);
}
