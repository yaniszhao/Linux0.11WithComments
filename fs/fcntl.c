/*
 *  linux/fs/fcntl.c
 *
 *  (C) 1991  Linus Torvalds
 */

// 本文件实现了文件控制系统调用 fcntl() 和两个文件句柄（描述符）复制系统调用dup() 和 dup2() 。
// dup2() 指定了新句柄的数值，而 dup() 则返回当前值最小的未用句柄。
// 句柄复制操作主要用在文件的标准输入 / 输出重定向和管道操作方面。


#include <string.h>
#include <errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>

#include <fcntl.h>
#include <sys/stat.h>

extern int sys_close(int fd);

// 复制文件句柄(描述符)。
// 参数 fd 是欲复制的文件句柄，arg 指定新文件句柄的最小数值。
// 返回新文件句柄或出错码。
static int dupfd(unsigned int fd, unsigned int arg)
{
	// 如果文件句柄值大于一个程序最多打开文件数 NR_OPEN，
	// 或者该句柄的文件结构不存在，则出错，返回出错码并退出。
	if (fd >= NR_OPEN || !current->filp[fd])
		return -EBADF;
	// 如果指定的新句柄值 arg 大于最多打开文件数，则出错，返回出错码并退出。
	if (arg >= NR_OPEN)
		return -EINVAL;
	// 在当前进程的文件结构指针数组中寻找索引号大于等于 arg 但还没有使用的项。
	while (arg < NR_OPEN)
		if (current->filp[arg])
			arg++;
		else
			break;
	// 如果找到的新句柄值 arg 大于最多打开文件数，则出错，返回出错码并退出。
	if (arg >= NR_OPEN)
		return -EMFILE;
	// 在执行时关闭标志位图中复位该句柄位。也即在运行 exec()类函数时不关闭该句柄。
	current->close_on_exec &= ~(1<<arg);
	// 令该文件结构指针等于原句柄 fd 的指针，并将文件引用计数增 1。
	(current->filp[arg] = current->filp[fd])->f_count++;
	return arg;	// 返回新的文件句柄。
}

// 复制文件句柄系统调用函数。
// 复制指定文件句柄 oldfd，新句柄值等于 newfd。如果 newfd 已经打开，则首先关闭之。
int sys_dup2(unsigned int oldfd, unsigned int newfd)
{
	sys_close(newfd);
	return dupfd(oldfd,newfd);
}

// 复制文件句柄系统调用函数。
// 复制指定文件句柄 oldfd，新句柄的值是当前最小的未用句柄。
int sys_dup(unsigned int fildes)
{
	return dupfd(fildes,0);
}

// 文件控制系统调用函数。
// 参数 fd 是文件句柄，cmd 是操作命令(参见 include/fcntl.h，23-30 行)。
int sys_fcntl(unsigned int fd, unsigned int cmd, unsigned long arg)
{	
	struct file * filp;

	// 如果文件句柄值大于一个进程最多打开文件数 NR_OPEN，或者该句柄的文件结构指针为空，
	// 则出错，返回出错码并退出。
	if (fd >= NR_OPEN || !(filp = current->filp[fd]))
		return -EBADF;
	// 根据不同命令 cmd 进行分别处理。
	switch (cmd) {
		case F_DUPFD:	// 复制文件句柄。
			return dupfd(fd,arg);
		case F_GETFD:	// 取文件句柄的执行时关闭标志。
			return (current->close_on_exec>>fd)&1;
		case F_SETFD:	// 设置句柄执行时关闭标志。arg 位 0 置位是设置，否则关闭。
			if (arg&1)
				current->close_on_exec |= (1<<fd);
			else
				current->close_on_exec &= ~(1<<fd);
			return 0;
		case F_GETFL:	// 取文件状态标志和访问模式。
			return filp->f_flags;
		case F_SETFL:	// 设置文件状态和访问模式(根据 arg 设置添加、非阻塞标志)。
			filp->f_flags &= ~(O_APPEND | O_NONBLOCK);
			filp->f_flags |= arg & (O_APPEND | O_NONBLOCK);
			return 0;
		case F_GETLK:	case F_SETLK:	case F_SETLKW:	// 未实现。
			return -1;
		default:
			return -1;
	}
}
