/*
 *  linux/kernel/blk_drv/ramdisk.c
 *
 *  Written by Theodore Ts'o, 12/2/91
 */
/* 由 Theodore Ts'o 编制，12/2/91 */
// Theodore Ts'o (Ted Ts'o)是 linux 社区中的著名人物。Linux 在世界范围内的流行也有他很大的
// 功劳，早在 Linux 操作系统刚问世时，他就怀着极大的热情为 linux 的发展提供了 maillist，并
// 在北美洲地区最早设立了 linux 的 ftp 站点（tsx-11.mit.edu），而且至今仍然为广大 linux 用户
// 提供服务。他对 linux 作出的最大贡献之一是提出并实现了 ext2 文件系统。该文件系统已成为
// linux 世界中事实上的文件系统标准。最近他又推出了 ext3 文件系统，大大提高了文件系统的
// 稳定性和访问效率。作为对他的推崇，第 97 期（2002 年 5 月）的 linuxjournal 期刊将他作为
// 了封面人物，并对他进行了采访。目前，他为 IBM linux 技术中心工作，并从事着有关 LSB
// (Linux Standard Base)等方面的工作。(他的主页：http://thunk.org/tytso/)

//本文件是内存虚拟盘（ Ram Disk ）驱动程序，由 Theodore Ts'o 编制。虚拟盘设备是一种利用物理内
//存来模拟实际磁盘存储数据的方式。其目的主要是为了提高对“磁盘”数据的读写操作速度。除了需要
//占用一些宝贵的内存资源外，其主要缺点是一旦系统崩溃或关闭，虚拟盘中的所有数据将全部消失。因
//此虚拟盘中通常存放一些系统命令等常用工具程序或临时数据，而非重要的输入文档。

#include <string.h>

#include <linux/config.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <asm/system.h>
#include <asm/segment.h>
#include <asm/memory.h>

#define MAJOR_NR 1	// RAM 盘主设备号是 1。主设备号必须在 blk.h 之前被定义。
#include "blk.h"

// 虚拟盘在内存中的起始位置。在 52 行初始化函数 rd_init()中
// 确定。参见(init/main.c,124)（缩写 rd_代表 ramdisk_）
char	*rd_start;
int	rd_length = 0;	// 虚拟盘所占内存大小（字节）。

// 虚拟盘当前请求项操作函数。程序结构与 do_hd_request()类似(hd.c,294)。
// 在低级块设备接口函数 ll_rw_block()建立了虚拟盘（rd）的请求项并添加到 rd 的链表中之后，
// 就会调用该函数对 rd 当前请求项进行处理。该函数首先计算当前请求项中指定的起始扇区对应
// 虚拟盘所处内存的起始位置 addr 和要求的扇区数对应的字节长度值 len，然后根据请求项中的
// 命令进行操作。若是写命令 WRITE，就把请求项所指缓冲区中的数据直接复制到内存位置 addr
// 处。若是读操作则反之。数据复制完成后即可直接调用 end_request()对本次请求项作结束处理。
// 然后跳转到函数开始处再去处理下一个请求项。
void do_rd_request(void)
{
	int	len;
	char	*addr;

	// 检测请求项的合法性，若已没有请求项则退出(参见 blk.h,127)。
	INIT_REQUEST;
	// 下面语句取得 ramdisk 的起始扇区对应的内存起始位置和内存长度。
	// 其中 sector << 9 表示 sector * 512，CURRENT 定义为(blk_dev[MAJOR_NR].current_request)。
	addr = rd_start + (CURRENT->sector << 9);
	len = CURRENT->nr_sectors << 9;
	// 如果子设备号不为 1 或者对应内存起始位置>虚拟盘末尾，则结束该请求，并跳转到 repeat 处。
	// 标号 repeat 定义在宏 INIT_REQUEST 内，位于宏的开始处，参见 blk.h，127 行。
	if ((MINOR(CURRENT->dev) != 1) || (addr+len > rd_start+rd_length)) {
		end_request(0);
		goto repeat;
	}
	// 如果是写命令(WRITE)，则将请求项中缓冲区的内容复制到 addr 处，长度为 len 字节。
	if (CURRENT-> cmd == WRITE) {
		(void ) memcpy(addr,
			      CURRENT->buffer,
			      len);
	// 如果是读命令(READ)，则将 addr 开始的内容复制到请求项中缓冲区中，长度为 len 字节。
	} else if (CURRENT->cmd == READ) {
		(void) memcpy(CURRENT->buffer, 
			      addr,
			      len);
	// 否则显示命令不存在，死机。
	} else
		panic("unknown ramdisk-command");
	// 请求项成功后处理，置更新标志。并继续处理本设备的下一请求项。
	end_request(1);
	goto repeat;
}

