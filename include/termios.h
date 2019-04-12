/*
¸ÃÎÄ¼şº¬ÓĞÖÕ¶Ë I/O ½Ó¿Ú¶¨Òå¡£°üÀ¨ termios Êı¾İ½á¹¹ºÍÒ»Ğ©¶ÔÍ¨ÓÃÖÕ¶Ë½Ó¿ÚÉèÖÃµÄº¯ÊıÔ­ĞÍ¡£
ÕâĞ©º¯ÊıÓÃÀ´¶ÁÈ¡»òÉèÖÃÖÕ¶ËµÄÊôĞÔ¡¢ÏßÂ·¿ØÖÆ¡¢¶ÁÈ¡»òÉèÖÃ²¨ÌØÂÊÒÔ¼°¶ÁÈ¡»òÉèÖÃÖÕ¶ËÇ°¶Ë½ø³ÌµÄ×é id ¡£
ËäÈ»ÕâÊÇ linux ÔçÆÚµÄÍ·ÎÄ¼ş£¬µ«ÒÑÍêÈ«·ûºÏÄ¿Ç°µÄ POSIX ±ê×¼£¬²¢×÷ÁËÊÊµ±µÄÀ©Õ¹¡£

ÔÚ¸ÃÎÄ¼şÖĞ¶¨ÒåµÄÁ½¸öÖÕ¶ËÊı¾İ½á¹¹ termio ºÍ termios ÊÇ·Ö±ğÊôÓÚÁ½Àà UNIX ÏµÁĞ£¨»ò¿ÌÂ¡£©£¬ 
termioÊÇÔÚ AT&T ÏµÍ³ V ÖĞ¶¨ÒåµÄ£¬¶ø termios ÊÇ POSIX ±ê×¼Ö¸¶¨µÄ¡£
Á½¸ö½á¹¹»ù±¾Ò»Ñù£¬Ö»ÊÇ termio Ê¹ÓÃ¶ÌÕûÊıÀàĞÍ¶¨ÒåÄ£Ê½±êÖ¾¼¯£¬¶ø termios Ê¹ÓÃ³¤ÕûÊı¶¨ÒåÄ£Ê½±êÖ¾¼¯¡£
ÓÉÓÚÄ¿Ç°ÕâÁ½ÖÖ½á¹¹¶¼ÔÚÊ¹ÓÃ£¬Òò´ËÎªÁË¼æÈİĞÔ£¬´ó¶àÊıÏµÍ³¶¼Í¬Ê±Ö§³ÖËüÃÇ¡£
ÁíÍâ£¬ÒÔÇ°Ê¹ÓÃµÄÊÇÒ»ÀàËÆµÄ sgtty ½á¹¹£¬Ä¿Ç°ÒÑ»ù±¾²»ÓÃ¡£
*/


#ifndef _TERMIOS_H
#define _TERMIOS_H

#define TTY_BUF_SIZE 1024

/* 0x54 is just a magic number to make these relatively uniqe ('T') */
/* 0x54 Ö»ÊÇÒ»¸öÄ§Êı£¬Ä¿µÄÊÇÎªÁËÊ¹ÕâĞ©³£ÊıÎ¨Ò»('T') */

// tty Éè±¸µÄ ioctl µ÷ÓÃÃüÁî¼¯¡£ioctl ½«ÃüÁî±àÂëÔÚµÍÎ»×ÖÖĞ¡£
// ÏÂÃæÃû³Æ TC[*]µÄº¬ÒåÊÇ tty ¿ØÖÆÃüÁî¡£
// È¡ÏàÓ¦ÖÕ¶Ë termios ½á¹¹ÖĞµÄĞÅÏ¢(²Î¼û tcgetattr())
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

// ´°¿Ú´óĞ¡(Window size)ÊôĞÔ½á¹¹¡£ÔÚ´°¿Ú»·¾³ÖĞ¿ÉÓÃÓÚ»ùÓÚÆÁÄ»µÄÓ¦ÓÃ³ÌĞò¡£
// ioctls ÖĞµÄ TIOCGWINSZ ºÍ TIOCSWINSZ ¿ÉÓÃÀ´¶ÁÈ¡»òÉèÖÃÕâĞ©ĞÅÏ¢¡£
struct winsize {
	unsigned short ws_row;		// ´°¿Ú×Ö·ûĞĞÊı¡£
	unsigned short ws_col;		// ´°¿Ú×Ö·ûÁĞÊı¡£
	unsigned short ws_xpixel;	// ´°¿Ú¿í¶È£¬ÏóËØÖµ¡£
	unsigned short ws_ypixel;	// ´°¿Ú¸ß¶È£¬ÏóËØÖµ¡£
};

// AT&T ÏµÍ³ V µÄ termio ½á¹¹¡£
#define NCC 8					// termio ½á¹¹ÖĞ¿ØÖÆ×Ö·ûÊı×éµÄ³¤¶È¡£
struct termio {
	unsigned short c_iflag;		/* input mode flags */		// ÊäÈëÄ£Ê½±êÖ¾¡£
	unsigned short c_oflag;		/* output mode flags */		// Êä³öÄ£Ê½±êÖ¾¡£
	unsigned short c_cflag;		/* control mode flags */	// ¿ØÖÆÄ£Ê½±êÖ¾¡£
	unsigned short c_lflag;		/* local mode flags */		// ±¾µØÄ£Ê½±êÖ¾¡£
	unsigned char c_line;		/* line discipline */		// ÏßÂ·¹æ³Ì£¨ËÙÂÊ£©¡£
	unsigned char c_cc[NCC];	/* control characters */	// ¿ØÖÆ×Ö·ûÊı×é¡£
};

