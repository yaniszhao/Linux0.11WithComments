/*
 *  linux/kernel/system_call.s
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *  system_call.s  contains the system-call low-level handling routines.
 * This also contains the timer-interrupt handler, as some of the code is
 * the same. The hd- and flopppy-interrupts are also here.
 *
 * NOTE: This code handles signal-recognition, which happens every time
 * after a timer-interrupt and after each system call. Ordinary interrupts
 * dont handle signal-recognition, as that would clutter them up totally
 * unnecessarily.
 *
 * ;system_call.s 文件包含系统调用(system-call)底层处理子程序。由于有些代码比较类似，所以
 * ;同时也包括时钟中断处理(timer-interrupt)句柄。硬盘和软盘的中断处理程序也在这里。
 *
 * ;注意：这段代码处理信号(signal)识别，在每次时钟中断和系统调用之后都会进行识别。一般
 * ;中断信号并不处理信号识别，因为会给系统造成混乱。
 *
 * Stack layout in 'ret_from_system_call':
 *//;从系统调用返回('ret_from_system_call')时堆栈的内容
 *	 0(%esp) - %eax
 *	 4(%esp) - %ebx
 *	 8(%esp) - %ecx
 *	 C(%esp) - %edx
 *	10(%esp) - %fs
 *	14(%esp) - %es
 *	18(%esp) - %ds
 *	1C(%esp) - %eip
 *	20(%esp) - %cs
 *	24(%esp) - %eflags
 *	28(%esp) - %oldesp
 *	2C(%esp) - %oldss
 */
//;system_call.s 文件包含系统调用(system-call)底层处理子程序。
//;由于有些代码比较类似，所以同时也包括时钟中断处理(timer-interrupt)句柄。
//;硬盘和软盘的中断处理程序也在这里。
//;注意:这段代码处理信号(signal)识别，在每次时钟中断和系统调用之后都会进行识别。
//;一般中断信号并不处理信号识别，因为会给系统造成混乱。

SIG_CHLD	= 17		//;定义 SIG_CHLD 信号(子进程停止或结束)

EAX		= 0x00
EBX		= 0x04
ECX		= 0x08
EDX		= 0x0C
FS		= 0x10
ES		= 0x14
DS		= 0x18
EIP		= 0x1C
CS		= 0x20
EFLAGS		= 0x24
OLDESP		= 0x28
OLDSS		= 0x2C

//;以下这些是任务结构(task_struct)中变量的偏移值
state	= 0		# these are offsets into the task-struct. //;进程状态码
counter	= 4		//;任务运行时间计数(递减)(滴答数)，运行时间片
priority = 8		//;运行优先数。任务开始运行时 counter=priority，越大则运行时间越长
signal	= 12		//;是信号位图，每个比特位代表一种信号，信号值=位偏移值+1
sigaction = 16		# MUST be 16 (=len of sigaction) // ;sigaction 结构长度必须是 16 字节
blocked = (33*16)	//;受阻塞信号位图的偏移量

//;以下定义在 sigaction 结构中的偏移量
# offsets within sigaction
sa_handler = 0		// ;信号处理过程的句柄(描述符)
sa_mask = 4		// ;信号量屏蔽码
sa_flags = 8		// ;信号集
sa_restorer = 12	// ;恢复函数指针

nr_system_calls = 72	//;Linux 0.11 版内核中的系统调用总数

/*
 * Ok, I get parallel printer interrupts while using the floppy for some
 * strange reason. Urgel. Now I just ignore them.
 */
.globl _system_call,_sys_fork,_timer_interrupt,_sys_execve
.globl _hd_interrupt,_floppy_interrupt,_parallel_interrupt
.globl _device_not_available, _coprocessor_error

.align 2
bad_sys_call://;错误的系统调用号
	movl $-1,%eax	//;eax 中置-1，退出中断
	iret
.align 2
reschedule://;重新执行调度程序入口
	pushl $ret_from_sys_call
	jmp _schedule
