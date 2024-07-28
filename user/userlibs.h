#ifndef __USERLIBS_H__
#define __USERLIBS_H__

#include <stdarg.h>


#define	PAGESIZE	(1UL << 21)
#define	PAGE_SIZE	PAGESIZE

// ====================== printf
#define ZEROPAD	1		/* pad with zero */
#define SIGN	2		/* unsigned/signed long */
#define PLUS	4		/* show plus */
#define SPACE	8		/* space if plus */
#define LEFT	16		/* left justified */
#define SPECIAL	32		/* 0x */
#define SMALL	64		/* use 'abcdef' instead of 'ABCDEF' */

#define is_digit(c)	((c) >= '0' && (c) <= '9')

#define do_div(n,base) ({ \
int __res; \
__asm__("divq %%rcx":"=a" (n),"=d" (__res):"0" (n),"1" (0),"c" (base)); \
__res; })


// ====================== malloc

#define	SIZE_ALIGN	(8 * sizeof(unsigned long))
#define	PAGESIZE	(1UL << 21)
#define	PAGE_SIZE	PAGESIZE

// ====================== 
#define	SYSTEM_REBOOT	(1UL << 0)
#define	SYSTEM_POWEROFF	(1UL << 1)

unsigned long reboot(unsigned long cmd,void * arg);


// ====================== 
struct dirent
{
	long d_offset;
	long d_type;
	long d_namelen;
	char d_name[];
};

struct DIR
{
	int fd;
	int buf_pos;
	int buf_end;
	char buf[256];
};

int getdents(int fd,struct dirent *buf,long count);

struct DIR* opendir(const char *path);
int closedir(struct DIR *dir);
struct dirent *readdir(struct DIR *dir);

//////////////////////////////////////////////////
//					extern vars					//
//////////////////////////////////////////////////
extern int errno;


//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

int wait(int * status);
int waitpid(int pid, int *status, int options);
// ===========================

/*
		From => To memory copy Num bytes
*/

inline void * memcpy(void *From,void * To,long Num);


/*
		FirstPart = SecondPart		=>	 0
		FirstPart > SecondPart		=>	 1
		FirstPart < SecondPart		=>	-1
*/

inline int memcmp(void * FirstPart,void * SecondPart,long Count);

/*
		set memory at Address with C ,number is Count
*/

inline void * memset(void * Address,unsigned char C,long Count);

/*
		string copy
*/

inline char * strcpy(char * Dest,char * Src);


/*
		string copy number bytes
*/

inline char * strncpy(char * Dest,char * Src,long Count);


/*
		string cat Dest + Src
*/

inline char * strcat(char * Dest,char * Src);


/*
		string compare FirstPart and SecondPart
		FirstPart = SecondPart =>  0
		FirstPart > SecondPart =>  1
		FirstPart < SecondPart => -1
*/

inline int strcmp(char * FirstPart,char * SecondPart);


/*
		string compare FirstPart and SecondPart with Count Bytes
		FirstPart = SecondPart =>  0
		FirstPart > SecondPart =>  1
		FirstPart < SecondPart => -1
*/

inline int strncmp(char * FirstPart,char * SecondPart,long Count);
/*

*/

inline int strlen(char * String);

// ==============================

int printf(const char *fmt, ...);

// ====================== malloc

void * malloc(long size);


#endif

