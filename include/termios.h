/*
���ļ������ն� I/O �ӿڶ��塣���� termios ���ݽṹ��һЩ��ͨ���ն˽ӿ����õĺ���ԭ�͡�
��Щ����������ȡ�������ն˵����ԡ���·���ơ���ȡ�����ò������Լ���ȡ�������ն�ǰ�˽��̵��� id ��
��Ȼ���� linux ���ڵ�ͷ�ļ���������ȫ����Ŀǰ�� POSIX ��׼���������ʵ�����չ��

�ڸ��ļ��ж���������ն����ݽṹ termio �� termios �Ƿֱ��������� UNIX ϵ�У����¡���� 
termio���� AT&T ϵͳ V �ж���ģ��� termios �� POSIX ��׼ָ���ġ�
�����ṹ����һ����ֻ�� termio ʹ�ö��������Ͷ���ģʽ��־������ termios ʹ�ó���������ģʽ��־����
����Ŀǰ�����ֽṹ����ʹ�ã����Ϊ�˼����ԣ������ϵͳ��ͬʱ֧�����ǡ�
���⣬��ǰʹ�õ���һ���Ƶ� sgtty �ṹ��Ŀǰ�ѻ������á�
*/


#ifndef _TERMIOS_H
#define _TERMIOS_H

#define TTY_BUF_SIZE 1024

/* 0x54 is just a magic number to make these relatively uniqe ('T') */
/* 0x54 ֻ��һ��ħ����Ŀ����Ϊ��ʹ��Щ����Ψһ('T') */

// tty �豸�� ioctl ���������ioctl ����������ڵ�λ���С�
// �������� TC[*]�ĺ����� tty �������
// ȡ��Ӧ�ն� termios �ṹ�е���Ϣ(�μ� tcgetattr())
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

// ���ڴ�С(Window size)���Խṹ���ڴ��ڻ����п����ڻ�����Ļ��Ӧ�ó���
// ioctls �е� TIOCGWINSZ �� TIOCSWINSZ ��������ȡ��������Щ��Ϣ��
struct winsize {
	unsigned short ws_row;		// �����ַ�������
	unsigned short ws_col;		// �����ַ�������
	unsigned short ws_xpixel;	// ���ڿ�ȣ�����ֵ��
	unsigned short ws_ypixel;	// ���ڸ߶ȣ�����ֵ��
};

// AT&T ϵͳ V �� termio �ṹ��
#define NCC 8					// termio �ṹ�п����ַ�����ĳ��ȡ�
struct termio {
	unsigned short c_iflag;		/* input mode flags */		// ����ģʽ��־��
	unsigned short c_oflag;		/* output mode flags */		// ���ģʽ��־��
	unsigned short c_cflag;		/* control mode flags */	// ����ģʽ��־��
	unsigned short c_lflag;		/* local mode flags */		// ����ģʽ��־��
	unsigned char c_line;		/* line discipline */		// ��·��̣����ʣ���
	unsigned char c_cc[NCC];	/* control characters */	// �����ַ����顣
};

// POSIX �� termios �ṹ��
#define NCCS 17					// termios �ṹ�п����ַ�����ĳ��ȡ�
struct termios {
	unsigned long c_iflag;		/* input mode flags */
	unsigned long c_oflag;		/* output mode flags */
	unsigned long c_cflag;		/* control mode flags */
	unsigned long c_lflag;		/* local mode flags */
	unsigned char c_line;		/* line discipline */
	unsigned char c_cc[NCCS];	/* control characters */
};

/* c_cc characters */			/* c_cc �����е��ַ� */
#define VINTR 0					// c_cc[VINTR] = INTR (^C)��\003���ж��ַ���
#define VQUIT 1					// c_cc[VQUIT] = QUIT (^\)��\034���˳��ַ���
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

/* c_iflag bits */				/* c_iflag ����λ */
// termios �ṹ����ģʽ�ֶ� c_iflag ���ֱ�־�ķ��ų�����
#define IGNBRK 	0000001 // ����ʱ���� BREAK ������
#define BRKINT 	0000002 // �� BREAK ʱ���� SIGINT �źš�
#define IGNPAR 	0000004 // ������żУ�������ַ���
#define PARMRK 	0000010 // �����żУ���
#define INPCK 	0000020 // ����������żУ�顣
#define ISTRIP 	0000040 // �����ַ��� 8 λ��
#define INLCR 	0000100 // ����ʱ�����з� NL ӳ��ɻس��� CR��
#define IGNCR 	0000200 // ���Իس��� CR��
#define ICRNL 	0000400 // ������ʱ���س��� CR ӳ��ɻ��з� NL��
#define IUCLC 	0001000 // ������ʱ����д�ַ�ת����Сд�ַ���
#define IXON 	0002000 // ����ʼ/ֹͣ��XON/XOFF��������ơ�
#define IXANY 	0004000 // �����κ��ַ����������
#define IXOFF 	0010000 // ����ʼ/ֹͣ��XON/XOFF��������ơ�
#define IMAXBEL 0020000 // ���������ʱ���塣


/* c_oflag bits */			/* c_oflag ����λ */
// termios �ṹ�����ģʽ�ֶ� c_oflag ���ֱ�־�ķ��ų���
#define OPOST	0000001		// ִ���������
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

/* c_cflag bit meaning */		/* c_cflag ����λ�ĺ��� */
// termios �ṹ�п���ģʽ��־�ֶ� c_cflag ��־�ķ��ų�����8 ����������
#define CBAUD	0000017		// ��������λ�����롣�
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

/* c_lflag bits */			/* c_lflag ����λ */
// termios �ṹ�б���ģʽ��־�ֶ� c_lflag �ķ��ų�����
#define ISIG	0000001		// ���յ��ַ� INTR��QUIT��SUSP �� DSUSP��������Ӧ���źš�
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

/* modem lines */				/* modem ��·�źŷ��ų��� */
#define TIOCM_LE	0x001		// ��·����(Line Enable)��
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

/* tcflow() and TCXONC use these */	/* tcflow()�� TCXONC ʹ����Щ���ų��� */
#define	TCOOFF		0				// ���������
#define	TCOON		1
#define	TCIOFF		2
#define	TCION		3

/* tcflush() and TCFLSH use these */	/* tcflush()�� TCFLSH ʹ����Щ���ų��� */
#define	TCIFLUSH	0					// ����յ������ݵ�������
#define	TCOFLUSH	1
#define	TCIOFLUSH	2

/* tcsetattr uses these */				/* tcsetattr()ʹ����Щ���ų��� */
#define	TCSANOW		0					// �ı�����������
#define	TCSADRAIN	1
#define	TCSAFLUSH	2

typedef int speed_t;					// ��������ֵ���͡�

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
