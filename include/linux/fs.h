/*
 * This file has definitions for some important file table
 * structures etc.
 */
 /* 本文件含有某些重要文件表结构的定义等。*/

#ifndef _FS_H
#define _FS_H

#include <sys/types.h>

/* devices are as follows: (same as minix, so we can use the minix
 * file system. These are major numbers.)
 *
 * 0 - unused (nodev)
 * 1 - /dev/mem
 * 2 - /dev/fd
 * 3 - /dev/hd
 * 4 - /dev/ttyx
 * 5 - /dev/tty
 * 6 - /dev/lp
 * 7 - unnamed pipes
 */
/*
 * 系统所含的设备如下：（与 minix 系统的一样，所以我们可以使用 minix 的
 * 文件系统。以下这些是主设备号。）
 *
 * 0 - 没有用到（nodev）
 * 1 - /dev/mem 内存设备。
 * 2 - /dev/fd 软盘设备。
 * 3 - /dev/hd 硬盘设备。
 * 4 - /dev/ttyx tty 串行终端设备。
 * 5 - /dev/tty tty 终端设备。
 * 6 - /dev/lp 打印设备。
 * 7 - unnamed pipes 没有命名的管道。
 */

#define IS_SEEKABLE(x) ((x)>=1 && (x)<=3)	// 是否是可以寻找定位的设备。

#define READ 0
#define WRITE 1
#define READA 2		/* read-ahead - don't pause */
#define WRITEA 3	/* "write-ahead" - silly, but somewhat useful */

void buffer_init(long buffer_end);

//设备号 = 主设备号 *256 + 次设备号
//也即 dev_no = (major<<8) + minor
//从 linux 内核 0.95 版后已经不使用这种烦琐的命名方式，而是使用与现在相同的命名方法了。
#define MAJOR(a) (((unsigned)(a))>>8)	// 取高字节（主设备号）。
#define MINOR(a) ((a)&0xff)				// 取低字节（次设备号）。

#define NAME_LEN 14		// 文件名字长度值。
#define ROOT_INO 1		// 根 i 节点。

#define I_MAP_SLOTS 8		// i 节点位图槽数。
#define Z_MAP_SLOTS 8		// 逻辑块（区段块）位图槽数。
#define SUPER_MAGIC 0x137F	// 文件系统魔数。

#define NR_OPEN 20
#define NR_INODE 32
#define NR_FILE 64
#define NR_SUPER 8
#define NR_HASH 307
#define NR_BUFFERS nr_buffers
#define BLOCK_SIZE 1024			// 数据块长度。
#define BLOCK_SIZE_BITS 10		// 数据块长度所占比特位数。
#ifndef NULL
#define NULL ((void *) 0)
#endif

// 每个逻辑块可存放的 i 节点数。
#define INODES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct d_inode)))
// 每个逻辑块可存放的目录项数。
#define DIR_ENTRIES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct dir_entry)))

// 管道头、管道尾、管道大小、管道空？、管道满？、管道头指针递增。
#define PIPE_HEAD(inode) ((inode).i_zone[0])
#define PIPE_TAIL(inode) ((inode).i_zone[1])
#define PIPE_SIZE(inode) ((PIPE_HEAD(inode)-PIPE_TAIL(inode))&(PAGE_SIZE-1))
#define PIPE_EMPTY(inode) (PIPE_HEAD(inode)==PIPE_TAIL(inode))
#define PIPE_FULL(inode) (PIPE_SIZE(inode)==(PAGE_SIZE-1))
#define INC_PIPE(head) \
__asm__("incl %0\n\tandl $4095,%0"::"m" (head))

typedef char buffer_block[BLOCK_SIZE];	// 块缓冲区。

struct buffer_head {
	char * b_data;			/* pointer to data block (1024 bytes) */	//指向真正的数据块
	unsigned long b_blocknr;	/* block number */				//对应设备的块号
	unsigned short b_dev;		/* device (0 = free) */			//对应设备
	unsigned char b_uptodate;									//是否从设备中更新数据进buffer
	unsigned char b_dirt;		/* 0-clean,1-dirty */			//脏位
	unsigned char b_count;		/* users using this block */	//被多少用户使用
	unsigned char b_lock;		/* 0 - ok, 1 -locked */			//是否被锁定
	struct task_struct * b_wait;								//等待使用进程队列
	struct buffer_head * b_prev;								//同一设备已分配的上一个块
	struct buffer_head * b_next;								//同一设备已分配的下一个块
	struct buffer_head * b_prev_free;							//上一个空闲块
	struct buffer_head * b_next_free;							//下一个空闲块
};

// 磁盘上的索引节点(i 节点)数据结构。
struct d_inode {
	unsigned short i_mode;
	unsigned short i_uid;
	unsigned long i_size;
	unsigned long i_time;
	unsigned char i_gid;
	unsigned char i_nlinks;
	unsigned short i_zone[9];
};

