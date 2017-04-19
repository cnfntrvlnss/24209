/*************************************************************************
    > File Name: myiofuncs.c
    > Author: ma6174
    > Mail: ma6174@163.com 
    > Created Time: Sat 18 Jul 2015 09:36:47 PM PDT
 ************************************************************************/

#include<stdio.h>
#include <unistd.h>
#include <setjmp.h>
#include <signal.h>

static sigjmp_buf  jmpbuf;
static volatile int canjump;
static void myfun(int signo)
{
	if(canjump == 0) return;
	canjump = 0;
	siglongjmp(jmpbuf, 1);
}
char *fgetstimeout(char *buf, size_t num, size_t secs)
{
	unsigned int prealrmnum = 0;
	void (*oldhder)(int) = signal(SIGALRM, myfun);
	char *retbuf = NULL;
	if(sigsetjmp(jmpbuf, 1) == 0)
	{
		canjump = 1;
		prealrmnum = alarm(secs);
		retbuf  = fgets(buf, num, stdin);
		alarm(0);
	}
	signal(SIGALRM, oldhder);
	alarm(prealrmnum);
	return retbuf;
}

#if 0
int main()
{
	char buf[216];
	fprintf(stdout, "you should input some thing within 3 secs: ");
	char * retbuf = fgetstimeout(buf, 216, 3);
	if(retbuf == NULL){
		fprintf(stdout, "\ntime out!\n");
	}
	else{
		fprintf(stdout, "%s", retbuf);
	}
	return 0;
}
#endif
