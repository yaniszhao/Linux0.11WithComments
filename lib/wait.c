/*
�ó���������� waitpid() �� wait() ������������������̻�ȡ�����ӽ���֮һ��״̬��Ϣ������ѡ
�������ȡ�Ѿ���ֹ��ֹͣ���ӽ���״̬��Ϣ������������������������ӽ��̵�״̬��Ϣ���򱨸��
˳���ǲ�ָ���ġ�
*/

/*
 *  linux/lib/wait.c
 *
 *  (C) 1991  Linus Torvalds
 */

#define __LIBRARY__
#include <unistd.h>
#include <sys/wait.h>

// �ȴ�������ֹϵͳ���ú�����
// �������ṹ��Ӧ�ں�����pid_t waitpid(pid_t pid, int * wait_stat, int options)
//
// ������pid - �ȴ�����ֹ���̵Ľ��� id������������ָ����������������ض���ֵ��
// wait_stat - ���ڴ��״̬��Ϣ��options - WNOHANG �� WUNTRACED ���� 0��
_syscall3(pid_t,waitpid,pid_t,pid,int *,wait_stat,int,options)

// wait()ϵͳ���á�ֱ�ӵ��� waitpid()������
pid_t wait(int * wait_stat)
{
	return waitpid(-1,wait_stat,0);
}
