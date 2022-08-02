
/*
信号提供了一种处理异步事件的方法。信号也被称为是一种软中断。通过向一个进程发送信号，
我们可以控制进程的执行状态（暂停、继续或终止）。
本文件定义了内核中使用的所有信号的名称和基本操作函数。
其中最为重要的函数是改变指定信号处理方式的函数 signal() 和 sigaction() 。
从本文件中可以看出， Linux 内核实现了 POSIX.1 所要求的所有 20 个信号。
因此我们可以说 Linux在一开始设计时就已经完全考虑到与标准的兼容性了。
具体函数的实现见程序 kernel/signal.c 。
*/


#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <sys/types.h>

typedef int sig_atomic_t;							// 定义信号原子操作类型。
typedef unsigned int sigset_t;		/* 32 bits */	// 定义信号集类型。

#define _NSIG             32						// 定义信号种类 -- 32 种。
#define NSIG		_NSIG

// 以下这些是 Linux 0.11 内核中定义的信号。其中包括了 POSIX.1 要求的所有 20 个信号。
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
/* OK，我还没有实现 sigactions 的编制，但在头文件中仍希望遵守 POSIX 标准 */
#define SA_NOCLDSTOP	1			// 当子进程处于停止状态，就不对 SIGCHLD 处理。
#define SA_NOMASK	0x40000000		// 不阻止在指定的信号处理程序(信号句柄)中再收到该信号。
#define SA_ONESHOT	0x80000000		// 信号句柄一旦被调用过就恢复到默认处理句柄。

// 以下常量用于 sigprocmask(how, )-- 改变阻塞信号集(屏蔽码)。用于改变该函数的行为。
#define SIG_BLOCK          0	/* for blocking signals */
								// 在阻塞信号集中加上给定的信号集。
#define SIG_UNBLOCK        1	/* for unblocking signals */
								// 从阻塞信号集中删除指定的信号集。
#define SIG_SETMASK        2	/* for setting the signal mask */
								// 设置阻塞信号集（信号屏蔽码）。

// 以下两个常数符号都表示指向无返回值的函数指针，且都有一个 int 整型参数。这两个指针值是
// 逻辑上讲实际上不可能出现的函数地址值。可作为下面 signal 函数的第二个参数。用于告知内核，
// 让内核处理信号或忽略对信号的处理。使用方法可参见 kernel/signal.c 程序，第 94-96 行。
#define SIG_DFL		((void (*)(int))0)	/* default signal handling */
										// 默认的信号处理程序（信号句柄）。
#define SIG_IGN		((void (*)(int))1)	/* ignore signal */
										// 忽略信号的处理程序。

// 下面是 sigaction 的数据结构。
// sa_handler 是对应某信号指定要采取的行动。可以用上面的 SIG_DFL，或 SIG_IGN 来忽略该信号，
// 也可以是指向处理该信号函数的一个指针。
// sa_mask 给出了对信号的屏蔽码，在信号程序执行时将阻塞对这些信号的处理。
// sa_flags 指定改变信号处理过程的信号集。它是由 37-39 行的位标志定义的。
// sa_restorer 是恢复函数指针，由函数库 Libc 提供，用于清理用户态堆栈。参见 kernel/signal.c
// 另外，引起触发信号处理的信号也将被阻塞，除非使用了 SA_NOMASK 标志。								
struct sigaction {
	void (*sa_handler)(int);//从源码中可以推测函数的参数是信号的值
	sigset_t sa_mask;
	int sa_flags;
	void (*sa_restorer)(void);
};

// 下面的很多函数都没有代码，应该不是系统调用，是libc实现的吗?

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
