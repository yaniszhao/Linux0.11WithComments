/*
 *  linux/kernel/hd.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * This is the low-level hd interrupt support. It traverses the
 * request-list, using interrupts to jump between functions. As
 * all the functions are called within interrupts, we may not
 * sleep. Special care is recommended.
 * 
 *  modified by Drew Eckhardt to check nr of hd's from the CMOS.
 */
/*
 * 本程序是底层硬盘中断辅助程序。主要用于扫描请求列表，使用中断在函数之间跳转。
 * 由于所有的函数都是在中断里调用的，所以这些函数不可以睡眠。请特别注意。
 * 由 Drew Eckhardt 修改，利用 CMOS 信息检测硬盘数。
 */

#include <linux/config.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/hdreg.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/segment.h>

// 必须在 blk.h 文件之前定义下面的主设备号常数，因为 blk.h 文件中要用到该常数。
#define MAJOR_NR 3	// 硬盘主设备号是 3。
#include "blk.h"

//读CMOS参数宏函数。
#define CMOS_READ(addr) ({ \
outb_p(0x80|addr,0x70); \
inb_p(0x71); \
})

/* Max read/write errors/sector */
#define MAX_ERRORS	7			// 读/写一个扇区时允许的最多出错次数。
#define MAX_HD		2			// 系统支持的最多硬盘数。

static void recal_intr(void);	// 硬盘中断程序在复位操作时会调用的重新校正函数

static int recalibrate = 1;		// 重新校正标志。将磁头移动到 0 柱面。
static int reset = 1;			// 复位标志。当发生读写错误时会设置该标志，以复位硬盘和控制器。

/*
 *  This struct defines the HD's and their types.
 */
/* 下面结构定义了硬盘参数及类型 */
// 各字段分别是磁头数、每磁道扇区数、柱面数、写前预补偿柱面号、磁头着陆区柱面号、控制字节。 
// 它们的含义请参见程序列表后的说明。
struct hd_i_struct {
	int head,sect,cyl,wpcom,lzone,ctl;
	};

// 如果已经在 include/linux/config.h 头文件中定义了 HD_TYPE，
// 就取其中定义好的参数作为hd_info[]的数据。
// 否则，先默认都设为 0 值，在 setup()函数中会进行设置。
#ifdef HD_TYPE
struct hd_i_struct hd_info[] = { HD_TYPE };
#define NR_HD ((sizeof (hd_info))/(sizeof (struct hd_i_struct)))	// 计算硬盘个数。
#else
struct hd_i_struct hd_info[] = { {0,0,0,0,0,0},{0,0,0,0,0,0} };
static int NR_HD = 0;	//上面是2个，这里又是0，奇怪
#endif

// 定义硬盘分区结构。给出每个分区的物理起始扇区号、分区扇区总数。
// 其中 5 的倍数处的项(例如 hd[0]和 hd[5]等)代表整个硬盘中的参数。
static struct hd_struct {
	long start_sect;	//分区起始扇区号
	long nr_sects;		//分区扇区总数
} hd[5*MAX_HD]={{0,0},};

// 读端口 port，共读 nr 字，保存在 buf 中。
// INS指令可从DX指出的外设端口输入一个字节或字到由ES: DI指定的存储器中。
#define port_read(port,buf,nr) \
__asm__("cld;rep;insw"::"d" (port),"D" (buf),"c" (nr):"cx","di")

// 写端口 port，共写 nr 字，从 buf 中取数据。
#define port_write(port,buf,nr) \
__asm__("cld;rep;outsw"::"d" (port),"S" (buf),"c" (nr):"cx","si")

extern void hd_interrupt(void);	// 硬盘中断过程
extern void rd_load(void);		// 虚拟盘创建加载函数

