/***************************************************
* 
* r3 user test app for shell exec
*
***************************************************/


#include "../user/userlibs.h"

int main(int argc,char *argv[])
{
	int i = 0;
	printf("\nHello World!	_from user test app\n");
	printf("argc:%d,argv:%#018lx\n",argc,argv);
	for(i = 0;i<argc;i++)
		printf("argv[%d]:%s\n",i,argv[i]);
	exit(0);
	return 0;
}

