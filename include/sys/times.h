//该头文件中主要定义了文件访问与修改时间结构 tms 。
//它将由 times() 函数返回。其中 time_t 是在sys/types.h 中定义的。
//还定义了一个函数原型 times() 。


#ifndef _TIMES_H
#define _TIMES_H

#include <sys/types.h>

struct tms {
	time_t tms_utime;	// 用户使用的 CPU 时间。
	time_t tms_stime;	// 系统（内核）CPU 时间。
	time_t tms_cutime;	// 已终止的子进程使用的用户 CPU 时间。
	time_t tms_cstime;	// 已终止的子进程使用的系统 CPU 时间。
};

extern time_t times(struct tms * tp);

#endif
