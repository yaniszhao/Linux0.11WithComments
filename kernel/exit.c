/*
 *  linux/kernel/exit.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/tty.h>
#include <asm/segment.h>

int sys_pause(void);		// 把进程置为睡眠状态的系统调用
int sys_close(int fd);		// 关闭指定文件的系统调用

// 释放指定进程占用的任务槽及其任务数据结构所占用的内存。
// 参数 p 是任务数据结构的指针。该函数在后面的 sys_kill()和 sys_waitpid()中被调用。
// 扫描任务指针数组表 task[]以寻找指定的任务。如果找到，则首先清空该任务槽，
// 然后释放该任务数据结构所占用的内存页面，最后执行调度函数并在返回时立即退出。
// 如果在任务数组表中没有找到指定任务对应的项，则内核 panic
void release(struct task_struct * p)
{
	int i;

	if (!p)
		return;
	for (i=1 ; i<NR_TASKS ; i++)
		if (task[i]==p) {
			task[i]=NULL;
			free_page((long)p);//因为PCB是动态分配的内存，所以要free
			schedule();//注意释放了之后会调度
			return;
		}
	// 指定任务若不存在则死机
	panic("trying to release non-existent task");
}

// 向指定任务(*p)发送信号(sig)，权限为 priv。
// 参数:sig -- 信号值;
// p -- 指定任务的指针;
// priv -- 强制发送信号的标志。即不需要考虑进程用户属性或级别而能发送信号的权利。
// 该函数首先在判断参数的正确性，然后判断条件是否满足。
// 如果满足就向指定进程发送信号 sig 并退出，否则返回未许可错误号。
static inline int send_sig(long sig,struct task_struct * p,int priv)
{//内核态可以直接发送信号，用户态需要调用系统调用sys_kill来发送信号
	if (!p || sig<1 || sig>32)
		return -EINVAL;
	// 判断权限:
	// 如果强制发送标志置位，或者当前进程的有效用户标识符(euid)就是指定进程的 euid(也即是自己)，
	// 或者当前进程是超级用户，则在进程位图中添加该信号，否则出错退出。
	if (priv || (current->euid==p->euid) || suser())
		p->signal |= (1<<(sig-1));
	else
		return -EPERM;
	return 0;
}

// 终止会话(session)。
static void kill_session(void)
{
	struct task_struct **p = NR_TASKS + task;
	
	while (--p > &FIRST_TASK) {//除0进程外相同 session id 的进程，全部挂断
		if (*p && (*p)->session == current->session)
			(*p)->signal |= 1<<(SIGHUP-1);
	}
}

/*
 * XXX need to check permissions needed to send signals to process
 * groups, etc. etc.  kill() permissions semantics are tricky!
 */
 /* 为了向进程组等发送信号，XXX 需要检查许可。kill()的许可机制非常巧妙! */
// 系统调用 kill()可用于向任何进程或进程组发送任何信号，而并非只是杀死进程。 
// 参数 pid 是进程号;sig 是需要发送的信号。
// 如果 pid 值>0，则信号被发送给进程号是 pid 的进程。
// 如果 pid=0，那么信号就会被发送给当前进程的进程组中的所有进程。
// 如果 pid=-1，则信号 sig 就会发送给除第一个进程外的所有进程。
// 如果 pid < -1，则信号 sig 将发送给进程组-pid 的所有进程。
// 如果信号 sig 为 0，则不发送信号，但仍会进行错误检查。如果成功则返回 0。
// 该函数扫描任务数组表，并根据 pid 的值对满足条件的进程发送指定的信号 sig。若 pid 等于 0，
// 表明当前进程是进程组组长，因此需要向所有组内的进程强制发送信号 sig。
int sys_kill(int pid,int sig)
{
	struct task_struct **p = NR_TASKS + task;
	int err, retval = 0;

	if (!pid) while (--p > &FIRST_TASK) {			//pid为0，进程组长使用pid=0才有意义
		if (*p && (*p)->pgrp == current->pid) 
			if (err=send_sig(sig,*p,1))
				retval = err;
	} else if (pid>0) while (--p > &FIRST_TASK) {	//pid大于0，指定pid
		if (*p && (*p)->pid == pid) 
			if (err=send_sig(sig,*p,0))
				retval = err;
	} else if (pid == -1) while (--p > &FIRST_TASK)	//pid=-1，除0号进程所有有权限的进程
		if (err = send_sig(sig,*p,0))
			retval = err;
	else while (--p > &FIRST_TASK)					//小于-1，对应-pid进程组，前提也得有权限
		if (*p && (*p)->pgrp == -pid)
			if (err = send_sig(sig,*p,0))
				retval = err;
	return retval;
}

