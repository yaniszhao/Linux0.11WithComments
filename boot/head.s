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
.globl _idt,_gdt,_pg_dir,_tmp_floppy_area	//;Ҫ�õķ�������

_pg_dir:	//;�����뱻ִ�е������ʱ����Щ�����û���ˣ�������ҳĿ¼
startup_32:

	movl $0x10,%eax	//;����GNU�����˵��ÿ��ֱ����Ҫ��$��ʼ�������Ǳ�ʾ��ַ
	mov %ax,%ds	//;���ø������ݶμĴ���00010 0 00
	mov %ax,%es
	mov %ax,%fs
	mov %ax,%gs
	lss _stack_start,%esp	//;_stack_start��sched.c�ļ��ж���
				//;lss--ͨ��6���ֽڵ����ݽṹ����ss:sp
				//;����������0֮ǰ�����ں˴���ջ���Ժ���������0��1���û�̬ջ

	call setup_idt		//;��Ϊ���ﺯ������Ҫ�õ�ջ����֮ǰ�͵ó�ʼ��ջ
	call setup_gdt		//;������ʱ���жϱ�

	//;���ڶ��������еĶ��޳���setup�����8MB�ĳ��˱��������õ�16MB
	//;��������ٴζ����жμĴ���ִ�м��ز����Ǳ����
	movl $0x10,%eax		# reload all the segment registers
	mov %ax,%ds		# after changing gdt. CS was already
	mov %ax,%es		# reloaded in 'setup_gdt'
	mov %ax,%fs		//;��Ϊ�޸���gdt��������Ҫ����װ�����еĶμĴ���
	mov %ax,%gs		//;CS ����μĴ����Ѿ��� setup_gdt �����¼��ع���
	lss _stack_start,%esp

	//;����A20��ַ���Ƿ��Ѿ����������õķ��������ڴ��ַ 0x000000 ��д������һ����ֵ��
	//;Ȼ���ڴ��ַ 0x100000(1M)���Ƿ�Ҳ�������ֵ��
	//;���һֱ��ͬ�Ļ�����һֱ�Ƚ���ȥ��Ҳ����ѭ����������
	//;��ʾ��ַ A20 ��û��ѡͨ������ں˾Ͳ���ʹ�� 1M �����ڴ�
	xorl %eax,%eax
1:	incl %eax		# check that A20 really IS enabled
	movl %eax,0x000000	# loop forever if it isnt
	cmpl %eax,0x100000
	je 1b
/*
 * NOTE! 486 should set bit 16, to check for write-protect in supervisor
 * mode. Then it would be unnecessary with the "verify_area()"-calls.
 * 486 users probably want to set the NE (#5) bit also, so as to use
 * int 16 for math errors.	//;�����ѧЭ������оƬ�Ƿ����
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
.align 2	//;���ֽڶ���
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
setup_idt:	//;������ʱ���жϱ�
	lea ignore_int,%edx	//;lea��ȡignore_int�ĵ�ַ�����ǵ�ַ�µ�ֵ
	movl $0x00080000,%eax	//;��16λ���0x0008
	movw %dx,%ax		/* selector = 0x0008 = cs */	//;���õ�16λ
				//;��ʱeax���˱���ĵ�4���ֽ�
	movw $0x8E00,%dx	/* interrupt gate - dpl=0, present */	//;���ø�4���ֽ�
				//;HHHH8E00 0008LLLL 
	lea _idt,%edi	//;idt����ڱ��ļ�����
	mov $256,%ecx		//;�ܹ�256�����ÿ��8�ֽڣ���2KB�ڴ�
rp_sidt:
	movl %eax,(%edi)	//;����4�ֽ�
	movl %edx,4(%edi)	//;����4�ֽ�
	addl $8,%edi		//;׼������8�ֽ�
	dec %ecx		//;�����һ�����
	jne rp_sidt		//;�ظ�256��
	lidt idt_descr		//;����6�ֽ�48λ�Ļ���ַ���޳�
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
	lgdt gdt_descr	//;ֻ��Ҫ����6�ֽ�48λ�Ļ���ַ���޳�
			//;������ͨ���ֶ�����
	ret

/*
 * I put the kernel page tables right after the page directory,
 * using 4 of them to span 16 Mb of physical memory. People with
 * more than 16MB will have to expand this.
 */
 //;�ڴ�ҳ��ֱ�ӷ���ҳĿ¼֮��ʹ���� 4 ������Ѱַ 16 Mb �������ڴ档
 //;������ж��� 16 Mb ���ڴ棬����Ҫ��������������޸ġ�

 //;ҳ������庬��:

 
.org 0x1000	//;��ƫ��0x1000����ʼ�ǵ�1��ҳ��(ƫ��0��ʼ�������ҳ��Ŀ¼)
pg0:

.org 0x2000
pg1:

.org 0x3000
pg2:

.org 0x4000
pg3:

.org 0x5000	//;���⿪ʼ����ҳ�����ҳĿ¼��
/*
 * tmp_floppy_area is used by the floppy-driver when DMA cannot
 * reach to a buffer-block. It needs to be aligned, so that it isnt
 * on a 64kB border.
 */
 //;��DMA(ֱ�Ӵ洢������)���ܷ��ʻ����ʱ�������tmp_floppy_area�ڴ��Ϳɹ�������������ʹ�á�
 //;���ַ��Ҫ��������������Ͳ����Խ 64kB �߽�
_tmp_floppy_area:
	.fill 1024,1,0	//;1024 �ÿ�� 1 �ֽڣ������ֵ 0

