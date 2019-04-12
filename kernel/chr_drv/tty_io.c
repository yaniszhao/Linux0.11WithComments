/*
 *  linux/kernel/tty_io.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * 'tty_io.c' gives an orthogonal feeling to tty's, be they consoles
 * or rs-channels. It also implements echoing, cooked mode etc.
 *
 * Kill-line thanks to John T Kohl.
 */
/*
 * 'tty_io.c'给 tty 一种非相关的感觉，是控制台还是串行通道。该程序同样
 * 实现了回显、规范(熟)模式等。
 *
 * Kill-line，谢谢 John T Kahl。
 */
#include <ctype.h>
#include <errno.h>
#include <signal.h>

// 下面给出相应信号在信号位图中的对应比特位。
#define ALRMMASK (1<<(SIGALRM-1))		// 警告(alarm)信号屏蔽位。
#define KILLMASK (1<<(SIGKILL-1))		// 终止(kill)信号屏蔽位。
#define INTMASK (1<<(SIGINT-1))			// 键盘中断(int)信号屏蔽位。
#define QUITMASK (1<<(SIGQUIT-1))		// 键盘退出(quit)信号屏蔽位。
#define TSTPMASK (1<<(SIGTSTP-1))		// tty 发出的停止进程(tty stop)信号屏蔽位。

#include <linux/sched.h>
#include <linux/tty.h>
#include <asm/segment.h>
#include <asm/system.h>

#define _L_FLAG(tty,f)	((tty)->termios.c_lflag & f)	// 取 termios 结构中的本地模式标志。
#define _I_FLAG(tty,f)	((tty)->termios.c_iflag & f)	// 取 termios 结构中的输入模式标志。
#define _O_FLAG(tty,f)	((tty)->termios.c_oflag & f)	// 取 termios 结构中的输出模式标志。

// 取 termios 结构中本地模式标志集中的一个标志位。
#define L_CANON(tty)	_L_FLAG((tty),ICANON)	// 取本地模式标志集中规范（熟）模式标志位。
#define L_ISIG(tty)	_L_FLAG((tty),ISIG)			// 取信号标志位。
#define L_ECHO(tty)	_L_FLAG((tty),ECHO)			// 取回显字符标志位。
#define L_ECHOE(tty)	_L_FLAG((tty),ECHOE)	// 规范模式时，取回显擦出标志位。
#define L_ECHOK(tty)	_L_FLAG((tty),ECHOK)	// 规范模式时，取 KILL 擦除当前行标志位。
#define L_ECHOCTL(tty)	_L_FLAG((tty),ECHOCTL)	// 取回显控制字符标志位。
#define L_ECHOKE(tty)	_L_FLAG((tty),ECHOKE)	// 规范模式时，取 KILL 擦除行并回显标志位。

// 取 termios 结构中输入模式标志中的一个标志位。
#define I_UCLC(tty)	_I_FLAG((tty),IUCLC)		// 取输入模式标志集中大写到小写转换标志位。
#define I_NLCR(tty)	_I_FLAG((tty),INLCR)		// 取换行符 NL 转回车符 CR 标志位。
#define I_CRNL(tty)	_I_FLAG((tty),ICRNL)		// 取回车符 CR 转换行符 NL 标志位。
#define I_NOCR(tty)	_I_FLAG((tty),IGNCR)		// 取忽略回车符 CR 标志位。

// 取 termios 结构中输出模式标志中的一个标志位。
#define O_POST(tty)	_O_FLAG((tty),OPOST)		// 取输出模式标志集中执行输出处理标志。
#define O_NLCR(tty)	_O_FLAG((tty),ONLCR)		// 取换行符 NL 转回车换行符 CR-NL 标志。
#define O_CRNL(tty)	_O_FLAG((tty),OCRNL)		// 取回车符 CR 转换行符 NL 标志。
#define O_NLRET(tty)	_O_FLAG((tty),ONLRET)	// 取换行符 NL 执行回车功能的标志。
#define O_LCUC(tty)	_O_FLAG((tty),OLCUC)		// 取小写转大写字符标志。

