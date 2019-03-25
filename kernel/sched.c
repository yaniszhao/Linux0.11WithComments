/*
 *  linux/kernel/sched.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * 'sched.c' is the main kernel file. It contains scheduling primitives
 * (sleep_on, wakeup, schedule etc) as well as a number of simple system
 * call functions (type getpid(), which just extracts a field from
 * current-task
 */

//sched.c 是内核中有关任务调度函数的程序，其中包括有关调度的基本函数
//(sleep_on、wakeup、 schedule 等)以及一些简单的系统调用函数(比如 getpid())。
//另外 Linus 为了编程的方便，考虑到软盘驱 动器程序定时的需要，也将操作软盘的几个函数放到了这里。

//这几个基本函数的代码虽然不长，但有些抽象，比较难以理解。
//这里仅对调度函数 schedule()作一些说明。

//另两个值得一提的函数是自动进入睡眠函数 sleep_on()和唤醒函数 wake_up()，这两个函数虽然很短， 
//却要比 schedule()函数难理解。这里用图示的方法加以解释。简单地说，
//sleep_on()函数的主要功能是当 一个进程(或任务)所请求的资源正忙或不在内存中时暂时切换出去，
//放在等待队列中等待一段时间。 当切换回来后再继续运行。
//放入等待队列的方式是利用了函数中的 tmp 指针作为各个正在等待任务的联 系。
//函数中共牵涉到对三个任务指针操作:*p、tmp 和 current，*p 是等待队列头指针，
//如文件系统内存 i 节点的 i_wait 指针、内存缓冲操作中的 buffer_wait 指针等;
//tmp 是临时指针;current 是当前任务指针。 
//对于这些指针在内存中的变化情况我们可以用图 5-5 的示意图说明。图中的长条表示内存字节序列。


#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/sys.h>
#include <linux/fdreg.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/segment.h>

#include <signal.h>

//取信号nr在信号位图中对应位的二进制数值。信号编号1-32。
//比如信号 5 的位图数值 = 1<<(5-1) = 16 = 00010000b
#define _S(nr) (1<<((nr)-1))

// 除了 SIGKILL 和 SIGSTOP 信号以外其它都是
// 可阻塞的(...10111111111011111111b)
#define _BLOCKABLE (~(_S(SIGKILL) | _S(SIGSTOP)))

// 显示任务号 nr 的进程号、进程状态和内核堆栈空闲字节数(大约)
void show_task(int nr,struct task_struct * p)
{
	int i,j = 4096-sizeof(struct task_struct);

	printk("%d: pid=%d, state=%d, ",nr,p->pid,p->state);
	i=0;
	//p+1即除开PCB之后，为0的内存即为空闲的，说明这4K一开始是被初始化了的
	while (i<j && !((char *)(p+1))[i])
		i++;
	printk("%d (of %d) chars free in kernel stack\n\r",i,j);
}

// 显示所有任务的任务号、进程号、进程状态和内核堆栈空闲字节数(大约)
// 任务号和进程号的区别?
void show_stat(void)
{
	int i;

	//NR_TASKS 是系统能容纳的最大进程(任务)数量(64 个)
	for (i=0;i<NR_TASKS;i++)
		if (task[i])
			show_task(i,task[i]);
}

// 定义每个时间片的滴答数
#define LATCH (1193180/HZ)

extern void mem_use(void);

extern int timer_interrupt(void);
extern int system_call(void);

//因为一个任务的数据结构与其内核态堆栈放在同一内存页中，
//所以从堆栈段寄存器 ss 可以获得其数据段选择符
union task_union {
	struct task_struct task;
	char stack[PAGE_SIZE];
};

// 定义初始任务的数据，这里是0号进程。不是1号init进程
static union task_union init_task = {INIT_TASK,};

// 前面的限定符 volatile，英文解释是易变、不稳定的意思。
// 这里是要求 gcc 不要对该变量进行优化处理，也不要挪动位置，
// 因为也许别的程序会来修改它的值。
long volatile jiffies=0;	// 从开机开始算起的滴答数时间值(10ms/滴答)

//注意这里单位是秒
long startup_time=0;		// 开机时间。从 1970:0:0:0 开始计时的秒数

