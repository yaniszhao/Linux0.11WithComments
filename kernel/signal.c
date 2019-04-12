/*
 *  linux/kernel/signal.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>

#include <signal.h>

volatile void do_exit(int error_code);

// 获取当前任务信号屏蔽位图(屏蔽码)
int sys_sgetmask()
{
	return current->blocked;
}

// 设置新的信号屏蔽位图。SIGKILL 不能被屏蔽。返回值是原信号屏蔽位图
int sys_ssetmask(int newmask)
{
	int old=current->blocked;

	//1表示屏蔽，0才表示有效。kill和stop不能屏蔽，为了超级用户可以杀停进程。
	current->blocked = newmask & ~(1<<(SIGKILL-1));
	return old;
}

// 复制 sigaction 数据到 fs 数据段 to 处，fs一般指向用户态数据段
// from ==> fs:to
static inline void save_old(char * from,char * to)
{
	int i;

	verify_area(to, sizeof(struct sigaction));//判断大小是否够，以及写时复制。
	for (i=0 ; i< sizeof(struct sigaction) ; i++) {
		put_fs_byte(*from,to);
		from++;
		to++;
	}
}

// 把 sigaction 数据从 fs 数据段 from 位置复制到 to 处
// fs:from ==> to
static inline void get_new(char * from,char * to)
{
	int i;

	for (i=0 ; i< sizeof(struct sigaction) ; i++)
		*(to++) = get_fs_byte(from++);
}

// signal()系统调用。类似于 sigaction()。为指定的信号安装新的信号句柄(信号处理程序)。
// 信号句柄可以是用户指定的函数，也可以是 SIG_DFL(默认句柄)或 SIG_IGN(忽略)。
// 参数 signum --指定的信号;handler -- 指定的句柄;restorer –恢复函数指针，
// 该函数由 Libc库提供。
// 用于在信号处理程序结束后恢复系统调用返回时几个寄存器的原有值以及系统调用的返回值，
// 就好象系统调用没有执行过信号处理程序而直接返回到用户程序一样。
// 函数返回原信号句柄。
int sys_signal(int signum, long handler, long restorer)
{
	struct sigaction tmp;

	if (signum<1 || signum>32 || signum==SIGKILL)
		return -1;
	tmp.sa_handler = (void (*)(int)) handler;
	tmp.sa_mask = 0;
	// 该句柄只使用 1 次后就恢复到默认值，并允许信号在自己的处理句柄中收到
	tmp.sa_flags = SA_ONESHOT | SA_NOMASK;//相当于sys_sigaction的一种特殊的情形
	tmp.sa_restorer = (void (*)(void)) restorer;
	handler = (long) current->sigaction[signum-1].sa_handler;//返回原来的处理函数地址
	current->sigaction[signum-1] = tmp;
	return handler;//handler是long啊返回int ?
}

// sigaction()系统调用。改变进程在收到一个信号时的操作。
// signum 是除了 SIGKILL 以外的任何信号。如果新操作(action)不为空则新操作被安装。
// 如果 oldaction 指针不为空，则原操作被保留到 oldaction。成功则返回 0，否则为-1。
int sys_sigaction(int signum, const struct sigaction * action,
	struct sigaction * oldaction)
{
	struct sigaction tmp;

	if (signum<1 || signum>32 || signum==SIGKILL)
		return -1;

	tmp = current->sigaction[signum-1];

	//为signum信号设置新的操作。注意action是用户态的地址。
	get_new((char *) action,
		(char *) (signum-1+current->sigaction));
	if (oldaction)
		save_old((char *) &tmp,(char *) oldaction);
	if (current->sigaction[signum-1].sa_flags & SA_NOMASK)
		current->sigaction[signum-1].sa_mask = 0;
	else	//不允许本进程在处理信号时再收到同样的信号
		current->sigaction[signum-1].sa_mask |= (1<<(signum-1));
	return 0;
}


// 系统调用中断处理程序中真正的信号处理程序(在 kernel/system_call.s,119 行)。 
// 该段代码的主要作用是将信号的处理句柄插入到用户程序堆栈中，
// 并在本系统调用结束返回后立刻执行信号句柄程序，然后继续执行用户的程序。
// 信号处理函数放到用户态去执行而不是放到内核态执行，可以减少内核处理时间，而且更安全。
void do_signal(long signr,long eax, long ebx, long ecx, long edx,
	long fs, long es, long ds,
	long eip, long cs, long eflags,
	unsigned long * esp, long ss)
{
	unsigned long sa_handler;
	long old_eip=eip;//用户态eip
	struct sigaction * sa = current->sigaction + signr - 1;
	int longs;
	unsigned long * tmp_esp;

	sa_handler = (unsigned long) sa->sa_handler;
	if (sa_handler==1)	//信号句柄为 SIG_IGN(忽略)，则返回
		return;
	if (!sa_handler) {//如果句柄为 SIG_DFL(默认处理)
		if (signr==SIGCHLD)	//信号是SIGCHLD 则返回。应该要处理子进程尸体吧，还没实现?
			return;
		else				//否则终止进程的执行
			do_exit(1<<(signr-1));
	}
	//如果该信号句柄只需使用一次，则将该句柄置空，下次使用的是默认处理
	if (sa->sa_flags & SA_ONESHOT)
		sa->sa_handler = NULL;

	// 下面这段代码将信号处理句柄插入到用户堆栈中，同时也将 sa_restorer,signr,
	// 进程屏蔽码(如果SA_NOMASK 没置位),eax,ecx,edx 作为参数以及原调用系统调用
	// 的程序返回指针及标志寄存器值压入堆栈。
	// 因此在本次系统调用中断(0x80)返回用户程序时会首先执行用户的信号句柄程序，
	// 然后再继续执行用户程序。
	
	*(&eip) = sa_handler;//返回到用户态去执行处理函数
	// 如果允许信号自己的处理句柄收到信号自己，则也需要将进程的阻塞码压入堆栈。
	// 注意，这里 longs 的结果应该选择(7*4):(8*4)，因为堆栈是以 4 字节为单位操作的
	longs = (sa->sa_flags & SA_NOMASK)?7:8;
	*(&esp) -= longs;//esp-=longs看成long*的减每次减的绝对值为4
	verify_area(esp,longs*4);// 并检查内存使用情况(例如如果内存超界则分配新页等)
	tmp_esp=esp;
	put_fs_long((long) sa->sa_restorer,tmp_esp++);	//放restorer处理函数入口，下面是处理函数的参数
	put_fs_long(signr,tmp_esp++);					//放信号值，同时是sa_handler函数的参数
	if (!(sa->sa_flags & SA_NOMASK))				//放屏蔽码
		put_fs_long(current->blocked,tmp_esp++);
	put_fs_long(eax,tmp_esp++);						//放eax
	put_fs_long(ecx,tmp_esp++);						//放ecx
	put_fs_long(edx,tmp_esp++);						//放edx
	put_fs_long(eflags,tmp_esp++);					//放eflags
	put_fs_long(old_eip,tmp_esp++);					//放原来的eip
	current->blocked |= sa->sa_mask;				//进程阻塞码(屏蔽码)添上 sa_mask 中的码位
}
