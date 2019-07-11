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
//却要比 schedule()函数难理解。这里用图示的方法加以解释。
//简单地说，sleep_on()函数的主要功能是当 一个进程(或任务)所请求的资源正忙或不在内存中时暂时切换出去，
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
* 'schedule()'是调度函数。这是个很好的代码!没有任何理由对它进行修改，
* 因为它可以在所有的环境下工作(比如能够对 IO-边界处理很好的响应等)。
* 只有一件事值得留意，那就是这里的信号处理代码。
* 注意!!任务 0 是个闲置('idle')任务，只有当没有其它任务可以运行时才调用它。
* 它不能被杀死，也不能睡眠。任务 0 中的状态信息'state'是从来不用的。
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

		//如果有进程counter大于0则取到值最大的那个进程调度。
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

// 把当前任务置为不可中断的等待状态，并让睡眠队列头的指针指向当前任务。
// 只有明确地唤醒时才会返回。该函数提供了进程与中断处理程序之间的同步机制。
// 函数参数*p 是放置等待任务的队列头指针。
void sleep_on(struct task_struct **p)
{
	struct task_struct *tmp;

	if (!p)
		return;
	if (current == &(init_task.task)) //0进程不能睡
		panic("task[0] trying to sleep");
	//函数虽然只有两个指针，一个指向前一个进入的进程，一个指向当前进入的进程。
	//但是多个进程的调用，就构成了一个队列
	tmp = *p;
	*p = current;
	current->state = TASK_UNINTERRUPTIBLE;
	schedule();
	// 只有当这个等待任务被唤醒时，调度程序才又返回到这里，则表示进程已被明确地唤醒。
	// 既然大家都在等待同样的资源，那么在资源可用时，就有必要唤醒所有等待该资源的进程。
	// 该函数嵌套调用，也会嵌套唤醒所有等待该资源的进程。
	// 注意是唤醒所以进程，所以大家在去竞争资源?
	if (tmp)
		tmp->state=0;//就绪态
}

// 将当前任务置为可中断的等待状态，并放入*p 指定的等待队列中。
// 可中断睡眠和不可中断睡眠区别主要在于能否响应信号。
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
	
	// 如果等待队列中还有等待任务，并且队列头指针所指向的任务不是当前任务时，
	// 则将该等待任务置为可运行的就绪状态，并重新执行调度程序。
	// 当指针*p 所指向的不是当前任务时，表示在当前任务被放入队列后，
	// 又有新的任务被插入等待队列中，因此，就应该同时也将所有其它的等待任务置为可运行状态。
	if (*p && *p != current) {//有头，且头不为当前进程
		(**p).state=0;
		goto repeat;
	}
	// 下面一句代码有误，应该是*p = tmp，让队列头指针指向其余等待任务，
	// 否则在当前任务之前插入等待队列的任务均被抹掉了。
	
	*p=NULL;
	if (tmp)
		tmp->state=0;
}

void wake_up(struct task_struct **p)
{
	if (p && *p) {//把头结点唤醒，各进程会依次把队列中的后续结点唤醒。
		(**p).state=0;
		*p=NULL;
	}
}

/*
 * OK, here are some floppy things that shouldn't be in the kernel
 * proper. They are here because the floppy needs a timer, and this
 * was the easiest way of doing it.
 */
 /*
  * 好了，从这里开始是一些有关软盘的子程序，本不应该放在内核的主要部分中的。将它们放在这里
  * 是因为软驱需要一个时钟，而放在这里是最方便的办法。 */

// 这里用于软驱定时处理的代码。
// 在阅读这段代码之前请先看一下块设备一章中有关软盘驱动程序(floppy.c)后面的说明。
// 或者到阅读软盘块设备驱动程序时在来看这段代码。
// 其中时间单位:1 个滴答 = 1/100 秒。
// 下面数组存放等待软驱马达启动到正常转速的进程指针。数组索引 0-3 分别对应软驱 A-D。
static struct task_struct * wait_motor[4] = {NULL,NULL,NULL,NULL};//四个等待队列
// 下面数组分别存放各软驱马达启动所需要的滴答数。程序中默认启动时间为 50 个滴答(0.5 秒)。
static int  mon_timer[4]={0,0,0,0};
// 下面数组分别存放各软驱在马达停转之前需维持的时间。程序中设定为 10000 个滴答(100 秒)。
static int moff_timer[4]={0,0,0,0};
// 对应软驱控制器中当前数字输出寄存器。该寄存器每位的定义如下: 
// 位 7-4:分别控制驱动器 D-A 马达的启动。1 - 启动;0 - 关闭。
// 位 3 :1 - 允许DMA和中断请求;0 - 禁止DMA和中断请求。 
// 位 2 :1 - 启动软盘控制器;0 - 复位软盘控制器。
// 位 1-0:00 - 11，用于选择控制的软驱 A-D。
unsigned char current_DOR = 0x0C;//0000 1 1 00

