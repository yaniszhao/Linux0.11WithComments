/*
 *  linux/boot/head.s
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *  head.s contains the 32-bit startup code.
 *
 * NOTE!!! Startup happens at absolute address 0x00000000, which is also where
 * the page directory will exist. The startup code will be overwritten by
 * the page directory.
 */
.text
.globl _idt,_gdt,_pg_dir,_tmp_floppy_area	//;要用的符号声明

_pg_dir:	//;当代码被执行到后面的时候，这些代码就没用了，被当做页目录
startup_32:

	movl $0x10,%eax	//;对于GNU汇编来说，每个直接数要以$开始，否则是表示地址
	mov %ax,%ds	//;设置各个数据段寄存器00010 0 00
	mov %ax,%es
	mov %ax,%fs
	mov %ax,%gs
	lss _stack_start,%esp	//;_stack_start在sched.c文件中定义
				//;lss--通过6个字节的数据结构设置ss:sp
				//;在运行任务0之前它是内核代码栈，以后用作任务0和1的用户态栈

	call setup_idt		//;因为这里函数调用要用到栈所以之前就得初始化栈
	call setup_gdt		//;设置临时的中断表

	//;由于段描述符中的段限长从setup程序的8MB改成了本程序设置的16MB
	//;因此这里再次对所有段寄存器执行加载操作是必须的
	movl $0x10,%eax		# reload all the segment registers
	mov %ax,%ds		# after changing gdt. CS was already
	mov %ax,%es		# reloaded in 'setup_gdt'
	mov %ax,%fs		//;因为修改了gdt，所以需要重新装载所有的段寄存器
	mov %ax,%gs		//;CS 代码段寄存器已经在 setup_gdt 中重新加载过了
	lss _stack_start,%esp

	//;测试A20地址线是否已经开启。采用的方法是向内存地址 0x000000 处写入任意一个数值，
	//;然后看内存地址 0x100000(1M)处是否也是这个数值。
	//;如果一直相同的话，就一直比较下去，也即死循环、死机。
	//;表示地址 A20 线没有选通，结果内核就不能使用 1M 以上内存
	xorl %eax,%eax
1:	incl %eax		# check that A20 really IS enabled
	movl %eax,0x000000	# loop forever if it isnt
	cmpl %eax,0x100000
	je 1b
/*
 * NOTE! 486 should set bit 16, to check for write-protect in supervisor
 * mode. Then it would be unnecessary with the "verify_area()"-calls.
 * 486 users probably want to set the NE (#5) bit also, so as to use
 * int 16 for math errors.	//;检查数学协处理器芯片是否存在
 */
	movl %cr0,%eax		# check math chip
	andl $0x80000011,%eax	# Save PG,PE,ET
/* "orl $0x10020,%eax" here for 486 might be good */
	orl $2,%eax		# set MP
	movl %eax,%cr0
	call check_x87
	jmp after_page_tables

/*
 * We depend on ET to be correct. This checks for 287/387.
 */
check_x87:
	fninit
	fstsw %ax
	cmpb $0,%al
	je 1f			/* no coprocessor: have to set bits */
	movl %cr0,%eax
	xorl $6,%eax		/* reset MP, set EM */
	movl %eax,%cr0
	ret
.align 2	//;四字节对齐
1:	.byte 0xDB,0xE4		/* fsetpm for 287, ignored by 387 */
	ret

/*
 *  setup_idt
 *
 *  sets up a idt with 256 entries pointing to
 *  ignore_int, interrupt gates. It then loads
 *  idt. Everything that wants to install itself
 *  in the idt-table may do so themselves. Interrupts
 *  are enabled elsewhere, when we can be relatively
 *  sure everything is ok. This routine will be over-
 *  written by the page tables.
 */
setup_idt:	//;设置临时的中断表
	lea ignore_int,%edx	//;lea是取ignore_int的地址而不是地址下的值
	movl $0x00080000,%eax	//;高16位设成0x0008
	movw %dx,%ax		/* selector = 0x0008 = cs */	//;设置低16位
				//;此时eax有了表项的低4个字节
	movw $0x8E00,%dx	/* interrupt gate - dpl=0, present */	//;设置高4个字节
				//;HHHH8E00 0008LLLL 
	lea _idt,%edi	//;idt表就在本文件后面
	mov $256,%ecx		//;总共256个表项，每个8字节，共2KB内存