/*
 * Returns amount of memory which needs to be reserved.
 */
 /* 返回内存虚拟盘 ramdisk 所需的内存量 */
// 虚拟盘初始化函数。确定虚拟盘在内存中的起始地址，长度。并对整个虚拟盘区清零。
long rd_init(long mem_start, int length)
{
	int	i;
	char	*cp;

	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;	// do_rd_request()。
	rd_start = (char *) mem_start;					// 对于 16MB 系统，该值为 4MB。
	rd_length = length;
	cp = rd_start;
	for (i=0; i < length; i++)
		*cp++ = '\0';
	return(length);
}

/*
 * If the root device is the ram disk, try to load it.
 * In order to do this, the root device is originally set to the
 * floppy, and we later change it to be ram disk.
 */
/*
 * 如果根文件系统设备(root device)是 ramdisk 的话，则尝试加载它。root device 原先是指向
 * 软盘的，我们将它改成指向 ramdisk。
 */
// 尝试把根文件系统加载到虚拟盘中。
// 该函数将在内核设置函数 setup()（hd.c，156 行）中被调用。另外，1 磁盘块 = 1024 字节。
void rd_load(void)
{
	struct buffer_head *bh;	// 高速缓冲块头指针。
	struct super_block	s;	// 文件超级块结构。
	/* 表示根文件系统映象文件在 boot 盘第 256 磁盘块开始处*/
	int		block = 256;	/* Start at block 256 */ 
	int		i = 1;			
	int		nblocks;
	char		*cp;		/* Move pointer */
	
	if (!rd_length)	// 如果 ramdisk 的长度为零，则退出。
		return;
	printk("Ram disk: %d bytes, starting at 0x%x\n", rd_length,
		(int) rd_start);		// 显示 ramdisk 的大小以及内存起始位置。
	if (MAJOR(ROOT_DEV) != 2)	// 如果此时根文件设备不是软盘，则退出。
		return;
	// 读软盘块 256+1,256,256+2。breada()用于读取指定的数据块，并标出还需要读的块，然后返回
	// 含有数据块的缓冲区指针。如果返回 NULL，则表示数据块不可读(fs/buffer.c,322)。
	// 这里 block+1 是指磁盘上的超级块。
	bh = breada(ROOT_DEV,block+1,block,block+2,-1);
	if (!bh) {
		printk("Disk error while looking for ramdisk!\n");
		return;
	}
	// 将 s 指向缓冲区中的磁盘超级块。(d_super_block 磁盘中超级块结构)。
	*((struct d_super_block *) &s) = *((struct d_super_block *) bh->b_data);
	brelse(bh);
	if (s.s_magic != SUPER_MAGIC)	// 如果超级块中魔数不对，则说明不是 minix 文件系统。
		/* No ram disk image present, assume normal floppy boot */
		/* 磁盘中没有 ramdisk 映像文件，退出去执行通常的软盘引导 */
		return;
	// 块数 = 逻辑块数(区段数) * 2^(每区段块数的次方)。
	// 如果数据块数大于内存中虚拟盘所能容纳的块数，则也不能加载，显示出错信息并返回。否则显示
	// 加载数据块信息。
	nblocks = s.s_nzones << s.s_log_zone_size;
	if (nblocks > (rd_length >> BLOCK_SIZE_BITS)) {
		printk("Ram disk image too big!  (%d blocks, %d avail)\n", 
			nblocks, rd_length >> BLOCK_SIZE_BITS);
		return;
	}
	printk("Loading %d bytes into ram disk... 0000k", 
		nblocks << BLOCK_SIZE_BITS);
	// cp 指向虚拟盘起始处，然后将磁盘上的根文件系统映象文件复制到虚拟盘上。
	cp = rd_start;
	while (nblocks) {
		if (nblocks > 2) 	// 如果需读取的块数多于 3 快则采用超前预读方式读数据块。
			bh = breada(ROOT_DEV, block, block+1, block+2, -1);
		else				// 否则就单块读取。
			bh = bread(ROOT_DEV, block);
		if (!bh) {
			printk("I/O error on block %d, aborting load\n", 
				block);
			return;
		}
		(void) memcpy(cp, bh->b_data, BLOCK_SIZE);	// 将缓冲区中的数据复制到 cp 处。
		brelse(bh);									// 释放缓冲区。
		printk("\010\010\010\010\010%4dk",i);		// 打印加载块计数值。
		cp += BLOCK_SIZE;							// 虚拟盘指针前移。
		block++;
		nblocks--;
		i++;
	}
	printk("\010\010\010\010\010done \n");
	ROOT_DEV=0x0101;								// 修改 ROOT_DEV 使其指向虚拟盘 ramdisk。
}
