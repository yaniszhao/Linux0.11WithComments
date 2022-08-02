/*
 *  linux/fs/stat.c
 *
 *  (C) 1991  Linus Torvalds
 */

//该程序实现取文件状态信息系统调用 stat() 和 fstat() ，并将信息存放在用户的文件状态结构缓冲区中。
//stat() 是利用文件名取信息，而 fstat() 是使用文件句柄 ( 描述符 ) 来取信息。

#include <errno.h>
#include <sys/stat.h>

#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>

// 复制文件状态信息。
// 参数 inode 是文件对应的 i 节点，statbuf 是 stat 文件状态结构指针，用于存放取得的状态信息。
static void cp_stat(struct m_inode * inode, struct stat * statbuf)
{
	struct stat tmp;
	int i;

	verify_area(statbuf,sizeof (* statbuf));
	tmp.st_dev = inode->i_dev;			// 文件所在的设备号。
	tmp.st_ino = inode->i_num;			// 文件 i 节点号。
	tmp.st_mode = inode->i_mode;		// 文件属性。
	tmp.st_nlink = inode->i_nlinks;		// 文件的连接数。
	tmp.st_uid = inode->i_uid;			// 文件的用户 id。
	tmp.st_gid = inode->i_gid;			// 文件的组 id。
	tmp.st_rdev = inode->i_zone[0];		// 设备号(如果文件是特殊的字符文件或块文件)。
	tmp.st_size = inode->i_size;		// 文件大小（字节数）（如果文件是常规文件）。
	tmp.st_atime = inode->i_atime;		// 最后访问时间。
	tmp.st_mtime = inode->i_mtime;		// 最后修改时间。
	tmp.st_ctime = inode->i_ctime;		// 最后节点修改时间。
	// 最后将这些状态信息复制到用户缓冲区中。
	for (i=0 ; i<sizeof (tmp) ; i++)
		put_fs_byte(((char *) &tmp)[i],&((char *) statbuf)[i]);
}

// 文件状态系统调用函数 - 根据文件名获取文件状态信息。
// 参数 filename 是指定的文件名，statbuf 是存放状态信息的缓冲区指针。
// 返回 0，若出错则返回出错码。
int sys_stat(char * filename, struct stat * statbuf)
{
	struct m_inode * inode;

	if (!(inode=namei(filename)))
		return -ENOENT;
	cp_stat(inode,statbuf);
	iput(inode);
	return 0;
}

// 文件状态系统调用 - 根据文件句柄获取文件状态信息。
// 参数 fd 是指定文件的句柄(描述符)，statbuf 是存放状态信息的缓冲区指针。
// 返回 0，若出错则返回出错码。
int sys_fstat(unsigned int fd, struct stat * statbuf)
{
	struct file * f;
	struct m_inode * inode;

	// 如果文件句柄值大于一个程序最多打开文件数 NR_OPEN，或者该句柄的文件结构指针为空，
	// 或者对应文件结构的 i 节点字段为空，则出错，返回出错码并退出。
	if (fd >= NR_OPEN || !(f=current->filp[fd]) || !(inode=f->f_inode))
		return -EBADF;
	cp_stat(inode,statbuf);
	return 0;
}
