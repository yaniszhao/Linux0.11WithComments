/*
 *  linux/kernel/fork.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *  'fork.c' contains the help-routines for the 'fork' system call
 * (see also system_call.s), and some misc functions ('verify_area').
 * Fork is rather simple, once you get the hang of it, but the memory
 * management can be a bitch. See 'mm/mm.c': 'copy_page_tables()'
 */
/*
 * 'fork.c'中含有系统调用'fork'的辅助子程序(参见 system_call.s)，以及一些其它函数
 * ('verify_area')。一旦你了解了 fork，就会发现它是非常简单的，但内存管理却有些难度。
 * 参见'mm/mm.c'中的'copy_page_tables()'。
 */

 
#include <errno.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>
#include <asm/system.h>

extern void write_verify(unsigned long address);

// 最新进程号，其值由 find_empty_process()生成。通过递增得到。
// last_pid除了fork子进程时的pid，还用于生成新进程的pid，所以用的全局变量。
long last_pid=0;

// 进程空间区域写前验证函数。
// 对当前进程地址从 addr 到 addr + size 这一段空间以页为单位执行写操作前的检测操作。
// 由于检测判断是以页面为单位进行操作，因此程序首先需要找出 addr 所在页面开始地址 start， 
// 然后 start 加上进程数据段基址，使这个 start 变换成 CPU 4G 线性空间中的地址。
// 最后循环调用 write_verify()对指定大小的内存空间进行写前验证。
// 若页面是只读的，则执行共享检验和复制页面操作(写时复制)。
void verify_area(void * addr,int size)
{
	unsigned long start;

	//要验证的是addr的页起始start到addr+size的位置，
	//要多验证start到addr那么多个位置
	start = (unsigned long) addr;
	size += start & 0xfff;
	start &= 0xfffff000;
	//用户态的地址是逻辑地址，加上段基址才是线性地址
	start += get_base(current->ldt[2]);//ds/ss段

	//一页一页的验证是否需要写时复制
	while (size>0) {
		size -= 4096;
		write_verify(start);
		start += 4096;
	}
}

// 设置新任务的代码和数据段基址、限长并复制页表。
// nr 为新任务号;p 是新任务数据结构的指针。
int copy_mem(int nr,struct task_struct * p)
{
	unsigned long old_data_base,new_data_base,data_limit;
	unsigned long old_code_base,new_code_base,code_limit;

	code_limit=get_limit(0x0f);//cs
	data_limit=get_limit(0x17);//ds
	old_code_base = get_base(current->ldt[1]);//cs
	old_data_base = get_base(current->ldt[2]);//ds
	if (old_data_base != old_code_base)// 0.11 版不支持代码和数据段分立的情况
		panic("We don't support separate I&D");
	if (data_limit < code_limit)//应该一样吧
		panic("Bad data_limit");
	//创建中新进程在线性地址空间中的基地址等于 64MB * 其任务号
	new_data_base = new_code_base = nr * 0x4000000;
	p->start_code = new_code_base;
	//设置新进程局部描述符表中段描述符中的基地址
	set_base(p->ldt[1],new_code_base);
	set_base(p->ldt[2],new_data_base);
	//设置新进程的页目录表项和页表项。即把新进程的线性地址内存页对应到实际物理地址内存页面上。
	if (copy_page_tables(old_data_base,new_data_base,data_limit)) {
		free_page_tables(new_data_base,data_limit);
		return -ENOMEM;
	}
	return 0;
}

/*
 *  Ok, this is the main fork-routine. It copies the system process
 * information (task[nr]) and sets up the necessary registers. It
 * also copies the data segment in it's entirety.
 */
/* OK，下面是主要的 fork 子程序。它复制系统进程信息(task[n])并且设置必要的寄存器。
 * 它还整个地复制数据段。 */
