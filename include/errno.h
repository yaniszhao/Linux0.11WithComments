/*
在系统或者标准 C 语言中有个名为 errno 的变量，关于在 C 标准中是否需要这个变量，
在 C 标准化组织（ X3J11 ）中引起了很大争论。但是争论的结果是没有去掉 errno ，
反而创建了名称为“ errno.h” 的头文件。
因为标准化组织希望每个库函数或数据对象都需要在一个相应的标准头文件中作出申明。

主要原因在于：对于内核中的每个系统调用，如果其返回值就是指定系统调用的结果值的话，
就很难报告出错情况。如果让每个函数返回一个对 / 错指示值，而结果值另行返回，
就不能很方便地得到系统调用的结果值。

解决的办法之一是将这两种方式加以组合：
对于一个特定的系统调用，可以指定一个与有效结果值范围有区别的出错返回值。
例如对于指针可以采用 null 值，对于 pid 可以返回 -1 值。
在许多其它情况下，只要不与结果值冲突都可以采用 -1 来表示出错值。
但是标准 C 库函数返回值仅告知是否发生出错，还必须从其它地方了解出错的类型，
因此采用了 errno 这个变量。为了与标准 C 库的设计机制兼容， 
Linux 内核中的库文件也采用了这种处理方法。因此也借用了标准 C 的这个头文件。
相关例子可参见 lib/open.c 程序以及 unistd.h 中的系统调用宏定义。
在某些情况下，程序虽然从返回的 -1 值知道出错了，但想知道具体的出错号，
就可以通过读取 errno 的值来确定最后一次错误的出错号。

本文件虽然只是定义了 Linux 系统中的一些出错码（出错号）的常量符号，
而且 Linus 考虑程序的兼容性也想把这些符号定义成与 POSIX 标准中的一样。
但是不要小看这个简单的代码，该文件也是 SCO公司指责 Linux 操作系统侵犯其版权所列出的文件的之一。
为了研究这个侵权问题，在 2003 年 12 月份，10 多个当前 Linux 内核的顶级开发人员在网上商讨对策。
其中包括 Linus 、 Alan Cox 、 H.J.Lu 、 Mitchell BlankJr 。
由于当前内核版本（ 2.4.x ）中的 errno.h 文件从 0.96c 版内核开始就没有变化过，
他们就一直“跟踪”到这些老版本的内核代码中。
最后 Linus 发现该文件是从 H.J.Lu 当时维护的 Libc 2.x 库中利用程序自动生成的，
其中包括了一些与 SCO 拥有版权的 UNIX 老版本（ V6 、 V7 等）相同的变量名。
*/


#ifndef _ERRNO_H
#define _ERRNO_H

/*
 * ok, as I hadn't got any other source of information about
 * possible error numbers, I was forced to use the same numbers
 * as minix.
 * Hopefully these are posix or something. I wouldn't know (and posix
 * isn't telling me - they want $$$ for their f***ing standard).
 *
 * We don't use the _SIGN cludge of minix, so kernel returns must
 * see to the sign by themselves.
 *
 * NOTE! Remember to change strerror() if you change this file!
 */

/*
 * ok，由于我没有得到任何其它有关出错号的资料，我只能使用与 minix 系统
 * 相同的出错号了。
 * 希望这些是 POSIX 兼容的或者在一定程度上是这样的，我不知道（而且 POSIX
 * 没有告诉我 - 要获得他们的混蛋标准需要出钱）。
 *
 * 我们没有使用 minix 那样的_SIGN 簇，所以内核的返回值必须自己辨别正负号。
 *
 * 注意！如果你改变该文件的话，记着也要修改 strerror()函数。
 */

// 系统调用以及很多库函数返回一个特殊的值以表示操作失败或出错。
// 这个值通常选择-1 或者其它一些特定的值来表示。但是这个返回值仅说明错误发生了。
// 如果需要知道出错的类型，就需要查看表示系统出错号的变量 errno。
// 该变量即在 errno.h 文件中申明。在程序开始执行时该变量值被初始化为 0。
extern int errno;

#define ERROR		99		// 一般错误。
#define EPERM		 1
#define ENOENT		 2
#define ESRCH		 3
#define EINTR		 4
#define EIO		 5
#define ENXIO		 6
#define E2BIG		 7
#define ENOEXEC		 8
#define EBADF		 9
#define ECHILD		10
#define EAGAIN		11
#define ENOMEM		12
#define EACCES		13
#define EFAULT		14
#define ENOTBLK		15
#define EBUSY		16
#define EEXIST		17
#define EXDEV		18
#define ENODEV		19
#define ENOTDIR		20
#define EISDIR		21
#define EINVAL		22
#define ENFILE		23
#define EMFILE		24
#define ENOTTY		25
#define ETXTBSY		26
#define EFBIG		27
#define ENOSPC		28
#define ESPIPE		29
#define EROFS		30
#define EMLINK		31
#define EPIPE		32
#define EDOM		33
#define ERANGE		34
#define EDEADLK		35
#define ENAMETOOLONG	36
#define ENOLCK		37
#define ENOSYS		38
#define ENOTEMPTY	39

#endif