.align 2
_system_call://;int 0x80 --linux 系统调用入口点(调用中断 int 0x80，eax 中是调用号)
	cmpl $nr_system_calls-1,%eax //;调用号如果超出范围的话就在 eax 中置-1 并退出
	ja bad_sys_call
	push %ds		//;保存原段寄存器值
	push %es
	push %fs
	pushl %edx
	pushl %ecx		# push %ebx,%ecx,%edx as parameters
	pushl %ebx		# to the system call
	movl $0x10,%edx		# set up ds,es to kernel space //;00010 0 00内核态
	mov %dx,%ds
	mov %dx,%es
	movl $0x17,%edx		# fs points to local data space//;00010 1 11用户态
	mov %dx,%fs
	call _sys_call_table(,%eax,4)	//;调用地址 = _sys_call_table + %eax*4
	pushl %eax			//;把系统调用返回值入栈
	movl _current,%eax
	//;查看当前任务的运行状态。如果不在就绪状态(state 不等于 0)就去执行调度程序。
	//;如果该任务在就绪状态，但其时间片已用完(counter=0)，则也去执行调度程序
	cmpl $0,state(%eax)		# state		//;比如sleep走到这就应该重新调度
	jne reschedule
	cmpl $0,counter(%eax)		# counter	//;比如时钟中断让counter减到0后
	je reschedule
ret_from_sys_call://;系统调用 C 函数返回后，对信号量进行识别处理
	//;首先判别当前任务是否是初始任务 task0，如果是则不必对其进行信号量方面的处理，直接返回
	movl _current,%eax		# task[0] cannot have signals
	cmpl _task,%eax			//;task0的PCB起始位置
	je 3f
	//;通过对原调用程序代码选择符的检查来判断调用程序是否是内核任务(例如任务 1)。
	//;如果是则直接退出中断。否则对于普通进程则需进行信号量的处理。
	//;这里比较选择符是否为普通用户代码段的选择符 0x000f (RPL=3，局部表，第 1 个段(代码段))，
	//;如果不是则跳转退出中断程序
	cmpw $0x0f,CS(%esp)		# was old code segment supervisor ?
	jne 3f
	//;如果原堆栈段选择符不为 0x17(也即原堆栈不在用户数据段中)，则也退出
	cmpw $0x17,OLDSS(%esp)		# was stack segment = 0x17 ?
	jne 3f
	movl signal(%eax),%ebx		//;取信号位图-->ebx，每 1 位代表 1 种信号，共 32 个信号
	movl blocked(%eax),%ecx		//;取阻塞(屏蔽)信号位图-->ecx
	notl %ecx			//;每位取反
	andl %ebx,%ecx			//;获得许可的信号位图
	bsfl %ecx,%ecx			//;从低位(位 0)开始扫描位图，看是否有 1 的位，
					//;若有，则 ecx 保留该位的偏移值(即第几位 0-31
	je 3f				//;如果没有信号则向前跳转退出
	btrl %ecx,%ebx			//;复位该信号(ebx 含有原 signal 位图)
	movl %ebx,signal(%eax)		//;重新保存 signal 位图信息
	incl %ecx			//;将信号调整为从 1 开始的数(1-32)
	pushl %ecx			//;信号值入栈作为调用 do_signal 的参数之一
	call _do_signal
	popl %eax			//;弹出信号值
3:	popl %eax
	popl %ebx
	popl %ecx
	popl %edx
	pop %fs
	pop %es
	pop %ds
	iret

//;下面这段代码处理协处理器发出的出错信号。跳转执行 C 函数 math_error()
//;返回后将跳转到 ret_from_sys_call 处继续执行
.align 2
_coprocessor_error:
	push %ds
	push %es
	push %fs
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax
	movl $0x10,%eax
	mov %ax,%ds
	mov %ax,%es
	movl $0x17,%eax
	mov %ax,%fs
	pushl $ret_from_sys_call
	jmp _math_error			//;执行 C 函数 math_error()

