/***************************************************
*
* syscall implemet in kernel
***************************************************/

#include "sys.h"
#include "lib.h"
#include "printk.h"
#include "VFS.h"
#include "memory.h"
#include "fat32.h"
#include "task.h"
#include "keyboard.h"

/*
normal
system call number:	rax
arg1:	rdi
arg2:	rsi
arg3:	rdx
arg4:	rcx
arg5:	r8
arg6:	r9

sysenter need rdx(rip) rcx(rsp)
syscall need rcx(rip) r11(rflags)

xchg rdx to r10, rcx to r11

*/

extern struct file *g_file_struct[TASK_FILE_MAX];
extern struct file_operations keyboard_fops;

unsigned long no_system_call(void)
{
	color_printk(RED,BLACK,"no_system_call is calling\n");
	return -ENOSYS;
}

/*	
__asm__	(
".global puts	\n\t"
".type	puts,	@function \n\t"
"puts:		\n\t"
"pushq	%r10	\n\t"
"pushq	%r11	\n\t"
"movq	$__NR_puts	,	%rax	\n\t"
"leaq	sysexit_return_address(%rip),	%r10	\n\t"
"movq	%rsp,	%r11		\n\t"
"sysenter			\n\t"
"sysexit_return_address:	\n\t"
"xchgq	%rdx,	%r10	\n\t"
"xchgq	%rcx,	%r11	\n\t"
"popq	%r11	\n\t"
"popq	%r10	\n\t"
);	
*/


unsigned long sys_putstring(char *string)
{
	// color_printk(GREEN,BLACK,"sys_putstring\n");
	color_printk(ORANGE,WHITE,string);
	return 0;
}

unsigned long sys_open(char *filename,int flags)
{
	char * path = NULL;
	long pathlen = 0;
	long error = 0;
	struct dir_entry * dentry = NULL;
	struct file * filp = NULL;
	struct file ** f = NULL;
	int fd = -1;
	int i;

	color_printk(GREEN,BLACK,"sys_open\n");
	path = (char *)kmalloc(PAGE_4K_SIZE,0);
	if(path == NULL)
		return -ENOMEM;
	memset(path,0,PAGE_4K_SIZE);
	pathlen = strnlen_user(filename,PAGE_4K_SIZE);
	if(pathlen <= 0)
	{
		kfree(path);
		return -EFAULT;
	}	
	else if(pathlen >= PAGE_4K_SIZE)
	{
		kfree(path);
		return -ENAMETOOLONG;
	}
	strncpy_from_user(filename,path,pathlen);

#if DEBUG
	// color_printk(RED, WHITE, "\n[DEBUG]%s: \n", __func__);
	// BochsMagicBreakpoint();
#endif
	dentry = path_walk(path,0);
	// kfree(path);

/////////////////
	if(dentry != NULL)
		color_printk(BLUE,WHITE,"\tFind file\n\tDIR_FirstCluster:%#018lx\tDIR_FileSize:%#018lx\n",((struct FAT32_inode_info *)(dentry->dir_inode->private_index_info))->first_cluster,dentry->dir_inode->file_size);
	else
		color_printk(BLUE,WHITE,"Can`t find file\n");

	if(dentry == NULL)
		return -ENOENT;

	if((flags & O_DIRECTORY) && (dentry->dir_inode->attribute != FS_ATTR_DIR))
		return -ENOTDIR;
	if(!(flags & O_DIRECTORY) && (dentry->dir_inode->attribute == FS_ATTR_DIR))
		return -EISDIR;

	// create device file
	// !!!commit after created
/*
	if(0 == strncmp(path, "/KEYBOARD.DEV", 13)){
		((struct FAT32_inode_info *)(dentry->dir_inode->private_index_info))->first_cluster |= 0xf0000000;
		dentry->dir_inode->sb->sb_ops->write_inode(dentry->dir_inode);
#if DEBUG
	color_printk(RED, WHITE, "\n[DEBUG]%s: open dev - %s\n", __func__, path);
	BochsMagicBreakpoint();
#endif
	}
*/
	kfree(path);
	

	filp = (struct file *)kmalloc(sizeof(struct file),0);
	memset(filp,0,sizeof(struct file));
	filp->dentry = dentry;
	filp->mode = flags;

	if(dentry->dir_inode->attribute & FS_ATTR_DEVICE)
		filp->f_ops = &keyboard_fops;	//////	find device file operation function
	else
		filp->f_ops = dentry->dir_inode->f_ops;
	if(filp->f_ops && filp->f_ops->open)
		error = filp->f_ops->open(dentry->dir_inode,filp);
	if(error != 1)
	{
		kfree(filp);
		return -EFAULT;
	}

	if(filp->mode & O_TRUNC)
	{
		filp->dentry->dir_inode->file_size = 0;
	}
	if(filp->mode & O_APPEND)
	{
		filp->position = filp->dentry->dir_inode->file_size;
	}

	f = g_file_struct;
	for(i = 0;i < TASK_FILE_MAX;i++)
		if(f[i] == NULL)
		{
			fd = i;
			break;
		}
	if(i == TASK_FILE_MAX)
	{
		kfree(filp);
		//// reclaim struct index_node & struct dir_entry
		return -EMFILE;
	}
	f[fd] = filp;

	return fd;
}

