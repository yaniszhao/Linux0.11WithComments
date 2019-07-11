!
!	setup.s		(C) 1991 Linus Torvalds
!
! setup.s is responsible for getting the system data from the BIOS,
! and putting them into the appropriate places in system memory.
! both setup.s and system has been loaded by the bootblock.
!
! This code asks the bios for memory/disk/other parameters, and
! puts them in a "safe" place: 0x90000-0x901FF, ie where the
! boot-block used to be. It is then up to the protected mode
! system to read them from there before the area is overwritten
! for buffer-blocks.
!

! NOTE! These had better be the same as in bootsect.s!

INITSEG  = 0x9000	! we move boot here - out of the way	//;576KB
SYSSEG   = 0x1000	! system loaded at 0x10000 (65536).	//;64KB
SETUPSEG = 0x9020	! this is the current segment		//;64KB+512B

.globl begtext, begdata, begbss, endtext, enddata, endbss
.text
begtext:
.data
begdata:
.bss
begbss:
.text

entry start
start:

! ok, the read went well so we get current cursor position and save it for
! posterity.	//;�õ����λ��

	mov	ax,#INITSEG	! this is done in bootsect already, but...
	mov	ds,ax
	mov	ah,#0x03	! read cursor pos
	xor	bh,bh
	int	0x10		! save it in known place, con_init fetches
	mov	[0],dx		! it from 0x90000. //;����ds�ε�ƫ��

! Get memory size (extended mem, kB)	//;�õ���չ�ڴ��С

	mov	ah,#0x88
	int	0x15
	mov	[2],ax	//;����ds�ε�ƫ��

! Get video-card data:	//;�õ��Կ���ʾģʽ

	mov	ah,#0x0f
	int	0x10
	mov	[4],bx		! bh = display page
	mov	[6],ax		! al = video mode, ah = window width

! check for EGA/VGA and some config parameters	//;�õ��Կ����ò���

	mov	ah,#0x12
	mov	bl,#0x10
	int	0x10
	mov	[8],ax
	mov	[10],bx
	mov	[12],cx

! Get hd0 data	//;��һ��Ӳ�̲�����

	mov	ax,#0x0000
	mov	ds,ax
	lds	si,[4*0x41]
	mov	ax,#INITSEG
	mov	es,ax
	mov	di,#0x0080
	mov	cx,#0x10
	rep
	movsb

! Get hd1 data	//;�ڶ���Ӳ�̲�����

	mov	ax,#0x0000
	mov	ds,ax
	lds	si,[4*0x46]
	mov	ax,#INITSEG
	mov	es,ax
	mov	di,#0x0090
	mov	cx,#0x10
	rep
	movsb

! Check that there IS a hd1 :-)	//;�ж��Ƿ���ڵڶ���Ӳ�̣��������������

	mov	ax,#0x01500
	mov	dl,#0x81
	int	0x13
	jc	no_disk1
	cmp	ah,#3
	je	is_disk1
no_disk1:
	mov	ax,#INITSEG
	mov	es,ax
	mov	di,#0x0090
	mov	cx,#0x10
	mov	ax,#0x00
	rep
	stosb
is_disk1:
	/*;׼����������ģʽ*/
	//;�ڱ���ģʽ�����п���֧�ֶ�����
	//;֧��4G�������ڴ�
	//;֧�������ڴ�
	//;֧���ڴ��ҳʽ����Ͷ�ʽ����
	//;֧����Ȩ��
! now we want to move to protected mode ...
				//;���жϣ�һֱ��main.c��ʼ����ſ���
	cli			! no interrupts allowed !

! first we move the system to its rightful place
	/*;�ƶ��ں�*/
	mov	ax,#0x0000	//;�ƶ�����ַ0��λ��
	cld			! 'direction'=0, movs moves forward	//;�����ƶ�����
do_move:	//;����������������forѭ���Ļ���������ô����
	mov	es,ax		! destination segment	//;ÿ��ѭ��Ŀ���ַ����0x10000��ʼΪ0
	add	ax,#0x1000
	cmp	ax,#0x9000
	jz	end_move
	mov	ds,ax		! source segment	//;ÿ��ѭ��Դ��ַ��0x10000��ʼΪ0x10000
	sub	di,di
	sub	si,si
	mov 	cx,#0x8000	//;�ظ��ƶ�0x8000��
	rep
	movsw			//;ÿ���ƶ�2���ֽڣ��ܾ���0x10000����
	jmp	do_move

! then we load the segment descriptors	//;������ʱ��idtr��gdtr����48λ��

end_move:
	mov	ax,#SETUPSEG	! right, forgot this at first. didnt work :-)
	mov	ds,ax	//;���ݶ���ʼΪsetup����ʼ
	lidt	idt_48		! load idt with 0,0
	lgdt	gdt_48		! load gdt with whatever appropriate

! that was painless, now we enable A20	//;����A20���1MB���ϵ��ڴ�

	call	empty_8042	//;�ȴ����뻺�����ա�ֻ�е����뻺����Ϊ��ʱ�ſ��Զ������д���
	mov	al,#0xD1		! command write	//;command write ! 0xD1 ������-��ʾҪд���ݵ�
	out	#0x64,al	//;8042��P2�˿ڡ�P2�˿ڵ�λ1����A20�ߵ�ѡͨ������Ҫд��0x60�ڡ�
	call	empty_8042
	mov	al,#0xDF		! A20 on
	out	#0x60,al
	call	empty_8042

