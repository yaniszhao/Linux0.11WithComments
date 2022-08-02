/*
pen() 系统调用用于将一个文件名转换成一个文件描述符。当调用成功时，返回的文件描述符将是
进程没有打开的最小数值的描述符。该调用创建一个新的打开文件，并不与任何其它进程共享。在执行
exec 函数时，该新的文件描述符将始终保持着打开状态。文件的读写指针被设置在文件开始位置。
*/
/*
 *  linux/lib/open.c
 *
 *  (C) 1991  Linus Torvalds
 */

#define __LIBRARY__
#include <unistd.h>
#include <stdarg.h>

// 打开文件函数。
// 打开并有可能创建一个文件。
// 参数：filename - 文件名；flag - 文件打开标志；...
// 返回：文件描述符，若出错则置出错码，并返回-1。
int open(const char * filename, int flag, ...)
{
	register int res;
	va_list arg;
	// 利用 va_start()宏函数，取得 flag 后面参数的指针，然后调用系统中断 int 0x80，功能 open 进行
	// 文件打开操作。
	// %0 - eax(返回的描述符或出错码)；%1 - eax(系统中断调用功能号__NR_open)；
	// %2 - ebx(文件名 filename)；%3 - ecx(打开文件标志 flag)；%4 - edx(后随参数文件属性 mode)。
	va_start(arg,flag);
	__asm__("int $0x80"
		:"=a" (res)
		:"0" (__NR_open),"b" (filename),"c" (flag),
		"d" (va_arg(arg,int)));
	// 系统中断调用返回值大于或等于 0，表示是一个文件描述符，则直接返回之。
	if (res>=0)
		return res;
	// 否则说明返回值小于 0，则代表一个出错码。设置该出错码并返回-1。
	errno = -res;
	return -1;
}
