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
 * ;system_call.s �ļ�����ϵͳ����(system-call)�ײ㴦���ӳ���������Щ����Ƚ����ƣ�����
 * ;ͬʱҲ����ʱ���жϴ���(timer-interrupt)�����Ӳ�̺����̵��жϴ������Ҳ�����
 *
 * ;ע�⣺��δ��봦���ź�(signal)ʶ����ÿ��ʱ���жϺ�ϵͳ����֮�󶼻����ʶ��һ��
 * ;�ж��źŲ��������ź�ʶ����Ϊ���ϵͳ��ɻ��ҡ�
 *
 * Stack layout in 'ret_from_system_call':
 *//;��ϵͳ���÷���('ret_from_system_call')ʱ��ջ������
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
//;system_call.s �ļ�����ϵͳ����(system-call)�ײ㴦���ӳ���
//;������Щ����Ƚ����ƣ�����ͬʱҲ����ʱ���жϴ���(timer-interrupt)�����
//;Ӳ�̺����̵��жϴ������Ҳ�����
//;ע��:��δ��봦���ź�(signal)ʶ����ÿ��ʱ���жϺ�ϵͳ����֮�󶼻����ʶ��
//;һ���ж��źŲ��������ź�ʶ����Ϊ���ϵͳ��ɻ��ҡ�

SIG_CHLD	= 17		//;���� SIG_CHLD �ź�(�ӽ���ֹͣ�����)

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

//;������Щ������ṹ(task_struct)�б�����ƫ��ֵ
state	= 0		# these are offsets into the task-struct. //;����״̬��
counter	= 4		//;��������ʱ�����(�ݼ�)(�δ���)������ʱ��Ƭ
priority = 8		//;����������������ʼ����ʱ counter=priority��Խ��������ʱ��Խ��
signal	= 12		//;���ź�λͼ��ÿ������λ����һ���źţ��ź�ֵ=λƫ��ֵ+1
sigaction = 16		# MUST be 16 (=len of sigaction) // ;sigaction �ṹ���ȱ����� 16 �ֽ�
blocked = (33*16)	//;�������ź�λͼ��ƫ����

//;���¶����� sigaction �ṹ�е�ƫ����
# offsets within sigaction
sa_handler = 0		// ;�źŴ�����̵ľ��(������)
sa_mask = 4		// ;�ź���������
sa_flags = 8		// ;�źż�
sa_restorer = 12	// ;�ָ�����ָ��

nr_system_calls = 72	//;Linux 0.11 ���ں��е�ϵͳ��������

/*
 * Ok, I get parallel printer interrupts while using the floppy for some
 * strange reason. Urgel. Now I just ignore them.
 */
.globl _system_call,_sys_fork,_timer_interrupt,_sys_execve
.globl _hd_interrupt,_floppy_interrupt,_parallel_interrupt
.globl _device_not_available, _coprocessor_error

.align 2
bad_sys_call://;�����ϵͳ���ú�
	movl $-1,%eax	//;eax ����-1���˳��ж�
	iret
.align 2
reschedule://;����ִ�е��ȳ������
	pushl $ret_from_sys_call
	jmp _schedule
.align 2
_system_call://;int 0x80 --linux ϵͳ������ڵ�(�����ж� int 0x80��eax ���ǵ��ú�)
	cmpl $nr_system_calls-1,%eax //;���ú����������Χ�Ļ����� eax ����-1 ���˳�
	ja bad_sys_call
	push %ds		//;����ԭ�μĴ���ֵ
	push %es
	push %fs
	pushl %edx
	pushl %ecx		# push %ebx,%ecx,%edx as parameters
	pushl %ebx		# to the system call
	movl $0x10,%edx		# set up ds,es to kernel space //;00010 0 00�ں�̬
	mov %dx,%ds
	mov %dx,%es
	movl $0x17,%edx		# fs points to local data space//;00010 1 11�û�̬
	mov %dx,%fs
	call _sys_call_table(,%eax,4)	//;���õ�ַ = _sys_call_table + %eax*4
	pushl %eax			//;��ϵͳ���÷���ֵ��ջ
	movl _current,%eax
	//;�鿴��ǰ���������״̬��������ھ���״̬(state ������ 0)��ȥִ�е��ȳ���
	//;����������ھ���״̬������ʱ��Ƭ������(counter=0)����Ҳȥִ�е��ȳ���
	cmpl $0,state(%eax)		# state		//;����sleep�ߵ����Ӧ�����µ���
	jne reschedule
	cmpl $0,counter(%eax)		# counter	//;����ʱ���ж���counter����0��
	je reschedule
