/*
 *  linux/kernel/printk.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * When in kernel-mode, we cannot use printf, as fs is liable to
 * point to 'interesting' things. Make a printf with fs-saving, and
 * all is well.
 */
/*
 * �������ں�ģʽʱ�����ǲ���ʹ�� printf����Ϊ�Ĵ��� fs ָ������������Ȥ�ĵط���
 * �Լ�����һ�� printf ����ʹ��ǰ���� fs��һ�оͽ���ˡ�*/

// printk()���ں���ʹ�õĴ�ӡ(��ʾ)������������ C ��׼�������е� print()��ͬ��
// ���±�д��ôһ ��������ԭ�������ں��в���ʹ��ר�����û�ģʽ�� fs �μĴ�����
// ��Ҫ���ȱ�������
// printk()��������ʹ �� svprintf()�Բ������и�ʽ������
// Ȼ���ڱ����� fs �μĴ���������µ��� tty_write()������Ϣ�Ĵ�ӡ ��ʾ��


#include <stdarg.h>
#include <stddef.h>

#include <linux/kernel.h>

static char buf[1024];//�ں˴�ӡ��Ϣ������

extern int vsprintf(char * buf, const char * fmt, va_list args);

int printk(const char *fmt, ...)
{
	va_list args;//char *
	int i;

	//��һ������fmt���棬��fmt��һ����ַ����������һ��char*����һ��������
	va_start(args, fmt);// �õ���һ��ʵ�ʲ���ֵ�ĵ�ַ��
	//�õ�buf
	i=vsprintf(buf,fmt,args);
	va_end(args);
	__asm__("push %%fs\n\t"	//����fs
		"push %%ds\n\t"		//ʹ���ں˵����ݶ���Ϊfs
		"pop %%fs\n\t"
		"pushl %0\n\t"			//����nr, ʵ����i
		"pushl $_buf\n\t"		//����buf
		"pushl $0\n\t"			//����channel
		"call _tty_write\n\t"	//int tty_write(unsigned channel, char * buf, int nr)
		"addl $8,%%esp\n\t"
		"popl %0\n\t"			//����ֵ
		"pop %%fs"				//�ָ�fs
		::"r" (i):"ax","cx","dx");	//֪ͨ���������Ĵ���ax,cx,dxֵ�����Ѿ��ı�
	return i;
}
