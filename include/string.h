/*
��ͷ�ļ�������Ƕ��������ʽ�����������ַ�������������Ϊ�����ִ���ٶ�ʹ������Ƕ������
�ڱ�׼ C ����Ҳ�ṩͬ�����Ƶ�ͷ�ļ���������ʵ�����ڱ�׼ C ���У�
�����ļ���ֻ������غ�����������
*/

#ifndef _STRING_H_
#define _STRING_H_

#ifndef NULL
#define NULL ((void *) 0)
#endif

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int size_t;
#endif

extern char * strerror(int errno);

/*
 * This string-include defines all string functions as inline
 * functions. Use gcc. It also assumes ds=es=data space, this should be
 * normal. Most of the string-functions are rather heavily hand-optimized,
 * see especially strtok,strstr,str[c]spn. They should work, but are not
 * very easy to understand. Everything is done entirely within the register
 * set, making the functions fast and clean. String instructions have been
 * used through-out, making for "slightly" unclear code :-)
 *
 *		(C) 1991 Linus Torvalds
 */
/*
 * ����ַ���ͷ�ļ�����Ƕ��������ʽ�����������ַ�������������ʹ�� gcc ʱ��ͬʱ
 * �ٶ��� ds=es=���ݿռ䣬��Ӧ���ǳ���ġ���������ַ����������Ǿ��ֹ����д���
 * �Ż��ģ������Ǻ��� strtok��strstr��str[c]spn������Ӧ����������������ȴ������
 * ô������⡣���еĲ��������϶���ʹ�üĴ���������ɵģ���ʹ�ú������������ࡣ
 * ���еط���ʹ�����ַ���ָ�����ʹ�ô��롰��΢���������?
 *
 * (C) 1991 Linus Torvalds
 */

// ��һ���ַ���(src)��������һ���ַ���(dest)��ֱ������ NULL �ַ���ֹͣ��
// ������dest - Ŀ���ַ���ָ�룬src - Դ�ַ���ָ�롣
// %0 - esi(src)��%1 - edi(dest)��
extern inline char * strcpy(char * dest,const char *src)
{
__asm__("cld\n"
	"1:\tlodsb\n\t"
	"stosb\n\t"
	"testb %%al,%%al\n\t"
	"jne 1b"
	::"S" (src),"D" (dest):"si","di","ax");
return dest;
}

// ����Դ�ַ��� count ���ֽڵ�Ŀ���ַ�����
// ���Դ������С�� count ���ֽڣ��͸��ӿ��ַ�(NULL)��Ŀ���ַ�����
// ������dest - Ŀ���ַ���ָ�룬src - Դ�ַ���ָ�룬count - �����ֽ�����
// %0 - esi(src)��%1 - edi(dest)��%2 - ecx(count)��
extern inline char * strncpy(char * dest,const char *src,int count)
{
__asm__("cld\n"
	"1:\tdecl %2\n\t"
	"js 2f\n\t"
	"lodsb\n\t"
	"stosb\n\t"
	"testb %%al,%%al\n\t"
	"jne 1b\n\t"
	"rep\n\t"
	"stosb\n"
	"2:"
	::"S" (src),"D" (dest),"c" (count):"si","di","ax","cx");
return dest;
}

// ��Դ�ַ���������Ŀ���ַ�����ĩβ����
// ������dest - Ŀ���ַ���ָ�룬src - Դ�ַ���ָ�롣
// %0 - esi(src)��%1 - edi(dest)��%2 - eax(0)��%3 - ecx(-1)��
extern inline char * strcat(char * dest,const char * src)
{
__asm__("cld\n\t"
	"repne\n\t"
	"scasb\n\t"
	"decl %1\n"
	"1:\tlodsb\n\t"
	"stosb\n\t"
	"testb %%al,%%al\n\t"
	"jne 1b"
	::"S" (src),"D" (dest),"a" (0),"c" (0xffffffff):"si","di","ax","cx");
return dest;
}