// 通知父进程 -- 向进程 pid 发送信号 SIGCHLD:默认情况下子进程将停止或终止。
// 如果没有找到父进程，则自己释放。但根据 POSIX.1 要求，若父进程已先行终止，
// 则子进程应该被初始进程 1 收容。
static void tell_father(int pid)
{
	int i;

	if (pid)//pid的值是父进程的pid
		for (i=0;i<NR_TASKS;i++) {
			if (!task[i])
				continue;
			if (task[i]->pid != pid)
				continue;
			task[i]->signal |= (1<<(SIGCHLD-1));
			return;
		}
/* if we don't find any fathers, we just release ourselves */
/* This is not really OK. Must change it to make father 1 */
	printk("BAD BAD - no father found\n\r");
	release(current);//找不到父进程才把自己的PCB回收，一般都不会没有父进程的
}

// 程序退出处理程序。code 是错误码。
int do_exit(long code)
{
	int i;

	free_page_tables(get_base(current->ldt[1]),get_limit(0x0f));//释放cs段
	free_page_tables(get_base(current->ldt[2]),get_limit(0x17));//释放ds/ss段

	//把子进程交给1号进程处理，似乎父进程死了子进程并不用死
	for (i=0 ; i<NR_TASKS ; i++)		
		if (task[i] && task[i]->father == current->pid) {	//处理当前进程的子进程
			task[i]->father = 1;							//让1号进程去收留子进程
			if (task[i]->state == TASK_ZOMBIE)				//若子进程已死，让1号进程处理尸体
				/* assumption task[1] is always init */
				(void) send_sig(SIGCHLD, task[1], 1);
		}
		
	// 关闭当前进程打开着的所有文件
	for (i=0 ; i<NR_OPEN ; i++)
		if (current->filp[i])
			sys_close(i);
		
	// 对当前进程的工作目录 pwd、根目录 root 以及程序的 i 节点进行同步操作，并分别置空(释放)
	iput(current->pwd); 	//同步pwd
	current->pwd=NULL;
	iput(current->root);	//同步root
	current->root=NULL;
	iput(current->executable);	//同步程序i结点
	current->executable=NULL;

	// 如果当前进程是会话头领(leader)进程并且其有控制终端，则释放该终端
	if (current->leader && current->tty >= 0)
		tty_table[current->tty].pgrp = 0;

	// 如果当前进程上次使用过协处理器，则将 last_task_used_math 置空
	if (last_task_used_math == current)
		last_task_used_math = NULL;

	// 如果当前进程是 leader 进程，则终止该会话的所有相关进程。
	// 如果被杀进程是leader进程，则同一session的进程都要被杀掉，
	// 这就是为什么守护进程要重建session的原因。
	if (current->leader)
		kill_session();

	// 把当前进程置为僵死状态，表明当前进程已经释放了资源。并保存将由父进程读取的退出码。
	// 除0号进程外。最起码都有一个父进程，最差也被1号进程收养，
	// 且进程退出时最后都要被父进程放到1号进程去，所以所有进程都是被1号进程处理尸体。
	current->state = TASK_ZOMBIE;
	current->exit_code = code;		//这个变量就是专门放退出的错误码的
	tell_father(current->father);	//告诉父进程自己先挂了
	schedule();						//重新调度进程运行，以让父进程处理僵死进程其它的善后事宜
	return (-1);	/* just to suppress warnings */
}

// 系统调用 exit()。终止进程
int sys_exit(int error_code)
{
	return do_exit((error_code&0xff)<<8);
}


