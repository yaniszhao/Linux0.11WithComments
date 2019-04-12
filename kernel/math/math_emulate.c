/*
 * linux/kernel/math/math_emulate.c
 *
 * (C) 1991 Linus Torvalds
 */

/*
 * This directory should contain the math-emulation code.
 * Currently only results in a signal.
 */
/*
 * ��Ŀ¼��Ӧ�ð�����ѧ������롣Ŀǰ������һ���źš�
 */

//��ѧЭ���������洦������ļ����ó���Ŀǰ��û��ʵ�ֶ���ѧЭ�������ķ�����롣
//��ʵ����Э�����������쳣�ж�ʱ���õ����� C ������ 
// math_emulate() �����û������а���Э������ָ��ʱ���Խ�������Э�������쳣�źš�

#include <signal.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>

// Э���������溯����
// �жϴ��������õ� C �������μ�(kernel/math/system_call.s��169 ��)��
void math_emulate(long edi, long esi, long ebp, long sys_call_ret,
	long eax,long ebx,long ecx,long edx,
	unsigned short fs,unsigned short es,unsigned short ds,
	unsigned long eip,unsigned short cs,unsigned long eflags,
	unsigned short ss, unsigned long esp)
{
	unsigned char first, second;

/* 0x0007 means user code space */
	if (cs != 0x000F) {
		printk("math_emulate: %04x:%08x\n\r",cs,eip);
		panic("Math emulation needed in kernel");
	}
	first = get_fs_byte((char *)((*&eip)++));
	second = get_fs_byte((char *)((*&eip)++));
	printk("%04x:%08x %02x %02x\n\r",cs,eip-2,first,second);
	current->signal |= 1<<(SIGFPE-1);
}

// Э����������������
// �жϴ��������õ� C �������μ�(kernel/math/system_call.s��145 ��)��
void math_error(void)
{
	__asm__("fnclex");
	if (last_task_used_math)
		last_task_used_math->signal |= 1<<(SIGFPE-1);
}
