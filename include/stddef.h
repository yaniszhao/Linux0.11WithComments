/*
stddef.h 头文件的名称也是有 C 标准化组织（ X3J11 ）创建的，含义是标准（ std ）定义 (def) 。
主要用于存放一些“标准定义”。另外一个内容容易混淆的头文件是 stdlib.h ，也是由标准化组织建立的。 
stdlib.h主要用来申明一些不与其它头文件类型相关的各种函数。
但这两个头文件中的内容常常让人搞不清哪些申明在哪个头文件中。
*/


#ifndef _STDDEF_H
#define _STDDEF_H

#ifndef _PTRDIFF_T
#define _PTRDIFF_T
typedef long ptrdiff_t;			// 两个指针相减结果的类型。
#endif

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned long size_t;	// sizeof 返回的类型。
#endif

#undef NULL
#define NULL ((void *)0)		// 空指针。

// 成员在类型中的偏移位置
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)	//用0，有点东西

#endif