//;int7 -- 设备不存在或协处理器不存在(Coprocessor not available)。
//;如果控制寄存器 CR0 的 EM 标志置位，则当 CPU 执行一个 ESC 转义指令时就会引发该中断，
//;这样就可以有机会让这个中断处理程序模拟 ESC 转义指令(169 行)。
//;CR0的TS标志是在CPU执行任务转换时设置的。
//;TS可以用来确定什么时候协处理器中的内容(上下文)与CPU 正在执行的任务不匹配了。
//;当 CPU 在运行一个转义指令时发现 TS 置位了，就会引发该中断。
//;此时就应该恢复新任务的协处理器执行状态(165 行)。参见(kernel/sched.c,77)中的说明。
//;该中断最后将转移到标号 ret_from_sys_call 处执行下去(检测并处理信号)。
.align 2
_device_not_available:
	push %ds
	push %es
	push %fs
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax
	movl $0x10,%eax
	mov %ax,%ds
	mov %ax,%es
	movl $0x17,%eax
	mov %ax,%fs
	pushl $ret_from_sys_call
	clts				# clear TS so that we can use math
	movl %cr0,%eax
	//;EM (math emulation bit)如果不是 EM 引起的中断，则恢复新任务协处理器状态，
	testl $0x4,%eax			# EM (math emulation bit)
	je _math_state_restore
	pushl %ebp
	pushl %esi
	pushl %edi
	call _math_emulate
	popl %edi
	popl %esi
	popl %ebp
	ret				//;这里的 ret 将跳转到 ret_from_sys_call

//;int32 -- (int 0x20) 时钟中断处理程序。
//;中断频率被设置为 100Hz(include/linux/sched.h,5)，
//;定时芯片 8253/8254 是在(kernel/sched.c,406)处初始化的。
//;因此这里 jiffies 每 10 毫秒加 1。
//;这段代码将 jiffies 增 1，发送结束中断指令给 8259 控制器，
//;然后用当前特权级作为参数调用 C 函数 do_timer(long CPL)。
//;当调用返回时转去检测并处理信号。
.align 2
_timer_interrupt:
	push %ds		# save ds,es and put kernel data space
	push %es		# into them. %fs is used by _system_call
	push %fs
	pushl %edx		# we save %eax,%ecx,%edx as gcc doesnt
	pushl %ecx		# save those across function calls. %ebx
	pushl %ebx		# is saved as we use that in ret_sys_call
	pushl %eax
	movl $0x10,%eax
	mov %ax,%ds
	mov %ax,%es
	movl $0x17,%eax
	mov %ax,%fs
	incl _jiffies		//;滴答数加1
	//;由于初始化中断控制芯片时没有采用自动 EOI，所以这里需要发指令结束该硬件中断
	movb $0x20,%al		# EOI to interrupt controller #1
	outb %al,$0x20
	//;下面3句从CS选择符中取出当前特权级别(0 或 3)并压入堆栈，作为 do_timer 的参数
	movl CS(%esp),%eax
	andl $3,%eax		# %eax is CPL (0 or 3, 0=supervisor)
	pushl %eax
	//;do_timer(CPL)执行任务切换、计时等工作
	call _do_timer		# 'do_timer(long CPL)' does everything from
				# task switching to accounting ...
	addl $4,%esp		//;处理传入do_timer参数的栈
	jmp ret_from_sys_call

//;这是 sys_execve()系统调用。取中断调用程序的代码指针作为参数调用 C 函数 do_execve()
.align 2
_sys_execve:
	lea EIP(%esp),%eax
	pushl %eax
	call _do_execve
	addl $4,%esp
	ret

//;sys_fork()调用，用于创建子进程，是 system_call 功能 2。
//;首先调用 C 函数 find_empty_process()，取得一个进程号 pid。
//;若返回负数则说明目前任务数组已满。然后调用 copy_process()复制进程。
.align 2
_sys_fork:
	call _find_empty_process	//;找到一个空的PCB项
	testl %eax,%eax
	js 1f
	push %gs			//;下面几个入栈是传递参数
	pushl %esi
	pushl %edi
	pushl %ebp
	pushl %eax
	call _copy_process		//;复制进程
	addl $20,%esp			//;恢复函数调用的参数栈
1:	ret

//;int 46 -- (int 0x2E) 硬盘中断处理程序，响应硬件中断请求 IRQ14。
//;当硬盘操作完成或出错就会发出此中断信号。
//;首先向 8259A 中断控制从芯片发送结束硬件中断指令(EOI)，
//;然后取变量 do_hd 中的函数指针放入 edx 寄存器中，并置 do_hd 为 NULL，
//;接着判断 edx 函数指针是否为空。如果为空，则给 edx 赋值指向unexpected_hd_interrupt()，
//;用于显示出错信息。随后向 8259A 主芯片送 EOI 指令，并调用 edx 中指针指向的函数:
//;read_intr()、write_intr()或 unexpected_hd_interrupt()。
_hd_interrupt:
	pushl %eax
	pushl %ecx
	pushl %edx
	push %ds
	push %es
	push %fs
	movl $0x10,%eax
	mov %ax,%ds
	mov %ax,%es
	movl $0x17,%eax
	mov %ax,%fs
	//;由于初始化中断控制芯片时没有采用自动 EOI，所以这里需要发指令结束该硬件中断。
	movb $0x20,%al
	outb %al,$0xA0		# EOI to interrupt controller #1
	jmp 1f			# give port chance to breathe
1:	jmp 1f			//;延时作用。
1:	xorl %edx,%edx
	xchgl _do_hd,%edx	//;do_hd定义为一个函数指针，将被赋值read_intr()或write_intr()函数地址。
				//;放到 edx 寄存器后就将 do_hd 指针变量置为 NULL。
	testl %edx,%edx		//;若do_hd本来就为NULL则执行$_unexpected_hd_interrupt
	jne 1f
	movl $_unexpected_hd_interrupt,%edx
1:	outb %al,$0x20		//;送主 8259A 中断控制器 EOI 指令(结束硬件中断)
	call *%edx		# "interesting" way of handling intr. //;调用C的处理函数
	pop %fs
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %eax
	iret

//;int38 -- (int 0x26) 软盘驱动器中断处理程序，响应硬件中断请求 IRQ6。
//;其处理过程与上面对硬盘的处理基本一样。(kernel/blk_drv/floppy.c)。
//;首先向 8259A 中断控制器主芯片发送 EOI 指令，
//;然后取变量 do_floppy 中的函数指针放入 eax 寄存器中，
//;并置 do_floppy 为 NULL，接着判断 eax 函数指针是否为空。
//;如为空，则给 eax 赋值指向unexpected_floppy_interrupt ()，用于显示出错信息。
//;随后调用 eax 指向的函数: rw_interrupt, seek_interrupt,recal_interrupt,reset_interrupt
//;或 unexpected_floppy_interrupt。
_floppy_interrupt:
	pushl %eax
	pushl %ecx
	pushl %edx
	push %ds
	push %es
	push %fs
	movl $0x10,%eax
	mov %ax,%ds
	mov %ax,%es
	movl $0x17,%eax
	mov %ax,%fs
	movb $0x20,%al
	outb %al,$0x20		# EOI to interrupt controller #1
	xorl %eax,%eax
	xchgl _do_floppy,%eax	//;四个函数中的一个或者为NULL
	testl %eax,%eax
	jne 1f
	movl $_unexpected_floppy_interrupt,%eax //;为NULL则置为这个函数
1:	call *%eax		# "interesting" way of handling intr.
	pop %fs
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %eax
	iret
	
//;int 39 -- (int 0x27) 并行口中断处理程序，对应硬件中断请求信号 IRQ7。
//;本版本内核还未实现。这里只是发送 EOI 指令。
_parallel_interrupt:
	pushl %eax
	movb $0x20,%al
	outb %al,$0x20
	popl %eax
	iret
