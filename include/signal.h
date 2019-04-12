
/*
�ź��ṩ��һ�ִ����첽�¼��ķ������ź�Ҳ����Ϊ��һ�����жϡ�ͨ����һ�����̷����źţ�
���ǿ��Կ��ƽ��̵�ִ��״̬����ͣ����������ֹ����
���ļ��������ں���ʹ�õ������źŵ����ƺͻ�������������
������Ϊ��Ҫ�ĺ����Ǹı�ָ���źŴ���ʽ�ĺ��� signal() �� sigaction() ��
�ӱ��ļ��п��Կ����� Linux �ں�ʵ���� POSIX.1 ��Ҫ������� 20 ���źš�
������ǿ���˵ Linux��һ��ʼ���ʱ���Ѿ���ȫ���ǵ����׼�ļ������ˡ�
���庯����ʵ�ּ����� kernel/signal.c ��
*/


#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <sys/types.h>

typedef int sig_atomic_t;							// �����ź�ԭ�Ӳ������͡�
typedef unsigned int sigset_t;		/* 32 bits */	// �����źż����͡�

#define _NSIG             32						// �����ź����� -- 32 �֡�
#define NSIG		_NSIG

// ������Щ�� Linux 0.11 �ں��ж�����źš����а����� POSIX.1 Ҫ������� 20 ���źš�
#define SIGHUP		 1
#define SIGINT		 2
#define SIGQUIT		 3
#define SIGILL		 4
#define SIGTRAP		 5
#define SIGABRT		 6
#define SIGIOT		 6
#define SIGUNUSED	 7
#define SIGFPE		 8
#define SIGKILL		 9
#define SIGUSR1		10
#define SIGSEGV		11
#define SIGUSR2		12
#define SIGPIPE		13
#define SIGALRM		14
#define SIGTERM		15
#define SIGSTKFLT	16
#define SIGCHLD		17
#define SIGCONT		18
#define SIGSTOP		19
#define SIGTSTP		20
#define SIGTTIN		21
#define SIGTTOU		22

/* Ok, I haven't implemented sigactions, but trying to keep headers POSIX */
/* OK���һ�û��ʵ�� sigactions �ı��ƣ�����ͷ�ļ�����ϣ������ POSIX ��׼ */
#define SA_NOCLDSTOP	1			// ���ӽ��̴���ֹͣ״̬���Ͳ��� SIGCHLD ����
#define SA_NOMASK	0x40000000		// ����ֹ��ָ�����źŴ������(�źž��)�����յ����źš�
#define SA_ONESHOT	0x80000000		// �źž��һ�������ù��ͻָ���Ĭ�ϴ�������

// ���³������� sigprocmask(how, )-- �ı������źż�(������)�����ڸı�ú�������Ϊ��
#define SIG_BLOCK          0	/* for blocking signals */
								// �������źż��м��ϸ������źż���
#define SIG_UNBLOCK        1	/* for unblocking signals */
								// �������źż���ɾ��ָ�����źż���
#define SIG_SETMASK        2	/* for setting the signal mask */
								// ���������źż����ź������룩��

// ���������������Ŷ���ʾָ���޷���ֵ�ĺ���ָ�룬�Ҷ���һ�� int ���Ͳ�����������ָ��ֵ��
// �߼��Ͻ�ʵ���ϲ����ܳ��ֵĺ�����ֵַ������Ϊ���� signal �����ĵڶ������������ڸ�֪�ںˣ�
// ���ں˴����źŻ���Զ��źŵĴ���ʹ�÷����ɲμ� kernel/signal.c ���򣬵� 94-96 �С�
#define SIG_DFL		((void (*)(int))0)	/* default signal handling */
										// Ĭ�ϵ��źŴ�������źž������
#define SIG_IGN		((void (*)(int))1)	/* ignore signal */
										// �����źŵĴ������

// ������ sigaction �����ݽṹ��
// sa_handler �Ƕ�Ӧĳ�ź�ָ��Ҫ��ȡ���ж�������������� SIG_DFL���� SIG_IGN �����Ը��źţ�
// Ҳ������ָ������źź�����һ��ָ�롣
// sa_mask �����˶��źŵ������룬���źų���ִ��ʱ����������Щ�źŵĴ���
// sa_flags ָ���ı��źŴ�����̵��źż��������� 37-39 �е�λ��־����ġ�
// sa_restorer �ǻָ�����ָ�룬�ɺ����� Libc �ṩ�����������û�̬��ջ���μ� kernel/signal.c
// ���⣬���𴥷��źŴ�����ź�Ҳ��������������ʹ���� SA_NOMASK ��־��								
struct sigaction {
	void (*sa_handler)(int);//��Դ���п����Ʋ⺯���Ĳ������źŵ�ֵ
	sigset_t sa_mask;
	int sa_flags;
	void (*sa_restorer)(void);
};

// ����ĺܶຯ����û�д��룬Ӧ�ò���ϵͳ���ã���libcʵ�ֵ���?

void (*signal(int _sig, void (*_func)(int)))(int);
int raise(int sig);
int kill(pid_t pid, int sig);
int sigaddset(sigset_t *mask, int signo);
int sigdelset(sigset_t *mask, int signo);
int sigemptyset(sigset_t *mask);
int sigfillset(sigset_t *mask);
int sigismember(sigset_t *mask, int signo); /* 1 - is, 0 - not, -1 error */
int sigpending(sigset_t *set);
int sigprocmask(int how, sigset_t *set, sigset_t *oldset);
int sigsuspend(sigset_t *sigmask);
int sigaction(int sig, struct sigaction *act, struct sigaction *oldact);

#endif /* _SIGNAL_H */