/* This may be used only once, enforced by 'static int callable' */
/* 下面该函数只在初始化时被调用一次。用静态变量 callable 作为可调用标志。*/
// 该函数的参数由初始化程序 init/main.c 的 init 子程序设置为指向 0x90080 处，
// 此处存放着 setup.s 程序从 BIOS 取得的 2 个硬盘的基本参数表(32 字节)。
// 硬盘参数表信息参见下面列表后的说明。
// 本函数主要功能是读取 CMOS 和硬盘参数表信息，用于设置硬盘分区结构 hd，
// 并加载 RAM 虚拟盘和根文件系统。
int sys_setup(void * BIOS)
{
	static int callable = 1;
	int i,drive;
	unsigned char cmos_disks;
	struct partition *p;
	struct buffer_head * bh;

	if (!callable)//保证这个函数只能被执行一次
		return -1;
	callable = 0;

// 如果没有在 config.h 中定义硬盘参数，就从 0x90080 处读入。	
#ifndef HD_TYPE
	for (drive=0 ; drive<2 ; drive++) {
		hd_info[drive].cyl = *(unsigned short *) BIOS;			//柱面数
		hd_info[drive].head = *(unsigned char *) (2+BIOS);		//磁头数
		hd_info[drive].wpcom = *(unsigned short *) (5+BIOS);	//写前预补偿柱面号
		hd_info[drive].ctl = *(unsigned char *) (8+BIOS);		//控制字节
		hd_info[drive].lzone = *(unsigned short *) (12+BIOS);	//磁头着陆区柱面号
		hd_info[drive].sect = *(unsigned char *) (14+BIOS);		//每磁道扇区数
		BIOS += 16;	// 每个硬盘的参数表长 16 字节，这里 BIOS 指向下一个表
	}

	// setup.s 程序在取 BIOS 中的硬盘参数表信息时，如果只有 1 个硬盘，
	// 就会将对应第 2 个硬盘的 16 字节全部清零。
	// 因此这里只要判断第 2 个硬盘柱面数是否为 0 就可以知道有没有第 2 个硬盘了。
	if (hd_info[1].cyl)
		NR_HD=2;
	else
		NR_HD=1;
#endif

	for (i=0 ; i<NR_HD ; i++) {	//初始化代表整块硬盘那个分区
		hd[i*5].start_sect = 0;						// 硬盘起始扇区号
		hd[i*5].nr_sects = hd_info[i].head*			// 硬盘总扇区数。
				hd_info[i].sect*hd_info[i].cyl;
	}

	/*
		We querry CMOS about hard disks : it could be that 
		we have a SCSI/ESDI/etc controller that is BIOS
		compatable with ST-506, and thus showing up in our
		BIOS table, but not register compatable, and therefore
		not present in CMOS.

		Furthurmore, we will assume that our ST-506 drives
		<if any> are the primary drives in the system, and 
		the ones reflected as drive 1 or 2.

		The first drive is stored in the high nibble of CMOS
		byte 0x12, the second in the low nibble.  This will be
		either a 4 bit drive type or 0xf indicating use byte 0x19 
		for an 8 bit type, drive 1, 0x1a for drive 2 in CMOS.

		Needless to say, a non-zero value means we have 
		an AT controller hard disk for that drive.

		
	*/

	/*
	 * 我们对 CMOS 有关硬盘的信息有些怀疑:可能会出现这样的情况，我们有一块 SCSI/ESDI/等的 控制器，
	 * 它是以 ST-506 方式与 BIOS 兼容的，因而会出现在我们的 BIOS 参数表中，但却又不 是寄存器兼容的，
	 * 因此这些参数在 CMOS 中又不存在。
	 * 另外，我们假设 ST-506 驱动器(如果有的话)是系统中的基本驱动器，也即以驱动器 1 或 2 出现的驱动器。
	 * 第 1 个驱动器参数存放在 CMOS 字节 0x12 的高半字节中，第 2 个存放在低半字节中。
	 * 该 4 位字节 信息可以是驱动器类型，也可能仅是 0xf。
	 * 0xf 表示使用 CMOS 中 0x19 字节作为驱动器 1 的 8 位 类型字节，
	 * 使用 CMOS 中 0x1A 字节作为驱动器 2 的类型字节。 
	 * 总之，一个非零值意味着我们有一个 AT 控制器硬盘兼容的驱动器。
	 */

	// 这里根据上述原理来检测硬盘到底是否是 AT 控制器兼容的。
	// 有关 CMOS 信息请参见 4.2.3.1 节。
	if ((cmos_disks = CMOS_READ(0x12)) & 0xf0)
		if (cmos_disks & 0x0f)
			NR_HD = 2;
		else
			NR_HD = 1;
	else
		NR_HD = 0;

	// 若 NR_HD=0，则两个硬盘都不是 AT 控制器兼容的，硬盘数据结构清零。 
	// 若 NR_HD=1，则将第 2 个硬盘的参数清零。
	for (i = NR_HD ; i < 2 ; i++) {//有趣的写法，省去了判断
		hd[i*5].start_sect = 0;
		hd[i*5].nr_sects = 0;
	}

	// 读取每一个硬盘上第 1 块数据，获取分区表信息。
	for (drive=0 ; drive<NR_HD ; drive++) {
		if (!(bh = bread(0x300 + drive*5,0))) {//读取第一个数据块到缓冲区
			printk("Unable to read partition table of drive %d\n\r",
				drive);
			panic("");
		}
		if (bh->b_data[510] != 0x55 || (unsigned char)
		    bh->b_data[511] != 0xAA) {	// 判断硬盘信息有效标志'55AA'
			printk("Bad partition table on drive %d\n\r",drive);
			panic("");
		}
		p = 0x1BE + (void *)bh->b_data;	// 分区表位于硬盘第 1 扇区的 0x1BE 处。
		for (i=1;i<5;i++,p++) {//每块硬盘读4个分区表
			hd[i+5*drive].start_sect = p->start_sect;
			hd[i+5*drive].nr_sects = p->nr_sects;
		}
		brelse(bh);	// 分区表位于硬盘第 1 扇区的 0x1BE 处。
	}
	if (NR_HD)		//有硬盘且已读入，则打印信息。
		printk("Partition table%s ok.\n\r",(NR_HD>1)?"s":"");//打印信息还考虑加复数，哈哈
	rd_load();		// 加载根文件系统到RAMDISK
	mount_root();	// 加载超级块和根目录到高速缓冲区
	return (0);
}

