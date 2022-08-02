/*
 *  linux/kernel/rs_io.s
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *	rs_io.s
 *
 * This module implements the rs232 io interrupts.
 */	/* ;该程序模块实现 rs232 输入输出中断处理程序。*/

.text
.globl _rs1_interrupt,_rs2_interrupt
// ;size 是读写队列缓冲区的字节长度。
size	= 1024				/* must be power of two !
					   and must match the value
					   in tty_io.c!!! */

/* these are the offsets into the read/write buffer structures */
/* ;以下这些是读写缓冲结构中的偏移量 */
// ;对应定义在 include/linux/tty.h 文件中 tty_queue 结构中各变量的偏移量。
rs_addr = 0			// ;串行端口号字段偏移（端口号是 0x3f8 或 0x2f8）。
head = 4			// ;缓冲区中头指针字段偏移。
tail = 8			// ;缓冲区中尾指针字段偏移。
proc_list = 12			// ;等待该缓冲的进程字段偏移。
buf = 16			// ;缓冲区字段偏移。

startup	= 256		/* chars left in write queue when we restart it */
			/* ;当写队列里还剩 256 个字符空间(WAKEUP_CHARS)时，我们就可以写 */

/*
 * These are the actual interrupt routines. They look where
 * the interrupt is coming from, and take appropriate action.
 */	/* ;这些是实际的中断程序。程序首先检查中断的来源，然后执行相应的处理。*/
.align 2
_rs1_interrupt:	// ;串行端口 1 中断处理程序入口点。
	pushl $_table_list+8	// ;tty 表中对应串口 1 的读写缓冲指针的地址入栈(tty_io.c，99)。
	jmp rs_int		// ;字符缓冲队列结构格式请参见 include/linux/tty.h，第 16 行。
.align 2
_rs2_interrupt:	// ;串行端口 2 中断处理程序入口点。
	pushl $_table_list+16	// ;tty 表中对应串口 2 的读写缓冲队列指针的地址入栈。
rs_int:
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax
	push %es
	push %ds		/* as this is an interrupt, we cannot */
	pushl $0x10		/* know that bs is ok. Load it */
	pop %ds			/* ;由于这是一个中断程序，我们不知道 ds 是否正确，*/
	pushl $0x10		/* ;所以加载它们(让 ds、es 指向内核数据段 */
	pop %es
	movl 24(%esp),%edx	// ;将缓冲队列指针地址存入 edx 寄存器，
	movl (%edx),%edx	// ;取读缓冲队列结构指针(地址)-->edx
	movl rs_addr(%edx),%edx	// ;取串口 1（或串口 2）的端口号-->edx。
	addl $2,%edx		/* interrupt ident. reg */	/* ;edx 指向中断标识寄存器 */
				// ;中断标识寄存器端口是 0x3fa(0x2fa)
rep_int:
	xorl %eax,%eax		// ;eax 清零。
	inb %dx,%al		// ;取中断标识字节，用以判断中断来源(有 4 种中断情况)。
	testb $1,%al		// ;首先判断有无待处理的中断(位 0=1 无中断；=0 有中断)。
	jne end			// ;若无待处理中断，则跳转至退出处理处 end。
	cmpb $6,%al		/* this shouldnt happen, but ... */	/* ;这不会发生，但是…*/
	ja end			// ;al 值>6? 是则跳转至 end（没有这种状态）。
	movl 24(%esp),%ecx	// ;再取缓冲队列指针地址-->ecx。
	pushl %edx		// ;将中断标识寄存器端口号 0x3fa(0x2fa)入栈。
	subl $2,%edx		// ;0x3f8(0x2f8)。
	call jmp_table(,%eax,2)		/* NOTE! not *4, bit0 is 0 already */
					/* ;不乘 4，位 0 已是 0*/
// ;上面语句是指，当有待处理中断时，al 中位 0=0，位 2-1 是中断类型，因此相当于已经将中断类型
// ;乘了 2，这里再乘 2，获得跳转表（第 79 行）对应各中断类型地址，并跳转到那里去作相应处理。
// ;中断来源有 4 种：modem 状态发生变化；要写（发送）字符；要读（接收）字符；线路状态发生变化。
// ;要发送字符中断是通过设置发送保持寄存器标志实现的。在 serial.c 程序中的 rs_write()函数中，
// ;当写缓冲队列中有数据时，就会修改中断允许寄存器内容，添加上发送保持寄存器中断允许标志，
// ;从而在系统需要发送字符时引起串行中断发生。
	popl %edx		// ;弹出中断标识寄存器端口号 0x3fa（或 0x2fa）。
	jmp rep_int		// ;跳转，继续判断有无待处理中断并继续处理。
end:	movb $0x20,%al		// ;向中断控制器发送结束中断指令 EOI。
	outb %al,$0x20		/* EOI */
	pop %ds
	pop %es
	popl %eax
	popl %ebx
	popl %ecx
	popl %edx
	addl $4,%esp		# jump over _table_list entry	//;丢弃缓冲队列指针地址。
	iret

// ;各中断类型处理程序地址跳转表，共有 4 种中断来源：
// ;modem 状态变化中断，写字符中断，读字符中断，线路状态有问题中断。
jmp_table:
	.long modem_status,write_char,read_char,line_status

// ;由于 modem 状态发生变化而引发此次中断。通过读 modem 状态寄存器对其进行复位操作。
.align 2
modem_status:
	addl $6,%edx		/* clear intr by reading modem status reg */
	inb %dx,%al		/* ;通过读 modem 状态寄存器进行复位(0x3fe) */
	ret