int copy_process(int nr,long ebp,long edi,long esi,long gs,long none,
		long ebx,long ecx,long edx,
		long fs,long es,long ds,
		long eip,long cs,long eflags,long esp,long ss)
{
	struct task_struct *p;
	int i;
	struct file *f;

	p = (struct task_struct *) get_free_page();//获得新的页给PCB
	if (!p)
		return -EAGAIN;

	// 将新任务结构指针放入任务数组中。
	// 其中 nr 为任务号，由前面 find_empty_process()返回
	task[nr] = p;

	// 复制所有结构内的内容
	*p = *current;	/* NOTE! this doesn't copy the supervisor stack */
					/* 注意!这样做不会复制超级用户的堆栈(只复制当前进程内容) */
	// 修改特别的内容
	p->state = TASK_UNINTERRUPTIBLE; 	//将新进程的状态先置为不可中断等待状态，以免被执行
	p->pid = last_pid;					//新进程号。由前面调用 find_empty_process()得到
	p->father = current->pid;			//设置父进程
	p->counter = p->priority;			//初始化运行counter数，默认值为15
	p->signal = 0;						//初始化信号位图
	p->alarm = 0;						//初始化报警定时(滴答数)，0表示无报警定时
	p->leader = 0;		/* process leadership doesn't inherit */ //领导权不能继承
	p->utime = p->stime = 0;			//初始化utime和stime
	p->cutime = p->cstime = 0;			//初始化子进程utime和stime
	p->start_time = jiffies;			//设置开始时间
	//设置任务状态段tss
	p->tss.back_link = 0;
	p->tss.esp0 = PAGE_SIZE + (long) p;	//内核态的esp，刚好在PCB的末尾
	p->tss.ss0 = 0x10;					//内核态ss
	//ss1:esp1和ss2:esp2没有用
	p->tss.eip = eip;					//用户态eip，和父进程一样
	p->tss.eflags = eflags;				//用户态eflags，和父进程一样
	p->tss.eax = 0;						//子进程的fork函数返回的是0，但是父进程返回子进程的pid
	p->tss.ecx = ecx;
	p->tss.edx = edx;
	p->tss.ebx = ebx;
	p->tss.esp = esp;					//新进程的esp用的父进程的，所以要求父进程的栈要尽量干净
	p->tss.ebp = ebp;
	p->tss.esi = esi;
	p->tss.edi = edi;
	p->tss.es = es & 0xffff;			//虽然段选择符都是16位，但是数据结构用的是long是32位
	p->tss.cs = cs & 0xffff;
	p->tss.ss = ss & 0xffff;
	p->tss.ds = ds & 0xffff;
	p->tss.fs = fs & 0xffff;
	p->tss.gs = gs & 0xffff;
	p->tss.ldt = _LDT(nr);				//用于设置ldtr
	p->tss.trace_bitmap = 0x80000000;	//高 16 位有效

	// 如果当前任务使用了协处理器，就保存其上下文。
	if (last_task_used_math == current)
		__asm__("clts ; fnsave %0"::"m" (p->tss.i387));

	// 设置新任务的代码和数据段基址、限长并复制页表。
	// 如果出错(返回值不是 0)，则复位任务数组中相应项并释放为该新任务分配的内存页。
	if (copy_mem(nr,p)) {
		task[nr] = NULL;
		free_page((long) p);
		return -EAGAIN;
	}

	// 继承父进程的打开的文件。
	// 如果父进程中有文件是打开的，则将对应文件的打开次数增 1。
	for (i=0; i<NR_OPEN;i++)
		if (f=p->filp[i])
			f->f_count++;
	if (current->pwd)//继承pwd
		current->pwd->i_count++;
	if (current->root)//继承根目录
		current->root->i_count++;
	if (current->executable)//继承执行文件
		current->executable->i_count++;

	//注册tss和ldt的入口地址
	set_tss_desc(gdt+(nr<<1)+FIRST_TSS_ENTRY,&(p->tss));
	set_ldt_desc(gdt+(nr<<1)+FIRST_LDT_ENTRY,&(p->ldt));
	p->state = TASK_RUNNING;	/* do this last, just in case */
								/* 最后再将新任务设置成可运行状态，以防万一 */
	return last_pid;// 返回新进程号(与任务号是不同的)
}

// 为新进程取得不重复的进程号 last_pid，并返回在任务数组中的任务号(数组 index)。
// 进程号是1到MAX_LONG，而任务号是task数组中的下标
int find_empty_process(void)
{
	int i;
	//循环得到一个没被用过的正数作为新的pid
	repeat:
		if ((++last_pid)<0) last_pid=1;//循环到负数之后要变回正数
		for(i=0 ; i<NR_TASKS ; i++)//如果已经被分配了就判断下一个数满足与否
			if (task[i] && task[i]->pid == last_pid) goto repeat;

	//找到一个空的任务块并返回其下标即任务号
	for(i=1 ; i<NR_TASKS ; i++)
		if (!task[i])
			return i;
	return -EAGAIN;
}
