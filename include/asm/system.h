/* 该文件中定义了设置或修改描述符 / 中断门等的嵌入式汇编宏。*/

//1、ss段描述符
//0x17中的17用二进制表示是00010111。最后两位表示特权级是用户态还是内核态。
//linux0.1.1中特权级分为0,1,2,3共4个特权级，用户态是3，内核态是0，因此它表示的是用户态。
//倒数第三位是1，表示从LDT中获取描述符，
//因此第4~5位的10表示从LDT的第2项（从第0项开始）中得到有关用户栈段的描述符。

//2、cs段描述符
//0x0f中的0f为00001111，也是用户特权级，从LDT中的第1项（从第0项开始）中取得用户态代码段描述符。
//当iret返回时，程序将数据出栈给ss，esp，eflags，cs，eip，之后就可以使进程进入用户态。

//注意，这里的中断返回指令 iret 并不会造成 CPU 去执行任务切换操作，因为在执行这个函数之前，
//标志位 NT 已经在 sched_init() 中被复位。
//在 NT 复位时执行 iret 指令不会造成 CPU 执行任务切换操作。
//任务 0 的执行纯粹是人工启动的。
#define move_to_user_mode() \
__asm__ ("movl %%esp,%%eax\n\t" \
	"pushl $0x17\n\t" \				//ss，只是变了特权级?
	"pushl %%eax\n\t" \				//sp，没变，因为之前的都是内核态代码但是用的是用户态的栈
	"pushfl\n\t" \					//flag
	"pushl $0x0f\n\t" \				//cs，只是变了特权级?
	"pushl $1f\n\t" \				//ip
	"iret\n" \
	"1:\tmovl $0x17,%%eax\n\t" \	//处理其他段ds、es、fs、gs
	"movw %%ax,%%ds\n\t" \
	"movw %%ax,%%es\n\t" \
	"movw %%ax,%%fs\n\t" \
	"movw %%ax,%%gs" \
	:::"ax")

#define sti() __asm__ ("sti"::)	// 开中断嵌入汇编宏函数。
#define cli() __asm__ ("cli"::)	// 关中断。
#define nop() __asm__ ("nop"::)	// 空操作。

#define iret() __asm__ ("iret"::)	// 中断返回。

// 设置门描述符宏函数。
// 函数入口地址是由 seg:addr 共同组成的，这里的seg用的是0x80即内核代码段。
// 对于type : 14 - 80386 32-bit interrupt gate; 15 - 80386 32-bit trap gate
#define _set_gate(gate_addr,type,dpl,addr) \
__asm__ ("movw %%dx,%%ax\n\t" \		//给eax低两个字节
	"movw %0,%%dx\n\t" \			//给edx低两个字节
	"movl %%eax,%1\n\t" \			//eax的值放到低四个字节:0x0008 + addr低两个字节
	"movl %%edx,%2" \				//edx的值放到高四个字节:addr高两个字节 + type&dp
	: \
	: "i" ((short) (0x8000+(dpl<<13)+(type<<8))), \
	"o" (*((char *) (gate_addr))), \
	"o" (*(4+(char *) (gate_addr))), \
	"d" ((char *) (addr)),"a" (0x00080000))

// 设置中断门函数。
// 参数：n - 中断号；addr - 中断程序偏移地址。
// &idt[n]对应中断号在中断描述符表中的偏移值；中断描述符的类型是 14，特权级是 0。
#define set_intr_gate(n,addr) \
	_set_gate(&idt[n],14,0,addr)

// 设置陷阱门函数。
// 参数：n - 中断号；addr - 中断程序偏移地址。
// &idt[n]对应中断号在中断描述符表中的偏移值；中断描述符的类型是 15，特权级是 0。
#define set_trap_gate(n,addr) \
	_set_gate(&idt[n],15,0,addr)

// 设置系统调用门函数。
// 参数：n - 中断号；addr - 中断程序偏移地址。
// &idt[n]对应中断号在中断描述符表中的偏移值；中断描述符的类型是 15，特权级是 3。
#define set_system_gate(n,addr) \
	_set_gate(&idt[n],15,3,addr)

// 设置段描述符函数。
// 参数：gate_addr -描述符地址；type -描述符中类型域值；dpl -描述符特权层值；
// base - 段的基地址；limit - 段限长。（参见段描述符的格式）
#define _set_seg_desc(gate_addr,type,dpl,base,limit) {\
	*(gate_addr) = ((base) & 0xff000000) | \				// 描述符低 4 字节。
		(((base) & 0x00ff0000)>>16) | \
		((limit) & 0xf0000) | \
		((dpl)<<13) | \
		(0x00408000) | \
		((type)<<8); \
	*((gate_addr)+1) = (((base) & 0x0000ffff)<<16) | \		// 描述符高 4 字节。
		((limit) & 0x0ffff); }

// 在全局表中设置任务状态段/局部表描述符。
// 参数：n - 在全局表中描述符项 n 所对应的地址；addr - 状态段/局部表所在内存的基地址。
// type - 描述符中的标志类型字节。
// %0 - eax(地址 addr)；%1 - (描述符项 n 的地址)；%2 - (描述符项 n 的地址偏移 2 处)；
// %3 - (描述符项 n 的地址偏移 4 处)；%4 - (描述符项 n 的地址偏移 5 处)；
// %5 - (描述符项 n 的地址偏移 6 处)；%6 - (描述符项 n 的地址偏移 7 处)；
#define _set_tssldt_desc(n,addr,type) \
__asm__ ("movw $104,%1\n\t" \	// 将 TSS 长度放入描述符长度域(第 0-1 字节)。
	"movw %%ax,%2\n\t" \		// 将基地址的低字放入描述符第 2-3 字节。
	"rorl $16,%%eax\n\t" \		// 将基地址高字移入 ax 中。
	"movb %%al,%3\n\t" \		// 将基地址高字中低字节移入描述符第 4 字节。
	"movb $" type ",%4\n\t" \	// 将标志类型字节移入描述符的第 5 字节。
	"movb $0x00,%5\n\t" \		// 描述符的第 6 字节置 0。
	"movb %%ah,%6\n\t" \		// 将基地址高字中高字节移入描述符第 7 字节。
	"rorl $16,%%eax" \			// eax 清零。
	::"a" (addr), "m" (*(n)), "m" (*(n+2)), "m" (*(n+4)), \
	 "m" (*(n+5)), "m" (*(n+6)), "m" (*(n+7)) \
	)

// 在全局表中设置任务状态段描述符。
// n - 是该描述符的指针；addr - 是描述符中的基地址值。任务状态段描述符的类型是 0x89。
#define set_tss_desc(n,addr) _set_tssldt_desc(((char *) (n)),addr,"0x89")
// 在全局表中设置局部表描述符。
// n - 是该描述符的指针；addr - 是描述符中的基地址值。局部表描述符的类型是 0x82。
#define set_ldt_desc(n,addr) _set_tssldt_desc(((char *) (n)),addr,"0x82")
