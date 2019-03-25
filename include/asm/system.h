//1��ss��������
//0x17�е�17�ö����Ʊ�ʾ��00010111�������λ��ʾ��Ȩ�����û�̬�����ں�̬��
//linux0.1.1����Ȩ����Ϊ0,1,2,3��4����Ȩ�����û�̬��3���ں�̬��0���������ʾ�����û�̬��
//��������λ��1����ʾ��LDT�л�ȡ��������
//��˵�4~5λ��10��ʾ��LDT�ĵ�2��ӵ�0�ʼ���еõ��й��û�ջ�ε���������

//2��cs��������
//0x0f�е�0fΪ00001111��Ҳ���û���Ȩ������LDT�еĵ�1��ӵ�0�ʼ����ȡ���û�̬�������������
//��iret����ʱ���������ݳ�ջ��ss��esp��eflags��cs��eip��֮��Ϳ���ʹ���̽����û�̬��
#define move_to_user_mode() \
__asm__ ("movl %%esp,%%eax\n\t" \
	"pushl $0x17\n\t" \				//ss��ֻ�Ǳ�����Ȩ��?
	"pushl %%eax\n\t" \				//sp��û�䣬��Ϊ֮ǰ�Ķ����ں�̬���뵫���õ����û�̬��ջ
	"pushfl\n\t" \					//flag
	"pushl $0x0f\n\t" \				//cs��ֻ�Ǳ�����Ȩ��?
	"pushl $1f\n\t" \				//ip
	"iret\n" \
	"1:\tmovl $0x17,%%eax\n\t" \	//����������ds��es��fs��gs
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

//��Ȩ����0
#define set_trap_gate(n,addr) \
	_set_gate(&idt[n],15,0,addr)

//��Ȩ����3
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