// tty 数据结构的 tty_table 数组。其中包含三个初始化项数据，分别对应控制台、串口终端 1 和
// 串口终端 2 的初始化数据。
struct tty_struct tty_table[] = {
	{
		{ICRNL,		/* change incoming CR to NL */			/* 将输入的 CR 转换为 NL */
		OPOST|ONLCR,	/* change outgoing NL to CRNL */	/* 将输出的 NL 转 CRNL */
		0,													// 控制模式标志初始化为 0。
		ISIG | ICANON | ECHO | ECHOCTL | ECHOKE,			// 本地模式标志。
		0,		/* console termio */						// 控制台 termio。
		INIT_C_CC},											// 控制字符数组。
		0,			/* initial pgrp */						// 所属初始进程组。
		0,			/* initial stopped */					// 初始停止标志。
		con_write,											// tty 写函数指针。
		{0,0,0,0,""},		/* console read-queue */		// tty 控制台读队列。
		{0,0,0,0,""},		/* console write-queue */		// tty 控制台写队列。
		{0,0,0,0,""}		/* console secondary queue */	// tty 控制台辅助(第二)队列。
	},{
		{0, /* no translation */					// 输入模式标志。0，无须转换。
		0,  /* no translation */					// 输出模式标志。0，无须转换。
		B2400 | CS8,								// 控制模式标志。波特率 2400bps，8 位数据位。
		0,											// 本地模式标志 0。
		0,											// 行规程 0。
		INIT_C_CC},									// 控制字符数组。
		0,											// 所属初始进程组。
		0,											// 初始停止标志。
		rs_write,									// 串口 1 tty 写函数指针。
		{0x3f8,0,0,0,""},		/* rs 1 */			// 串行终端 1 读缓冲队列。
		{0x3f8,0,0,0,""},							// 串行终端 1 写缓冲队列。
		{0,0,0,0,""}								// 串行终端 1 辅助缓冲队列。
	},{
		{0, /* no translation */
		0,  /* no translation */
		B2400 | CS8,
		0,
		0,
		INIT_C_CC},
		0,
		0,
		rs_write,
		{0x2f8,0,0,0,""},		/* rs 2 */
		{0x2f8,0,0,0,""},
		{0,0,0,0,""}
	}
};

/*
 * these are the tables used by the machine code handlers.
 * you can implement pseudo-tty's or something by changing
 * them. Currently not done.
 */
/*
 * 下面是汇编程序使用的缓冲队列地址表。通过修改你可以实现
 * 伪 tty 终端或其它终端类型。目前还没有这样做。
 */
// tty 缓冲队列地址表。rs_io.s 汇编程序使用，用于取得读写缓冲队列地址。
struct tty_queue * table_list[]={
	&tty_table[0].read_q, &tty_table[0].write_q,
	&tty_table[1].read_q, &tty_table[1].write_q,
	&tty_table[2].read_q, &tty_table[2].write_q
	};

// tty 终端初始化函数。
// 初始化串口终端和控制台终端。
void tty_init(void)
{
	rs_init();
	con_init();
}

// tty 键盘中断（^C）字符处理函数。
// 向 tty 结构中指明的（前台）进程组中所有的进程发送指定的信号 mask，通常该信号是 SIGINT。
void tty_intr(struct tty_struct * tty, int mask)
{
	int i;

	if (tty->pgrp <= 0)
		return;
	for (i=0;i<NR_TASKS;i++)
		if (task[i] && task[i]->pgrp==tty->pgrp)
			task[i]->signal |= mask;
}

// 如果队列缓冲区空则让进程进入可中断的睡眠状态。
// 参数：queue - 指定队列的指针。
// 进程在取队列缓冲区中字符时调用此函数。
static void sleep_if_empty(struct tty_queue * queue)
{
	cli();
	while (!current->signal && EMPTY(*queue))
		interruptible_sleep_on(&queue->proc_list);
	sti();
}

