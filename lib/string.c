/*
所有字符串操作函数已经在 string.h 中实现，因此 string.c 程序仅包含 string.h 头文件。
*/

/*
 *  linux/lib/string.c
 *
 *  (C) 1991  Linus Torvalds
 */

#ifndef __GNUC__
#error I want gcc!
#endif

#define extern
#define inline
#define __LIBRARY__
#include <string.h>
