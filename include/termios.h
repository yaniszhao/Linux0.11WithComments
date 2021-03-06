/*
该文件含有终端 I/O 接口定义。包括 termios 数据结构和一些对通用终端接口设置的函数原型。
这些函数用来读取或设置终端的属性、线路控制、读取或设置波特率以及读取或设置终端前端进程的组 id 。
虽然这是 linux 早期的头文件，但已完全符合目前的 POSIX 标准，并作了适当的扩展。

在该文件中定义的两个终端数据结构 termio 和 termios 是分别属于两类 UNIX 系列（或刻隆）， 
termio是在 AT&T 系统 V 中定义的，而 termios 是 POSIX 标准指定的。
两个结构基本一样，只是 termio 使用短整数类型定义模式标志集，而 termios 使用长整数定义模式标志集。
由于目前这两种结构都在使用，因此为了兼容性，大多数系统都同时支持它们。
另外，以前使用的是一类似的 sgtty 结构，目前已基本不用。
*/


#ifndef _TERMIOS_H
#define _TERMIOS_H

#define TTY_BUF_SIZE 1024

/* 0x54 is just a magic number to make these relatively uniqe ('T') */
/* 0x54 只是一个魔数，目的是为了使这些常数唯一('T') */

// tty 设备的 ioctl 调用命令集。ioctl 将命令编码在低位字中。
// 下面名称 TC[*]的含义是 tty 控制命令。
// 取相应终端 termios 结构中的信息(参见 tcgetattr())
#define TCGETS		0x5401
#define TCSETS		0x5402
#define TCSETSW		0x5403
#define TCSETSF		0x5404
#define TCGETA		0x5405
#define TCSETA		0x5406
#define TCSETAW		0x5407
#define TCSETAF		0x5408
#define TCSBRK		0x5409
#define TCXONC		0x540A
#define TCFLSH		0x540B
#define TIOCEXCL	0x540C
#define TIOCNXCL	0x540D
#define TIOCSCTTY	0x540E
#define TIOCGPGRP	0x540F
#define TIOCSPGRP	0x5410
#define TIOCOUTQ	0x5411
#define TIOCSTI		0x5412
#define TIOCGWINSZ	0x5413
#define TIOCSWINSZ	0x5414
#define TIOCMGET	0x5415
#define TIOCMBIS	0x5416
#define TIOCMBIC	0x5417
#define TIOCMSET	0x5418
#define TIOCGSOFTCAR	0x5419
#define TIOCSSOFTCAR	0x541A
#define TIOCINQ		0x541B

// 窗口大小(Window size)属性结构。在窗口环境中可用于基于屏幕的应用程序。
// ioctls 中的 TIOCGWINSZ 和 TIOCSWINSZ 可用来读取或设置这些信息。
struct winsize {
	unsigned short ws_row;		// 窗口字符行数。
	unsigned short ws_col;		// 窗口字符列数。
	unsigned short ws_xpixel;	// 窗口宽度，象素值。
	unsigned short ws_ypixel;	// 窗口高度，象素值。
};

// AT&T 系统 V 的 termio 结构。
#define NCC 8					// termio 结构中控制字符数组的长度。
struct termio {
	unsigned short c_iflag;		/* input mode flags */		// 输入模式标志。
	unsigned short c_oflag;		/* output mode flags */		// 输出模式标志。
	unsigned short c_cflag;		/* control mode flags */	// 控制模式标志。
	unsigned short c_lflag;		/* local mode flags */		// 本地模式标志。
	unsigned char c_line;		/* line discipline */		// 线路规程（速率）。
	unsigned char c_cc[NCC];	/* control characters */	// 控制字符数组。
};

// POSIX 的 termios 结构。
#define NCCS 17					// termios 结构中控制字符数组的长度。
struct termios {
	unsigned long c_iflag;		/* input mode flags */
	unsigned long c_oflag;		/* output mode flags */
	unsigned long c_cflag;		/* control mode flags */
	unsigned long c_lflag;		/* local mode flags */
	unsigned char c_line;		/* line discipline */
	unsigned char c_cc[NCCS];	/* control characters */
};

/* c_cc characters */			/* c_cc 数组中的字符 */
#define VINTR 0					// c_cc[VINTR] = INTR (^C)，\003，中断字符。
#define VQUIT 1					// c_cc[VQUIT] = QUIT (^\)，\034，退出字符。
#define VERASE 2
#define VKILL 3
#define VEOF 4
#define VTIME 5
#define VMIN 6
#define VSWTC 7
#define VSTART 8
#define VSTOP 9
#define VSUSP 10
#define VEOL 11
#define VREPRINT 12
#define VDISCARD 13
#define VWERASE 14
#define VLNEXT 15
#define VEOL2 16

/* c_iflag bits */				/* c_iflag 比特位 */
// termios 结构输入模式字段 c_iflag 各种标志的符号常数。
#define IGNBRK 	0000001 // 输入时忽略 BREAK 条件。
#define BRKINT 	0000002 // 在 BREAK 时产生 SIGINT 信号。
#define IGNPAR 	0000004 // 忽略奇偶校验出错的字符。
#define PARMRK 	0000010 // 标记奇偶校验错。
#define INPCK 	0000020 // 允许输入奇偶校验。
#define ISTRIP 	0000040 // 屏蔽字符第 8 位。
#define INLCR 	0000100 // 输入时将换行符 NL 映射成回车符 CR。
#define IGNCR 	0000200 // 忽略回车符 CR。
#define ICRNL 	0000400 // 在输入时将回车符 CR 映射成换行符 NL。
#define IUCLC 	0001000 // 在输入时将大写字符转换成小写字符。
#define IXON 	0002000 // 允许开始/停止（XON/XOFF）输出控制。
#define IXANY 	0004000 // 允许任何字符重启输出。
#define IXOFF 	0010000 // 允许开始/停止（XON/XOFF）输入控制。
#define IMAXBEL 0020000 // 输入队列满时响铃。