// POSIX µÄ termios ½á¹¹¡£
#define NCCS 17					// termios ½á¹¹ÖĞ¿ØÖÆ×Ö·ûÊı×éµÄ³¤¶È¡£
struct termios {
	unsigned long c_iflag;		/* input mode flags */
	unsigned long c_oflag;		/* output mode flags */
	unsigned long c_cflag;		/* control mode flags */
	unsigned long c_lflag;		/* local mode flags */
	unsigned char c_line;		/* line discipline */
	unsigned char c_cc[NCCS];	/* control characters */
};

/* c_cc characters */			/* c_cc Êı×éÖĞµÄ×Ö·û */
#define VINTR 0					// c_cc[VINTR] = INTR (^C)£¬\003£¬ÖĞ¶Ï×Ö·û¡£
#define VQUIT 1					// c_cc[VQUIT] = QUIT (^\)£¬\034£¬ÍË³ö×Ö·û¡£
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

/* c_iflag bits */				/* c_iflag ±ÈÌØÎ» */
// termios ½á¹¹ÊäÈëÄ£Ê½×Ö¶Î c_iflag ¸÷ÖÖ±êÖ¾µÄ·ûºÅ³£Êı¡£
#define IGNBRK 	0000001 // ÊäÈëÊ±ºöÂÔ BREAK Ìõ¼ş¡£
#define BRKINT 	0000002 // ÔÚ BREAK Ê±²úÉú SIGINT ĞÅºÅ¡£
#define IGNPAR 	0000004 // ºöÂÔÆæÅ¼Ğ£Ñé³ö´íµÄ×Ö·û¡£
#define PARMRK 	0000010 // ±ê¼ÇÆæÅ¼Ğ£Ñé´í¡£
#define INPCK 	0000020 // ÔÊĞíÊäÈëÆæÅ¼Ğ£Ñé¡£
#define ISTRIP 	0000040 // ÆÁ±Î×Ö·ûµÚ 8 Î»¡£
#define INLCR 	0000100 // ÊäÈëÊ±½«»»ĞĞ·û NL Ó³Éä³É»Ø³µ·û CR¡£
#define IGNCR 	0000200 // ºöÂÔ»Ø³µ·û CR¡£
#define ICRNL 	0000400 // ÔÚÊäÈëÊ±½«»Ø³µ·û CR Ó³Éä³É»»ĞĞ·û NL¡£
#define IUCLC 	0001000 // ÔÚÊäÈëÊ±½«´óĞ´×Ö·û×ª»»³ÉĞ¡Ğ´×Ö·û¡£
#define IXON 	0002000 // ÔÊĞí¿ªÊ¼/Í£Ö¹£¨XON/XOFF£©Êä³ö¿ØÖÆ¡£
#define IXANY 	0004000 // ÔÊĞíÈÎºÎ×Ö·ûÖØÆôÊä³ö¡£
#define IXOFF 	0010000 // ÔÊĞí¿ªÊ¼/Í£Ö¹£¨XON/XOFF£©ÊäÈë¿ØÖÆ¡£
#define IMAXBEL 0020000 // ÊäÈë¶ÓÁĞÂúÊ±ÏìÁå¡£


/* c_oflag bits */			/* c_oflag ±ÈÌØÎ» */
// termios ½á¹¹ÖĞÊä³öÄ£Ê½×Ö¶Î c_oflag ¸÷ÖÖ±êÖ¾µÄ·ûºÅ³£Êı
#define OPOST	0000001		// Ö´ĞĞÊä³ö´¦Àí¡£
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

/* c_cflag bit meaning */		/* c_cflag ±ÈÌØÎ»µÄº¬Òå */
// termios ½á¹¹ÖĞ¿ØÖÆÄ£Ê½±êÖ¾×Ö¶Î c_cflag ±êÖ¾µÄ·ûºÅ³£Êı£¨8 ½øÖÆÊı£©¡£
#define CBAUD	0000017		// ´«ÊäËÙÂÊÎ»ÆÁ±ÎÂë¡££
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

/* c_lflag bits */			/* c_lflag ±ÈÌØÎ» */
// termios ½á¹¹ÖĞ±¾µØÄ£Ê½±êÖ¾×Ö¶Î c_lflag µÄ·ûºÅ³£Êı¡£
#define ISIG	0000001		// µ±ÊÕµ½×Ö·û INTR¡¢QUIT¡¢SUSP »ò DSUSP£¬²úÉúÏàÓ¦µÄĞÅºÅ¡£
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

/* modem lines */				/* modem ÏßÂ·ĞÅºÅ·ûºÅ³£Êı */
#define TIOCM_LE	0x001		// ÏßÂ·ÔÊĞí(Line Enable)¡£
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

/* tcflow() and TCXONC use these */	/* tcflow()ºÍ TCXONC Ê¹ÓÃÕâĞ©·ûºÅ³£Êı */
#define	TCOOFF		0				// ¹ÒÆğÊä³ö¡£
#define	TCOON		1
#define	TCIOFF		2
#define	TCION		3

/* tcflush() and TCFLSH use these */	/* tcflush()ºÍ TCFLSH Ê¹ÓÃÕâĞ©·ûºÅ³£Êı */
#define	TCIFLUSH	0					// Çå½ÓÊÕµ½µÄÊı¾İµ«²»¶Á¡£
#define	TCOFLUSH	1
#define	TCIOFLUSH	2

/* tcsetattr uses these */				/* tcsetattr()Ê¹ÓÃÕâĞ©·ûºÅ³£Êı */
#define	TCSANOW		0					// ¸Ä±äÁ¢¼´·¢Éú¡£
#define	TCSADRAIN	1
#define	TCSAFLUSH	2

typedef int speed_t;					// ²¨ÌØÂÊÊıÖµÀàĞÍ¡£

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