// ��Դ�ַ����� count ���ֽڸ��Ƶ�Ŀ���ַ�����ĩβ���������һ���ַ���
// ������dest - Ŀ���ַ�����src - Դ�ַ�����count - �����Ƶ��ֽ�����
// %0 - esi(src)��%1 - edi(dest)��%2 - eax(0)��%3 - ecx(-1)��%4 - (count)��
extern inline char * strncat(char * dest,const char * src,int count)
{
__asm__("cld\n\t"
	"repne\n\t"
	"scasb\n\t"
	"decl %1\n\t"
	"movl %4,%3\n"
	"1:\tdecl %3\n\t"
	"js 2f\n\t"
	"lodsb\n\t"
	"stosb\n\t"
	"testb %%al,%%al\n\t"
	"jne 1b\n"
	"2:\txorl %2,%2\n\t"
	"stosb"
	::"S" (src),"D" (dest),"a" (0),"c" (0xffffffff),"g" (count)
	:"si","di","ax","cx");
return dest;
}

// ��һ���ַ�������һ���ַ������бȽϡ�
// ������cs - �ַ��� 1��ct - �ַ��� 2��
// %0 - eax(__res)����ֵ��%1 - edi(cs)�ַ��� 1 ָ�룬%2 - esi(ct)�ַ��� 2 ָ�롣
// ���أ������ 1 > �� 2���򷵻� 1���� 1 = �� 2���򷵻� 0���� 1 < �� 2���򷵻�-1��
extern inline int strcmp(const char * cs,const char * ct)
{
register int __res __asm__("ax");
__asm__("cld\n"
	"1:\tlodsb\n\t"
	"scasb\n\t"
	"jne 2f\n\t"
	"testb %%al,%%al\n\t"
	"jne 1b\n\t"
	"xorl %%eax,%%eax\n\t"
	"jmp 3f\n"
	"2:\tmovl $1,%%eax\n\t"
	"jl 3f\n\t"
	"negl %%eax\n"
	"3:"
	:"=a" (__res):"D" (cs),"S" (ct):"si","di");
return __res;
}

// �ַ��� 1 ���ַ��� 2 ��ǰ count ���ַ����бȽϡ�
// ������cs - �ַ��� 1��ct - �ַ��� 2��count - �Ƚϵ��ַ�����
// %0 - eax(__res)����ֵ��%1 - edi(cs)�� 1 ָ�룬%2 - esi(ct)�� 2 ָ�룬%3 - ecx(count)��
// ���أ������ 1 > �� 2���򷵻� 1���� 1 = �� 2���򷵻� 0���� 1 < �� 2���򷵻�-1��
extern inline int strncmp(const char * cs,const char * ct,int count)
{
register int __res __asm__("ax");
__asm__("cld\n"
	"1:\tdecl %3\n\t"
	"js 2f\n\t"
	"lodsb\n\t"
	"scasb\n\t"
	"jne 3f\n\t"
	"testb %%al,%%al\n\t"
	"jne 1b\n"
	"2:\txorl %%eax,%%eax\n\t"
	"jmp 4f\n"
	"3:\tmovl $1,%%eax\n\t"
	"jl 4f\n\t"
	"negl %%eax\n"
	"4:"
	:"=a" (__res):"D" (cs),"S" (ct),"c" (count):"si","di","cx");
return __res;
}

// ���ַ�����Ѱ�ҵ�һ��ƥ����ַ���
// ������s - �ַ�����c - ��Ѱ�ҵ��ַ���
// %0 - eax(__res)��%1 - esi(�ַ���ָ�� s)��%2 - eax(�ַ� c)��
// ���أ������ַ����е�һ�γ���ƥ���ַ���ָ�롣��û���ҵ�ƥ����ַ����򷵻ؿ�ָ�롣
extern inline char * strchr(const char * s,char c)
{
register char * __res __asm__("ax");
__asm__("cld\n\t"
	"movb %%al,%%ah\n"
	"1:\tlodsb\n\t"
	"cmpb %%ah,%%al\n\t"
	"je 2f\n\t"
	"testb %%al,%%al\n\t"
	"jne 1b\n\t"
	"movl $1,%1\n"
	"2:\tmovl %1,%0\n\t"
	"decl %0"
	:"=a" (__res):"S" (s),"0" (c):"si");
return __res;
}