// 判断并循环等待驱动器就绪。
// 读硬盘控制器状态寄存器端口 HD_STATUS(0x1f7)，并循环检测驱动器就绪比特位和控制器忙位。 
// 如果返回值为 0，则表示等待超时出错，否则 OK。
static int controller_ready(void)
{
	int retries=10000;

	while (--retries && (inb_p(HD_STATUS)&0xc0)!=0x40);
	return (retries);
}

// 检测硬盘执行命令后的状态。(win_表示温切斯特硬盘的缩写)
// 读取状态寄存器中的命令执行结果状态。返回 0 表示正常，1 出错。
// 如果执行命令错，则再读错误寄存器 HD_ERROR(0x1f1)。
static int win_result(void)
{
	int i=inb_p(HD_STATUS);

	if ((i & (BUSY_STAT | READY_STAT | WRERR_STAT | SEEK_STAT | ERR_STAT))
		== (READY_STAT | SEEK_STAT))
		return(0); /* ok */
	if (i&1) i=inb(HD_ERROR);
	return (1);
}

// 向硬盘控制器发送命令块。
// 调用参数:drive - 硬盘号(0-1); nsect - 读写扇区数;
// sect - 起始扇区; head - 磁头号;
// cyl - 柱面号; cmd - 命令码(参见控制器命令列表，表 6.3);
// *intr_addr() - 硬盘中断发生时处理程序中将调用的 C 处理函数。
static void hd_out(unsigned int drive,unsigned int nsect,unsigned int sect,
		unsigned int head,unsigned int cyl,unsigned int cmd,
		void (*intr_addr)(void))
{
	register int port asm("dx"); 	//port 变量对应寄存器 dx；这写法，之前没遇到过。

	if (drive>1 || head>15)			// 如果驱动器号(0,1)>1 或磁头号>15，则程序不支持。
		panic("Trying to write bad sector");
	if (!controller_ready())		// 如果等待一段时间后仍未就绪则出错，死机。
		panic("HD controller not ready");

	//这是个函数指针，又DEVICE_INTR宏定义后分配
	// do_hd 函数指针将在硬盘中断程序中被调用。
	do_hd = intr_addr;		
	
	outb_p(hd_info[drive].ctl,HD_CMD);		// 向控制寄存器(0x3f6)输出控制字节。
	port=HD_DATA;							// 置 dx 为数据寄存器端口(0x1f0)。
	outb_p(hd_info[drive].wpcom>>2,++port);	// 参数:写预补偿柱面号(需除 4)。
	outb_p(nsect,++port);					// 参数:读/写扇区总数。�
	outb_p(sect,++port);					// 参数:起始扇区。
	outb_p(cyl,++port);						// 参数:柱面号低 8 位。
	outb_p(cyl>>8,++port);					// 参数:柱面号高 8 位。
	outb_p(0xA0|(drive<<4)|head,++port);	// 参数:驱动器号+磁头号。
	outb(cmd,++port);						// 命令:硬盘控制命令。
}