// 若队列缓冲区满则让进程进入可中断的睡眠状态。
// 参数：queue - 指定队列的指针。
// 进程在往队列缓冲区中写入时调用此函数。
static void sleep_if_full(struct tty_queue * queue)
{
	if (!FULL(*queue))
		return;
	cli();
	while (!current->signal && LEFT(*queue)<128)
		interruptible_sleep_on(&queue->proc_list);
	sti();
}

// 等待按键。
// 如果控制台的读队列缓冲区空则让进程进入可中断的睡眠状态。
void wait_for_keypress(void)
{
	sleep_if_empty(&tty_table[0].secondary);
}

// 复制成规范模式字符序列。
// 将指定 tty 终端队列缓冲区中的字符复制成规范(熟)模式字符并存放在辅助队列(规范模式队列)中。
// 参数：tty - 指定终端的 tty 结构。
void copy_to_cooked(struct tty_struct * tty)
{
	signed char c;

	while (!EMPTY(tty->read_q) && !FULL(tty->secondary)) {
		GETCH(tty->read_q,c);
		if (c==13)
			if (I_CRNL(tty))
				c=10;
			else if (I_NOCR(tty))
				continue;
			else ;
		else if (c==10 && I_NLCR(tty))
			c=13;
		if (I_UCLC(tty))
			c=tolower(c);
		if (L_CANON(tty)) {
			if (c==KILL_CHAR(tty)) {
				/* deal with killing the input line */
				while(!(EMPTY(tty->secondary) ||
				        (c=LAST(tty->secondary))==10 ||
				        c==EOF_CHAR(tty))) {
					if (L_ECHO(tty)) {
						if (c<32)
							PUTCH(127,tty->write_q);
						PUTCH(127,tty->write_q);
						tty->write(tty);
					}
					DEC(tty->secondary.head);
				}
				continue;
			}
			if (c==ERASE_CHAR(tty)) {
				if (EMPTY(tty->secondary) ||
				   (c=LAST(tty->secondary))==10 ||
				   c==EOF_CHAR(tty))
					continue;
				if (L_ECHO(tty)) {
					if (c<32)
						PUTCH(127,tty->write_q);
					PUTCH(127,tty->write_q);
					tty->write(tty);
				}
				DEC(tty->secondary.head);
				continue;
			}
			if (c==STOP_CHAR(tty)) {
				tty->stopped=1;
				continue;
			}
			if (c==START_CHAR(tty)) {
				tty->stopped=0;
				continue;
			}
		}
		if (L_ISIG(tty)) {
			if (c==INTR_CHAR(tty)) {
				tty_intr(tty,INTMASK);
				continue;
			}
			if (c==QUIT_CHAR(tty)) {
				tty_intr(tty,QUITMASK);
				continue;
			}
		}
		if (c==10 || c==EOF_CHAR(tty))
			tty->secondary.data++;
		if (L_ECHO(tty)) {
			if (c==10) {
				PUTCH(10,tty->write_q);
				PUTCH(13,tty->write_q);
			} else if (c<32) {
				if (L_ECHOCTL(tty)) {
					PUTCH('^',tty->write_q);
					PUTCH(c+64,tty->write_q);
				}
			} else
				PUTCH(c,tty->write_q);
			tty->write(tty);
		}
		PUTCH(c,tty->secondary);
	}
	wake_up(&tty->secondary.proc_list);
}

