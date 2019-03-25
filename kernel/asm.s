/*
 *  linux/kernel/asm.s
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * asm.s contains the low-level code for most hardware faults.
 * page_exception is handled by the mm, so that isnt here. This
 * file also handles (hopefully) fpu-exceptions due to TS-bit, as
 * the fpu must be properly saved/resored. This hasnt been tested.
 */
/*
* asm.s 程序中包括大部分的硬件故障(或出错)处理的底层次代码。页异常是由内存管理程序
* mm 处理的，所以不在这里。此程序还处理(希望是这样)由于 TS-位而造成的 fpu 异常，
* 因为 fpu 必须正确地进行保存/恢复处理，这些还没有测试过。
*/

//本代码文件主要涉及对 Intel 保留的中断 int0--int16 的处理(int17-int31 留作今后使用)。
//以下是一些全局函数名的声明，其原形在 traps.c 中说明。
.globl _divide_error,_debug,_nmi,_int3,_overflow,_bounds,_invalid_op
.globl _double_fault,_coprocessor_segment_overrun
.globl _invalid_TSS,_segment_not_present,_stack_segment
.globl _general_protection,_coprocessor_error,_irq13,_reserved

_divide_error://零除出错
	pushl $_do_divide_error	//真正的处理函数
no_error_code:	//这里是无错误码处理的入口处，用这个处理的do函数都是errorcode和sp两个参数
	xchgl %eax,(%esp)	//eax 被交换入栈
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %edi
	pushl %esi
	pushl %ebp
	push %ds	// !!16 位的段寄存器入栈后也要占用 4 个字节
	push %es
	push %fs
	pushl $0		//第二个参数error code
	lea 44(%esp),%edx	//取原调用返回地址处即用户态堆栈指针位置，并压入堆栈，lea是取地址
	pushl %edx
	movl $0x10,%edx	//内核代码数据段选择符，原来的可能是用户态中断过来的
	mov %dx,%ds
	mov %dx,%es
	mov %dx,%fs
	call *%eax //调用C函数do_divide_error，两个参数的调用函数上两个栈是参数
	addl $8,%esp	//准备恢复寄存器，让堆栈指针重新指向寄存器fs入栈处，因为sp上面两个就是fs了
	pop %fs
	pop %es
	pop %ds
	popl %ebp
	popl %esi
	popl %edi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	iret

_debug://int1debug 调试中断入口点
	pushl $_do_int3		# _do_debug
	jmp no_error_code

_nmi://int2非屏蔽中断
	pushl $_do_nmi
	jmp no_error_code

_int3://调试打断点
	pushl $_do_int3
	jmp no_error_code

_overflow://int4溢出出错
	pushl $_do_overflow
	jmp no_error_code

_bounds://int5边界检查出错中断
	pushl $_do_bounds
	jmp no_error_code

_invalid_op://int6 -- 无效操作指令出错中断
	pushl $_do_invalid_op
	jmp no_error_code

_coprocessor_segment_overrun://int9 -- 协处理器段超出出错中断
	pushl $_do_coprocessor_segment_overrun
	jmp no_error_code

_reserved://int15 – 保留
	pushl $_do_reserved
	jmp no_error_code

//int45 -- ( = 0x20 + 13 ) 数学协处理器(Coprocessor)发出的中断。
//当协处理器执行完一个操作时就会发出 IRQ13 中断信号，以通知 CPU 操作完成
_irq13:
	pushl %eax
	xorb %al,%al
	outb %al,$0xF0
	movb $0x20,%al
	outb %al,$0x20
	jmp 1f
1:	jmp 1f
1:	outb %al,$0xA0
	popl %eax
	jmp _coprocessor_error


//以下中断在调用时会在中断返回地址之后将出错号压入堆栈，因此返回时也需要将出错号弹出。
_double_fault:	//int8 -- 双出错故障。
	pushl $_do_double_fault
error_code:	//这里是带错误码处理的入口处，用这个处理的do函数也都是errorcode和sp两个参数
	xchgl %eax,4(%esp)		# error code <-> %eax
	xchgl %ebx,(%esp)		# &function <-> %ebx
	pushl %ecx
	pushl %edx
	pushl %edi
	pushl %esi
	pushl %ebp
	push %ds
	push %es
	push %fs
	pushl %eax			# error code	//第二个参数
	lea 44(%esp),%eax		# offset	//第一个参数esp
	pushl %eax
	movl $0x10,%eax	//内核代码数据段选择符，原来的可能是用户态中断过来的
	mov %ax,%ds
	mov %ax,%es
	mov %ax,%fs
	call *%ebx	//调用c函数，两个参数已入栈
	addl $8,%esp	//准备恢复寄存器，让堆栈指针重新指向寄存器fs入栈处，因为sp上面两个就是fs了
	pop %fs
	pop %es
	pop %ds
	popl %ebp
	popl %esi
	popl %edi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	iret

_invalid_TSS://int10无效的任务状态段
	pushl $_do_invalid_TSS
	jmp error_code

_segment_not_present://int11 -- 段不存在
	pushl $_do_segment_not_present
	jmp error_code

_stack_segment://int12 -- 堆栈段错误
	pushl $_do_stack_segment
	jmp error_code

_general_protection://int13 -- 一般保护性出错
	pushl $_do_general_protection
	jmp error_code

	
//int7 -- 设备不存在(_device_not_available)在(kernel/system_call.s,148)
//int14 -- 页错误(_page_fault)在(mm/page.s,14)
//int16 -- 协处理器错误(_coprocessor_error)在(kernel/system_call.s,131) 
//时钟中断 int 0x20 (_timer_interrupt)在(kernel/system_call.s,176)
//系统调用 int 0x80 (_system_call)在(kernel/system_call.s,80)