// 等待硬盘就绪。也即循环等待主状态控制器忙标志位复位。
// 若仅有就绪或寻道结束标志置位，则成功，返回 0。
// 若经过一段时间仍为忙，则返回 1。
static int drive_busy(void)
{
	unsigned int i;

	for (i = 0; i < 10000; i++)					// 循环等待就绪标志位置位。
		if (READY_STAT == (inb_p(HD_STATUS) & (BUSY_STAT|READY_STAT)))
			break;
	i = inb(HD_STATUS);							// 再取主控制器状态字节。
	i &= BUSY_STAT | READY_STAT | SEEK_STAT;	// 检测忙位、就绪位和寻道结束位
	if (i == READY_STAT | SEEK_STAT)			// 若仅有就绪或寻道结束标志，则返回 0。
		return(0);
	printk("HD controller times out\n\r");		// 否则等待超时，显示信息。并返回 1。
	return(1);
}

// 诊断复位(重新校正)硬盘控制器。
static void reset_controller(void)
{
	int	i;

	outb(4,HD_CMD);							// 向控制寄存器端口发送控制字节(4-复位)
	for(i = 0; i < 100; i++) nop();			// 等待一段时间(循环空操作)
	outb(hd_info[0].ctl & 0x0f ,HD_CMD);	// 再发送正常的控制字节(不禁止重试、重读)。
	if (drive_busy())						// 若等待硬盘就绪超时，则显示出错信息。
		printk("HD-controller still busy\n\r");
	if ((i = inb(HD_ERROR)) != 1)			// 取错误寄存器，若不等于 1(无错误)则出错。
		printk("HD-controller reset failed: %02x\n\r",i);
}

// 复位硬盘 nr。首先复位(重新校正)硬盘控制器。然后发送硬盘控制器命令“建立驱动器参数”，
// 其中 recal_intr()是在硬盘中断处理程序中调用的重新校正处理函数。
static void reset_hd(int nr)
{
	//复位(重新校正)硬盘控制器
	reset_controller();	
	//发送硬盘控制器命令“建立驱动器参数”
	hd_out(nr,hd_info[nr].sect,hd_info[nr].sect,hd_info[nr].head-1,
		hd_info[nr].cyl,WIN_SPECIFY,&recal_intr);
}

// 意外硬盘中断调用函数。
// 发生意外硬盘中断时，硬盘中断处理程序中调用的默认 C 处理函数。
// 在被调用函数指针为空时调用该函数。
void unexpected_hd_interrupt(void)
{
	printk("Unexpected HD interrupt\n\r");
}

// 读写硬盘失败处理调用函数。
static void bad_rw_intr(void)
{
	// 如果读扇区时的出错次数大于或等于 7 次时，则结束请求并唤醒等待该请求的进程，
	// 而且对应缓冲区更新标志复位(没有更新)。
	if (++CURRENT->errors >= MAX_ERRORS)
		end_request(0);
	// 如果读一扇区时的出错次数已经大于 3 次， 则要求执行复位硬盘控制器操作。
	if (CURRENT->errors > MAX_ERRORS/2)
		reset = 1;
}