// 指定软驱启动到正常运转状态所需等待时间。
// nr -- 软驱号(0-3)，返回值为滴答数。
int ticks_to_floppy_on(unsigned int nr)
{
	// 选中软驱标志(kernel/blk_drv/floppy.c,122)。
	// 所选软驱对应数字输出寄存器中启动马达比特位。
	// mask 高 4 位是各软驱启动马达标志。
	extern unsigned char selected;
	unsigned char mask = 0x10 << nr;

	if (nr>3)	// 系统最多有 4 个软驱。
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

// 等待指定软驱马达启动所需的一段时间，然后返回。
// 设置指定软驱的马达启动到正常转速所需的延时，然后睡眠等待。
// 在定时中断过程中会一直递减判断这里设定的延时值。
// 当延时到期，就会唤醒这里的等待进程。
void floppy_on(unsigned int nr)
{
	cli();
	// 如果马达启动定时还没到，就一直把当前进程置为不可中断睡眠状态并放入等待马达运行的队列中。
	while (ticks_to_floppy_on(nr))
		sleep_on(nr+wait_motor);
	sti();
}

// 置关闭相应软驱马达停转定时器(3 秒)。
// 若不使用该函数明确关闭指定的软驱马达，则在马达开启 100 秒之后也会被关闭。
void floppy_off(unsigned int nr)
{
	moff_timer[nr]=3*HZ;
}

// 软盘定时处理子程序。更新马达启动定时值和马达关闭停转计时值。
// 该子程序会在时钟定时中断过程中被调用，因此系统每经过一个滴答(10ms)就会被调用一次，
// 随时更新马达开启或停转定时器的值。
// 如果某一个马达停转定时到，则将数字输出寄存器马达启动位复位。
void do_floppy_timer(void)
{
	int i;
	unsigned char mask = 0x10;

	for (i=0 ; i<4 ; i++,mask <<= 1) {
		if (!(mask & current_DOR))	// 如果不是 DOR 指定的马达则跳过
			continue;
		if (mon_timer[i]) {
			if (!--mon_timer[i])	// 如果马达启动定时到则唤醒进程
				wake_up(i+wait_motor);
		} else if (!moff_timer[i]) {// 如果马达停转定时到则复位相应马达启动位，
			current_DOR &= ~mask;	// 并更新数字输出寄存器。
			outb(current_DOR,FD_DOR);
		} else
			moff_timer[i]--;	// 马达停转计时递减
	}
}

#define TIME_REQUESTS 64 // 最多可有 64 个定时器链表(64 个任务)

// 定时器链表结构和定时器数组。
// 这里数据结构类似静态链表。
static struct timer_list {
	long jiffies;				// 定时滴答数。
	void (*fn)();				// 定时处理程序。
	struct timer_list * next;	// 下一个定时器。
} timer_list[TIME_REQUESTS], * next_timer = NULL;

// 添加定时器。输入参数为指定的定时值(滴答数)和相应的处理程序指针。 
// 软盘驱动程序(floppy.c)利用该函数执行启动或关闭马达的延时操作。 
// jiffies – 以 10 毫秒计的滴答数;*fn()- 定时时间到时执行的函数。
// 注意这个定时器是内核定时器，主要是给软盘用的，用户不能用。
void add_timer(long jiffies, void (*fn)(void))
{
	struct timer_list * p;

	if (!fn)
		return;
	cli();
	
	// 如果定时值<=0，则立刻调用其处理程序。并且该定时器不加入链表中。
	if (jiffies <= 0)
		(fn)();
	else {
		// 从定时器数组中，找一个空闲项。
		for (p = timer_list ; p < timer_list + TIME_REQUESTS ; p++)
			if (!p->fn)
				break;
		// 如果已经用完了定时器数组，则系统崩溃
		if (p >= timer_list + TIME_REQUESTS)
			panic("No more time requests free");
		// 向定时器数据结构填入相应信息。并链入链表头。
		p->fn = fn;
		p->jiffies = jiffies;
		p->next = next_timer;//类似头插法
		next_timer = p;
		// 链表项按定时值从小到大排序。在排序时减去排在前面需要的滴答数，
		// 这样在处理定时器时只要查看链表头的第一项的定时是否到期即可。
		// 这段程序好象没有考虑周全。如果新插入的定时器值 < 原来头一个定时器值时，
		// 也应该将所有后面的定时值均减去新的第 1 个的定时值。
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

// 时钟中断 C 函数处理程序。 
// 参数 cpl 是当前特权级 0 或 3，0 表示内核代码在执行。
// 对于一个进程由于执行时间片用完时，则进行任务切换。并执行一个计时更新工作。
void do_timer(long cpl)
{
	extern int beepcount;			// 扬声器发声时间滴答数
	extern void sysbeepstop(void);	// 关闭扬声器

	// 如果发声计数次数到，则关闭发声。
	//(向 0x61 口发送命令，复位位 0 和 1。位 0 控制 8253 计数器 2 的工作，位 1 控制扬声器)。
	if (beepcount)
		if (!--beepcount)
			sysbeepstop();

	//特权级由内核栈中保存的进入内核的CS段选择符来得到的
	if (cpl)				//特权级0或者3
		current->utime++;	//用户使用时间加1个滴答
	else
		current->stime++;//内核使用时间加1个滴答

	// 如果有用户的定时器存在，则将链表第 1 个定时器的值减 1。
	// 如果已等于 0，则调用相应的处理程序，并将该处理程序指针置为空。
	// 然后去掉该项定时器。
	if (next_timer) {
		next_timer->jiffies--;
		while (next_timer && next_timer->jiffies <= 0) {
			void (*fn)(void);//局部变量，函数指针的声明格式
			
			fn = next_timer->fn;
			next_timer->fn = NULL;// 去掉该项定时器。
			next_timer = next_timer->next;
			(fn)();//处理函数
		}
	}
	
	// 如果当前软盘控制器 FDC 的数字输出寄存器中马达启动位有置位的，则执行软盘定时程序(245 行)
	if (current_DOR & 0xf0)
		do_floppy_timer();
	// 如果进程运行时间还没完，则退出。处理完中断后继续执行本进程的代码。
	if ((--current->counter)>0) return;
	current->counter=0;
	// 对于内核态程序，不依赖 counter 值进行调度。
	// 这点很重要，因为内核肯定要把事做完。
	if (!cpl) return;
	//时间片用完了要重新调度
	schedule();
}

// 系统调用功能 - 设置报警定时时间值(秒)。
// 如果参数 seconds>0，则设置该新的定时值并返回原定时值。否则返回 0。
int sys_alarm(long seconds)
{
	int old = current->alarm;

	if (old)
		old = (old - jiffies) / HZ;//换成秒
	//换成滴答数，注意结果是要加上开机到现在的滴答数
	current->alarm = (seconds>0)?(jiffies+HZ*seconds):0;
	return (old);
}

// 取当前进程号 pid。
int sys_getpid(void)
{
	return current->pid;
}

// 取父进程号 ppid。
int sys_getppid(void)
{
	return current->father;
}

// 取用户号 uid。
int sys_getuid(void)
{
	return current->uid;
}

// 取 euid。
int sys_geteuid(void)
{
	return current->euid;
}

// 取组号 gid。
int sys_getgid(void)
{
	return current->gid;
}

// 取 egid。
int sys_getegid(void)
{
	return current->egid;
}

// 系统调用功能 -- 降低对 CPU 的使用优先权(有人会用吗?)。
// 应该限制 increment 大于 0，否则的话,可使优先权增大!!
int sys_nice(long increment)
{
	if (current->priority-increment>0)
		current->priority -= increment;
	return 0;
}

// 调度程序的初始化子程序。
void sched_init(void)
{
	int i;
	struct desc_struct * p;// 描述符表结构指针。gdt和ldt

	// sigaction 是存放有关信号状态的结构
	if (sizeof(struct sigaction) != 16)
		panic("Struct sigaction MUST be 16 bytes");

	// 设置初始任务(任务 0)的任务状态段描述符和局部数据表描述符
	set_tss_desc(gdt+FIRST_TSS_ENTRY,&(init_task.task.tss));//tss0
	set_ldt_desc(gdt+FIRST_LDT_ENTRY,&(init_task.task.ldt));//ldt0
	// 清任务数组和描述符表项(注意 i=1 开始，所以初始任务的描述符还在)
	p = gdt+2+FIRST_TSS_ENTRY;
	for(i=1;i<NR_TASKS;i++) {
		task[i] = NULL;
		p->a=p->b=0;
		p++;
		p->a=p->b=0;
		p++;
	}
/* Clear NT, so that we won't have troubles with that later on */
/* 清除标志寄存器中的位 NT，这样以后就不会有麻烦 */
// NT 标志用于控制程序的递归调用(Nested Task)。当 NT 置位时，那么当前中断任务执行
// iret 指令时就会引起任务切换。NT 指出 TSS 中的 back_link 字段是否有效。
	__asm__("pushfl ; andl $0xffffbfff,(%esp) ; popfl");

	// 注意!!是将 GDT 中相应 LDT 描述符的选择符加载到 ldtr。
	// 只明确加载这一次，以后新任务LDT 的加载，是 CPU 根据 TSS 中的 LDT 项自动加载。
	ltr(0);					// 将任务 0 的 TSS 加载到任务寄存器 tr。
	lldt(0);				// 将局部描述符表加载到局部描述符表寄存器

	// 下面代码用于初始化 8253 定时器
	outb_p(0x36,0x43);		/* binary, mode 3, LSB/MSB, ch 0 */
	outb_p(LATCH & 0xff , 0x40);	/* LSB */		// 定时值低字节
	outb(LATCH >> 8 , 0x40);	/* MSB */			// 定时值高字节
	set_intr_gate(0x20,&timer_interrupt);		// 设置时钟中断处理程序句柄(设置时钟中断门)
	outb(inb_p(0x21)&~0x01,0x21);				// 修改中断控制器屏蔽码，允许时钟中断。

	// 设置系统调用中断门
	set_system_gate(0x80,&system_call);//系统调用中断的注册
}
