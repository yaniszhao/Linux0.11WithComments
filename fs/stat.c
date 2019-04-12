/*
 *  linux/fs/stat.c
 *
 *  (C) 1991  Linus Torvalds
 */

//�ó���ʵ��ȡ�ļ�״̬��Ϣϵͳ���� stat() �� fstat() ��������Ϣ������û����ļ�״̬�ṹ�������С�
//stat() �������ļ���ȡ��Ϣ���� fstat() ��ʹ���ļ���� ( ������ ) ��ȡ��Ϣ��

#include <errno.h>
#include <sys/stat.h>

#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>

// �����ļ�״̬��Ϣ��
// ���� inode ���ļ���Ӧ�� i �ڵ㣬statbuf �� stat �ļ�״̬�ṹָ�룬���ڴ��ȡ�õ�״̬��Ϣ��
static void cp_stat(struct m_inode * inode, struct stat * statbuf)
{
	struct stat tmp;
	int i;

	verify_area(statbuf,sizeof (* statbuf));
	tmp.st_dev = inode->i_dev;			// �ļ����ڵ��豸�š�
	tmp.st_ino = inode->i_num;			// �ļ� i �ڵ�š�
	tmp.st_mode = inode->i_mode;		// �ļ����ԡ�
	tmp.st_nlink = inode->i_nlinks;		// �ļ�����������
	tmp.st_uid = inode->i_uid;			// �ļ����û� id��
	tmp.st_gid = inode->i_gid;			// �ļ����� id��
	tmp.st_rdev = inode->i_zone[0];		// �豸��(����ļ���������ַ��ļ�����ļ�)��
	tmp.st_size = inode->i_size;		// �ļ���С���ֽ�����������ļ��ǳ����ļ�����
	tmp.st_atime = inode->i_atime;		// ������ʱ�䡣
	tmp.st_mtime = inode->i_mtime;		// ����޸�ʱ�䡣
	tmp.st_ctime = inode->i_ctime;		// ���ڵ��޸�ʱ�䡣
	// �����Щ״̬��Ϣ���Ƶ��û��������С�
	for (i=0 ; i<sizeof (tmp) ; i++)
		put_fs_byte(((char *) &tmp)[i],&((char *) statbuf)[i]);
}

// �ļ�״̬ϵͳ���ú��� - �����ļ�����ȡ�ļ�״̬��Ϣ��
// ���� filename ��ָ�����ļ�����statbuf �Ǵ��״̬��Ϣ�Ļ�����ָ�롣
// ���� 0���������򷵻س����롣
int sys_stat(char * filename, struct stat * statbuf)
{
	struct m_inode * inode;

	if (!(inode=namei(filename)))
		return -ENOENT;
	cp_stat(inode,statbuf);
	iput(inode);
	return 0;
}

// �ļ�״̬ϵͳ���� - �����ļ������ȡ�ļ�״̬��Ϣ��
// ���� fd ��ָ���ļ��ľ��(������)��statbuf �Ǵ��״̬��Ϣ�Ļ�����ָ�롣
// ���� 0���������򷵻س����롣
int sys_fstat(unsigned int fd, struct stat * statbuf)
{
	struct file * f;
	struct m_inode * inode;

	// ����ļ����ֵ����һ�����������ļ��� NR_OPEN�����߸þ�����ļ��ṹָ��Ϊ�գ�
	// ���߶�Ӧ�ļ��ṹ�� i �ڵ��ֶ�Ϊ�գ���������س����벢�˳���
	if (fd >= NR_OPEN || !(f=current->filp[fd]) || !(inode=f->f_inode))
		return -EBADF;
	cp_stat(inode,statbuf);
	return 0;
}
