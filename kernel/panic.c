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
 * 该函数在整个内核中使用(包括在 头文件*.h, 内存管理程序 mm 和文件系统 fs 中)，
 * 用以指出主要的出错问题。 */

//当内核程序出错时，则调用函数 panic()，显示错误信息并使系统进入死循环。
//在内核程序的许多地 方，若出现严重出错时就要调用到该函数。
//在很多情况下，调用 panic()函数是一种简明的处理方法。
//这 样做很好地遵循了 UNIX“尽量简明”的原则。

#include <linux/kernel.h>
#include <linux/sched.h>

void sys_sync(void);	/* it's really int */ /* 实际上是整型 int (fs/buffer.c,44) */

// 该函数用来显示内核中出现的重大错误信息，并运行文件系统同步函数，
// 然后进入死循环 -- 死机。
// 如果当前进程是任务 0 的话，还说明是交换任务出错，并且还没有运行文件系统同步函数。

volatile void panic(const char * s)
{
	printk("Kernel panic: %s\n\r",s);
	if (current == task[0])
		printk("In swapper task - not syncing\n\r");
	else
		sys_sync();
	for(;;);
}
