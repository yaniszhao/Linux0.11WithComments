/*
 *  linux/fs/char_dev.c
 *
 *  (C) 1991  Linus Torvalds
 */

// Ϊwrite��readϵͳ�����ṩ�ַ��豸�Ķ���д����

#include <errno.h>
#include <sys/types.h>

#include <linux/sched.h>
#include <linux/kernel.h>

#include <asm/segment.h>
#include <asm/io.h>

extern int tty_read(unsigned minor,char * buf,int count);
extern int tty_write(unsigned minor,char * buf,int count);

// �����ַ��豸��д����ָ�����͡�
typedef (*crw_ptr)(int rw,unsigned minor,char * buf,int count,off_t * pos);

// �����ն˶�д����������
// ������rw - ��д���minor - �ն����豸�ţ�buf - ��������cout - ��д�ֽ�����
// pos - ��д������ǰָ�룬�����ն˲�������ָ�����á�
// ���أ�ʵ�ʶ�д���ֽ�����
static int rw_ttyx(int rw,unsigned minor,char * buf,int count,off_t * pos)
{//ttyx
	return ((rw==READ)?tty_read(minor,buf,count):
		tty_write(minor,buf,count));
}

// �ն˶�д����������
// ͬ�� rw_ttyx()��ֻ�������˶Խ����Ƿ��п����ն˵ļ�⡣
static int rw_tty(int rw,unsigned minor,char * buf,int count, off_t * pos)
{//tty
	if (current->tty<0)	// ������û�ж�Ӧ�Ŀ����նˣ��򷵻س���š�
		return -EPERM;
	return rw_ttyx(rw,current->tty,buf,count,pos);
}

// �ڴ����ݶ�д��δʵ�֡�
static int rw_ram(int rw,char * buf, int count, off_t *pos)
{
	return -EIO;
}

// �ڴ����ݶ�д����������δʵ�֡�
static int rw_mem(int rw,char * buf, int count, off_t * pos)
{
	return -EIO;
}

// �ں���������д������δʵ�֡�
static int rw_kmem(int rw,char * buf, int count, off_t * pos)
{
	return -EIO;
}

// �˿ڶ�д����������
// ������rw - ��д���buf - ��������cout - ��д�ֽ�����pos - �˿ڵ�ַ��
// ���أ�ʵ�ʶ�д���ֽ�����
static int rw_port(int rw,char * buf, int count, off_t * pos)
{
	int i=*pos;
	// ������Ҫ���д���ֽ��������Ҷ˿ڵ�ַС�� 64k ʱ��ѭ��ִ�е����ֽڵĶ�д������
	while (count-->0 && i<65536) {
		// ���Ƕ������Ӷ˿� i �ж�ȡһ�ֽ����ݲ��ŵ��û��������С�
		if (rw==READ)
			put_fs_byte(inb(i),buf++);
		// ����д�������û����ݻ�������ȡһ�ֽ�������˿� i��
		else
			outb(get_fs_byte(buf++),i);
		i++;	// ǰ��һ���˿�??
	}
	// �����/д���ֽ���������Ӧ������дָ�롣
	i -= *pos;	
	*pos += i;
	return i;	// ���ض�/д���ֽ�����
}

// �ڴ��д����������
static int rw_memory(int rw, unsigned minor, char * buf, int count, off_t * pos)
{
	switch(minor) {	// �����ڴ��豸���豸�ţ��ֱ���ò�ͬ���ڴ��д������
		case 0:
			return rw_ram(rw,buf,count,pos);
		case 1:
			return rw_mem(rw,buf,count,pos);
		case 2:
			return rw_kmem(rw,buf,count,pos);
		case 3:
			return (rw==READ)?0:count;	/* rw_null */
		case 4:
			return rw_port(rw,buf,count,pos);
		default:
			return -EIO;
	}
}

#define NRDEVS ((sizeof (crw_table))/(sizeof (crw_ptr)))	// ����ϵͳ���豸������

// �ַ��豸��д����ָ���
static crw_ptr crw_table[]={
	NULL,		/* nodev */
	rw_memory,	/* /dev/mem etc */
	NULL,		/* /dev/fd */
	NULL,		/* /dev/hd */
	rw_ttyx,	/* /dev/ttyx */
	rw_tty,		/* /dev/tty */
	NULL,		/* /dev/lp */
	NULL};		/* unnamed pipes */

// �ַ��豸��д����������
// ������rw - ��д���dev - �豸�ţ�buf - ��������count - ��д�ֽ�����pos -��дָ�롣
// ���أ�ʵ�ʶ�/д�ֽ�����
int rw_char(int rw,int dev, char * buf, int count, off_t * pos)
{
	crw_ptr call_addr;

	if (MAJOR(dev)>=NRDEVS)	// ����豸�ų���ϵͳ�豸�����򷵻س����롣
		return -ENODEV;
	if (!(call_addr=crw_table[MAJOR(dev)]))	// �ҵ���Ӧ�Ķ�д����
		return -ENODEV;
	// ���ö�Ӧ�豸�Ķ�д����������������ʵ�ʶ�/д���ֽ�����
	return call_addr(rw,MINOR(dev),buf,count,pos);	
}