after_page_tables:	//;����ļ�����ջ������Ϊ�˽���main��׼��
	pushl $0		# These are the parameters to main :-)	//;envp
	pushl $0		//;argv
	pushl $0		//;argc
	pushl $L6		# return address for main, if it decides to.//;���غ�ִ�е�ָ��
	pushl $_main		//;main��ʵ��û�õ�ǰ����������
	jmp setup_paging	//;������ת������ҳ����
L6:	//;main��Ӧ�û᷵�أ��������˾���ѭ��
	jmp L6			# main should never return here, but
				# just in case, we know what happens.

/* This is the default interrupt "handler" :-) */
int_msg:
	.asciz "Unknown interrupt\n\r"
.align 2	//;���ֽڶ���
ignore_int:	//;��ʱ���жϴ������
	pushl %eax
	pushl %ecx
	pushl %edx
	push %ds	//;������ע��!!ds,es,fs,gs ����Ȼ�� 16 λ�ļĴ�����
	push %es	//;����ջ����Ȼ���� 32 λ����ʽ��ջ��Ҳ����Ҫռ�� 4 ���ֽڵĶ�ջ�ռ�
	push %fs
	movl $0x10,%eax	//;�������ö�ѡ���00010 0 00
	mov %ax,%ds
	mov %ax,%es
	mov %ax,%fs
	pushl $int_msg	//;printk�Ĳ���
	call _printk
	popl %eax	//;�����ջ���printk�Ĳ���
	pop %fs		//;�ָ����������
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %eax
	iret		//;�жϷ���


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
//;����ӳ���ͨ�����ÿ��ƼĴ��� cr0 �ı�־(PG λ 31)���������ڴ�ķ�ҳ�����ܣ�
//;�����ø���ҳ��������ݣ��Ժ��ӳ��ǰ 16 MB �������ڴ档��ҳ���ٶ���������Ƿ���
//;��ַӳ��(Ҳ����ֻ�� 4Mb �Ļ��������ó����� 4Mb ���ڴ��ַ)��
//;ע��!�������е������ַ��Ӧ��������ӳ�����к��ӳ�䣬��ֻ���ں�ҳ���������
//;ֱ��ʹ��>1Mb �ĵ�ַ�����С�һ�㡱������ʹ�õ��� 1Mb �ĵ�ַ�ռ䣬������ʹ�þֲ�����
//;�ռ䣬��ַ�ռ佫��ӳ�䵽����һЩ�ط�ȥ -- mm(�ڴ�������)�������Щ�µġ�
//;������Щ�ж��� 16Mb �ڴ�ļһ� �C ����̫�����ˣ��һ�û�У�Ϊʲô�����?���������
//;������������޸İɡ�(ʵ���ϣ��Ⲣ��̫���ѵġ�ͨ��ֻ���޸�һЩ�����ȡ��Ұ�������
//;Ϊ 16Mb����Ϊ�ҵĻ�������ô�����������ܳ����������(��Ȼ���ҵĻ����Ǻܱ��˵�?)��
//;���Ѿ�ͨ������ĳ���־��������Ҫ�Ķ��ĵط�(������16Mb��)�����Ҳ��ܱ�֤����Щ
//;�Ķ�������)��

//;1 ҳ�ڴ泤���� 4096 �ֽ�
//;ҳĿ¼����ϵͳ���н��̹��õģ�������� 4 ҳҳ�����������ں�ר��
//;�����µĽ��̣�ϵͳ�������ڴ���Ϊ������ҳ����ҳ��
//;Ҳ����˵ÿ�����̻����Լ���ҳ����

//;һ������ռ4���ֽ�
.align 2
setup_paging:
	//;���ȶ� 5 ҳ�ڴ�(1 ҳĿ¼ + 4 ҳҳ��)����
	movl $1024*5,%ecx		/* 5 pages - pg_dir+4 page tables */
	xorl %eax,%eax
	xorl %edi,%edi			/* pg_dir is at 0x000 */
	cld;rep;stosl	//;stos��eax�е�ֵ������ES:EDIָ��ĵ�ַ

	//;ҳĿ¼��ֻ��Ҫ����4��
	//;ҳ�����ʼ��ַ����0x0000X000��7�����0x0000X007
	//;ҳ�����ڵĵ�ַ = 0x0000X007 & 0xfffff000 = 0xX000
	//;ҳ������Ա�־ = 0x0000X007 & 0x00000fff = 0x07����ʾ��ҳ���ڡ��û��ɶ�д
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
	ret			/* this also flushes prefetch-queue */	//;ȥִ��main

.align 2
.word 0
idt_descr:
	.word 256*8-1		# idt contains 256 entries	//;����
	.long _idt		//;����ַ
.align 2
.word 0
gdt_descr:
	.word 256*8-1		# so does gdt (not that thats any
	.long _gdt		# magic number, but it works for me :^)

	.align 3		//;8�ֽڶ���
_idt:	.fill 256,8,0		# idt is uninitialized	//;256 �ÿ�� 8 �ֽڣ��� 0

_gdt:	.quad 0x0000000000000000	/* NULL descriptor */	//;��һ������
	.quad 0x00c09a0000000fff	/* 16Mb */ //;�ں˴����00 c09a 000000 0fff
					//;base-00000000��limit-0fff�ĳ���16M
					//;c09a-1100 0000 1001 1010
	.quad 0x00c0920000000fff	/* 16Mb */ //;�ں����ݶ�00 c092 0000000 fff
					//;base-00000000��limit-0fff�ĳ���16M
					//;c092-1100 0000 1001 0010
	.quad 0x0000000000000000	/* TEMPORARY - dont use */
	.fill 252,8,0			/* space for LDT's and TSS's etc */ //;����ldt��tss
