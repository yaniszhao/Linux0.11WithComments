/*
 *  linux/lib/_exit.c
 *
 *  (C) 1991  Linus Torvalds
 */

#define __LIBRARY__
#include <unistd.h>

// �ں�ʹ�õĳ���(�˳�)��ֹ������
// ֱ�ӵ���ϵͳ�ж� int 0x80�����ܺ�__NR_exit��
// ������exit_code - �˳��롣
volatile void _exit(int exit_code)
{
	__asm__("int $0x80"::"a" (__NR_exit),"b" (exit_code));
}
