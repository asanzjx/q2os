/***************************************************
*
* user init app
*
***************************************************/

#include "syscall.h"
#include "userlibs.h"
#include "keyboard.h"
#include "shell.h"




//////////////////////////////////////////////////
//					Constants					//
//////////////////////////////////////////////////
#define TEST_FILE_RW	1
#define TEST_MALLOC	1
// #define TEST_KEYBOARD_DEVICE	1
#define UserDebugPrint(buf)	\
	printf("[%s]\t", __func__);	\
	printf(buf);	\
	__asm__ __volatile__("xchgw %bx, %bx")



//////////////////////////////////////////////////
//					extern vars					//
//////////////////////////////////////////////////
extern struct buildincmd shell_internal_cmd[];
extern char *current_dir;

//////////////////////////////////////////////////
//					Global vars					//
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////



// must in the first, or after ld, error
int main()
{
	
	char hello_strs[] = "\n[%s]Hello, World _from user init\n";
	putstring(hello_strs);

	// Test: file open / close / read /write
/*
	char string[]="/123/B.TXT";
	unsigned char buf[32] = {0};
	int fd = open(string,0);
	write(fd,string,20);
	lseek(fd,5,SEEK_SET);
	read(fd,buf,30);
	close(fd);
	putstring(buf);
*/

	// Test: fork and memory test
/*
	if(fork() == 0){
		putstring("child process\n");
		// free function need update
		void *child_addr = malloc(200);
		free(child_addr);
	}
	else{
		putstring("parent process\n");
		void *parrent_addr = malloc(100);
		free(parrent_addr);
		parrent_addr = malloc(300);
		free(parrent_addr);
	}
	while(1);
*/

#if TEST_KEYBOARD_DEVICE
	char device_path[] = "/KEYBOARD.DEV";
	int device_fd = open(device_path, 0);

	printf("[user:%s] test keyboard devuce start..............", __func__);
	int fd0 = 50;
	int key = 0;
	while(fd0--){
		// printf("[user:%s] test keyboard devuce start analysis key..............", __func__);

		// logical address out of bounds (32212304278/4193280) - aborting command
		key = analysis_keycode(device_fd);
		if(key)
			printf("[user:%s]key:%c\n", __func__, key);
		// __asm__ __volatile__("xchgw %bx, %bx");
	}
	close(device_fd);

	printf("[user:%s] test keyboard devuce end..............", __func__);
	__asm__ __volatile__("xchgw %bx, %bx");

	while(1);
#endif

	// --- shell
	int fd = 0;
	unsigned char buf[256] = {0};
	char path[] = "/KEYBOARD.DEV";
	int index = -1;

	current_dir = "/";
	fd = open(path,0);

	printf("\n[+]================== Q2OS SHELL ==================[+]\n");	
	while(1)
	{
		int argc = 0;
		char ** argv = NULL;
		printf(">>> [SHELL] $ ");
		memset(buf,0,256);

		// UserDebugPrint("ttttttttttt");

		int key = 0;
		int count = 0;

		while(1)
		{
			key = analysis_keycode(fd);
			if(key == '\n')
				break;
			else if(key)
			{
				buf[count++] = key;
				printf("%c",key);
			}			
			else
				continue;
		}
		printf("\n");

		index = parse_command(buf,&argc,&argv);

		if(index < 0)
			printf("Input Error,No Command Found!\n");
		else
			run_command(index,argc,argv);	//argc,argv

	}
	close(fd);
	
	return 0;
}