// 系统调用 waitpid()。挂起当前进程，直到 pid 指定的子进程退出(终止)或者收到要求终止 
// 该进程的信号，或者是需要调用一个信号句柄(信号处理程序)。如果 pid 所指的子进程早已 
// 退出(已成所谓的僵死进程)，则本调用将立刻返回。子进程使用的所有资源将释放。
// 如果 pid > 0, 表示等待进程号等于 pid 的子进程。
// 如果 pid = 0, 表示等待进程组号等于当前进程组号的任何子进程。
// 如果 pid < -1, 表示等待进程组号等于 pid 绝对值的任何子进程。
// 如果 pid = -1, 表示等待任何子进程。
// 若 options = WUNTRACED，表示如果子进程是停止的，也马上返回(无须跟踪)。 
// 若 options = WNOHANG，表示如果没有子进程退出或终止就马上返回。
// 如果返回状态指针 stat_addr 不为空，则就将状态信息保存到那里。
int sys_waitpid(pid_t pid,unsigned long * stat_addr, int options)
{
	int flag, code;
	struct task_struct ** p;

	//判断大小是否够，以及写时复制。
	verify_area(stat_addr,4);//注意stat_addr是用户态的地址
repeat:
	flag=0;
	for(p = &LAST_TASK ; p > &FIRST_TASK ; --p) {
		if (!*p || *p == current)//不能为空，不能是本进程
			continue;
		if ((*p)->father != current->pid)//必须是本进程的子进程
			continue;
		if (pid>0) {//大于0，则为对应子进程
			if ((*p)->pid != pid)
				continue;
		} else if (!pid) {//等于0，则本组进程的子进程任一进程
			if ((*p)->pgrp != current->pgrp)
				continue;
		} else if (pid != -1) {//小于-1，则为-pid的组进程的任一进程，也要是子进程
			if ((*p)->pgrp != -pid)
				continue;
		}

		//若有一个满足或者pid=-1，都会走到这里判断其状态
		switch ((*p)->state) {
			case TASK_STOPPED://有WUNTRACED标志，则即使为停止状态也返回
				if (!(options & WUNTRACED))
					continue;
				put_fs_long(0x7f,stat_addr);//设子进程退出错误码为127
				//这里返回的对应子进程的pid，这里系统调用返回后，用户态可以在eax中得到
				return (*p)->pid;
			case TASK_ZOMBIE://进程处理尸体应该就是这个地方了
				//注意，除了1号进程会调用sys_waitpid外，父进程创建子进程后也可能会调用，
				//所以一般的父进程也可以得到子进程的运行时间，也可以清理子进程的尸体。
				current->cutime += (*p)->utime;//记录子进程的utime
				current->cstime += (*p)->stime;//记录子进程的stime
				flag = (*p)->pid;
				code = (*p)->exit_code;
				release(*p);	//PCB的内容已经不需要了，可以free了，这一步很重要
				put_fs_long(code,stat_addr);//返回子进程的退出错误码
				return flag;//返回子进程pid
			default:
				flag=1;
				continue;
		}
	}

	//走到这里是没有找到一个合适子进程。
	//flag为0主要是子进程不合适，也就是pid这个参数不太恰当。
	//flag为1主要是状态不合适，但是有符合的pid的子进程，所以要再判断下，
	//如果有WNOHANG就直接返回，如果没有就阻塞等待到有信号发送过来。
	if (flag) {
		if (options & WNOHANG)//直接返回
			return 0;
		current->state=TASK_INTERRUPTIBLE;//阻塞，这个是信号可以唤醒的
		schedule();//去调度其他进程

		//走到这的时候，说明进程收到了信号。
		//若是SIGCHLD信号说明是有子进程挂了，直接可以goto到处理子进程的代码处。
		//若是收到了其他信号(但是这时不应该收到其他信号的的)，所以退出处理函数。
		if (!(current->signal &= ~(1<<(SIGCHLD-1))))
			goto repeat;
		else
			return -EINTR;
	}
	return -ECHILD;
}