rp_sidt:
	movl %eax,(%edi)	//;填充低4字节
	movl %edx,4(%edi)	//;填充高4字节
	addl $8,%edi		//;准备填充后8字节
	dec %ecx		//;完成了一次填充
	jne rp_sidt		//;重复256次
	lidt idt_descr		//;加载6字节48位的基地址和限长
	ret

/*
 *  setup_gdt
 *
 *  This routines sets up a new gdt and loads it.
 *  Only two entries are currently built, the same
 *  ones that were built in init.s. The routine
 *  is VERY complicated at two whole lines, so this
 *  rather long comment is certainly needed :-).
 *  This routine will beoverwritten by the page tables.
 */
setup_gdt:
	lgdt gdt_descr	//;只需要加载6字节48位的基地址和限长
			//;表项是通过手动填充的
	ret

/*
 * I put the kernel page tables right after the page directory,
 * using 4 of them to span 16 Mb of physical memory. People with
 * more than 16MB will have to expand this.
 */
 //;内存页表直接放在页目录之后，使用了 4 个表来寻址 16 Mb 的物理内存。
 //;如果你有多于 16 Mb 的内存，就需要在这里进行扩充修改。

 //;页表项具体含义:

 
.org 0x1000	//;从偏移0x1000处开始是第1个页表(偏移0开始处将存放页表目录)
pg0:

.org 0x2000
pg1:

.org 0x3000
pg2:

.org 0x4000
pg3:

.org 0x5000	//;从这开始不是页表或者页目录了
/*
 * tmp_floppy_area is used by the floppy-driver when DMA cannot
 * reach to a buffer-block. It needs to be aligned, so that it isnt
 * on a 64kB border.
 */
 //;当DMA(直接存储器访问)不能访问缓冲块时，下面的tmp_floppy_area内存块就可供软盘驱动程序使用。
 //;其地址需要对齐调整，这样就不会跨越 64kB 边界
_tmp_floppy_area:
	.fill 1024,1,0	//;1024 项，每项 1 字节，填充数值 0

after_page_tables:	//;下面的几个入栈操作是为了进入main做准备
	pushl $0		# These are the parameters to main :-)	//;envp
	pushl $0		//;argv
	pushl $0		//;argc
	pushl $L6		# return address for main, if it decides to.//;返回后执行的指令
	pushl $_main		//;main其实都没用到前面三个参数
	jmp setup_paging	//;段内跳转，设置页表项
L6:	//;main不应该会返回，若返回了就死循环
	jmp L6			# main should never return here, but
				# just in case, we know what happens.

/* This is the default interrupt "handler" :-) */
int_msg:
	.asciz "Unknown interrupt\n\r"
.align 2	//;四字节对齐
ignore_int:	//;临时的中断处理程序
	pushl %eax
	pushl %ecx
	pushl %edx
	push %ds	//;这里请注意!!ds,es,fs,gs 等虽然是 16 位的寄存器，
	push %es	//;但入栈后仍然会以 32 位的形式入栈，也即需要占用 4 个字节的堆栈空间
	push %fs
	movl $0x10,%eax	//;重新设置段选择符00010 0 00
	mov %ax,%ds
	mov %ax,%es
	mov %ax,%fs
	pushl $int_msg	//;printk的参数
	call _printk
	popl %eax	//;处理掉栈理的printk的参数
	pop %fs		//;恢复保存的数据
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %eax
	iret		//;中断返回


/*
 * Setup_paging
 *
 * This routine sets up paging by setting the page bit
 * in cr0. The page tables are set up, identity-mapping
 * the first 16MB. The pager assumes that no illegal
 * addresses are produced (ie >4Mb on a 4Mb machine).
 *
 * NOTE! Although all physical memory should be identity
 * mapped by this routine, only the kernel page functions
 * use the >1Mb addresses directly. All "normal" functions
 * use just the lower 1Mb, or the local data space, which
 * will be mapped to some other place - mm keeps track of
 * that.
 *
 * For those with more memory than 16 Mb - tough luck. Ive
 * not got it, why should you :-) The source is here. Change
 * it. (Seriously - it shouldn't be too difficult. Mostly
 * change some constants etc. I left it at 16Mb, as my machine
 * even cannot be extended past that (ok, but it was cheap :-)
 * I've tried to show which constants to change by having
 * some kind of marker at them (search for "16Mb"), but I
 * won't guarantee that's all :-( )
 */