// Ѱ���ַ�����ָ���ַ����һ�γ��ֵĵط��������������ַ�����
// ������s - �ַ�����c - ��Ѱ�ҵ��ַ���
// %0 - edx(__res)��%1 - edx(0)��%2 - esi(�ַ���ָ�� s)��%3 - eax(�ַ� c)��
// ���أ������ַ��������һ�γ���ƥ���ַ���ָ�롣��û���ҵ�ƥ����ַ����򷵻ؿ�ָ�롣
extern inline char * strrchr(const char * s,char c)
{
register char * __res __asm__("dx");
__asm__("cld\n\t"
	"movb %%al,%%ah\n"
	"1:\tlodsb\n\t"
	"cmpb %%ah,%%al\n\t"
	"jne 2f\n\t"
	"movl %%esi,%0\n\t"
	"decl %0\n"
	"2:\ttestb %%al,%%al\n\t"
	"jne 1b"
	:"=d" (__res):"0" (0),"S" (s),"a" (c):"ax","si");
return __res;
}

// ���ַ��� 1 ��Ѱ�ҵ� 1 ���ַ����У����ַ������е��κ��ַ����������ַ��� 2 �С�
// ������cs - �ַ��� 1 ָ�룬ct - �ַ��� 2 ָ�롣
// %0 - esi(__res)��%1 - eax(0)��%2 - ecx(-1)��%3 - esi(�� 1 ָ�� cs)��%4 - (�� 2 ָ�� ct)��
// �����ַ��� 1 �а����ַ��� 2 ���κ��ַ����׸��ַ����еĳ���ֵ��
extern inline int strspn(const char * cs, const char * ct)
{
register char * __res __asm__("si");
__asm__("cld\n\t"
	"movl %4,%%edi\n\t"
	"repne\n\t"
	"scasb\n\t"
	"notl %%ecx\n\t"
	"decl %%ecx\n\t"
	"movl %%ecx,%%edx\n"
	"1:\tlodsb\n\t"
	"testb %%al,%%al\n\t"
	"je 2f\n\t"
	"movl %4,%%edi\n\t"
	"movl %%edx,%%ecx\n\t"
	"repne\n\t"
	"scasb\n\t"
	"je 1b\n"
	"2:\tdecl %0"
	:"=S" (__res):"a" (0),"c" (0xffffffff),"0" (cs),"g" (ct)
	:"ax","cx","dx","di");
return __res-cs;
}

// Ѱ���ַ��� 1 �в������ַ��� 2 ���κ��ַ����׸��ַ����С�
// ������cs - �ַ��� 1 ָ�룬ct - �ַ��� 2 ָ�롣
// %0 - esi(__res)��%1 - eax(0)��%2 - ecx(-1)��%3 - esi(�� 1 ָ�� cs)��%4 - (�� 2 ָ�� ct)��
// �����ַ��� 1 �в������ַ��� 2 ���κ��ַ����׸��ַ����еĳ���ֵ��
extern inline int strcspn(const char * cs, const char * ct)
{
register char * __res __asm__("si");
__asm__("cld\n\t"
	"movl %4,%%edi\n\t"
	"repne\n\t"
	"scasb\n\t"
	"notl %%ecx\n\t"
	"decl %%ecx\n\t"
	"movl %%ecx,%%edx\n"
	"1:\tlodsb\n\t"
	"testb %%al,%%al\n\t"
	"je 2f\n\t"
	"movl %4,%%edi\n\t"
	"movl %%edx,%%ecx\n\t"
	"repne\n\t"
	"scasb\n\t"
	"jne 1b\n"
	"2:\tdecl %0"
	:"=S" (__res):"a" (0),"c" (0xffffffff),"0" (cs),"g" (ct)
	:"ax","cx","dx","di");
return __res-cs;
}