struct task_struct *current = &(init_task.task);	// 当前任务指针(初始化为初始任务)
struct task_struct *last_task_used_math = NULL;		// 使用过协处理器任务的指针

//task里面装的只是指针，具体的空间要重新申请
struct task_struct * task[NR_TASKS] = {&(init_task.task), };	// 定义任务指针数组

// 定义用户堆栈给用户态使用，4K。指针指在最后一项。
// 起始一开始0进程没切到用户态时，栈用的也是这个地方。
long user_stack [ PAGE_SIZE>>2 ] ;	//四字节为单位所以要除以4

struct {	//用于通过lss设置ss:sp
	long * a;	//指向栈顶元素的最低的一个字节
	short b;	//00010 0 00
					//上面的PAGE_SIZE>>2 是定义空间大小，而这里是为了把地址指到最后
	} stack_start = { & user_stack [PAGE_SIZE>>2] , 0x10 };

/*
 *  'math_state_restore()' saves the current math information in the
 * old math state array, and gets the new ones from the current task
 */
 /* 将当前协处理器内容保存到老协处理器状态数组中，并将当前任务的协处理器
  * 内容加载进协处理器。 */
void math_state_restore()
{
	if (last_task_used_math == current)
		return;
	__asm__("fwait");
	if (last_task_used_math) {
		__asm__("fnsave %0"::"m" (last_task_used_math->tss.i387));
	}
	last_task_used_math=current;
	if (current->used_math) {
		__asm__("frstor %0"::"m" (current->tss.i387));
	} else {
		__asm__("fninit"::);
		current->used_math=1;
	}
}

/*
 *  'schedule()' is the scheduler function. This is GOOD CODE! There
 * probably won't be any reason to change this, as it should work well
 * in all circumstances (ie gives IO-bound processes good response etc).
 * The one thing you might take a look at is the signal-handler code here.
 *
 *   NOTE!!  Task 0 is the 'idle' task, which gets called when no other
 * tasks can run. It can not be killed, and it cannot sleep. The 'state'
 * information in task[0] is never used.
 */
	
/*
* 'schedule()'是调度函数。这是个很好的代码!没有任何理由对它进行修改，因为它可以在所有的
* 环境下工作(比如能够对 IO-边界处理很好的响应等)。只有一件事值得留意，那就是这里的信号
* 处理代码。
* 注意!!任务 0 是个闲置('idle')任务，只有当没有其它任务可以运行时才调用它。它不能被杀
* 死，也不能睡眠。任务 0 中的状态信息'state'是从来不用的。
*/
void schedule(void)
{
	int i,next,c;
	//注意task装的只是指针，所以这里用指针的指针
	struct task_struct ** p;

/* check alarm, wake up any interruptible tasks that have got a signal */
/* 检测 alarm(进程的报警定时值)，唤醒任何已得到信号的可中断任务 */
	for(p = &LAST_TASK ; p > &FIRST_TASK ; --p)
		if (*p) {
			// 如果设置过任务的定时值 alarm，并且已经过期(alarm<jiffies),
			// 则在信号位图中置 SIGALRM 信号。然后清 alarm。该信号的默认操作是终止进程。
			if ((*p)->alarm && (*p)->alarm < jiffies) {
					(*p)->signal |= (1<<(SIGALRM-1));
					(*p)->alarm = 0;
				}
			// 如果信号位图中除被阻塞的信号外还有其它信号，不一定就是alarm引起的，
			// 并且任务处于可中断状态，则置任务为就绪状态。
			if (((*p)->signal & ~(_BLOCKABLE & (*p)->blocked)) &&
			(*p)->state==TASK_INTERRUPTIBLE)
				(*p)->state=TASK_RUNNING;
		}

/* this is the scheduler proper: */
/* 这里是调度程序的主要部分 */
	while (1) {
		c = -1;
		next = 0;
		i = NR_TASKS;
		p = &task[NR_TASKS];
		// 这段代码也是从任务数组的最后一个任务开始循环处理，并跳过不含任务的数组槽。
		// 比较每个就绪状态任务的 counter(任务运行时间的递减滴答计数)值，哪一个值大，
		// 运行时间还不长，next 就指向哪个的任务号。
		while (--i) {
			if (!*--p)
				continue;
			//注意TASK_RUNNING是就绪态
			if ((*p)->state == TASK_RUNNING && (*p)->counter > c)
				c = (*p)->counter, next = i;
		}

		//如果有进程counter大于0则取到值最大的那个进程调度
		//如果大家都在阻塞，走到这c为-1还是不为0，next为0则会去切换0号进程。
		if (c) break;
		
		//如果就绪态的进程counter都用完了，则重新根据优先级分配counter。
		//counter 值的计算方式为 counter = counter /2 + priority。
		//这里计算过程不考虑进程的状态，但是阻塞的进程counter没用完还是会受到影响。		
		for(p = &LAST_TASK ; p > &FIRST_TASK ; --p)
			if (*p)
				(*p)->counter = ((*p)->counter >> 1) +
						(*p)->priority;
	}

	//调度函数会在系统空闲时去执行任务 0。
	//此时任务 0 仅执行pause()系统调用，并又会调用schedule函数。
	switch_to(next);
}