! well, that went ok, I hope. Now we have to reprogram the interrupts :-(
! we put them right after the intel-reserved hardware interrupts, at
! int 0x20-0x2F. There they won't mess up anything. Sadly IBM really
! messed this up with the original PC, and they haven't been able to
! rectify it afterwards. Thus the bios puts interrupts at 0x08-0x0f,
! which is used for the internal hardware interrupts as well. We just
! have to reprogram the 8259's, and it isn't fun.
	/*;��8259�жϿ��������±��*/
	mov	al,#0x11		! initialization sequence
	out	#0x20,al		! send it to 8259A-1
	.word	0x00eb,0x00eb		! jmp $+2, jmp $+2
	out	#0xA0,al		! and to 8259A-2
	.word	0x00eb,0x00eb
	mov	al,#0x20		! start of hardware ints (0x20)
	out	#0x21,al
	.word	0x00eb,0x00eb
	mov	al,#0x28		! start of hardware ints 2 (0x28)
	out	#0xA1,al
	.word	0x00eb,0x00eb
	mov	al,#0x04		! 8259-1 is master
	out	#0x21,al
	.word	0x00eb,0x00eb
	mov	al,#0x02		! 8259-2 is slave
	out	#0xA1,al
	.word	0x00eb,0x00eb
	mov	al,#0x01		! 8086 mode for both
	out	#0x21,al
	.word	0x00eb,0x00eb
	out	#0xA1,al
	.word	0x00eb,0x00eb
	mov	al,#0xFF		! mask off all interrupts for now
	out	#0x21,al
	.word	0x00eb,0x00eb
	out	#0xA1,al

! well, that certainly wasnt fun :-(. Hopefully it works, and we dont
! need no steenking BIOS anyway (except for the initial loading :-).
! The BIOS-routine wants lots of unnecessary data, and its less
! "interesting" anyway. This is how REAL programmers do it.
!
! Well, now is the time to actually move into protected mode. To make
! things as simple as possible, we do no register set-up or anything,
! we let the gnu-compiled 32-bit programs do that. We just jump to
! absolute address 0x00000, in 32-bit protected mode.
//;������Ҫ��ζ�� BIOS ��(���˳�ʼ�ļ���)��
//;BIOS�ӳ���Ҫ��ܶ಻��Ҫ�����ݣ�������һ�㶼ûȤ��
//;���ǡ��������ĳ���Ա�������¡�

	mov	ax,#0x0001	! protected mode (PE) bit	//;����ģʽ����λ(PE)
	lmsw	ax		! This is it!	//;lmsw:Load Machine Status Word�޸ĵ�CRO
	jmpi	0,8		! jmp offset 0 of segment 8 (cs)//;��ת��cs��8��ƫ��0������head
				//;8==>00001 0 00��ʾgdt��ѡ��1����

! This routine checks that the keyboard command queue is empty
! No timeout is used - if this hangs there is something wrong with
! the machine, and we probably couldnt proceed anyway.
empty_8042:
	.word	0x00eb,0x00eb	//;����������תָ��Ļ�����(��ת����һ��)���൱����ʱ�ղ�����
	in	al,#0x64	! 8042 status port
	test	al,#2		! is input buffer full?
	jnz	empty_8042	! yes - loop
	ret

gdt:	//;һ������ռ8���ֽڣ�����ֻ��ʱ������3����
	//;��������:|��8λ���ֻ�ַ|16λ����|24λ���ֻ�ַ|16λ���޳�|
	//;���޳���ҳ4KBΪ��λ

	//;��0���
	.word	0,0,0,0		! dummy

	//;��1������ϵͳ�Ĵ���Σ���csΪ0x08ʱѡ�ľ������
	//;��������Ϊ00|C09A|000000|07FF
	.word	0x07FF		! 8Mb - limit=2047 (2048*4096=8Mb)
	.word	0x0000		! base address=0
	.word	0x9A00		! code read/exec	//;�ɶ���ִ��
	.word	0x00C0		! granularity=4096, 386

	//;��2������ϵͳ�����ݶΣ���ds��Ϊ0x10ʱѡ�ľ������
	//;��������Ϊ00|C092|000000|07FF
	.word	0x07FF		! 8Mb - limit=2047 (2048*4096=8Mb)
	.word	0x0000		! base address=0
	.word	0x9200		! data read/write	//;�ɶ���д
	.word	0x00C0		! granularity=4096, 386

idt_48:	//;�����ǰ�����ط��ĵ�ַ��idtr���ǰ������48λ6�ֽڵ����ݸ�idtr
	.word	0			! idt limit=0	//;��ʱ�ı�������
	.word	0,0			! idt base=0L

gdt_48:	//;�����ǰ�����ط��ĵ�ַ��gdtr���ǰ������48λ6�ֽڵ����ݸ�gdtr
	.word	0x800		! gdt limit=2048, 256 GDT entries //;�͵�ַ���޳�
				//;0x800Ϊ2KB��һ������ռ8���ֽڣ�ֻ��256��
	.word	512+gdt,0x9	! gdt base = 0X9xxxx	//;�ߵ�ַ����ʼ
				//;��ʵ����SETUPSEG+gdt��0x90000+512+gdt
	
.text
endtext:
.data
enddata:
.bss
endbss:
