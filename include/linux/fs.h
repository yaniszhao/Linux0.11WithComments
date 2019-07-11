/*
 * This file has definitions for some important file table
 * structures etc.
 */
 /* ���ļ�����ĳЩ��Ҫ�ļ���ṹ�Ķ���ȡ�*/

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
 * ϵͳ�������豸���£����� minix ϵͳ��һ�����������ǿ���ʹ�� minix ��
 * �ļ�ϵͳ��������Щ�����豸�š���
 *
 * 0 - û���õ���nodev��
 * 1 - /dev/mem �ڴ��豸��
 * 2 - /dev/fd �����豸��
 * 3 - /dev/hd Ӳ���豸��
 * 4 - /dev/ttyx tty �����ն��豸��
 * 5 - /dev/tty tty �ն��豸��
 * 6 - /dev/lp ��ӡ�豸��
 * 7 - unnamed pipes û�������Ĺܵ���
 */

#define IS_SEEKABLE(x) ((x)>=1 && (x)<=3)	// �Ƿ��ǿ���Ѱ�Ҷ�λ���豸��

#define READ 0
#define WRITE 1
#define READA 2		/* read-ahead - don't pause */
#define WRITEA 3	/* "write-ahead" - silly, but somewhat useful */

void buffer_init(long buffer_end);

//�豸�� = ���豸�� *256 + ���豸��
//Ҳ�� dev_no = (major<<8) + minor
//�� linux �ں� 0.95 ����Ѿ���ʹ�����ַ�����������ʽ������ʹ����������ͬ�����������ˡ�
#define MAJOR(a) (((unsigned)(a))>>8)	// ȡ���ֽڣ����豸�ţ���
#define MINOR(a) ((a)&0xff)				// ȡ���ֽڣ����豸�ţ���

#define NAME_LEN 14		// �ļ����ֳ���ֵ��
#define ROOT_INO 1		// �� i �ڵ㡣

#define I_MAP_SLOTS 8		// i �ڵ�λͼ������
#define Z_MAP_SLOTS 8		// �߼��飨���ο飩λͼ������
#define SUPER_MAGIC 0x137F	// �ļ�ϵͳħ����

#define NR_OPEN 20
#define NR_INODE 32
#define NR_FILE 64
#define NR_SUPER 8
#define NR_HASH 307
#define NR_BUFFERS nr_buffers
#define BLOCK_SIZE 1024			// ���ݿ鳤�ȡ�
#define BLOCK_SIZE_BITS 10		// ���ݿ鳤����ռ����λ����
#ifndef NULL
#define NULL ((void *) 0)
#endif

// ÿ���߼���ɴ�ŵ� i �ڵ�����
#define INODES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct d_inode)))
// ÿ���߼���ɴ�ŵ�Ŀ¼������
#define DIR_ENTRIES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct dir_entry)))

// �ܵ�ͷ���ܵ�β���ܵ���С���ܵ��գ����ܵ��������ܵ�ͷָ�������
#define PIPE_HEAD(inode) ((inode).i_zone[0])
#define PIPE_TAIL(inode) ((inode).i_zone[1])
#define PIPE_SIZE(inode) ((PIPE_HEAD(inode)-PIPE_TAIL(inode))&(PAGE_SIZE-1))
#define PIPE_EMPTY(inode) (PIPE_HEAD(inode)==PIPE_TAIL(inode))
#define PIPE_FULL(inode) (PIPE_SIZE(inode)==(PAGE_SIZE-1))
#define INC_PIPE(head) \
__asm__("incl %0\n\tandl $4095,%0"::"m" (head))

typedef char buffer_block[BLOCK_SIZE];	// �黺������

struct buffer_head {
	char * b_data;			/* pointer to data block (1024 bytes) */	//ָ�����������ݿ�
	unsigned long b_blocknr;	/* block number */				//��Ӧ�豸�Ŀ��
	unsigned short b_dev;		/* device (0 = free) */			//��Ӧ�豸
	unsigned char b_uptodate;									//�Ƿ���豸�и������ݽ�buffer
	unsigned char b_dirt;		/* 0-clean,1-dirty */			//��λ
	unsigned char b_count;		/* users using this block */	//�������û�ʹ��
	unsigned char b_lock;		/* 0 - ok, 1 -locked */			//�Ƿ�����
	struct task_struct * b_wait;								//�ȴ�ʹ�ý��̶���
	struct buffer_head * b_prev;								//ͬһ�豸�ѷ������һ����
	struct buffer_head * b_next;								//ͬһ�豸�ѷ������һ����
	struct buffer_head * b_prev_free;							//��һ�����п�
	struct buffer_head * b_next_free;							//��һ�����п�
};