//;这个子程序通过设置控制寄存器 cr0 的标志(PG 位 31)来启动对内存的分页处理功能，
//;并设置各个页表项的内容，以恒等映射前 16 MB 的物理内存。分页器假定不会产生非法的
//;地址映射(也即在只有 4Mb 的机器上设置出大于 4Mb 的内存地址)。
//;注意!尽管所有的物理地址都应该由这个子程序进行恒等映射，但只有内核页面管理函数能
//;直接使用>1Mb 的地址。所有“一般”函数仅使用低于 1Mb 的地址空间，或者是使用局部数据
//;空间，地址空间将被映射到其它一些地方去 -- mm(内存管理程序)会管理这些事的。
//;对于那些有多于 16Mb 内存的家伙 – 真是太幸运了，我还没有，为什么你会有?。代码就在
//;这里，对它进行修改吧。(实际上，这并不太困难的。通常只需修改一些常数等。我把它设置
//;为 16Mb，因为我的机器再怎么扩充甚至不能超过这个界限(当然，我的机器是很便宜的?)。
//;我已经通过设置某类标志来给出需要改动的地方(搜索“16Mb”)，但我不能保证作这些
//;改动就行了)。

//;1 页内存长度是 4096 字节
//;页目录表是系统所有进程公用的，而这里的 4 页页表则是属于内核专用
//;对于新的进程，系统会在主内存区为其申请页面存放页表
//;也就是说每个进程会有自己的页表项

//;一个表项占4个字节
.align 2
setup_paging:
	//;首先对 5 页内存(1 页目录 + 4 页页表)清零
	movl $1024*5,%ecx		/* 5 pages - pg_dir+4 page tables */
	xorl %eax,%eax
	xorl %edi,%edi			/* pg_dir is at 0x000 */
	cld;rep;stosl	//;stos将eax中的值拷贝到ES:EDI指向的地址

	//;页目录项只需要设置4个
	//;页表的起始地址都是0x0000X000加7后就是0x0000X007
	//;页表所在的地址 = 0x0000X007 & 0xfffff000 = 0xX000
	//;页表的属性标志 = 0x0000X007 & 0x00000fff = 0x07，表示该页存在、用户可读写
	movl $pg0+7,_pg_dir		/* set present bit/user r/w */
	movl $pg1+7,_pg_dir+4		/*  --------- " " --------- */
	movl $pg2+7,_pg_dir+8		/*  --------- " " --------- */
	movl $pg3+7,_pg_dir+12		/*  --------- " " --------- */
	movl $pg3+4092,%edi
	movl $0xfff007,%eax		/*  16Mb - 4096 + 7 (r/w user,p) */
	std
1:	stosl			/* fill pages backwards - more efficient :-) */
	subl $0x1000,%eax
	jge 1b
	xorl %eax,%eax		/* pg_dir is at 0x0000 */
	movl %eax,%cr3		/* cr3 - page directory start */
	movl %cr0,%eax
	orl $0x80000000,%eax
	movl %eax,%cr0		/* set paging (PG) bit */
	ret			/* this also flushes prefetch-queue */	//;去执行main

.align 2
.word 0
idt_descr:
	.word 256*8-1		# idt contains 256 entries	//;表长度
	.long _idt		//;基地址
.align 2
.word 0
gdt_descr:
	.word 256*8-1		# so does gdt (not that thats any
	.long _gdt		# magic number, but it works for me :^)

	.align 3		//;8字节对齐
_idt:	.fill 256,8,0		# idt is uninitialized	//;256 项，每项 8 字节，填 0

_gdt:	.quad 0x0000000000000000	/* NULL descriptor */	//;第一个不用
	.quad 0x00c09a0000000fff	/* 16Mb */ //;内核代码段00 c09a 000000 0fff
					//;base-00000000，limit-0fff改成了16M
					//;c09a-1100 0000 1001 1010
	.quad 0x00c0920000000fff	/* 16Mb */ //;内核数据段00 c092 0000000 fff
					//;base-00000000，limit-0fff改成了16M
					//;c092-1100 0000 1001 0010
	.quad 0x0000000000000000	/* TEMPORARY - dont use */
	.fill 252,8,0			/* space for LDT's and TSS's etc */ //;用于ldt和tss