// ���ַ��� 1 ��Ѱ���׸��������ַ��� 2 �е��κ��ַ���
// ������cs - �ַ��� 1 ��ָ�룬ct - �ַ��� 2 ��ָ�롣
// %0 -esi(__res)��%1 -eax(0)��%2 -ecx(0xffffffff)��%3 -esi(�� 1 ָ�� cs)��%4 -(�� 2 ָ�� ct)��
// �����ַ��� 1 ���׸������ַ��� 2 ���ַ���ָ�롣
extern inline char * strpbrk(const char * cs,const char * ct)
{
register char * __res __asm__("si");
__asm__("cld\n\t"
	"movl %4,%%edi\n\t"
	"repne\n\t"
	"scasb\n\t"
	"notl %%ecx\n\t"
	"decl %%ecx\n\t"
	"movl %%ecx,%%edx\n"
	"1:\tlodsb\n\t"
	"testb %%al,%%al\n\t"
	"je 2f\n\t"
	"movl %4,%%edi\n\t"
	"movl %%edx,%%ecx\n\t"
	"repne\n\t"
	"scasb\n\t"
	"jne 1b\n\t"
	"decl %0\n\t"
	"jmp 3f\n"
	"2:\txorl %0,%0\n"
	"3:"
	:"=S" (__res):"a" (0),"c" (0xffffffff),"0" (cs),"g" (ct)
	:"ax","cx","dx","di");
return __res;
}

// ���ַ��� 1 ��Ѱ���׸�ƥ�������ַ��� 2 ���ַ�����
// ������cs - �ַ��� 1 ��ָ�룬ct - �ַ��� 2 ��ָ�롣
// %0 -eax(__res)��%1 -eax(0)��%2 -ecx(0xffffffff)��%3 -esi(�� 1 ָ�� cs)��%4 -(�� 2 ָ�� ct)��
// ���أ������ַ��� 1 ���׸�ƥ���ַ��� 2 ���ַ���ָ�롣
extern inline char * strstr(const char * cs,const char * ct)
{
register char * __res __asm__("ax");
__asm__("cld\n\t" \
	"movl %4,%%edi\n\t"
	"repne\n\t"
	"scasb\n\t"
	"notl %%ecx\n\t"
	"decl %%ecx\n\t"	/* NOTE! This also sets Z if searchstring='' */
	"movl %%ecx,%%edx\n"
	"1:\tmovl %4,%%edi\n\t"
	"movl %%esi,%%eax\n\t"
	"movl %%edx,%%ecx\n\t"
	"repe\n\t"
	"cmpsb\n\t"
	"je 2f\n\t"		/* also works for empty string, see above */
	"xchgl %%eax,%%esi\n\t"
	"incl %%esi\n\t"
	"cmpb $0,-1(%%eax)\n\t"
	"jne 1b\n\t"
	"xorl %%eax,%%eax\n\t"
	"2:"
	:"=a" (__res):"0" (0),"c" (0xffffffff),"S" (cs),"g" (ct)
	:"cx","dx","di","si");
return __res;
}

// �����ַ������ȡ�
// ������s - �ַ�����
// %0 - ecx(__res)��%1 - edi(�ַ���ָ�� s)��%2 - eax(0)��%3 - ecx(0xffffffff)��
// ���أ������ַ����ĳ��ȡ�
extern inline int strlen(const char * s)
{
register int __res __asm__("cx");
__asm__("cld\n\t"
	"repne\n\t"
	"scasb\n\t"
	"notl %0\n\t"
	"decl %0"
	:"=c" (__res):"D" (s),"a" (0),"0" (0xffffffff):"di");
return __res;
}