// 读操作中断调用函数。将在硬盘读命令结束时引发的中断过程中被调用。
// 该函数首先判断此次读命令操作是否出错。若命令结束后控制器还处于忙状态，或者命令执行错误， 
// 则处理硬盘操作失败问题，接着请求硬盘作复位处理并执行其它请求项。
// 如果读命令没有出错，则从数据寄存器端口把一个扇区的数据读到请求项的缓冲区中，并递减请求项 
// 所需读取的扇区数值。若递减后不等于 0，表示本项请求还有数据没取完，于是直接返回，等待硬盘 
// 在读出另一个扇区数据后的中断。否则表明本请求项所需的所有扇区都已读完，于是处理本次请求项 
// 结束事宜。最后再次调用 do_hd_request()，去处理其它硬盘请求项。
static void read_intr(void)
{
	if (win_result()) {			// 若控制器忙、读写错或命令执行错，
		bad_rw_intr();			// 则进行读写硬盘失败处理，
		do_hd_request();		// 然后再次请求硬盘作相应(复位)处理。
		return;
	}

	// 将数据从数据寄存器口读到请求结构缓冲区。
	port_read(HD_DATA,CURRENT->buffer,256);// 注意:256 是指内存字，也即 512 字节。

	CURRENT->errors = 0;			// 清出错次数。
	CURRENT->buffer += 512;			// 调整缓冲区指针，指向新的空区。
	CURRENT->sector++;				// 起始扇区号加 1，

	if (--CURRENT->nr_sectors) {	// 如果所需读出的扇区数还没有读完，
		do_hd = &read_intr;			// 则再次置硬盘调用 C 函数指针为 read_intr()，
		return;// 因为硬盘中断处理程序每次调用 do_hd 时都会将该函数指针置空。参见 system_call.s
	}
	end_request(1);					// 若全部扇区数据已经读完，则处理请求结束事宜。
	do_hd_request();				//执行其它硬盘请求操作。
}

// 写扇区中断调用函数。在硬盘中断处理程序中被调用。
// 在写命令执行后，会产生硬盘中断信号，执行硬盘中断处理程序，
// 此时在硬盘中断处理程序中调用的 C 函数指针 do_hd()已经指向 write_intr()，
// 因此会在写操作完成(或出错)后，执行该函数。
static void write_intr(void)
{
	if (win_result()) {				// 如果硬盘控制器返回错误信息，
		bad_rw_intr();				// 则首先进行硬盘读写失败处理，
		do_hd_request();			// 然后再次请求硬盘作相应(复位)处理，
		return;						// 然后返回(也退出了此次硬盘中断)。
	}
	if (--CURRENT->nr_sectors) {	// 否则将欲写扇区数减 1，若还有扇区要写，则
		CURRENT->sector++;			// 当前请求起始扇区号+1，
		CURRENT->buffer += 512;		// 调整请求缓冲区指针，
		do_hd = &write_intr;		// 置硬盘中断程序调用函数指针为 write_intr()，
		port_write(HD_DATA,CURRENT->buffer,256);// 再向数据寄存器端口写 256 字。
		return;									// 返回等待硬盘再次完成写操作后的中断处理。
	}
	end_request(1);					// 若全部扇区数据已经写完，则处理请求结束事宜，
	do_hd_request();				// 执行其它硬盘请求操作。
}

// 硬盘重新校正（复位）中断调用函数。在硬盘中断处理程序中被调用。
// 如果硬盘控制器返回错误信息，则首先进行硬盘读写失败处理，然后请求硬盘作相应(复位)处理。
static void recal_intr(void)
{
	if (win_result())
		bad_rw_intr();
	do_hd_request();
}

