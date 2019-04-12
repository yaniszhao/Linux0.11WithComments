
/*
C ���Ե�����ص�֮һ����������Ա�Զ��������Ŀ�ɱ�ĺ�����
Ϊ�˷�����Щ�ɱ�����б��еĲ���������Ҫ�õ� stdarg.h �ļ��еĺꡣ 
stdarg.h ͷ�ļ��� C ��׼����֯���� BSD ϵͳ�� varargs.h �ļ��޸Ķ��ɡ�
*/


#ifndef _STDARG_H
#define _STDARG_H

typedef char *va_list;		// ���� va_list ��һ���ַ�ָ�����͡�

/* Amount of space required in an argument list for an arg of type TYPE.
   TYPE may alternatively be an expression whose type is used.  */
/* �������������Ϊ TYPE �� arg �����б���Ҫ��Ŀռ�������
 * TYPE Ҳ������ʹ�ø����͵�һ�����ʽ */

// ������䶨����ȡ����� TYPE ���͵��ֽڳ���ֵ���� int ����(4)�ı�����
#define __va_rounded_size(TYPE)  \
  (((sizeof (TYPE) + sizeof (int) - 1) / sizeof (int)) * sizeof (int))

// ��������������ú�ʵ�֣�ʹ AP ָ�򴫸������Ŀɱ������ĵ�һ��������
// �ڵ�һ�ε��� va_arg �� va_end ֮ǰ���������ȵ��øú�����
// 17 ���ϵ�__builtin_saveregs()���� gcc �Ŀ���� libgcc2.c �ж���ģ����ڱ���Ĵ�����
// ����˵���ɲμ� gcc �ֲ��½ڡ�Target Description Macros���е�
// ��Implementing the Varargs Macros��С�ڡ�
#ifndef __sparc__
#define va_start(AP, LASTARG) 						\
 (AP = ((char *) &(LASTARG) + __va_rounded_size (LASTARG)))
#else
//__builtin_saveregs�ǰѷżĴ�����ֵ�ŵ��ڴ�
#define va_start(AP, LASTARG) 						\
 (__builtin_saveregs (),						\
  AP = ((char *) &(LASTARG) + __va_rounded_size (LASTARG)))
#endif

// ����ú����ڱ����ú������һ���������ء�va_end �����޸� AP ʹ�������µ���
// va_start ֮ǰ���ܱ�ʹ�á�va_end ������ va_arg �������еĲ������ٱ����á�
void va_end (va_list);		/* Defined in gnulib */
#define va_end(AP)

//AP���ƣ�������AP֮ǰ��ַ���Ӧ��ֵ
#define va_arg(AP, TYPE)						\
 (AP += __va_rounded_size (TYPE),					\
  *((TYPE *) (AP - __va_rounded_size (TYPE))))

#endif /* _STDARG_H */