unsigned long sys_close(int fd)
{
	struct file * filp = NULL;

	color_printk(GREEN,BLACK,"sys_close:%d\n",fd);
	if(fd < 0 || fd >= TASK_FILE_MAX)
		return -EBADF;

	filp = g_file_struct[fd];
	if(filp->f_ops && filp->f_ops->close)
		filp->f_ops->close(filp->dentry->dir_inode,filp);

	kfree(filp);
	g_file_struct[fd] = NULL;

	return 0;
}

unsigned long sys_read(int fd,void * buf,long count)
{
	struct file * filp = NULL;
	unsigned long ret = 0;

	// color_printk(GREEN,BLACK,"sys_read:%d\n",fd);
	if(fd < 0 || fd >= TASK_FILE_MAX)
		return -EBADF;
	if(count < 0)
		return -EINVAL;

	filp = g_file_struct[fd];
	if(NULL == filp){
		color_printk(GREEN,BLACK,"[-]%s filp is null!!!\n", __func__);
		BochsMagicBreakpoint();
		return -1;
	}
	if(filp->f_ops && filp->f_ops->read)
		ret = filp->f_ops->read(filp,buf,count,&filp->position);
	return ret;
}

unsigned long sys_write(int fd,void * buf,long count)
{
	struct file * filp = NULL;
	unsigned long ret = 0;

	// color_printk(GREEN,BLACK,"sys_write:%d\n",fd);
	if(fd < 0 || fd >= TASK_FILE_MAX)
		return -EBADF;
	if(count < 0)
		return -EINVAL;

	filp = g_file_struct[fd];
	if(filp->f_ops && filp->f_ops->write)
		ret = filp->f_ops->write(filp,buf,count,&filp->position);
	return ret;
}


unsigned long sys_lseek(int filds,long offset,int whence)
{
	struct file * filp = NULL;
	unsigned long ret = 0;

	color_printk(GREEN,BLACK,"sys_lseek:%d\n",filds);
	if(filds < 0 || filds >= TASK_FILE_MAX)
		return -EBADF;
	if(whence < 0 || whence >= SEEK_MAX)
		return -EINVAL;

	filp = g_file_struct[filds];
	if(filp->f_ops && filp->f_ops->lseek)
		ret = filp->f_ops->lseek(filp,offset,whence);
	return ret;
}

unsigned long sys_fork()
{
	struct pt_regs *regs = (struct pt_regs *)current->thread->rsp0 -1;
	color_printk(GREEN,BLACK,"sys_fork\n");
	return do_fork(regs,0,regs->rsp,0);	
}

unsigned long sys_vfork()
{
	struct pt_regs *regs = (struct pt_regs *)current->thread->rsp0 -1;
	color_printk(GREEN,BLACK,"sys_vfork\n");
	return do_fork(regs,CLONE_VM | CLONE_FS | CLONE_SIGNAL,regs->rsp,0);
}

unsigned long sys_execve()
{
	char * pathname = NULL;
	long pathlen = 0;
	long error = 0;
	struct pt_regs *regs = (struct pt_regs *)current->thread->rsp0 -1;

	color_printk(GREEN,BLACK,"sys_execve\n");
	pathname = (char *)kmalloc(PAGE_4K_SIZE,0);
	if(pathname == NULL)
		return -ENOMEM;
	memset(pathname,0,PAGE_4K_SIZE);
	pathlen = strnlen_user((char *)regs->rdi,PAGE_4K_SIZE);
	if(pathlen <= 0)
	{
		kfree(pathname);
		return -EFAULT;
	}
	else if(pathlen >= PAGE_4K_SIZE)
	{
		kfree(pathname);
		return -ENAMETOOLONG;
	}
	strncpy_from_user((char *)regs->rdi,pathname,pathlen);
	
	error = do_execve(regs,pathname,(char **)regs->rsi,NULL);

	kfree(pathname);
	return error;
}

