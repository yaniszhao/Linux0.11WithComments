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
! posterity.	//;得到光标位置

	mov	ax,#INITSEG	! this is done in bootsect already, but...
	mov	ds,ax
	mov	ah,#0x03	! read cursor pos
	xor	bh,bh
	int	0x10		! save it in known place, con_init fetches
	mov	[0],dx		! it from 0x90000. //;基于ds段的偏移

! Get memory size (extended mem, kB)	//;得到扩展内存大小

	mov	ah,#0x88
	int	0x15
	mov	[2],ax	//;基于ds段的偏移

! Get video-card data:	//;得到显卡显示模式

	mov	ah,#0x0f
	int	0x10
	mov	[4],bx		! bh = display page
	mov	[6],ax		! al = video mode, ah = window width

! check for EGA/VGA and some config parameters	//;得到显卡配置参数

	mov	ah,#0x12
	mov	bl,#0x10
	int	0x10
	mov	[8],ax
	mov	[10],bx
	mov	[12],cx

! Get hd0 data	//;第一块硬盘参数表

	mov	ax,#0x0000
	mov	ds,ax
	lds	si,[4*0x41]
	mov	ax,#INITSEG
	mov	es,ax
	mov	di,#0x0080
	mov	cx,#0x10
	rep
	movsb

! Get hd1 data	//;第二块硬盘参数表

	mov	ax,#0x0000
	mov	ds,ax
	lds	si,[4*0x46]
	mov	ax,#INITSEG
	mov	es,ax
	mov	di,#0x0090
	mov	cx,#0x10
	rep
	movsb

! Check that there IS a hd1 :-)	//;判断是否存在第二块硬盘，不存在则表清零

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
	/*;准备开启保护模式*/
	//;在保护模式下运行可以支持多任务
	//;支持4G的物理内存
	//;支持虚拟内存
	//;支持内存的页式管理和段式管理
	//;支持特权级
! now we want to move to protected mode ...
				//;关中断，一直到main.c初始化后才开的
	cli			! no interrupts allowed !

! first we move the system to its rightful place
	/*;移动内核*/
	mov	ax,#0x0000	//;移动到地址0的位置
	cld			! 'direction'=0, movs moves forward	//;设置移动方向
do_move:	//;这个代码有助于理解for循环的汇编代码是怎么样的
	mov	es,ax		! destination segment	//;每次循环目标地址都加0x10000起始为0
	add	ax,#0x1000
	cmp	ax,#0x9000
	jz	end_move
	mov	ds,ax		! source segment	//;每次循环源地址加0x10000起始为0x10000
	sub	di,di
	sub	si,si
	mov 	cx,#0x8000	//;重复移动0x8000次
	rep
	movsw			//;每次移动2个字节，总就是0x10000次了
	jmp	do_move

! then we load the segment descriptors	//;设置临时的idtr和gdtr都是48位的

end_move:
	mov	ax,#SETUPSEG	! right, forgot this at first. didnt work :-)
	mov	ds,ax	//;数据段起始为setup的起始
	lidt	idt_48		! load idt with 0,0
	lgdt	gdt_48		! load gdt with whatever appropriate

! that was painless, now we enable A20	//;开启A20获得1MB以上的内存

	call	empty_8042	//;等待输入缓冲器空。只有当输入缓冲器为空时才可以对其进行写命令。
	mov	al,#0xD1		! command write	//;command write ! 0xD1 命令码-表示要写数据到
	out	#0x64,al	//;8042的P2端口。P2端口的位1用于A20线的选通。数据要写到0x60口。
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
	/*;对8259中断控制器重新编程*/
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
//;不再需要乏味的 BIOS 了(除了初始的加载)。
//;BIOS子程序要求很多不必要的数据，而且它一点都没趣。
//;那是“真正”的程序员所做的事。

	mov	ax,#0x0001	! protected mode (PE) bit	//;保护模式比特位(PE)
	lmsw	ax		! This is it!	//;lmsw:Load Machine Status Word修改的CRO
	jmpi	0,8		! jmp offset 0 of segment 8 (cs)//;跳转至cs段8，偏移0处，即head
				//;8==>00001 0 00表示gdt中选第1个项

! This routine checks that the keyboard command queue is empty
! No timeout is used - if this hangs there is something wrong with
! the machine, and we probably couldnt proceed anyway.
empty_8042:
	.word	0x00eb,0x00eb	//;这是两个跳转指令的机器码(跳转到下一句)，相当于延时空操作。
	in	al,#0x64	! 8042 status port
	test	al,#2		! is input buffer full?
	jnz	empty_8042	! yes - loop
	ret

gdt:	//;一个表项占8个字节，这里只临时设置了3个项
	//;表项内容:|高8位部分基址|16位属性|24位部分基址|16位段限长|
	//;段限长以页4KB为单位

	//;第0项不用
	.word	0,0,0,0		! dummy

	//;第1项用于系统的代码段，当cs为0x08时选的就是这个
	//;具体内容为00|C09A|000000|07FF
	.word	0x07FF		! 8Mb - limit=2047 (2048*4096=8Mb)
	.word	0x0000		! base address=0
	.word	0x9A00		! code read/exec	//;可读可执行
	.word	0x00C0		! granularity=4096, 386

	//;第2项用于系统的数据段，当ds等为0x10时选的就是这个
	//;具体内容为00|C092|000000|07FF
	.word	0x07FF		! 8Mb - limit=2047 (2048*4096=8Mb)
	.word	0x0000		! base address=0
	.word	0x9200		! data read/write	//;可读可写
	.word	0x00C0		! granularity=4096, 386

idt_48:	//;并不是把这个地方的地址给idtr而是把这里的48位6字节的数据给idtr
	.word	0			! idt limit=0	//;临时的表，无数据
	.word	0,0			! idt base=0L

gdt_48:	//;并不是把这个地方的地址给gdtr而是把这里的48位6字节的数据给gdtr
	.word	0x800		! gdt limit=2048, 256 GDT entries //;低地址表限长
				//;0x800为2KB，一个表项占8个字节，只有256项
	.word	512+gdt,0x9	! gdt base = 0X9xxxx	//;高地址表起始
				//;其实就是SETUPSEG+gdt即0x90000+512+gdt
	
.text
endtext:
.data
enddata:
.bss
endbss:
