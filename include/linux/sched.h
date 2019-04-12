/*
调度程序头文件，定义了任务结构 task_struct 、初始任务 0 的数据，
还有一些有关描述符参数设置和获取的嵌入式汇编函数宏语句。
*/

#ifndef _SCHED_H
#define _SCHED_H

#define NR_TASKS 64	// 系统中同时最多任务（进程）数。
#define HZ 100	//一秒多少个滴答，一个滴答10ms所以一秒就100个滴答

#define FIRST_TASK task[0]	// 任务 0 比较特殊，所以特意给它单独定义一个符号。
#define LAST_TASK task[NR_TASKS-1]	// 任务数组中的最后一项任务。

#include <linux/head.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <signal.h>

#if (NR_OPEN > 32)
#error "Currently the close-on-exec-flags are in one word, max 32 files/proc"
#endif

// 这里定义了进程运行可能处的状态。
#define TASK_RUNNING		0			//这里是就绪态，不是运行态
#define TASK_INTERRUPTIBLE	1		// 进程处于可中断等待状态。受信号影响
#define TASK_UNINTERRUPTIBLE	2	// 进程处于不可中断等待状态，主要用于 I/O 操作等待。
#define TASK_ZOMBIE		3			// 进程处于僵死状态，已经停止运行，但父进程还没发信号。
#define TASK_STOPPED		4		// 进程已停止。

#ifndef NULL
#define NULL ((void *) 0)
#endif

extern int copy_page_tables(unsigned long from, unsigned long to, long size);
extern int free_page_tables(unsigned long from, unsigned long size);

extern void sched_init(void);
extern void schedule(void);
extern void trap_init(void);
extern void panic(const char * str);
extern int tty_write(unsigned minor,char * buf,int count);

typedef int (*fn_ptr)();	// 定义函数指针类型。

// 下面是数学协处理器使用的结构，主要用于保存进程切换时 i387 的执行状态信息。
struct i387_struct {
	long	cwd;	// 控制字(Control word)。
	long	swd;	// 状态字(Status word)。
	long	twd;	// 标记字(Tag word)。
	long	fip;	// 协处理器代码指针。
	long	fcs;	// 协处理器代码段寄存器。
	long	foo;	// 内存操作数的偏移位置。
	long	fos;	// 内存操作数的段值。
	long	st_space[20];	/* 8*10 bytes for each FP-reg = 80 bytes */
							// 8 个 10 字节的协处理器累加器。
};

// 任务状态段数据结构（参见列表后的 TSS 信息）。
struct tss_struct {
	long	back_link;	/* 16 high bits zero */
	long	esp0;
	long	ss0;		/* 16 high bits zero */
	long	esp1;
	long	ss1;		/* 16 high bits zero */
	long	esp2;
	long	ss2;		/* 16 high bits zero */
	long	cr3;
	long	eip;
	long	eflags;
	long	eax,ecx,edx,ebx;
	long	esp;
	long	ebp;
	long	esi;
	long	edi;
	long	es;		/* 16 high bits zero */
	long	cs;		/* 16 high bits zero */
	long	ss;		/* 16 high bits zero */
	long	ds;		/* 16 high bits zero */
	long	fs;		/* 16 high bits zero */
	long	gs;		/* 16 high bits zero */
	long	ldt;		/* 16 high bits zero */
	long	trace_bitmap;	/* bits: trace 0, bitmap 16-31 */
	struct i387_struct i387;
};

// 这里是任务（进程）数据结构，或称为进程描述符。
// ==========================
// long state 任务的运行状态（-1 不可运行，0 可运行(就绪)，>0 已停止）。
// long counter 任务运行时间计数(递减)（滴答数），运行时间片。
// long priority 运行优先数。任务开始运行时 counter = priority，越大运行越长。
// long signal 信号。是位图，每个比特位代表一种信号，信号值=位偏移值+1。
// struct sigaction sigaction[32] 信号执行属性结构，对应信号将要执行的操作和标志信息。
// long blocked 进程信号屏蔽码（对应信号位图）。
// --------------------------
// int exit_code 任务执行停止的退出码，其父进程会取。
// unsigned long start_code 代码段地址。
// unsigned long end_code 代码长度（字节数）。
// unsigned long end_data 代码长度 + 数据长度（字节数）。
// unsigned long brk 总长度（字节数）。
// unsigned long start_stack 堆栈段地址。
// long pid 进程标识号(进程号)。
// long father 父进程号。
// long pgrp 父进程组号。
// long session 会话号。
// long leader 会话首领。
// unsigned short uid 用户标识号（用户 id）。
// unsigned short euid 有效用户 id。
// unsigned short suid 保存的用户 id。
// unsigned short gid 组标识号（组 id）。
// unsigned short egid 有效组 id。
// unsigned short sgid 保存的组 id。
// long alarm 报警定时值（滴答数）。
// long utime 用户态运行时间（滴答数）。
// long stime 系统态运行时间（滴答数）。
// long cutime 子进程用户态运行时间。
// long cstime 子进程系统态运行时间。
// long start_time 进程开始运行时刻。
// unsigned short used_math 标志：是否使用了协处理器。
// --------------------------
// int tty 进程使用 tty 的子设备号。-1 表示没有使用。
// unsigned short umask 文件创建属性屏蔽位。
// struct m_inode * pwd 当前工作目录 i 节点结构。
// struct m_inode * root 根目录 i 节点结构。
// struct m_inode * executable 执行文件 i 节点结构。
// unsigned long close_on_exec 执行时关闭文件句柄位图标志。（参见 include/fcntl.h）
// struct file * filp[NR_OPEN] 进程使用的文件表结构。
// --------------------------
// struct desc_struct ldt[3] 本任务的局部表描述符。0-空，1-代码段 cs，2-数据和堆栈段 ds&ss。
// --------------------------
// struct tss_struct tss 本进程的任务状态段信息结构。
// ==========================

struct task_struct {
/* these are hardcoded - don't touch */
	long state;	/* -1 unrunnable, 0 runnable, >0 stopped */	//状态
	long counter;	//时间片计数
	long priority;	//优先级，用于分配counter
	long signal;	//信号位图
	struct sigaction sigaction[32];	//信号处理
	long blocked;	/* bitmap of masked signals */ //一表示屏蔽，零表示有效
/* various fields */
	int exit_code;	//进程退出后的退出码，交给处理他尸体的进程
	unsigned long start_code,end_code,end_data,brk,start_stack;	//内存分配
	long pid,father,pgrp,session,leader;	//进程id
	unsigned short uid,euid,suid;	//用户权限id
	unsigned short gid,egid,sgid;	//用户组权限id
	long alarm;	//告警计数
	long utime,stime,cutime,cstime,start_time;	//运行时间
	unsigned short used_math;	//数学协处理器
/* file system info */
	int tty;		/* -1 if no tty, so it must be signed */
	unsigned short umask;	//用户权限的掩码
	struct m_inode * pwd;	//当前工作目录
	struct m_inode * root;	//用户的根目录
	struct m_inode * executable;	//执行程序文件的inode
	unsigned long close_on_exec;	//
	struct file * filp[NR_OPEN];	//打开的文件指针
/* ldt for this task 0 - zero 1 - cs 2 - ds&ss */
	struct desc_struct ldt[3];	//cs ds ss
/* tss for this task */
	struct tss_struct tss;		//tss
};

/*
 *  INIT_TASK is used to set up the first task table, touch at
 * your own risk!. Base=0, limit=0x9ffff (=640kB)
 */
/*
 * INIT_TASK 用于设置第 1 个任务表，若想修改，责任自负?！
 * 基址 Base = 0，段长 limit = 0x9ffff（=640kB）。
 */
// 对应上面任务结构的第 1 个任务的信息。
#define INIT_TASK \
/* state etc */	{ 0,15,15, \
/* signals */	0,{{},},0, \
/* ec,brk... */	0,0,0,0,0,0, \
/* pid etc.. */	0,-1,0,0,0, \
/* uid etc */	0,0,0,0,0,0, \
/* alarm */	0,0,0,0,0,0, \
/* math */	0, \
/* fs info */	-1,0022,NULL,NULL,NULL,0, \
/* filp */	{NULL,}, \
	{ \
		{0,0}, \
/* ldt */	{0x9f,0xc0fa00}, \
		{0x9f,0xc0f200}, \
	}, \
/*tss*/	{0,PAGE_SIZE+(long)&init_task,0x10,0,0,0,0,(long)&pg_dir,\
	 0,0,0,0,0,0,0,0, \
	 0,0,0x17,0x17,0x17,0x17,0x17,0x17, \
	 _LDT(0),0x80000000, \
		{} \
	}, \
}

extern struct task_struct *task[NR_TASKS];		// 任务数组。
extern struct task_struct *last_task_used_math;	// 上一个使用过协处理器的进程。
extern struct task_struct *current;				// 当前进程结构指针变量。
extern long volatile jiffies;					// 从开机开始算起的滴答数（10ms/滴答）。
extern long startup_time;						// 开机时间。从 1970:0:0:0 开始计时的秒数。

#define CURRENT_TIME (startup_time+jiffies/HZ)	// 当前时间（秒数）。

extern void add_timer(long jiffies, void (*fn)(void));
extern void sleep_on(struct task_struct ** p);
extern void interruptible_sleep_on(struct task_struct ** p);
extern void wake_up(struct task_struct ** p);

/*
 * Entry into gdt where to find first TSS. 0-nul, 1-cs, 2-ds, 3-syscall
 * 4-TSS0, 5-LDT0, 6-TSS1 etc ...
 */
/*
 * 寻找第 1 个 TSS 在全局表中的入口。0-没有用 nul，1-代码段 cs，2-数据段 ds，3-系统段 syscall
 * 4-任务状态段 TSS0，5-局部表 LTD0，6-任务状态段 TSS1，等。
 */
// 全局表中第 1 个任务状态段(TSS)描述符的选择符索引号。
#define FIRST_TSS_ENTRY 4
// 全局表中第 1 个局部描述符表(LDT)描述符的选择符索引号。
#define FIRST_LDT_ENTRY (FIRST_TSS_ENTRY+1)
// 宏定义，计算在全局表中第 n 个任务的 TSS 描述符的索引号（选择符）。
#define _TSS(n) ((((unsigned long) n)<<4)+(FIRST_TSS_ENTRY<<3))
// 宏定义，计算在全局表中第 n 个任务的 LDT 描述符的索引号。
#define _LDT(n) ((((unsigned long) n)<<4)+(FIRST_LDT_ENTRY<<3))
// 宏定义，加载第 n 个任务的任务寄存器 tr。
#define ltr(n) __asm__("ltr %%ax"::"a" (_TSS(n)))
// 宏定义，加载第 n 个任务的局部描述符表寄存器 ldtr。
#define lldt(n) __asm__("lldt %%ax"::"a" (_LDT(n)))
// 取当前运行任务的任务号（是任务数组中的索引值，与进程号 pid 不同）。
// 返回：n - 当前任务号。用于( kernel/traps.c, 79)。
#define str(n) \
__asm__("str %%ax\n\t" \
	"subl %2,%%eax\n\t" \
	"shrl $4,%%eax" \
	:"=a" (n) \
	:"a" (0),"i" (FIRST_TSS_ENTRY<<3))
/*
 *	switch_to(n) should switch tasks to task nr n, first
 * checking that n isn't the current task, in which case it does nothing.
 * This also clears the TS-flag if the task we switched to has used
 * tha math co-processor latest.
 */
/*
 * switch_to(n)将切换当前任务到任务 nr，即 n。首先检测任务 n 不是当前任务，
 * 如果是则什么也不做退出。如果我们切换到的任务最近（上次运行）使用过数学
 * 协处理器的话，则还需复位控制寄存器 cr0 中的 TS 标志。
 */
// 跳转到一个任务的 TSS 段选择符组成的地址处会造成 CPU 进行任务切换操作。
// 输入：%0 - 偏移地址(*&__tmp.a)； %1 - 存放新 TSS 的选择符；
// dx - 新任务 n 的 TSS 段选择符；ecx - 新任务指针 task[n]。
// 其中临时数据结构__tmp 用于组建 177 行远跳转（far jump）指令的操作数。该操作数由 4 字节偏移
// 地址和 2 字节的段选择符组成。因此__tmp 中 a 的值是 32 位偏移值，而 b 的低 2 字节是新 TSS 段的
// 选择符（高 2 字节不用）。跳转到 TSS 段选择符会造成任务切换到该 TSS 对应的进程。对于造成任务
// 切换的长跳转，a 值无用。177 行上的内存间接跳转指令使用 6 字节操作数作为跳转目的地的长指针，
// 其格式为：jmp 16 位段选择符：32 位偏移值。但在内存中操作数的表示顺序与这里正好相反。
// 在判断新任务上次执行是否使用过协处理器时，是通过将新任务状态段地址与保存在
// last_task_used_math 变量中的使用过协处理器的任务状态段地址进行比较而作出的，
// 参见 kernel/sched.c 中函数 math_state_restore()。
#define switch_to(n) {\
struct {long a,b;} __tmp; \
__asm__("cmpl %%ecx,_current\n\t" \	// 任务 n 是当前任务吗?(current ==task[n]?)
	"je 1f\n\t" \					// 是，则什么都不做，退出。
	"movw %%dx,%1\n\t" \			// 将新任务 16 位选择符存入__tmp.b 中。
	"xchgl %%ecx,_current\n\t" \	// current = task[n]；ecx = 被切换出的任务。
	"ljmp %0\n\t" \					// 执行长跳转至*&__tmp，造成任务切换。
									// 在任务切换回来后才会继续执行下面的语句。
	"cmpl %%ecx,_last_task_used_math\n\t" \	// 新任务上次使用过协处理器吗？
	"jne 1f\n\t" \							// 没有则跳转，退出。
	"clts\n" \								// 新任务上次使用过协处理器，则清 cr0 的 TS 标志。
	"1:" \
	::"m" (*&__tmp.a),"m" (*&__tmp.b), \
	"d" (_TSS(n)),"c" ((long) task[n])); \
}

// 页面地址对准。（在内核代码中没有任何地方引用!!）
#define PAGE_ALIGN(n) (((n)+0xfff)&0xfffff000)

//通过段基址设置gdt或ldt
#define _set_base(addr,base) \
__asm__("movw %%dx,%0\n\t" \	// 基址 base 低 16 位(位 15-0)-->[addr+2]。
	"rorl $16,%%edx\n\t" \		// edx 中基址高 16 位(位 31-16)-->dx。
	"movb %%dl,%1\n\t" \		// 基址高 16 位中的低 8 位(位 23-16)-->[addr+4]。
	"movb %%dh,%2" \			// 基址高 16 位中的高 8 位(位 31-24)-->[addr+7]。
	::"m" (*((addr)+2)), \
	  "m" (*((addr)+4)), \
	  "m" (*((addr)+7)), \
	  "d" (base) \
	:"dx")

//通过段基址设置gdt或ldt
#define _set_limit(addr,limit) \
__asm__("movw %%dx,%0\n\t" \		// 段长 limit 低 16 位(位 15-0)-->[addr]。
	"rorl $16,%%edx\n\t" \			// edx 中的段长高 4 位(位 19-16)-->dl。
	"movb %1,%%dh\n\t" \			// 取原[addr+6]字节-->dh，其中高 4 位是些标志。
	"andb $0xf0,%%dh\n\t" \			// 清 dh 的低 4 位(将存放段长的位 19-16)。
	"orb %%dh,%%dl\n\t" \			// 将原高 4 位标志和段长的高 4 位(位 19-16)合成 1 字节，
	"movb %%dl,%1" \				// 并放会[addr+6]处。
	::"m" (*(addr)), \
	  "m" (*((addr)+6)), \
	  "d" (limit) \
	:"dx")

// 设置局部描述符表中 ldt 描述符的基地址字段。
#define set_base(ldt,base) _set_base( ((char *)&(ldt)) , base )
// 设置局部描述符表中 ldt 描述符的段长字段。
#define set_limit(ldt,limit) _set_limit( ((char *)&(ldt)) , (limit-1)>>12 )

//得到段基址，功能与_set_base()正好相反。
#define _get_base(addr) ({\
unsigned long __base; \
__asm__("movb %3,%%dh\n\t" \	// 取[addr+7]处基址高 16 位的高 8 位(位 31-24)-->dh。
	"movb %2,%%dl\n\t" \		// 取[addr+4]处基址高 16 位的低 8 位(位 23-16)-->dl。
	"shll $16,%%edx\n\t" \		// 基地址高 16 位移到 edx 中高 16 位处。
	"movw %1,%%dx" \			// 取[addr+2]处基址低 16 位(位 15-0)-->dx。
	:"=d" (__base) \			// 从而 edx 中含有 32 位的段基地址。
	:"m" (*((addr)+2)), \
	 "m" (*((addr)+4)), \
	 "m" (*((addr)+7))); \
__base;})

#define get_base(ldt) _get_base( ((char *)&(ldt)) )

//得到段限长
// 指令 lsl 是 Load Segment Limit 缩写。它从指定段描述符中取出分散的限长比特位拼成完整的
// 段限长值放入指定寄存器中。所得的段限长是实际字节数减 1，因此这里还需要加 1 后才返回。
// %0 - 存放段长值(字节数)；%1 - 段选择符 segment。
#define get_limit(segment) ({ \
unsigned long __limit; \
__asm__("lsll %1,%0\n\tincl %0":"=r" (__limit):"r" (segment)); \
__limit;})

#endif
