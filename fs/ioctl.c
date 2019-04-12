/*
 *  linux/fs/ioctl.c
 *
 *  (C) 1991  Linus Torvalds
 */

// ioctl.c �ļ�ʵ�������� / �������ϵͳ���� ioctl() ��
// ��Ҫ���� tty_ioctl() ���������ն˵� I/O ���п��ơ�


#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include <linux/sched.h>

extern int tty_ioctl(int dev, int cmd, int arg);	

typedef int (*ioctl_ptr)(int dev,int cmd,int arg);	// ���������������(ioctl)����ָ�롣

#define NRDEVS ((sizeof (ioctl_table))/(sizeof (ioctl_ptr)))	// ����ϵͳ���豸������

static ioctl_ptr ioctl_table[]={	// ioctl ��������ָ���
	NULL,		/* nodev */
	NULL,		/* /dev/mem */
	NULL,		/* /dev/fd */
	NULL,		/* /dev/hd */
	tty_ioctl,	/* /dev/ttyx */
	tty_ioctl,	/* /dev/tty */
	NULL,		/* /dev/lp */
	NULL};		/* named pipes */
	
// ϵͳ���ú��� - ����������ƺ�����
// ������fd - �ļ���������cmd - �����룻arg - ������
// ���أ��ɹ��򷵻� 0�����򷵻س�����
int sys_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg)
{	
	struct file * filp;
	int dev,mode;
	// ����ļ������������ɴ򿪵��ļ��������߶�Ӧ���������ļ��ṹָ��Ϊ�գ�
	// �򷵻س����룬�˳���
	if (fd >= NR_OPEN || !(filp = current->filp[fd]))
		return -EBADF;
	// ȡ��Ӧ�ļ������ԡ�������ļ������ַ��ļ���Ҳ���ǿ��豸�ļ����򷵻س����룬�˳���
	mode=filp->f_inode->i_mode;
	if (!S_ISCHR(mode) && !S_ISBLK(mode))
		return -EINVAL;
	// ���ַ�����豸�ļ��� i �ڵ���ȡ�豸�š�����豸�Ŵ���ϵͳ���е��豸�����򷵻س���š�
	dev = filp->f_inode->i_zone[0];
	if (MAJOR(dev) >= NRDEVS)
		return -ENODEV;
	// ������豸�� ioctl ����ָ�����û�ж�Ӧ�������򷵻س����롣
	if (!ioctl_table[MAJOR(dev)])
		return -ENOTTY;
	// ���򷵻�ʵ�� ioctl ���������룬�ɹ��򷵻� 0�����򷵻س����롣
	return ioctl_table[MAJOR(dev)](dev,cmd,arg);
}