/* c_oflag bits */			/* c_oflag 比特位 */
// termios 结构中输出模式字段 c_oflag 各种标志的符号常数
#define OPOST	0000001		// 执行输出处理。
#define OLCUC	0000002
#define ONLCR	0000004
#define OCRNL	0000010
#define ONOCR	0000020
#define ONLRET	0000040
#define OFILL	0000100
#define OFDEL	0000200
#define NLDLY	0000400
#define   NL0	0000000
#define   NL1	0000400
#define CRDLY	0003000
#define   CR0	0000000
#define   CR1	0001000
#define   CR2	0002000
#define   CR3	0003000
#define TABDLY	0014000
#define   TAB0	0000000
#define   TAB1	0004000
#define   TAB2	0010000
#define   TAB3	0014000
#define   XTABS	0014000
#define BSDLY	0020000
#define   BS0	0000000
#define   BS1	0020000
#define VTDLY	0040000
#define   VT0	0000000
#define   VT1	0040000
#define FFDLY	0040000
#define   FF0	0000000
#define   FF1	0040000

/* c_cflag bit meaning */		/* c_cflag 比特位的含义 */
// termios 结构中控制模式标志字段 c_cflag 标志的符号常数（8 进制数）。
#define CBAUD	0000017		// 传输速率位屏蔽码。�
#define  B0	0000000		/* hang up */
#define  B50	0000001
#define  B75	0000002
#define  B110	0000003
#define  B134	0000004
#define  B150	0000005
#define  B200	0000006
#define  B300	0000007
#define  B600	0000010
#define  B1200	0000011
#define  B1800	0000012
#define  B2400	0000013
#define  B4800	0000014
#define  B9600	0000015
#define  B19200	0000016
#define  B38400	0000017
#define EXTA B19200
#define EXTB B38400
#define CSIZE	0000060
#define   CS5	0000000
#define   CS6	0000020
#define   CS7	0000040
#define   CS8	0000060
#define CSTOPB	0000100
#define CREAD	0000200
#define CPARENB	0000400
#define CPARODD	0001000
#define HUPCL	0002000
#define CLOCAL	0004000
#define CIBAUD	03600000		/* input baud rate (not used) */
#define CRTSCTS	020000000000		/* flow control */

#define PARENB CPARENB
#define PARODD CPARODD

/* c_lflag bits */			/* c_lflag 比特位 */
// termios 结构中本地模式标志字段 c_lflag 的符号常数。
#define ISIG	0000001		// 当收到字符 INTR、QUIT、SUSP 或 DSUSP，产生相应的信号。
#define ICANON	0000002
#define XCASE	0000004
#define ECHO	0000010
#define ECHOE	0000020
#define ECHOK	0000040
#define ECHONL	0000100
#define NOFLSH	0000200
#define TOSTOP	0000400
#define ECHOCTL	0001000
#define ECHOPRT	0002000
#define ECHOKE	0004000
#define FLUSHO	0010000
#define PENDIN	0040000
#define IEXTEN	0100000

/* modem lines */				/* modem 线路信号符号常数 */
#define TIOCM_LE	0x001		// 线路允许(Line Enable)。
#define TIOCM_DTR	0x002
#define TIOCM_RTS	0x004
#define TIOCM_ST	0x008
#define TIOCM_SR	0x010
#define TIOCM_CTS	0x020
#define TIOCM_CAR	0x040
#define TIOCM_RNG	0x080
#define TIOCM_DSR	0x100
#define TIOCM_CD	TIOCM_CAR
#define TIOCM_RI	TIOCM_RNG

/* tcflow() and TCXONC use these */	/* tcflow()和 TCXONC 使用这些符号常数 */
#define	TCOOFF		0				// 挂起输出。
#define	TCOON		1
#define	TCIOFF		2
#define	TCION		3

/* tcflush() and TCFLSH use these */	/* tcflush()和 TCFLSH 使用这些符号常数 */
#define	TCIFLUSH	0					// 清接收到的数据但不读。
#define	TCOFLUSH	1
#define	TCIOFLUSH	2

/* tcsetattr uses these */				/* tcsetattr()使用这些符号常数 */
#define	TCSANOW		0					// 改变立即发生。
#define	TCSADRAIN	1
#define	TCSAFLUSH	2

typedef int speed_t;					// 波特率数值类型。

extern speed_t cfgetispeed(struct termios *termios_p);
extern speed_t cfgetospeed(struct termios *termios_p);
extern int cfsetispeed(struct termios *termios_p, speed_t speed);
extern int cfsetospeed(struct termios *termios_p, speed_t speed);
extern int tcdrain(int fildes);
extern int tcflow(int fildes, int action);
extern int tcflush(int fildes, int queue_selector);
extern int tcgetattr(int fildes, struct termios *termios_p);
extern int tcsendbreak(int fildes, int duration);
extern int tcsetattr(int fildes, int optional_actions,
	struct termios *termios_p);

#endif