// pause()系统调用。转换当前任务的状态为可中断的等待状态，并重新调度。
// 该系统调用将导致进程进入睡眠状态，直到收到一个信号。
// 该信号用于终止进程或者使进程调用一个信号捕获函数。
// 只有当捕获了一个信号，并且信号捕获处理函数返回，pause()才会返回。
// 此时 pause()返回值应该是-1，并且 errno 被置为 EINTR。这里还没有完全实现(直到 0.95 版)。
int sys_pause(void)
{
	current->state = TASK_INTERRUPTIBLE;
	schedule();
	return 0;
}

void sleep_on(struct task_struct **p)
{
	struct task_struct *tmp;

	if (!p)
		return;
	if (current == &(init_task.task))
		panic("task[0] trying to sleep");
	tmp = *p;
	*p = current;
	current->state = TASK_UNINTERRUPTIBLE;
	schedule();
	if (tmp)
		tmp->state=0;
}

void interruptible_sleep_on(struct task_struct **p)
{
	struct task_struct *tmp;

	if (!p)
		return;
	if (current == &(init_task.task))
		panic("task[0] trying to sleep");
	tmp=*p;
	*p=current;
repeat:	current->state = TASK_INTERRUPTIBLE;
	schedule();
	if (*p && *p != current) {
		(**p).state=0;
		goto repeat;
	}
	*p=NULL;
	if (tmp)
		tmp->state=0;
}

void wake_up(struct task_struct **p)
{
	if (p && *p) {
		(**p).state=0;
		*p=NULL;
	}
}

/*
 * OK, here are some floppy things that shouldn't be in the kernel
 * proper. They are here because the floppy needs a timer, and this
 * was the easiest way of doing it.
 */
static struct task_struct * wait_motor[4] = {NULL,NULL,NULL,NULL};
static int  mon_timer[4]={0,0,0,0};
static int moff_timer[4]={0,0,0,0};
unsigned char current_DOR = 0x0C;

int ticks_to_floppy_on(unsigned int nr)
{
	extern unsigned char selected;
	unsigned char mask = 0x10 << nr;

	if (nr>3)
		panic("floppy_on: nr>3");
	moff_timer[nr]=10000;		/* 100 s = very big :-) */
	cli();				/* use floppy_off to turn it off */
	mask |= current_DOR;
	if (!selected) {
		mask &= 0xFC;
		mask |= nr;
	}
	if (mask != current_DOR) {
		outb(mask,FD_DOR);
		if ((mask ^ current_DOR) & 0xf0)
			mon_timer[nr] = HZ/2;
		else if (mon_timer[nr] < 2)
			mon_timer[nr] = 2;
		current_DOR = mask;
	}
	sti();
	return mon_timer[nr];
}

void floppy_on(unsigned int nr)
{
	cli();
	while (ticks_to_floppy_on(nr))
		sleep_on(nr+wait_motor);
	sti();
}

void floppy_off(unsigned int nr)
{
	moff_timer[nr]=3*HZ;
}