extern char * ___strtok;	// ������ʱ���ָ�����汻�����ַ��� 1(s)��ָ�롣

// �����ַ��� 2 �е��ַ����ַ��� 1 �ָ�ɱ��(tokern)���С�
// ���� 1 �����ǰ��������������(token)�����У����ɷָ���ַ��� 2 �е�һ�������ַ��ֿ���
// ��һ�ε��� strtok()ʱ��������ָ���ַ��� 1 �е� 1 �� token ���ַ���ָ�룬���ڷ��� token ʱ��
// һ null �ַ�д���ָ����������ʹ�� null ��Ϊ�ַ��� 1 �ĵ��ã��������ַ�������ɨ���ַ��� 1��
// ֱ��û�� token Ϊֹ���ڲ�ͬ�ĵ��ù����У��ָ���� 2 ���Բ�ͬ��
// ������s - ��������ַ��� 1��ct - ���������ָ�����ַ��� 2��
// ��������%0 - ebx(__res)��%1 - esi(__strtok)��
// ������룺%2 - ebx(__strtok)��%3 - esi(�ַ��� 1 ָ�� s)��%4 - ���ַ��� 2 ָ�� ct����
// ���أ������ַ��� s �е� 1 �� token�����û���ҵ� token���򷵻�һ�� null ָ�롣
// ����ʹ���ַ��� s ָ��Ϊ null �ĵ��ã�����ԭ�ַ��� s ��������һ�� token��
extern inline char * strtok(char * s,const char * ct)
{
register char * __res __asm__("si");
__asm__("testl %1,%1\n\t"
	"jne 1f\n\t"
	"testl %0,%0\n\t"
	"je 8f\n\t"
	"movl %0,%1\n"
	"1:\txorl %0,%0\n\t"
	"movl $-1,%%ecx\n\t"
	"xorl %%eax,%%eax\n\t"
	"cld\n\t"
	"movl %4,%%edi\n\t"
	"repne\n\t"
	"scasb\n\t"
	"notl %%ecx\n\t"
	"decl %%ecx\n\t"
	"je 7f\n\t"			/* empty delimeter-string */
	"movl %%ecx,%%edx\n"
	"2:\tlodsb\n\t"
	"testb %%al,%%al\n\t"
	"je 7f\n\t"
	"movl %4,%%edi\n\t"
	"movl %%edx,%%ecx\n\t"
	"repne\n\t"
	"scasb\n\t"
	"je 2b\n\t"
	"decl %1\n\t"
	"cmpb $0,(%1)\n\t"
	"je 7f\n\t"
	"movl %1,%0\n"
	"3:\tlodsb\n\t"
	"testb %%al,%%al\n\t"
	"je 5f\n\t"
	"movl %4,%%edi\n\t"
	"movl %%edx,%%ecx\n\t"
	"repne\n\t"
	"scasb\n\t"
	"jne 3b\n\t"
	"decl %1\n\t"
	"cmpb $0,(%1)\n\t"
	"je 5f\n\t"
	"movb $0,(%1)\n\t"
	"incl %1\n\t"
	"jmp 6f\n"
	"5:\txorl %1,%1\n"
	"6:\tcmpb $0,(%0)\n\t"
	"jne 7f\n\t"
	"xorl %0,%0\n"
	"7:\ttestl %0,%0\n\t"
	"jne 8f\n\t"
	"movl %0,%1\n"
	"8:"
	:"=b" (__res),"=S" (___strtok)
	:"0" (___strtok),"1" (s),"g" (ct)
	:"ax","cx","dx","di");
return __res;
}