ret_from_sys_call://;ϵͳ���� C �������غ󣬶��ź�������ʶ����
	//;�����б�ǰ�����Ƿ��ǳ�ʼ���� task0��������򲻱ض�������ź�������Ĵ���ֱ�ӷ���
	movl _current,%eax		# task[0] cannot have signals
	cmpl _task,%eax			//;task0��PCB��ʼλ��
	je 3f
	//;ͨ����ԭ���ó������ѡ����ļ�����жϵ��ó����Ƿ����ں�����(�������� 1)��
	//;�������ֱ���˳��жϡ����������ͨ������������ź����Ĵ���
	//;����Ƚ�ѡ����Ƿ�Ϊ��ͨ�û�����ε�ѡ��� 0x000f (RPL=3���ֲ����� 1 ����(�����))��
	//;�����������ת�˳��жϳ���
	cmpw $0x0f,CS(%esp)		# was old code segment supervisor ?
	jne 3f
	//;���ԭ��ջ��ѡ�����Ϊ 0x17(Ҳ��ԭ��ջ�����û����ݶ���)����Ҳ�˳�
	cmpw $0x17,OLDSS(%esp)		# was stack segment = 0x17 ?
	jne 3f
	movl signal(%eax),%ebx		//;ȡ�ź�λͼ-->ebx��ÿ 1 λ���� 1 ���źţ��� 32 ���ź�
	movl blocked(%eax),%ecx		//;ȡ����(����)�ź�λͼ-->ecx
	notl %ecx			//;ÿλȡ��
	andl %ebx,%ecx			//;�����ɵ��ź�λͼ
	bsfl %ecx,%ecx			//;�ӵ�λ(λ 0)��ʼɨ��λͼ�����Ƿ��� 1 ��λ��
					//;���У��� ecx ������λ��ƫ��ֵ(���ڼ�λ 0-31
	je 3f				//;���û���ź�����ǰ��ת�˳�
	btrl %ecx,%ebx			//;��λ���ź�(ebx ����ԭ signal λͼ)
	movl %ebx,signal(%eax)		//;���±��� signal λͼ��Ϣ
	incl %ecx			//;���źŵ���Ϊ�� 1 ��ʼ����(1-32)
	pushl %ecx			//;�ź�ֵ��ջ��Ϊ���� do_signal �Ĳ���֮һ
	call _do_signal
	popl %eax			//;�����ź�ֵ
3:	popl %eax
	popl %ebx
	popl %ecx
	popl %edx
	pop %fs
	pop %es
	pop %ds
	iret

//;������δ��봦��Э�����������ĳ����źš���תִ�� C ���� math_error()
//;���غ���ת�� ret_from_sys_call ������ִ��
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
	jmp _math_error			//;ִ�� C ���� math_error()

//;int7 -- �豸�����ڻ�Э������������(Coprocessor not available)��
//;������ƼĴ��� CR0 �� EM ��־��λ���� CPU ִ��һ�� ESC ת��ָ��ʱ�ͻ��������жϣ�
//;�����Ϳ����л���������жϴ������ģ�� ESC ת��ָ��(169 ��)��
//;CR0��TS��־����CPUִ������ת��ʱ���õġ�
//;TS��������ȷ��ʲôʱ��Э�������е�����(������)��CPU ����ִ�е�����ƥ���ˡ�
//;�� CPU ������һ��ת��ָ��ʱ���� TS ��λ�ˣ��ͻ��������жϡ�
//;��ʱ��Ӧ�ûָ��������Э������ִ��״̬(165 ��)���μ�(kernel/sched.c,77)�е�˵����
//;���ж����ת�Ƶ���� ret_from_sys_call ��ִ����ȥ(��Ⲣ�����ź�)��
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
	//;EM (math emulation bit)������� EM ������жϣ���ָ�������Э������״̬��
	testl $0x4,%eax			# EM (math emulation bit)
	je _math_state_restore
	pushl %ebp
	pushl %esi
	pushl %edi
	call _math_emulate
	popl %edi
	popl %esi
	popl %ebp
	ret				//;����� ret ����ת�� ret_from_sys_call

//;int32 -- (int 0x20) ʱ���жϴ������
//;�ж�Ƶ�ʱ�����Ϊ 100Hz(include/linux/sched.h,5)��
//;��ʱоƬ 8253/8254 ����(kernel/sched.c,406)����ʼ���ġ�
//;������� jiffies ÿ 10 ����� 1��
//;��δ��뽫 jiffies �� 1�����ͽ����ж�ָ��� 8259 ��������
//;Ȼ���õ�ǰ��Ȩ����Ϊ�������� C ���� do_timer(long CPL)��
//;�����÷���ʱתȥ��Ⲣ�����źš�
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
	incl _jiffies		//;�δ�����1
	//;���ڳ�ʼ���жϿ���оƬʱû�в����Զ� EOI������������Ҫ��ָ�������Ӳ���ж�
	movb $0x20,%al		# EOI to interrupt controller #1
	outb %al,$0x20
	//;����3���CSѡ�����ȡ����ǰ��Ȩ����(0 �� 3)��ѹ���ջ����Ϊ do_timer �Ĳ���
	movl CS(%esp),%eax
	andl $3,%eax		# %eax is CPL (0 or 3, 0=supervisor)
	pushl %eax
	//;do_timer(CPL)ִ�������л�����ʱ�ȹ���
	call _do_timer		# 'do_timer(long CPL)' does everything from
				# task switching to accounting ...
	addl $4,%esp		//;������do_timer������ջ
	jmp ret_from_sys_call

