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
 * 当处于内核模式时，我们不能使用 printf，因为寄存器 fs 指向其它不感兴趣的地方。
 * 自己编制一个 printf 并在使用前保存 fs，一切就解决了。*/

// printk()是内核中使用的打印(显示)函数，功能与 C 标准函数库中的 print()相同。
// 重新编写这么一 个函数的原因是在内核中不能使用专用于用户模式的 fs 段寄存器，
// 需要首先保存它。
// printk()函数首先使 用 svprintf()对参数进行格式化处理，
// 然后在保存了 fs 段寄存器的情况下调用 tty_write()进行信息的打印 显示。


#include <stdarg.h>
#include <stddef.h>

#include <linux/kernel.h>

static char buf[1024];//内核打印信息缓存区

extern int vsprintf(char * buf, const char * fmt, va_list args);

int printk(const char *fmt, ...)
{
	va_list args;//char *
	int i;

	//第一参数在fmt后面，而fmt是一个地址，所以跳过一个char*即第一个参数。
	va_start(args, fmt);// 得到第一个实际参数值的地址。
	//得到buf
	i=vsprintf(buf,fmt,args);
	va_end(args);
	__asm__("push %%fs\n\t"	//保存fs
		"push %%ds\n\t"		//使用内核的数据段做为fs
		"pop %%fs\n\t"
		"pushl %0\n\t"			//参数nr, 实参是i
		"pushl $_buf\n\t"		//参数buf
		"pushl $0\n\t"			//参数channel
		"call _tty_write\n\t"	//int tty_write(unsigned channel, char * buf, int nr)
		"addl $8,%%esp\n\t"
		"popl %0\n\t"			//返回值
		"pop %%fs"				//恢复fs
		::"r" (i):"ax","cx","dx");	//通知编译器，寄存器ax,cx,dx值可能已经改变
	return i;
}
