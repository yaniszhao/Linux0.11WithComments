/*
¸ĂÎÄźţşŹÓĐÖŐśË I/O ˝ÓżÚś¨ŇĺĄŁ°üŔ¨ termios ĘýžÝ˝áššşÍŇťĐŠśÔÍ¨ÓĂÖŐśË˝ÓżÚÉčÖĂľÄşŻĘýÔ­ĐÍĄŁ
ŐâĐŠşŻĘýÓĂŔ´śÁČĄťňÉčÖĂÖŐśËľÄĘôĐÔĄ˘ĎßÂˇżŘÖĆĄ˘śÁČĄťňÉčÖĂ˛¨ĚŘÂĘŇÔź°śÁČĄťňÉčÖĂÖŐśËÇ°śË˝řłĚľÄ×é id ĄŁ
ËäČťŐâĘÇ linux ÔçĆÚľÄÍˇÎÄźţŁŹľŤŇŃÍęČŤˇűşĎÄżÇ°ľÄ POSIX ąę×źŁŹ˛˘×÷ÁËĘĘľąľÄŔŠŐšĄŁ

ÔÚ¸ĂÎÄźţÖĐś¨ŇĺľÄÁ˝¸öÖŐśËĘýžÝ˝ášš termio şÍ termios ĘÇˇÖąđĘôÓÚÁ˝Ŕŕ UNIX ĎľÁĐŁ¨ťňżĚÂĄŁŠŁŹ 
termioĘÇÔÚ AT&T ĎľÍł V ÖĐś¨ŇĺľÄŁŹśř termios ĘÇ POSIX ąę×źÖ¸ś¨ľÄĄŁ
Á˝¸ö˝áššťůąžŇťŃůŁŹÖťĘÇ termio ĘšÓĂśĚŐűĘýŔŕĐÍś¨ŇĺÄŁĘ˝ąęÖžźŻŁŹśř termios ĘšÓĂł¤ŐűĘýś¨ŇĺÄŁĘ˝ąęÖžźŻĄŁ
ÓÉÓÚÄżÇ°ŐâÁ˝ÖÖ˝áššśźÔÚĘšÓĂŁŹŇň´ËÎŞÁËźćČÝĐÔŁŹ´óśŕĘýĎľÍłśźÍŹĘąÖ§łÖËüĂÇĄŁ
ÁíÍâŁŹŇÔÇ°ĘšÓĂľÄĘÇŇťŔŕËĆľÄ sgtty ˝áššŁŹÄżÇ°ŇŃťůąž˛ťÓĂĄŁ
*/


#ifndef _TERMIOS_H
#define _TERMIOS_H

#define TTY_BUF_SIZE 1024

/* 0x54 is just a magic number to make these relatively uniqe ('T') */
/* 0x54 ÖťĘÇŇť¸öÄ§ĘýŁŹÄżľÄĘÇÎŞÁËĘšŐâĐŠłŁĘýÎ¨Ňť('T') */

// tty Éčą¸ľÄ ioctl ľ÷ÓĂĂüÁîźŻĄŁioctl ˝ŤĂüÁîąŕÂëÔÚľÍÎť×ÖÖĐĄŁ
// ĎÂĂćĂűłĆ TC[*]ľÄşŹŇĺĘÇ tty żŘÖĆĂüÁîĄŁ
// ČĄĎŕÓŚÖŐśË termios ˝áššÖĐľÄĐĹĎ˘(˛Îźű tcgetattr())
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

// ´°żÚ´óĐĄ(Window size)ĘôĐÔ˝áššĄŁÔÚ´°żÚťˇžłÖĐżÉÓĂÓÚťůÓÚĆÁÄťľÄÓŚÓĂłĚĐňĄŁ
// ioctls ÖĐľÄ TIOCGWINSZ şÍ TIOCSWINSZ żÉÓĂŔ´śÁČĄťňÉčÖĂŐâĐŠĐĹĎ˘ĄŁ
struct winsize {
	unsigned short ws_row;		// ´°żÚ×ÖˇűĐĐĘýĄŁ
	unsigned short ws_col;		// ´°żÚ×ÖˇűÁĐĘýĄŁ
	unsigned short ws_xpixel;	// ´°żÚżíśČŁŹĎóËŘÖľĄŁ
	unsigned short ws_ypixel;	// ´°żÚ¸ßśČŁŹĎóËŘÖľĄŁ
};

// AT&T ĎľÍł V ľÄ termio ˝áššĄŁ
#define NCC 8					// termio ˝áššÖĐżŘÖĆ×ÖˇűĘý×éľÄł¤śČĄŁ
struct termio {
	unsigned short c_iflag;		/* input mode flags */		// ĘäČëÄŁĘ˝ąęÖžĄŁ
	unsigned short c_oflag;		/* output mode flags */		// ĘäłöÄŁĘ˝ąęÖžĄŁ
	unsigned short c_cflag;		/* control mode flags */	// żŘÖĆÄŁĘ˝ąęÖžĄŁ
	unsigned short c_lflag;		/* local mode flags */		// ąžľŘÄŁĘ˝ąęÖžĄŁ
	unsigned char c_line;		/* line discipline */		// ĎßÂˇšćłĚŁ¨ËŮÂĘŁŠĄŁ
	unsigned char c_cc[NCC];	/* control characters */	// żŘÖĆ×ÖˇűĘý×éĄŁ
};

