/*
 *  linux/kernel/panic.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * This function is used through-out the kernel (includeinh mm and fs)
 * to indicate a major problem.
 */
/*
 * �ú����������ں���ʹ��(������ ͷ�ļ�*.h, �ڴ������� mm ���ļ�ϵͳ fs ��)��
 * ����ָ����Ҫ�ĳ������⡣ */

//���ں˳������ʱ������ú��� panic()����ʾ������Ϣ��ʹϵͳ������ѭ����
//���ں˳�������� �������������س���ʱ��Ҫ���õ��ú�����
//�ںܶ�����£����� panic()������һ�ּ����Ĵ�������
//�� �����ܺõ���ѭ�� UNIX��������������ԭ��

#include <linux/kernel.h>
#include <linux/sched.h>

void sys_sync(void);	/* it's really int */ /* ʵ���������� int (fs/buffer.c,44) */

// �ú���������ʾ�ں��г��ֵ��ش������Ϣ���������ļ�ϵͳͬ��������
// Ȼ�������ѭ�� -- ������
// �����ǰ���������� 0 �Ļ�����˵���ǽ�������������һ�û�������ļ�ϵͳͬ��������

volatile void panic(const char * s)
{
	printk("Kernel panic: %s\n\r",s);
	if (current == task[0])
		printk("In swapper task - not syncing\n\r");
	else
		sys_sync();
	for(;;);
}
