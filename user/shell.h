#ifndef __SHELL_H__
#define __SHELL_H__

struct	buildincmd
{
	char *name;
	int (*function)(int,char**);
};



extern int shift_l,shift_r,ctrl_l,ctrl_r,alt_l,alt_r;
extern unsigned char pausebreak_scode[];

extern unsigned int keycode_map_normal[NR_SCAN_CODES * MAP_COLS];

int cd_command(int argc,char **argv);

int ls_command(int argc,char **argv);

int pwd_command(int argc,char **argv);

int cat_command(int argc,char **argv);

int touch_command(int argc,char **argv);

int rm_command(int argc,char **argv);

int mkdir_command(int argc,char **argv);

int rmdir_command(int argc,char **argv);

int exec_command(int argc,char **argv);

int reboot_command(int argc,char **argv);

void run_command(int index,int argc,char **argv);

int parse_command(char * buf,int * argc,char ***argv);

unsigned char get_scancode(int fd);

int analysis_keycode(int fd);

#endif