void do_floppy_timer(void)
{
	int i;
	unsigned char mask = 0x10;

	for (i=0 ; i<4 ; i++,mask <<= 1) {
		if (!(mask & current_DOR))
			continue;
		if (mon_timer[i]) {
			if (!--mon_timer[i])
				wake_up(i+wait_motor);
		} else if (!moff_timer[i]) {
			current_DOR &= ~mask;
			outb(current_DOR,FD_DOR);
		} else
			moff_timer[i]--;
	}
}

#define TIME_REQUESTS 64

static struct timer_list {
	long jiffies;
	void (*fn)();
	struct timer_list * next;
} timer_list[TIME_REQUESTS], * next_timer = NULL;

void add_timer(long jiffies, void (*fn)(void))
{
	struct timer_list * p;

	if (!fn)
		return;
	cli();
	if (jiffies <= 0)
		(fn)();
	else {
		for (p = timer_list ; p < timer_list + TIME_REQUESTS ; p++)
			if (!p->fn)
				break;
		if (p >= timer_list + TIME_REQUESTS)
			panic("No more time requests free");
		p->fn = fn;
		p->jiffies = jiffies;
		p->next = next_timer;
		next_timer = p;
		while (p->next && p->next->jiffies < p->jiffies) {
			p->jiffies -= p->next->jiffies;
			fn = p->fn;
			p->fn = p->next->fn;
			p->next->fn = fn;
			jiffies = p->jiffies;
			p->jiffies = p->next->jiffies;
			p->next->jiffies = jiffies;
			p = p->next;
		}
	}
	sti();
}

void do_timer(long cpl)
{
	extern int beepcount;
	extern void sysbeepstop(void);

	if (beepcount)
		if (!--beepcount)
			sysbeepstop();

	if (cpl)//特权级0或者3
		current->utime++;
	else
		current->stime++;

	if (next_timer) {
		next_timer->jiffies--;
		while (next_timer && next_timer->jiffies <= 0) {
			void (*fn)(void);
			
			fn = next_timer->fn;
			next_timer->fn = NULL;
			next_timer = next_timer->next;
			(fn)();
		}
	}
	if (current_DOR & 0xf0)
		do_floppy_timer();
	if ((--current->counter)>0) return;
	current->counter=0;
	if (!cpl) return;
	schedule();
}

int sys_alarm(long seconds)
{
	int old = current->alarm;

	if (old)
		old = (old - jiffies) / HZ;
	current->alarm = (seconds>0)?(jiffies+HZ*seconds):0;
	return (old);
}

int sys_getpid(void)
{
	return current->pid;
}

int sys_getppid(void)
{
	return current->father;
}

int sys_getuid(void)
{
	return current->uid;
}

int sys_geteuid(void)
{
	return current->euid;
}

int sys_getgid(void)
{
	return current->gid;
}

int sys_getegid(void)
{
	return current->egid;
}

int sys_nice(long increment)
{
	if (current->priority-increment>0)
		current->priority -= increment;
	return 0;
}

void sched_init(void)
{
	int i;
	struct desc_struct * p;

	if (sizeof(struct sigaction) != 16)
		panic("Struct sigaction MUST be 16 bytes");
	set_tss_desc(gdt+FIRST_TSS_ENTRY,&(init_task.task.tss));//tss0
	set_ldt_desc(gdt+FIRST_LDT_ENTRY,&(init_task.task.ldt));//ldt0
	p = gdt+2+FIRST_TSS_ENTRY;
	for(i=1;i<NR_TASKS;i++) {
		task[i] = NULL;
		p->a=p->b=0;
		p++;
		p->a=p->b=0;
		p++;
	}
/* Clear NT, so that we won't have troubles with that later on */
	__asm__("pushfl ; andl $0xffffbfff,(%esp) ; popfl");
	ltr(0);
	lldt(0);
	outb_p(0x36,0x43);		/* binary, mode 3, LSB/MSB, ch 0 */
	outb_p(LATCH & 0xff , 0x40);	/* LSB */
	outb(LATCH >> 8 , 0x40);	/* MSB */
	set_intr_gate(0x20,&timer_interrupt);
	outb(inb_p(0x21)&~0x01,0x21);
	set_system_gate(0x80,&system_call);//系统调用中断的注册
}
