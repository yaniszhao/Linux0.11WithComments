/*
utsname.h 是系统名称结构头文件。其中定义了结构 utsname 以及函数原型 uname() 。 
POSIX 要求字符数组长度应该是不指定的，但是其中存储的数据需以 null 终止。
因此该版内核的 utsname 结构定义不符要求（数组长度都被定义为 9 ）。
*/

#ifndef _SYS_UTSNAME_H
#define _SYS_UTSNAME_H

#include <sys/types.h>

struct utsname {
	char sysname[9];	// 本版本操作系统的名称。
	char nodename[9];	// 与实现相关的网络中节点名称。
	char release[9];	// 本实现的当前发行级别。
	char version[9];	// 本次发行的版本级别。
	char machine[9];	// 系统运行的硬件类型名称。
};

extern int uname(struct utsname * utsbuf);

#endif