// �����ϵ������ڵ�(i �ڵ�)���ݽṹ��
struct d_inode {
	unsigned short i_mode;
	unsigned short i_uid;
	unsigned long i_size;
	unsigned long i_time;
	unsigned char i_gid;
	unsigned char i_nlinks;
	unsigned short i_zone[9];
};

// �������ڴ��е� i �ڵ�ṹ��ǰ 7 ���� d_inode ��ȫһ����
struct m_inode {
	unsigned short i_mode;			//�ļ������ͺ�����rwx
	unsigned short i_uid;			//�ļ��������û�id
	unsigned long i_size;			//�ļ�����(�ֽ�)
	unsigned long i_mtime;			//�޸�ʱ��(��1970�����𣬵�λ��)
	unsigned char i_gid;			//�ļ���������id
	unsigned char i_nlinks;			//���������������ļ�Ŀ¼��ָ���i���
	unsigned short i_zone[9];		//�ļ���ռ�����߼�������顣0-6ֱ�ӿ飻7һ�μ�ӣ�8���μ�ӡ�
/* these are in memory also */
	struct task_struct * i_wait;	//�ȴ��� i �ڵ�Ľ��̡�
	unsigned long i_atime;			//����ʱ��
	unsigned long i_ctime;			//����ʱ�䣬�����޸�ʱ��
	unsigned short i_dev;			//��Ӧ�豸
	unsigned short i_num;			//i����
	unsigned short i_count;			//���ں��б��򿪴�����0 ��ʾ�� i �ڵ���С�
	unsigned char i_lock;			//�Ƿ�����
	unsigned char i_dirt;			//��λ
	unsigned char i_pipe;			//�Ƿ��ǹܵ��ļ�
	unsigned char i_mount;			// ��װ��־���Ƿ���mount�㡣
	unsigned char i_seek;			// ��Ѱ��־(lseek ʱ)��
	unsigned char i_update;			// ���±�־��
};

// �ļ��ṹ���������ļ������ i �ڵ�֮�佨����ϵ��
struct file {
	unsigned short f_mode;		//�ļ�����ģʽrw
	unsigned short f_flags;		//�ļ��򿪺Ϳ��Ƶı�־
	unsigned short f_count;		//��Ӧ�ļ���������
	struct m_inode * f_inode;	//ָ���Ӧi���
	off_t f_pos;				//�ļ���ǰ��дָ��λ��
};

// �ڴ��д��̳�����ṹ��
struct super_block {
	unsigned short s_ninodes;				//i�����
	unsigned short s_nzones;				//�߼����� �� ������
	unsigned short s_imap_blocks;			//i���λͼ��ռ����
	unsigned short s_zmap_blocks;			//�߼���λͼ��ռ����
	unsigned short s_firstdatazone;			//��һ���߼����
	unsigned short s_log_zone_size;			//log2(���ݿ���/�߼���)
	unsigned long s_max_size;				//����ļ�����
	unsigned short s_magic;					//�ļ�ϵͳ����
/* These are only in memory */
	struct buffer_head * s_imap[8];			//i���λͼ�ڸ��ٻ����ָ������
	struct buffer_head * s_zmap[8];			//�߼���λͼ�ڸ��ٻ����ָ������
	unsigned short s_dev;					//�����������豸��
	struct m_inode * s_isup;				//����װ�ļ�ϵͳ��Ŀ¼i���
	struct m_inode * s_imount;				//���ļ�ϵͳ����װ����i���
	unsigned long s_time;					//�޸�ʱ��
	struct task_struct * s_wait;			//�ȴ���������Ľ���ָ��
	unsigned char s_lock;					//������־
	unsigned char s_rd_only;				//ֻ����־
	unsigned char s_dirt;					//��λ
};

// �����ϳ�����ṹ������ 125-132 ����ȫһ����
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

// �ļ�Ŀ¼��ṹ��
//Ŀ¼����ֻ���ļ�����inode�ſ�����߲����ٶ�
struct dir_entry {	
	unsigned short inode;
	char name[NAME_LEN];
};

extern struct m_inode inode_table[NR_INODE];		// ���� i �ڵ�����飨32 ���
extern struct file file_table[NR_FILE];				// �ļ������飨64 ���
extern struct super_block super_block[NR_SUPER];	// ���������飨8 ���
extern struct buffer_head * start_buffer;			// ��������ʼ�ڴ�λ�á�
extern int nr_buffers;								// ���������

// ���̲�������ԭ�͡�

extern void check_disk_change(int dev);
extern int floppy_change(unsigned int nr);
extern int ticks_to_floppy_on(unsigned int dev);
extern void floppy_on(unsigned int dev);
extern void floppy_off(unsigned int dev);

// �������ļ�ϵͳ���������õĺ���ԭ�͡�

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