// �ڴ�鸴�ơ���Դ��ַ src ����ʼ���� n ���ֽڵ�Ŀ�ĵ�ַ dest ����
// ������dest - ���Ƶ�Ŀ�ĵ�ַ��src - ���Ƶ�Դ��ַ��n - �����ֽ�����
// %0 - ecx(n)��%1 - esi(src)��%2 - edi(dest)��
extern inline void * memcpy(void * dest,const void * src, int n)
{
__asm__("cld\n\t"
	"rep\n\t"
	"movsb"
	::"c" (n),"S" (src),"D" (dest)
	:"cx","si","di");
return dest;
}

// �ڴ���ƶ���ͬ�ڴ�鸴�ƣ��������ƶ��ķ���
// ������dest - ���Ƶ�Ŀ�ĵ�ַ��src - ���Ƶ�Դ��ַ��n - �����ֽ�����
// �� dest<src ��%0 - ecx(n)��%1 - esi(src)��%2 - edi(dest)��
// ����%0 - ecx(n)��%1 - esi(src+n-1)��%2 - edi(dest+n-1)��
// ����������Ϊ�˷�ֹ�ڸ���ʱ������ص����ǡ�
extern inline void * memmove(void * dest,const void * src, int n)
{
if (dest<src)
__asm__("cld\n\t"
	"rep\n\t"
	"movsb"
	::"c" (n),"S" (src),"D" (dest)
	:"cx","si","di");
else
__asm__("std\n\t"
	"rep\n\t"
	"movsb"
	::"c" (n),"S" (src+n-1),"D" (dest+n-1)
	:"cx","si","di");
return dest;
}

// �Ƚ� n ���ֽڵ������ڴ棨�����ַ���������ʹ���� NULL �ֽ�Ҳ��ֹͣ�Ƚϡ�
// ������cs - �ڴ�� 1 ��ַ��ct - �ڴ�� 2 ��ַ��count - �Ƚϵ��ֽ�����
// %0 - eax(__res)��%1 - eax(0)��%2 - edi(�ڴ�� 1)��%3 - esi(�ڴ�� 2)��%4 - ecx(count)��
// ���أ����� 1>�� 2 ���� 1���� 1<�� 2������-1���� 1==�� 2���򷵻� 0��
extern inline int memcmp(const void * cs,const void * ct,int count)
{
register int __res __asm__("ax");
__asm__("cld\n\t"
	"repe\n\t"
	"cmpsb\n\t"
	"je 1f\n\t"
	"movl $1,%%eax\n\t"
	"jl 1f\n\t"
	"negl %%eax\n"
	"1:"
	:"=a" (__res):"0" (0),"D" (cs),"S" (ct),"c" (count)
	:"si","di","cx");
return __res;
}

// �� n �ֽڴ�С���ڴ��(�ַ���)��Ѱ��ָ���ַ���
// ������cs - ָ���ڴ���ַ��c - ָ�����ַ���count - �ڴ�鳤�ȡ�
// %0 - edi(__res)��%1 - eax(�ַ� c)��%2 - edi(�ڴ���ַ cs)��%3 - ecx(�ֽ��� count)��
// ���ص�һ��ƥ���ַ���ָ�룬���û���ҵ����򷵻� NULL �ַ���
extern inline void * memchr(const void * cs,char c,int count)
{
register void * __res __asm__("di");
if (!count)
	return NULL;
__asm__("cld\n\t"
	"repne\n\t"
	"scasb\n\t"
	"je 1f\n\t"
	"movl $1,%0\n"
	"1:\tdecl %0"
	:"=D" (__res):"a" (c),"D" (cs),"c" (count)
	:"cx");
return __res;
}

// ���ַ���дָ�������ڴ�顣
// ���ַ� c ��д s ָ����ڴ����򣬹��� count �ֽڡ�
// %0 - eax(�ַ� c)��%1 - edi(�ڴ��ַ)��%2 - ecx(�ֽ��� count)��
extern inline void * memset(void * s,char c,int count)
{
__asm__("cld\n\t"
	"rep\n\t"
	"stosb"
	::"a" (c),"D" (s),"c" (count)
	:"cx","di");
return s;
}

#endif
