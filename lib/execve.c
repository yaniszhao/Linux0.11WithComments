/*
 *  linux/lib/execve.c
 *
 *  (C) 1991  Linus Torvalds
 */

#define __LIBRARY__
#include <unistd.h>

// ���ز�ִ���ӽ���(��������)������
// ����õ��ú꺯����Ӧ��int execve(const char * file, char ** argv, char ** envp)��
// ������file - ��ִ�г����ļ�����argv - �����в���ָ�����飻envp - ��������ָ�����顣
// ֱ�ӵ�����ϵͳ�ж� int 0x80��������__NR_execve���μ� include/unistd.h �� fs/exec.c ����
_syscall3(int,execve,const char *,file,char **,argv,char **,envp)
