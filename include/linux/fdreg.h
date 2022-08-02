/*
该头文件用以说明软盘系统常用到的一些参数以及所使用的 I/O 端口。由于软盘驱动器的控制比较
烦琐，命令也多，因此在阅读代码之前，最好先参考有关微型计算机控制接口原理的书籍，了解软盘控
制器 (FDC) 的工作原理，然后你就会觉得这里的定义还是比较合理有序的。
fd--floppy disk
*/

/*
 * This file contains some defines for the floppy disk controller.
 * Various sources. Mostly "IBM Microcomputers: A Programmers
 * Handbook", Sanches and Canton.
 */
/*
 * 该文件中含有一些软盘控制器的一些定义。这些信息有多处来源，大多数取自 Sanches 和 Canton
 * 编著的"IBM 微型计算机：程序员手册"一书。
 */
 
#ifndef _FDREG_H
#define _FDREG_H

// 一些软盘类型函数的原型说明。
extern int ticks_to_floppy_on(unsigned int nr);
extern void floppy_on(unsigned int nr);
extern void floppy_off(unsigned int nr);
extern void floppy_select(unsigned int nr);
extern void floppy_deselect(unsigned int nr);

// 下面是有关软盘控制器一些端口和符号的定义。
/* Fd controller regs. S&C, about page 340 */
/* 软盘控制器(FDC)寄存器端口。摘自 S&C 书中约 340 页 */
#define FD_STATUS	0x3f4		// 主状态寄存器端口。
#define FD_DATA		0x3f5		// 数据端口。
#define FD_DOR		0x3f2		/* Digital Output Register */	// 数字输出寄存器（也称为数字控制寄存器）。
#define FD_DIR		0x3f7		/* Digital Input Register (read) */	// 数字输入寄存器。
#define FD_DCR		0x3f7		/* Diskette Control Register (write)*/	// 数据传输率控制寄存器。

/* Bits of main status register */
/* 主状态寄存器各比特位的含义 */
#define STATUS_BUSYMASK	0x0F		/* drive busy mask */	// 驱动器忙位（每位对应一个驱动器）。
#define STATUS_BUSY	0x10		/* FDC busy */		// 软盘控制器忙。
#define STATUS_DMA	0x20		/* 0- DMA mode */	// 0 - 为 DMA 数据传输模式，1 - 为非 DMA 模式。
#define STATUS_DIR	0x40		/* 0- cpu->fdc */	// 传输方向：0 - CPU --> fdc，1 - 相反。
#define STATUS_READY	0x80		/* Data reg ready */	// 数据寄存器就绪位。

/* Bits of FD_ST0 */
/*状态字节 0（ST0）各比特位的含义 */
#define ST0_DS		0x03		/* drive select mask */	// 驱动器选择号（发生中断时驱动器号）。
#define ST0_HA		0x04		/* Head (Address) */	// 磁头号。
#define ST0_NR		0x08		/* Not Ready */			// 磁盘驱动器未准备好。
#define ST0_ECE		0x10		/* Equipment chech error */	// 设备检测出错（零磁道校准出错）。
#define ST0_SE		0x20		/* Seek end */				// 寻道或重新校正操作执行结束。
#define ST0_INTR	0xC0		/* Interrupt code mask */	
								// 中断代码位（中断原因），00 - 命令正常结束；
								// 01 - 命令异常结束；10 - 命令无效；11 - FDD 就绪状态改变。

/* Bits of FD_ST1 */
/*状态字节 1（ST1）各比特位的含义 */
#define ST1_MAM		0x01		/* Missing Address Mark */
#define ST1_WP		0x02		/* Write Protect */
#define ST1_ND		0x04		/* No Data - unreadable */
#define ST1_OR		0x10		/* OverRun */
#define ST1_CRC		0x20		/* CRC error in data or addr */
#define ST1_EOC		0x80		/* End Of Cylinder */

/* Bits of FD_ST2 */
/*状态字节 2（ST2）各比特位的含义 */
#define ST2_MAM		0x01		/* Missing Addess Mark (again) */
#define ST2_BC		0x02		/* Bad Cylinder */
#define ST2_SNS		0x04		/* Scan Not Satisfied */
#define ST2_SEH		0x08		/* Scan Equal Hit */
#define ST2_WC		0x10		/* Wrong Cylinder */
#define ST2_CRC		0x20		/* CRC error in data field */
#define ST2_CM		0x40		/* Control Mark = deleted */

/* Bits of FD_ST3 */
/*状态字节 3（ST3）各比特位的含义 */
#define ST3_HA		0x04		/* Head (Address) */
#define ST3_TZ		0x10		/* Track Zero signal (1=track 0) */
#define ST3_WP		0x40		/* Write Protect */

/* Values for FD_COMMAND */
/* 软盘命令码 */
#define FD_RECALIBRATE	0x07		/* move to track 0 */	// 重新校正(磁头退到零磁道)。
#define FD_SEEK		0x0F		/* seek track */	// 磁头寻道。
#define FD_READ		0xE6		/* read with MT, MFM, SKip deleted */	// 读数据（MT 多磁道操作，MFM 格式，跳过删除数据）。
#define FD_WRITE	0xC5		/* write with MT, MFM */	// 写数据（MT，MFM）。
#define FD_SENSEI	0x08		/* Sense Interrupt Status */	// 检测中断状态。
#define FD_SPECIFY	0x03		/* specify HUT etc */	// 设定驱动器参数（步进速率、磁头卸载时间等）。

/* DMA commands */
#define DMA_READ	0x46	// DMA 读盘，DMA 方式字（送 DMA 端口 12，11）。
#define DMA_WRITE	0x4A	// DMA 写盘，DMA 方式字。

#endif