// ;由于线路状态发生变化而引起这次串行中断。通过读线路状态寄存器对其进行复位操作。
.align 2
line_status:
	addl $5,%edx		/* clear intr by reading line status reg. */
	inb %dx,%al		/* ;通过读线路状态寄存器进行复位(0x3fd) */
	ret

// ;由于串行设备（芯片）接收到字符而引起这次中断。将接收到的字符放到读缓冲队列 read_q 头
// ;指针（head）处，并且让该指针前移一个字符位置。若 head 指针已经到达缓冲区末端，则让其
// ;折返到缓冲区开始处。最后调用 C 函数 do_tty_interrupt()（也即 copy_to_cooked()），把读
// ;入的字符经过一定处理放入规范模式缓冲队列（辅助缓冲队列 secondary）中。
.align 2
read_char:
	inb %dx,%al		// ;读取字符-->al。
	movl %ecx,%edx		// ;当前串口缓冲队列指针地址-->edx
	subl $_table_list,%edx	// ;缓冲队列指针表首址 - 当前串口队列指针地址-->edx，
	shrl $3,%edx		// ;差值/8。对于串口 1 是 1，对于串口 2 是 2。
	movl (%ecx),%ecx		# read-queue	// ;取读缓冲队列结构地址-->ecx。
	movl head(%ecx),%ebx	// ;取读队列中缓冲头指针-->ebx。
	movb %al,buf(%ecx,%ebx)	// ;将字符放在缓冲区中头指针所指的位置。
	incl %ebx		// ;将头指针前移一字节。
	andl $size-1,%ebx	// ;用缓冲区大小对头指针进行模操作。指针不能超过缓冲区大小。
	cmpl tail(%ecx),%ebx	// ;缓冲区头指针与尾指针比较。
	je 1f			// ;若相等，表示缓冲区满，跳转到标号 1 处。
	movl %ebx,head(%ecx)	// ;保存修改过的头指针。
1:	pushl %edx		// ;将串口号压入堆栈(1- 串口 1，2 - 串口 2)，作为参数，
	call _do_tty_interrupt	// ;调用 tty 中断处理 C 函数
	addl $4,%esp		// ;丢弃入栈参数，并返回。
	ret

// ;由于设置了发送保持寄存器允许中断标志而引起此次中断。说明对应串行终端的写字符缓冲队列中
// ;有字符需要发送。于是计算出写队列中当前所含字符数，若字符数已小于 256 个则唤醒等待写操作
// ;进程。然后从写缓冲队列尾部取出一个字符发送，并调整和保存尾指针。如果写缓冲队列已空，则
// ;跳转到 write_buffer_empty 处处理写缓冲队列空的情况。
.align 2
write_char:
	movl 4(%ecx),%ecx		# write-queue	// ;取写缓冲队列结构地址-->ecx
	movl head(%ecx),%ebx		// ;取写队列头指针-->ebx。
	subl tail(%ecx),%ebx		// ;头指针 - 尾指针 = 队列中字符数。
	andl $size-1,%ebx		# nr chars in queue	// ;对指针取模运算
	je write_buffer_empty		// ;如果头指针 = 尾指针，说明写队列无字符，跳转处理。
	cmpl $startup,%ebx		// ;队列中字符数超过 256 个？
	ja 1f				// ;超过，则跳转处理。
	movl proc_list(%ecx),%ebx	# wake up sleeping process	// ;唤醒等待的进程。
					// ;取等待该队列的进程的指针，并判断是否为空。
	testl %ebx,%ebx			# is there any?	// ;有等待的进程吗？
	je 1f				// ;是空的，则向前跳转到标号 1 处。
	movl $0,(%ebx)			// ;否则将进程置为可运行状态(唤醒进程)。
1:	movl tail(%ecx),%ebx		// ;取尾指针。
	movb buf(%ecx,%ebx),%al		// ;从缓冲中尾指针处取一字符-->al。
	outb %al,%dx			// ;向端口 0x3f8(0x2f8)送出到保持寄存器中。
	incl %ebx			// ;尾指针前移。
	andl $size-1,%ebx		// ;尾指针若到缓冲区末端，则折回。
	movl %ebx,tail(%ecx)		// ;保存已修改过的尾指针。
	cmpl head(%ecx),%ebx		// ;尾指针与头指针比较，
	je write_buffer_empty		// ;若相等，表示队列已空，则跳转。
	ret
	
// ;处理写缓冲队列 write_q 已空的情况。若有等待写该串行终端的进程则唤醒之，然后屏蔽发送
// ;保持寄存器空中断，不让发送保持寄存器空时产生中断。
.align 2
write_buffer_empty:
	movl proc_list(%ecx),%ebx	# wake up sleeping process	// ;唤醒等待的进程。
					// ;取等待该队列的进程的指针，并判断是否为空。
	testl %ebx,%ebx			# is there any?	// ;有等待的进程吗？
	je 1f				// ;无，则向前跳转到标号 1 处。
	movl $0,(%ebx)			// ;否则将进程置为可运行状态(唤醒进程)。
1:	incl %edx			// ;指向端口 0x3f9(0x2f9)。
	inb %dx,%al			// ;读取中断允许寄存器。
	jmp 1f				// ;稍作延迟。
1:	jmp 1f
1:	andb $0xd,%al		/* disable transmit interrupt */
				/* ;屏蔽发送保持寄存器空中断（位 1） */
	outb %al,%dx		// ;写入 0x3f9(0x2f9)。
	ret