// 这是在内存中的 i 节点结构。前 7 项与 d_inode 完全一样。
struct m_inode {
	unsigned short i_mode;			//文件的类型和属性rwx
	unsigned short i_uid;			//文件宿主的用户id
	unsigned long i_size;			//文件长度(字节)
	unsigned long i_mtime;			//修改时间(从1970年算起，单位秒)
	unsigned char i_gid;			//文件宿主的组id
	unsigned char i_nlinks;			//链接数，即多少文件目录项指向该i结点
	unsigned short i_zone[9];		//文件所占盘上逻辑块号数组。0-6直接块；7一次间接；8二次间接。
/* these are in memory also */
	struct task_struct * i_wait;	//等待该 i 节点的进程。
	unsigned long i_atime;			//访问时间
	unsigned long i_ctime;			//创建时间，自身修改时间
	unsigned short i_dev;			//对应设备
	unsigned short i_num;			//i结点号
	unsigned short i_count;			//在内核中被打开次数，0 表示该 i 节点空闲。
	unsigned char i_lock;			//是否上锁
	unsigned char i_dirt;			//脏位
	unsigned char i_pipe;			//是否是管道文件
	unsigned char i_mount;			// 安装标志。是否是mount点。
	unsigned char i_seek;			// 搜寻标志(lseek 时)。
	unsigned char i_update;			// 更新标志。
};

// 文件结构（用于在文件句柄与 i 节点之间建立关系）
struct file {
	unsigned short f_mode;		//文件操作模式rw
	unsigned short f_flags;		//文件打开和控制的标志
	unsigned short f_count;		//对应文件描述符数
	struct m_inode * f_inode;	//指向对应i结点
	off_t f_pos;				//文件当前读写指针位置
};

// 内存中磁盘超级块结构。
struct super_block {
	unsigned short s_ninodes;				//i结点数
	unsigned short s_nzones;				//逻辑块数 或 区块数
	unsigned short s_imap_blocks;			//i结点位图所占块数
	unsigned short s_zmap_blocks;			//逻辑块位图所占块数
	unsigned short s_firstdatazone;			//第一个逻辑块号
	unsigned short s_log_zone_size;			//log2(数据块数/逻辑块)
	unsigned long s_max_size;				//最大文件长度
	unsigned short s_magic;					//文件系统幻数
/* These are only in memory */
	struct buffer_head * s_imap[8];			//i结点位图在高速缓冲块指针数组
	struct buffer_head * s_zmap[8];			//逻辑块位图在高速缓冲块指针数组
	unsigned short s_dev;					//超级块所在设备号
	struct m_inode * s_isup;				//被安装文件系统根目录i结点
	struct m_inode * s_imount;				//该文件系统被安装到的i结点
	unsigned long s_time;					//修改时间
	struct task_struct * s_wait;			//等待本超级快的进程指针
	unsigned char s_lock;					//锁定标志
	unsigned char s_rd_only;				//只读标志
	unsigned char s_dirt;					//脏位
};

// 磁盘上超级块结构。上面 125-132 行完全一样。
struct d_super_block {
	unsigned short s_ninodes;
	unsigned short s_nzones;
	unsigned short s_imap_blocks;
	unsigned short s_zmap_blocks;
	unsigned short s_firstdatazone;
	unsigned short s_log_zone_size;
	unsigned long s_max_size;
	unsigned short s_magic;
};

// 文件目录项结构。
//目录项中只有文件名和inode号可以提高查找速度
struct dir_entry {	
	unsigned short inode;
	char name[NAME_LEN];
};

extern struct m_inode inode_table[NR_INODE];		// 定义 i 节点表数组（32 项）。
extern struct file file_table[NR_FILE];				// 文件表数组（64 项）。
extern struct super_block super_block[NR_SUPER];	// 超级块数组（8 项）。
extern struct buffer_head * start_buffer;			// 缓冲区起始内存位置。
extern int nr_buffers;								// 缓冲块数。

// 磁盘操作函数原型。

extern void check_disk_change(int dev);
extern int floppy_change(unsigned int nr);
extern int ticks_to_floppy_on(unsigned int dev);
extern void floppy_on(unsigned int dev);
extern void floppy_off(unsigned int dev);

// 以下是文件系统操作管理用的函数原型。

extern void truncate(struct m_inode * inode);
extern void sync_inodes(void);
extern void wait_on(struct m_inode * inode);
extern int bmap(struct m_inode * inode,int block);
extern int create_block(struct m_inode * inode,int block);
extern struct m_inode * namei(const char * pathname);
extern int open_namei(const char * pathname, int flag, int mode,
	struct m_inode ** res_inode);
extern void iput(struct m_inode * inode);
extern struct m_inode * iget(int dev,int nr);
extern struct m_inode * get_empty_inode(void);
extern struct m_inode * get_pipe_inode(void);
extern struct buffer_head * get_hash_table(int dev, int block);
extern struct buffer_head * getblk(int dev, int block);
extern void ll_rw_block(int rw, struct buffer_head * bh);
extern void brelse(struct buffer_head * buf);
extern struct buffer_head * bread(int dev,int block);
extern void bread_page(unsigned long addr,int dev,int b[4]);
extern struct buffer_head * breada(int dev,int block,...);
extern int new_block(int dev);
extern void free_block(int dev, int block);
extern struct m_inode * new_inode(int dev);
extern void free_inode(struct m_inode * inode);
extern int sync_dev(int dev);
extern struct super_block * get_super(int dev);
extern int ROOT_DEV;

extern void mount_root(void);

#endif
