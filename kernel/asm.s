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
* asm.s �����а����󲿷ֵ�Ӳ������(�����)����ĵײ�δ��롣ҳ�쳣�����ڴ�������
* mm ����ģ����Բ�������˳��򻹴���(ϣ��������)���� TS-λ����ɵ� fpu �쳣��
* ��Ϊ fpu ������ȷ�ؽ��б���/�ָ�������Щ��û�в��Թ���
*/

//�������ļ���Ҫ�漰�� Intel �������ж� int0--int16 �Ĵ���(int17-int31 �������ʹ��)��
//������һЩȫ�ֺ���������������ԭ���� traps.c ��˵����
.globl _divide_error,_debug,_nmi,_int3,_overflow,_bounds,_invalid_op
.globl _double_fault,_coprocessor_segment_overrun
.globl _invalid_TSS,_segment_not_present,_stack_segment
.globl _general_protection,_coprocessor_error,_irq13,_reserved

_divide_error://�������
	pushl $_do_divide_error	//�����Ĵ�����
no_error_code:	//�������޴����봦�����ڴ�������������do��������errorcode��sp��������
	xchgl %eax,(%esp)	//eax ��������ջ
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %edi
	pushl %esi
	pushl %ebp
	push %ds	// !!16 λ�ĶμĴ�����ջ��ҲҪռ�� 4 ���ֽ�
	push %es
	push %fs
	pushl $0		//�ڶ�������error code
	lea 44(%esp),%edx	//ȡԭ���÷��ص�ַ�����û�̬��ջָ��λ�ã���ѹ���ջ��lea��ȡ��ַ
	pushl %edx
	movl $0x10,%edx	//�ں˴������ݶ�ѡ�����ԭ���Ŀ������û�̬�жϹ�����
	mov %dx,%ds
	mov %dx,%es
	mov %dx,%fs
	call *%eax //����C����do_divide_error�����������ĵ��ú���������ջ�ǲ���
	addl $8,%esp	//׼���ָ��Ĵ������ö�ջָ������ָ��Ĵ���fs��ջ������Ϊsp������������fs��
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

_debug://int1debug �����ж���ڵ�
	pushl $_do_int3		# _do_debug
	jmp no_error_code

_nmi://int2�������ж�
	pushl $_do_nmi
	jmp no_error_code

_int3://���Դ�ϵ�
	pushl $_do_int3
	jmp no_error_code

_overflow://int4�������
	pushl $_do_overflow
	jmp no_error_code

_bounds://int5�߽�������ж�
	pushl $_do_bounds
	jmp no_error_code

_invalid_op://int6 -- ��Ч����ָ������ж�
	pushl $_do_invalid_op
	jmp no_error_code

_coprocessor_segment_overrun://int9 -- Э�������γ��������ж�
	pushl $_do_coprocessor_segment_overrun
	jmp no_error_code

_reserved://int15 �C ����
	pushl $_do_reserved
	jmp no_error_code

//int45 -- ( = 0x20 + 13 ) ��ѧЭ������(Coprocessor)�������жϡ�
//��Э������ִ����һ������ʱ�ͻᷢ�� IRQ13 �ж��źţ���֪ͨ CPU �������
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


//�����ж��ڵ���ʱ�����жϷ��ص�ַ֮�󽫳����ѹ���ջ����˷���ʱҲ��Ҫ������ŵ�����
_double_fault:	//int8 -- ˫������ϡ�
	pushl $_do_double_fault
error_code:	//�����Ǵ������봦�����ڴ�������������do����Ҳ����errorcode��sp��������
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
	pushl %eax			# error code	//�ڶ�������
	lea 44(%esp),%eax		# offset	//��һ������esp
	pushl %eax
	movl $0x10,%eax	//�ں˴������ݶ�ѡ�����ԭ���Ŀ������û�̬�жϹ�����
	mov %ax,%ds
	mov %ax,%es
	mov %ax,%fs
	call *%ebx	//����c������������������ջ
	addl $8,%esp	//׼���ָ��Ĵ������ö�ջָ������ָ��Ĵ���fs��ջ������Ϊsp������������fs��
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

_invalid_TSS://int10��Ч������״̬��
	pushl $_do_invalid_TSS
	jmp error_code

_segment_not_present://int11 -- �β�����
	pushl $_do_segment_not_present
	jmp error_code

_stack_segment://int12 -- ��ջ�δ���
	pushl $_do_stack_segment
	jmp error_code

_general_protection://int13 -- һ�㱣���Գ���
	pushl $_do_general_protection
	jmp error_code

	
//int7 -- �豸������(_device_not_available)��(kernel/system_call.s,148)
//int14 -- ҳ����(_page_fault)��(mm/page.s,14)
//int16 -- Э����������(_coprocessor_error)��(kernel/system_call.s,131) 
//ʱ���ж� int 0x20 (_timer_interrupt)��(kernel/system_call.s,176)
//ϵͳ���� int 0x80 (_system_call)��(kernel/system_call.s,80)