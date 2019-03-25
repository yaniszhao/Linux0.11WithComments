//1、ss段描述符
//0x17中的17用二进制表示是00010111。最后两位表示特权级是用户态还是内核态。
//linux0.1.1中特权级分为0,1,2,3共4个特权级，用户态是3，内核态是0，因此它表示的是用户态。
//倒数第三位是1，表示从LDT中获取描述符，
//因此第4~5位的10表示从LDT的第2项（从第0项开始）中得到有关用户栈段的描述符。

//2、cs段描述符
//0x0f中的0f为00001111，也是用户特权级，从LDT中的第1项（从第0项开始）中取得用户态代码段描述符。
//当iret返回时，程序将数据出栈给ss，esp，eflags，cs，eip，之后就可以使进程进入用户态。
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

#define sti() __asm__ ("sti"::)
#define cli() __asm__ ("cli"::)
#define nop() __asm__ ("nop"::)

#define iret() __asm__ ("iret"::)

#define _set_gate(gate_addr,type,dpl,addr) \
__asm__ ("movw %%dx,%%ax\n\t" \
	"movw %0,%%dx\n\t" \
	"movl %%eax,%1\n\t" \
	"movl %%edx,%2" \
	: \
	: "i" ((short) (0x8000+(dpl<<13)+(type<<8))), \
	"o" (*((char *) (gate_addr))), \
	"o" (*(4+(char *) (gate_addr))), \
	"d" ((char *) (addr)),"a" (0x00080000))

#define set_intr_gate(n,addr) \
	_set_gate(&idt[n],14,0,addr)

//特权级是0
#define set_trap_gate(n,addr) \
	_set_gate(&idt[n],15,0,addr)

//特权级是3
#define set_system_gate(n,addr) \
	_set_gate(&idt[n],15,3,addr)

#define _set_seg_desc(gate_addr,type,dpl,base,limit) {\
	*(gate_addr) = ((base) & 0xff000000) | \
		(((base) & 0x00ff0000)>>16) | \
		((limit) & 0xf0000) | \
		((dpl)<<13) | \
		(0x00408000) | \
		((type)<<8); \
	*((gate_addr)+1) = (((base) & 0x0000ffff)<<16) | \
		((limit) & 0x0ffff); }

#define _set_tssldt_desc(n,addr,type) \
__asm__ ("movw $104,%1\n\t" \
	"movw %%ax,%2\n\t" \
	"rorl $16,%%eax\n\t" \
	"movb %%al,%3\n\t" \
	"movb $" type ",%4\n\t" \
	"movb $0x00,%5\n\t" \
	"movb %%ah,%6\n\t" \
	"rorl $16,%%eax" \
	::"a" (addr), "m" (*(n)), "m" (*(n+2)), "m" (*(n+4)), \
	 "m" (*(n+5)), "m" (*(n+6)), "m" (*(n+7)) \
	)

#define set_tss_desc(n,addr) _set_tssldt_desc(((char *) (n)),addr,"0x89")
#define set_ldt_desc(n,addr) _set_tssldt_desc(((char *) (n)),addr,"0x82")