// 执行硬盘读写请求操作。
// 若请求项是块设备的第 1 个，则块设备当前请求项指针会直接指向该请求项，并会立刻调用本函数执行读写操作。
// 否则在一个读写操作完成而引发的硬盘中断过程中，若还有请求项需要处理，则也会在中断过程中调用本函数。
void do_hd_request(void)
{
	int i,r;
	unsigned int block,dev;
	unsigned int sec,head,cyl;
	unsigned int nsect;

	// 检测请求项的合法性，若已没有请求项则退出。
	INIT_REQUEST;
	// 取设备号中的子设备号(见列表后对硬盘设备号的说明)。子设备号即是硬盘上的分区号。
	dev = MINOR(CURRENT->dev);
	block = CURRENT->sector;
	// 如果子设备号不存在或者起始扇区大于该分区扇区数-2，则结束该请求，并跳转到标号 repeat 处。
	// 因为一次要求读写 2 个扇区（512*2 字节），所以请求的扇区号不能大于分区中最后倒数第二个扇区号。
	if (dev >= 5*NR_HD || block+2 > hd[dev].nr_sects) {
		end_request(0);
		goto repeat;//repeat在INIT_REQUEST中定义的。
	}
	// 通过加上本分区的起始扇区号，把将所需读写的块对应到整个硬盘的绝对扇区号上。
	block += hd[dev].start_sect;
	dev /= 5;// 此时 dev 代表硬盘号（是第 1 个硬盘(0)还是第 2 个(1)）。
	// 下面嵌入汇编代码用来从硬盘信息结构中根据起始扇区号和每磁道扇区数计算在磁道中的
	// 扇区号(sec)、所在柱面号(cyl)和磁头号(head)。
	__asm__("divl %4":"=a" (block),"=d" (sec):"0" (block),"1" (0),
		"r" (hd_info[dev].sect));
	__asm__("divl %4":"=a" (cyl),"=d" (head):"0" (block),"1" (0),
		"r" (hd_info[dev].head));
	sec++;
	nsect = CURRENT->nr_sectors;
	// 如果 reset 标志是置位的，则执行复位操作。复位硬盘和控制器，并置需要重新校正标志，返回。
	if (reset) {
		reset = 0;
		recalibrate = 1;
		reset_hd(CURRENT_DEV);
		return;
	}
	// 如果重新校正标志(recalibrate)置位，则首先复位该标志，然后向硬盘控制器发送重新校正命令。
	// 该命令会执行寻道操作，让处于任何地方的磁头移动到 0 柱面。
	if (recalibrate) {
		recalibrate = 0;
		hd_out(dev,hd_info[CURRENT_DEV].sect,0,0,0,
			WIN_RESTORE,&recal_intr);
		return;
	}	
	// 如果当前请求是写扇区操作，则发送写命令，循环读取状态寄存器信息并判断请求服务标志
	// DRQ_STAT 是否置位。DRQ_STAT 是硬盘状态寄存器的请求服务位，表示驱动器已经准备好在主机和
	// 数据端口之间传输一个字或一个字节的数据。
	if (CURRENT->cmd == WRITE) {
		hd_out(dev,nsect,sec,head,cyl,WIN_WRITE,&write_intr);
		for(i=0 ; i<3000 && !(r=inb_p(HD_STATUS)&DRQ_STAT) ; i++)
			/* nothing */ ;
		// 如果请求服务 DRQ 置位则退出循环。若等到循环结束也没有置位，
		// 则表示此次写硬盘操作失败，去处理下一个硬盘请求。
		// 否则向硬盘控制器数据寄存器端口 HD_DATA 写入 1 个扇区的数据。
		if (!r) {
			bad_rw_intr();
			goto repeat;//repeat在INIT_REQUEST中定义的。
		}
		port_write(HD_DATA,CURRENT->buffer,256);
	// 如果当前请求是读硬盘扇区，则向硬盘控制器发送读扇区命令。
	} else if (CURRENT->cmd == READ) {
		hd_out(dev,nsect,sec,head,cyl,WIN_READ,&read_intr);
	} else
		panic("unknown hd-command");
}

// 硬盘系统初始化。
void hd_init(void)
{
	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;// do_hd_request()。
	// 设置硬盘中断门向量 int 0x2E(46)。
	set_intr_gate(0x2E,&hd_interrupt);
	// 复位接联的主 8259A int2 的屏蔽位，允许从片发出中断请求信号。
	outb_p(inb_p(0x21)&0xfb,0x21);
	// 复位硬盘的中断请求屏蔽位（在从片上），允许硬盘控制器发送中断请求信号。
	outb(inb_p(0xA1)&0xbf,0xA1);
}
