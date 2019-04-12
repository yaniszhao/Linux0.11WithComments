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
 * 该目录里应该包含数学仿真代码。目前仅产生一个信号。
 */

//数学协处理器仿真处理代码文件。该程序目前还没有实现对数学协处理器的仿真代码。
//仅实现了协处理器发生异常中断时调用的两个 C 函数。 
// math_emulate() 仅在用户程序中包含协处理器指令时，对进程设置协处理器异常信号。

#include <signal.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>

// 协处理器仿真函数。
// 中断处理程序调用的 C 函数，参见(kernel/math/system_call.s，169 行)。
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

// 协处理器出错处理函数。
// 中断处理程序调用的 C 函数，参见(kernel/math/system_call.s，145 行)。
void math_error(void)
{
	__asm__("fnclex");
	if (last_task_used_math)
		last_task_used_math->signal |= 1<<(SIGFPE-1);
}