//;���� sys_execve()ϵͳ���á�ȡ�жϵ��ó���Ĵ���ָ����Ϊ�������� C ���� do_execve()
.align 2
_sys_execve:
	lea EIP(%esp),%eax
	pushl %eax
	call _do_execve
	addl $4,%esp
	ret

//;sys_fork()���ã����ڴ����ӽ��̣��� system_call ���� 2��
//;���ȵ��� C ���� find_empty_process()��ȡ��һ�����̺� pid��
//;�����ظ�����˵��Ŀǰ��������������Ȼ����� copy_process()���ƽ��̡�
.align 2
_sys_fork:
	call _find_empty_process	//;�ҵ�һ���յ�PCB��
	testl %eax,%eax
	js 1f
	push %gs			//;���漸����ջ�Ǵ��ݲ���
	pushl %esi
	pushl %edi
	pushl %ebp
	pushl %eax
	call _copy_process		//;���ƽ���
	addl $20,%esp			//;�ָ��������õĲ���ջ
1:	ret

//;int 46 -- (int 0x2E) Ӳ���жϴ��������ӦӲ���ж����� IRQ14��
//;��Ӳ�̲�����ɻ����ͻᷢ�����ж��źš�
//;������ 8259A �жϿ��ƴ�оƬ���ͽ���Ӳ���ж�ָ��(EOI)��
//;Ȼ��ȡ���� do_hd �еĺ���ָ����� edx �Ĵ����У����� do_hd Ϊ NULL��
//;�����ж� edx ����ָ���Ƿ�Ϊ�ա����Ϊ�գ���� edx ��ֵָ��unexpected_hd_interrupt()��
//;������ʾ������Ϣ������� 8259A ��оƬ�� EOI ָ������� edx ��ָ��ָ��ĺ���:
//;read_intr()��write_intr()�� unexpected_hd_interrupt()��
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
	//;���ڳ�ʼ���жϿ���оƬʱû�в����Զ� EOI������������Ҫ��ָ�������Ӳ���жϡ�
	movb $0x20,%al
	outb %al,$0xA0		# EOI to interrupt controller #1
	jmp 1f			# give port chance to breathe
1:	jmp 1f			//;��ʱ���á�
1:	xorl %edx,%edx
	xchgl _do_hd,%edx	//;do_hd����Ϊһ������ָ�룬������ֵread_intr()��write_intr()������ַ��
				//;�ŵ� edx �Ĵ�����ͽ� do_hd ָ�������Ϊ NULL��
	testl %edx,%edx		//;��do_hd������ΪNULL��ִ��$_unexpected_hd_interrupt
	jne 1f
	movl $_unexpected_hd_interrupt,%edx
1:	outb %al,$0x20		//;���� 8259A �жϿ����� EOI ָ��(����Ӳ���ж�)
	call *%edx		# "interesting" way of handling intr. //;����C�Ĵ�����
	pop %fs
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %eax
	iret

//;int38 -- (int 0x26) �����������жϴ��������ӦӲ���ж����� IRQ6��
//;�䴦������������Ӳ�̵Ĵ������һ����(kernel/blk_drv/floppy.c)��
//;������ 8259A �жϿ�������оƬ���� EOI ָ�
//;Ȼ��ȡ���� do_floppy �еĺ���ָ����� eax �Ĵ����У�
//;���� do_floppy Ϊ NULL�������ж� eax ����ָ���Ƿ�Ϊ�ա�
//;��Ϊ�գ���� eax ��ֵָ��unexpected_floppy_interrupt ()��������ʾ������Ϣ��
//;������ eax ָ��ĺ���: rw_interrupt, seek_interrupt,recal_interrupt,reset_interrupt
//;�� unexpected_floppy_interrupt��
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
	xchgl _do_floppy,%eax	//;�ĸ������е�һ������ΪNULL
	testl %eax,%eax
	jne 1f
	movl $_unexpected_floppy_interrupt,%eax //;ΪNULL����Ϊ�������
1:	call *%eax		# "interesting" way of handling intr.
	pop %fs
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %eax
	iret
	
//;int 39 -- (int 0x27) ���п��жϴ�����򣬶�ӦӲ���ж������ź� IRQ7��
//;���汾�ں˻�δʵ�֡�����ֻ�Ƿ��� EOI ָ�
_parallel_interrupt:
	pushl %eax
	movb $0x20,%al
	outb %al,$0x20
	popl %eax
	iret
