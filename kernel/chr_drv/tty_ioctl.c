/*
 *  linux/kernel/chr_drv/tty_ioctl.c
 *
 *  (C) 1991  Linus Torvalds
 */

// 本文件用于字符设备的控制操作，实现了函数（系统调用） tty_ioctl() 。
// 程序通过使用该函数可以修改指定终端 termios 结构中的设置标志等信息。

#include <errno.h>
#include <termios.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/tty.h>

#include <asm/io.h>
#include <asm/segment.h>
#include <asm/system.h>

// 这是波特率因子数组（或称为除数数组）。波特率与波特率因子的对应关系参见列表后的说明。
static unsigned short quotient[] = {
	0, 2304, 1536, 1047, 857,
	768, 576, 384, 192, 96,
	64, 48, 24, 12, 6, 3
};

// 修改传输速率。
// 参数：tty - 终端对应的 tty 数据结构。
// 在除数锁存标志 DLAB(线路控制寄存器位 7)置位情况下，通过端口 0x3f8 和 0x3f9 向 UART 分别写入
// 波特率因子低字节和高字节。
static void change_speed(struct tty_struct * tty)
{
	unsigned short port,quot;

	if (!(port = tty->read_q.data))
		return;
	quot = quotient[tty->termios.c_cflag & CBAUD];
	cli();
	outb_p(0x80,port+3);		/* set DLAB */
	outb_p(quot & 0xff,port);	/* LS of divisor */
	outb_p(quot >> 8,port+1);	/* MS of divisor */
	outb(0x03,port+3);		/* reset DLAB */
	sti();
}

// 刷新 tty 缓冲队列。
// 参数：gueue - 指定的缓冲队列指针。
// 令缓冲队列的头指针等于尾指针，从而达到清空缓冲区(零字符)的目的。
static void flush(struct tty_queue * queue)
{
	cli();
	queue->head = queue->tail;
	sti();
}

// 等待字符发送出去。
static void wait_until_sent(struct tty_struct * tty)
{
	/* do nothing - not implemented */
}

// 发送 BREAK 控制符。
static void send_break(struct tty_struct * tty)
{
	/* do nothing - not implemented */
}

// 取终端 termios 结构信息。
// 参数：tty - 指定终端的 tty 结构指针；termios - 用户数据区 termios 结构缓冲区指针。
// 返回 0 。
static int get_termios(struct tty_struct * tty, struct termios * termios)
{
	int i;

	verify_area(termios, sizeof (*termios));
	for (i=0 ; i< (sizeof (*termios)) ; i++)
		put_fs_byte( ((char *)&tty->termios)[i] , i+(char *)termios );
	return 0;
}

// 设置终端 termios 结构信息。
// 参数：tty - 指定终端的 tty 结构指针；termios - 用户数据区 termios 结构指针。
// 返回 0 。
static int set_termios(struct tty_struct * tty, struct termios * termios)
{
	int i;

	for (i=0 ; i< (sizeof (*termios)) ; i++)
		((char *)&tty->termios)[i]=get_fs_byte(i+(char *)termios);
	change_speed(tty);
	return 0;
}

// 读取 termio 结构中的信息。
// 参数：tty - 指定终端的 tty 结构指针；termio - 用户数据区 termio 结构缓冲区指针。
// 返回 0。
static int get_termio(struct tty_struct * tty, struct termio * termio)
{
	int i;
	struct termio tmp_termio;

	verify_area(termio, sizeof (*termio));
	tmp_termio.c_iflag = tty->termios.c_iflag;
	tmp_termio.c_oflag = tty->termios.c_oflag;
	tmp_termio.c_cflag = tty->termios.c_cflag;
	tmp_termio.c_lflag = tty->termios.c_lflag;
	tmp_termio.c_line = tty->termios.c_line;
	for(i=0 ; i < NCC ; i++)
		tmp_termio.c_cc[i] = tty->termios.c_cc[i];
	for (i=0 ; i< (sizeof (*termio)) ; i++)
		put_fs_byte( ((char *)&tmp_termio)[i] , i+(char *)termio );
	return 0;
}

/*
 * This only works as the 386 is low-byt-first
 */ /* 下面的 termio 设置函数仅在 386 低字节在前的方式下可用。*/