// tty 读函数，从终端辅助缓冲队列中读取指定数量的字符，放到用户指定的缓冲区中。
// 参数：channel - 子设备号；buf – 用户缓冲区指针；nr - 欲读字节数。
// 返回已读字节数。
int tty_read(unsigned channel, char * buf, int nr)
{
	struct tty_struct * tty;
	char c, * b=buf;
	int minimum,time,flag=0;
	long oldalarm;

	if (channel>2 || nr<0) return -1;
	tty = &tty_table[channel];
	oldalarm = current->alarm;
	time = 10L*tty->termios.c_cc[VTIME];
	minimum = tty->termios.c_cc[VMIN];
	if (time && !minimum) {
		minimum=1;
		if (flag=(!oldalarm || time+jiffies<oldalarm))
			current->alarm = time+jiffies;
	}
	if (minimum>nr)
		minimum=nr;
	while (nr>0) {
		if (flag && (current->signal & ALRMMASK)) {
			current->signal &= ~ALRMMASK;
			break;
		}
		if (current->signal)
			break;
		if (EMPTY(tty->secondary) || (L_CANON(tty) &&
		!tty->secondary.data && LEFT(tty->secondary)>20)) {
			sleep_if_empty(&tty->secondary);
			continue;
		}
		do {
			GETCH(tty->secondary,c);
			if (c==EOF_CHAR(tty) || c==10)
				tty->secondary.data--;
			if (c==EOF_CHAR(tty) && L_CANON(tty))
				return (b-buf);
			else {
				put_fs_byte(c,b++);
				if (!--nr)
					break;
			}
		} while (nr>0 && !EMPTY(tty->secondary));
		if (time && !L_CANON(tty))
			if (flag=(!oldalarm || time+jiffies<oldalarm))
				current->alarm = time+jiffies;
			else
				current->alarm = oldalarm;
		if (L_CANON(tty)) {
			if (b-buf)
				break;
		} else if (b-buf >= minimum)
			break;
	}
	current->alarm = oldalarm;
	if (current->signal && !(b-buf))
		return -EINTR;
	return (b-buf);
}

// tty 写函数。把用户缓冲区中的字符写入 tty 的写队列中。
// 参数：channel - 子设备号；buf - 缓冲区指针；nr - 写字节数。
// 返回已写字节数。
int tty_write(unsigned channel, char * buf, int nr)
{
	static cr_flag=0;
	struct tty_struct * tty;
	char c, *b=buf;

	if (channel>2 || nr<0) return -1;
	tty = channel + tty_table;
	while (nr>0) {
		sleep_if_full(&tty->write_q);
		if (current->signal)
			break;
		while (nr>0 && !FULL(tty->write_q)) {
			c=get_fs_byte(b);
			if (O_POST(tty)) {
				if (c=='\r' && O_CRNL(tty))
					c='\n';
				else if (c=='\n' && O_NLRET(tty))
					c='\r';
				if (c=='\n' && !cr_flag && O_NLCR(tty)) {
					cr_flag = 1;
					PUTCH(13,tty->write_q);
					continue;
				}
				if (O_LCUC(tty))
					c=toupper(c);
			}
			b++; nr--;
			cr_flag = 0;
			PUTCH(c,tty->write_q);
		}
		tty->write(tty);
		if (nr>0)
			schedule();
	}
	return (b-buf);
}

/*
 * Jeh, sometimes I really like the 386.
 * This routine is called from an interrupt,
 * and there should be absolutely no problem
 * with sleeping even in an interrupt (I hope).
 * Of course, if somebody proves me wrong, I'll
 * hate intel for all time :-). We'll have to
 * be careful and see to reinstating the interrupt
 * chips before calling this, though.
 *
 * I don't think we sleep here under normal circumstances
 * anyway, which is good, as the task sleeping might be
 * totally innocent.
 */
/*
 * 呵，有时我是真得很喜欢 386。该子程序是从一个中断处理程序中调用的，即使在
 * 中断处理程序中睡眠也应该绝对没有问题(我希望如此)。当然，如果有人证明我是
 * 错的，那么我将憎恨 intel 一辈子?。但是我们必须小心，在调用该子程序之前需
 * 要恢复中断。
 *
 * 我不认为在通常环境下会处在这里睡眠，这样很好，因为任务睡眠是完全任意的。
 */
// tty 中断处理调用函数 - 执行 tty 中断处理。
// 参数：tty - 指定的 tty 终端号（0，1 或 2）。
// 将指定 tty 终端队列缓冲区中的字符复制成规范(熟)模式字符并存放在辅助队列(规范模式队列)中。
// 在串口读字符中断(rs_io.s, 109)和键盘中断(kerboard.S, 69)中调用。
void do_tty_interrupt(int tty)
{
	copy_to_cooked(tty_table+tty);
}

// 字符设备初始化函数。空，为以后扩展做准备。
void chr_dev_init(void)
{
}