unsigned long sys_exit(int exit_code)
{
	color_printk(GREEN,BLACK,"sys_exit\n");
	return do_exit(exit_code);
}

/*
	rusage reserved
*/

unsigned long sys_wait4(unsigned long pid,int *status,int options,void *rusage)
{
	long retval = 0;
	struct task_struct *child = NULL;
	struct task_struct *tsk = NULL;

	color_printk(GREEN,BLACK,"sys_wait4\n");
	for(tsk = &init_task_union.task;tsk->next != &init_task_union.task;tsk = tsk->next)
	{
		if(tsk->next->pid == pid)
		{
			child = tsk->next;
			break;
		}
	}

	if(child == NULL)
		return -ECHILD;
	if(options != 0)
		return -EINVAL;

	if(child->state == TASK_ZOMBIE)
	{
		copy_to_user(&child->exit_code,status,sizeof(int));
		tsk->next = child->next;
		exit_mm(child);
		kfree(child);
		return retval;
	}

	interruptible_sleep_on(&current->wait_childexit);

	copy_to_user(&child->exit_code,status,sizeof(long));
	tsk->next = child->next;
	exit_mm(child);
	kfree(child);
	return retval;
}

unsigned long sys_brk(unsigned long brk)
{
	unsigned long new_brk = PAGE_2M_ALIGN(brk);

	color_printk(GREEN,BLACK,"sys_brk\n");
	color_printk(RED,BLACK,"brk:%#018lx,new_brk:%#018lx,current->mm->end_brk:%#018lx\n",brk,new_brk,current->mm->end_brk);
	if(new_brk == 0)
		return current->mm->start_brk;
	if(new_brk < current->mm->end_brk)	//release  brk space
		return 0;	

	new_brk = do_brk(current->mm->end_brk,new_brk - current->mm->end_brk);	//expand brk space

	current->mm->end_brk = new_brk;
	return new_brk;
}

unsigned long sys_reboot(unsigned long cmd,void * arg)
{
	color_printk(GREEN,BLACK,"sys_reboot\n");
	switch(cmd)
	{
		case SYSTEM_REBOOT:
			io_out8(0x64,0xFE);
			break;

		case SYSTEM_POWEROFF:
			color_printk(RED,BLACK,"sys_reboot cmd SYSTEM_POWEROFF\n");
			break;

		default:
			color_printk(RED,BLACK,"sys_reboot cmd ERROR!\n");
			break;
	}
	return 0;
}


unsigned long sys_chdir(char *filename)
{
	char * path = NULL;
	long pathlen = 0;
	struct dir_entry * dentry = NULL;

	color_printk(GREEN,BLACK,"sys_chdir\n");
	path = (char *)kmalloc(PAGE_4K_SIZE,0);

	if(path == NULL)
		return -ENOMEM;
	memset(path,0,PAGE_4K_SIZE);
	pathlen = strnlen_user(filename,PAGE_4K_SIZE);
	if(pathlen <= 0)
	{
		kfree(path);
		return -EFAULT;
	}
	else if(pathlen >= PAGE_4K_SIZE)
	{
		kfree(path);
		return -ENAMETOOLONG;
	}
	strncpy_from_user(filename,path,pathlen);

	dentry = path_walk(path,0);
	kfree(path);
	
	if(dentry == NULL)
		return -ENOENT;
	if(dentry->dir_inode->attribute != FS_ATTR_DIR)
		return -ENOTDIR;
	return 0;
}

unsigned long sys_getdents(int fd, void * dirent, long count)
{
	struct file * filp = NULL;
	unsigned long ret = 0;

//	color_printk(GREEN,BLACK,"sys_getdents:%d\n",fd);
	if(fd < 0 || fd > TASK_FILE_MAX)
		return -EBADF;
	if(count < 0)
		return -EINVAL;

	// todo...need update
	filp = g_file_struct[fd];
	if(filp->f_ops && filp->f_ops->readdir)
		ret = filp->f_ops->readdir(filp,dirent,&fill_dentry);
	return ret;	
}