// 设置终端 termio 结构信息。
// 参数：tty - 指定终端的 tty 结构指针；termio - 用户数据区 termio 结构指针。
// 将用户缓冲区 termio 的信息复制到终端的 termios 结构中。返回 0 。
static int set_termio(struct tty_struct * tty, struct termio * termio)
{
	int i;
	struct termio tmp_termio;

	for (i=0 ; i< (sizeof (*termio)) ; i++)
		((char *)&tmp_termio)[i]=get_fs_byte(i+(char *)termio);
	*(unsigned short *)&tty->termios.c_iflag = tmp_termio.c_iflag;
	*(unsigned short *)&tty->termios.c_oflag = tmp_termio.c_oflag;
	*(unsigned short *)&tty->termios.c_cflag = tmp_termio.c_cflag;
	*(unsigned short *)&tty->termios.c_lflag = tmp_termio.c_lflag;
	tty->termios.c_line = tmp_termio.c_line;
	for(i=0 ; i < NCC ; i++)
		tty->termios.c_cc[i] = tmp_termio.c_cc[i];
	change_speed(tty);
	return 0;
}

// tty 终端设备的 ioctl 函数。
// 参数：dev - 设备号；cmd - ioctl 命令；arg - 操作参数指针。
int tty_ioctl(int dev, int cmd, int arg)
{
	struct tty_struct * tty;
	if (MAJOR(dev) == 5) {
		dev=current->tty;
		if (dev<0)
			panic("tty_ioctl: dev<0");
	} else
		dev=MINOR(dev);
	tty = dev + tty_table;
	switch (cmd) {
		case TCGETS:
			return get_termios(tty,(struct termios *) arg);
		case TCSETSF:
			flush(&tty->read_q); /* fallthrough */
		case TCSETSW:
			wait_until_sent(tty); /* fallthrough */
		case TCSETS:
			return set_termios(tty,(struct termios *) arg);
		case TCGETA:
			return get_termio(tty,(struct termio *) arg);
		case TCSETAF:
			flush(&tty->read_q); /* fallthrough */
		case TCSETAW:
			wait_until_sent(tty); /* fallthrough */
		case TCSETA:
			return set_termio(tty,(struct termio *) arg);
		case TCSBRK:
			if (!arg) {
				wait_until_sent(tty);
				send_break(tty);
			}
			return 0;
		case TCXONC:
			return -EINVAL; /* not implemented */
		case TCFLSH:
			if (arg==0)
				flush(&tty->read_q);
			else if (arg==1)
				flush(&tty->write_q);
			else if (arg==2) {
				flush(&tty->read_q);
				flush(&tty->write_q);
			} else
				return -EINVAL;
			return 0;
		case TIOCEXCL:
			return -EINVAL; /* not implemented */
		case TIOCNXCL:
			return -EINVAL; /* not implemented */
		case TIOCSCTTY:
			return -EINVAL; /* set controlling term NI */
		case TIOCGPGRP:
			verify_area((void *) arg,4);
			put_fs_long(tty->pgrp,(unsigned long *) arg);
			return 0;
		case TIOCSPGRP:
			tty->pgrp=get_fs_long((unsigned long *) arg);
			return 0;
		case TIOCOUTQ:
			verify_area((void *) arg,4);
			put_fs_long(CHARS(tty->write_q),(unsigned long *) arg);
			return 0;
		case TIOCINQ:
			verify_area((void *) arg,4);
			put_fs_long(CHARS(tty->secondary),
				(unsigned long *) arg);
			return 0;
		case TIOCSTI:
			return -EINVAL; /* not implemented */
		case TIOCGWINSZ:
			return -EINVAL; /* not implemented */
		case TIOCSWINSZ:
			return -EINVAL; /* not implemented */
		case TIOCMGET:
			return -EINVAL; /* not implemented */
		case TIOCMBIS:
			return -EINVAL; /* not implemented */
		case TIOCMBIC:
			return -EINVAL; /* not implemented */
		case TIOCMSET:
			return -EINVAL; /* not implemented */
		case TIOCGSOFTCAR:
			return -EINVAL; /* not implemented */
		case TIOCSSOFTCAR:
			return -EINVAL; /* not implemented */
		default:
			return -EINVAL;
	}
}