// POSIX ľÄ termios ˝áššĄŁ
#define NCCS 17					// termios ˝áššÖĐżŘÖĆ×ÖˇűĘý×éľÄł¤śČĄŁ
struct termios {
	unsigned long c_iflag;		/* input mode flags */
	unsigned long c_oflag;		/* output mode flags */
	unsigned long c_cflag;		/* control mode flags */
	unsigned long c_lflag;		/* local mode flags */
	unsigned char c_line;		/* line discipline */
	unsigned char c_cc[NCCS];	/* control characters */
};

/* c_cc characters */			/* c_cc Ęý×éÖĐľÄ×Öˇű */
#define VINTR 0					// c_cc[VINTR] = INTR (^C)ŁŹ\003ŁŹÖĐśĎ×ÖˇűĄŁ
#define VQUIT 1					// c_cc[VQUIT] = QUIT (^\)ŁŹ\034ŁŹÍËłö×ÖˇűĄŁ
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

/* c_iflag bits */				/* c_iflag ąČĚŘÎť */
// termios ˝áššĘäČëÄŁĘ˝×ÖśÎ c_iflag ¸÷ÖÖąęÖžľÄˇűşĹłŁĘýĄŁ
#define IGNBRK 	0000001 // ĘäČëĘąşöÂÔ BREAK ĚőźţĄŁ
#define BRKINT 	0000002 // ÔÚ BREAK Ęą˛úÉú SIGINT ĐĹşĹĄŁ
#define IGNPAR 	0000004 // şöÂÔĆćĹźĐŁŃéłö´íľÄ×ÖˇűĄŁ
#define PARMRK 	0000010 // ąęźÇĆćĹźĐŁŃé´íĄŁ
#define INPCK 	0000020 // ÔĘĐíĘäČëĆćĹźĐŁŃéĄŁ
#define ISTRIP 	0000040 // ĆÁąÎ×ÖˇűľÚ 8 ÎťĄŁ
#define INLCR 	0000100 // ĘäČëĘą˝ŤťťĐĐˇű NL ÓłÉäłÉťŘłľˇű CRĄŁ
#define IGNCR 	0000200 // şöÂÔťŘłľˇű CRĄŁ
#define ICRNL 	0000400 // ÔÚĘäČëĘą˝ŤťŘłľˇű CR ÓłÉäłÉťťĐĐˇű NLĄŁ
#define IUCLC 	0001000 // ÔÚĘäČëĘą˝Ť´óĐ´×Öˇű×ŞťťłÉĐĄĐ´×ÖˇűĄŁ
#define IXON 	0002000 // ÔĘĐíżŞĘź/ÍŁÖšŁ¨XON/XOFFŁŠĘäłöżŘÖĆĄŁ
#define IXANY 	0004000 // ÔĘĐíČÎşÎ×ÖˇűÖŘĆôĘäłöĄŁ
#define IXOFF 	0010000 // ÔĘĐíżŞĘź/ÍŁÖšŁ¨XON/XOFFŁŠĘäČëżŘÖĆĄŁ
#define IMAXBEL 0020000 // ĘäČëśÓÁĐÂúĘąĎěÁĺĄŁ


/* c_oflag bits */			/* c_oflag ąČĚŘÎť */
// termios ˝áššÖĐĘäłöÄŁĘ˝×ÖśÎ c_oflag ¸÷ÖÖąęÖžľÄˇűşĹłŁĘý
#define OPOST	0000001		// Ö´ĐĐĘäłö´ŚŔíĄŁ
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

/* c_cflag bit meaning */		/* c_cflag ąČĚŘÎťľÄşŹŇĺ */
// termios ˝áššÖĐżŘÖĆÄŁĘ˝ąęÖž×ÖśÎ c_cflag ąęÖžľÄˇűşĹłŁĘýŁ¨8 ˝řÖĆĘýŁŠĄŁ
#define CBAUD	0000017		// ´ŤĘäËŮÂĘÎťĆÁąÎÂëĄŁŁ
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

/* c_lflag bits */			/* c_lflag ąČĚŘÎť */
// termios ˝áššÖĐąžľŘÄŁĘ˝ąęÖž×ÖśÎ c_lflag ľÄˇűşĹłŁĘýĄŁ
#define ISIG	0000001		// ľąĘŐľ˝×Öˇű INTRĄ˘QUITĄ˘SUSP ťň DSUSPŁŹ˛úÉúĎŕÓŚľÄĐĹşĹĄŁ
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

/* modem lines */				/* modem ĎßÂˇĐĹşĹˇűşĹłŁĘý */
#define TIOCM_LE	0x001		// ĎßÂˇÔĘĐí(Line Enable)ĄŁ
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

/* tcflow() and TCXONC use these */	/* tcflow()şÍ TCXONC ĘšÓĂŐâĐŠˇűşĹłŁĘý */
#define	TCOOFF		0				// šŇĆđĘäłöĄŁ
#define	TCOON		1
#define	TCIOFF		2
#define	TCION		3

/* tcflush() and TCFLSH use these */	/* tcflush()şÍ TCFLSH ĘšÓĂŐâĐŠˇűşĹłŁĘý */
#define	TCIFLUSH	0					// Çĺ˝ÓĘŐľ˝ľÄĘýžÝľŤ˛ťśÁĄŁ
#define	TCOFLUSH	1
#define	TCIOFLUSH	2

/* tcsetattr uses these */				/* tcsetattr()ĘšÓĂŐâĐŠˇűşĹłŁĘý */
#define	TCSANOW		0					// ¸ÄąäÁ˘ź´ˇ˘ÉúĄŁ
#define	TCSADRAIN	1
#define	TCSAFLUSH	2

typedef int speed_t;					// ˛¨ĚŘÂĘĘýÖľŔŕĐÍĄŁ

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
