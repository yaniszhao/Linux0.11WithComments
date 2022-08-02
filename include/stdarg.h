
/*
C 语言的最大特点之一是允许编程人员自定义参数数目可变的函数。
为了访问这些可变参数列表中的参数，就需要用到 stdarg.h 文件中的宏。 
stdarg.h 头文件是 C 标准化组织根据 BSD 系统的 varargs.h 文件修改而成。
*/


#ifndef _STDARG_H
#define _STDARG_H

typedef char *va_list;		// 定义 va_list 是一个字符指针类型。

/* Amount of space required in an argument list for an arg of type TYPE.
   TYPE may alternatively be an expression whose type is used.  */
/* 下面给出了类型为 TYPE 的 arg 参数列表所要求的空间容量。
 * TYPE 也可以是使用该类型的一个表达式 */

// 下面这句定义了取整后的 TYPE 类型的字节长度值。是 int 长度(4)的倍数。
#define __va_rounded_size(TYPE)  \
  (((sizeof (TYPE) + sizeof (int) - 1) / sizeof (int)) * sizeof (int))

// 下面这个函数（用宏实现）使 AP 指向传给函数的可变参数表的第一个参数。
// 在第一次调用 va_arg 或 va_end 之前，必须首先调用该函数。
// 17 行上的__builtin_saveregs()是在 gcc 的库程序 libgcc2.c 中定义的，用于保存寄存器。
// 它的说明可参见 gcc 手册章节“Target Description Macros”中的
// “Implementing the Varargs Macros”小节。
#ifndef __sparc__
#define va_start(AP, LASTARG) 						\
 (AP = ((char *) &(LASTARG) + __va_rounded_size (LASTARG)))
#else
//__builtin_saveregs是把放寄存器的值放到内存
#define va_start(AP, LASTARG) 						\
 (__builtin_saveregs (),						\
  AP = ((char *) &(LASTARG) + __va_rounded_size (LASTARG)))
#endif

// 下面该宏用于被调用函数完成一次正常返回。va_end 可以修改 AP 使其在重新调用
// va_start 之前不能被使用。va_end 必须在 va_arg 读完所有的参数后再被调用。
void va_end (va_list);		/* Defined in gnulib */
#define va_end(AP)

//AP后移，并返回AP之前地址里对应的值
#define va_arg(AP, TYPE)						\
 (AP += __va_rounded_size (TYPE),					\
  *((TYPE *) (AP - __va_rounded_size (TYPE))))

#endif /* _STDARG_H */
