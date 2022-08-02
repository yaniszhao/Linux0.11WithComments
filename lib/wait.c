/*
该程序包括函数 waitpid() 和 wait() 。这两个函数允许进程获取与其子进程之一的状态信息。各种选
项允许获取已经终止或停止的子进程状态信息。如果存在两个或两个以上子进程的状态信息，则报告的
顺序是不指定的。
*/

/*
 *  linux/lib/wait.c
 *
 *  (C) 1991  Linus Torvalds
 */

#define __LIBRARY__
#include <unistd.h>
#include <sys/wait.h>

// 等待进程终止系统调用函数。
// 该下面宏结构对应于函数：pid_t waitpid(pid_t pid, int * wait_stat, int options)
//
// 参数：pid - 等待被终止进程的进程 id，或者是用于指定特殊情况的其它特定数值；
// wait_stat - 用于存放状态信息；options - WNOHANG 或 WUNTRACED 或是 0。
_syscall3(pid_t,waitpid,pid_t,pid,int *,wait_stat,int,options)

// wait()系统调用。直接调用 waitpid()函数。
pid_t wait(int * wait_stat)
{
	return waitpid(-1,wait_stat,0);
}
