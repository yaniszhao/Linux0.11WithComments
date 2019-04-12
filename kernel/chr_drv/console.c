/*
 *  linux/kernel/console.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *	console.c
 *
 * This module implements the console io functions
 *	'void con_init(void)'
 *	'void con_write(struct tty_queue * queue)'
 * Hopefully this will be a rather complete VT102 implementation.
 *
 * Beeping thanks to John T Kohl.
 */

/*
 * ��ģ��ʵ�ֿ���̨�����������
 * 'void con_init(void)'
 * 'void con_write(struct tty_queue * queue)'
 * ϣ������һ���ǳ������� VT102 ʵ�֡�
 *
 * ��л John T Kohl ʵ���˷���ָʾ��
 */


/*
 *  NOTE!!! We sometimes disable and enable interrupts for a short while
 * (to put a word in video IO), but this will work even for keyboard
 * interrupts. We know interrupts aren't enabled when getting a keyboard
 * interrupt, as we use trap-gates. Hopefully all is well.
 */

/*
 * ע��!!! ������ʱ���ݵؽ�ֹ�������ж�(�ڽ�һ����(word)�ŵ���Ƶ IO)������ʹ
 * ���ڼ����ж���Ҳ�ǿ��Թ����ġ���Ϊ����ʹ�������ţ���������֪���ڻ��һ��
 * �����ж�ʱ�ж��ǲ�����ġ�ϣ��һ�о�������
 */


/*
 * Code to check for different video-cards mostly by Galen Hunt,
 * <g-hunt@ee.utah.edu>
 */
/*
 * ��ⲻͬ��ʾ���Ĵ��������� Galen Hunt ��д�ģ�
 * <g-hunt@ee.utah.edu>
 */


 // ���ļ����ں�����ĳ���֮һ�������ܱȽϵ�һ��
 // ���е������ӳ�����Ϊ��ʵ���ն���Ļд���� con_write() �Լ������ն˳�ʼ��������
 // ����������ʾ����ʾ���ݵĴ��롣

#include <linux/sched.h>
#include <linux/tty.h>
#include <asm/io.h>
#include <asm/system.h>

/*
 * These are set up by the setup-routine at boot-time:
 */	/* ��Щ�������ӳ��� setup ����������ϵͳʱ���õĲ�����*/
// �μ��� boot/setup.s ��ע�ͣ��� setup �����ȡ�������Ĳ�����
#define ORIG_X			(*(unsigned char *)0x90000)	// ����кš�
#define ORIG_Y			(*(unsigned char *)0x90001)	// ����кš�
#define ORIG_VIDEO_PAGE		(*(unsigned short *)0x90004)			// ��ʾҳ�档
#define ORIG_VIDEO_MODE		((*(unsigned short *)0x90006) & 0xff)	// ��ʾģʽ��
#define ORIG_VIDEO_COLS 	(((*(unsigned short *)0x90006) & 0xff00) >> 8)	// �ַ�������
#define ORIG_VIDEO_LINES	(25)											// ��ʾ������
#define ORIG_VIDEO_EGA_AX	(*(unsigned short *)0x90008)
#define ORIG_VIDEO_EGA_BX	(*(unsigned short *)0x9000a)	// ��ʾ�ڴ��С��ɫ��ģʽ��
#define ORIG_VIDEO_EGA_CX	(*(unsigned short *)0x9000c)	// ��ʾ�����Բ�����

// ������ʾ����ɫ/��ɫ��ʾģʽ���ͷ��ų�����
#define VIDEO_TYPE_MDA		0x10	/* Monochrome Text Display	*/		/* ��ɫ�ı� */
#define VIDEO_TYPE_CGA		0x11	/* CGA Display 			*/			/* CGA ��ʾ�� */
#define VIDEO_TYPE_EGAM		0x20	/* EGA/VGA in Monochrome Mode	*/	 /* EGA/VGA ��ɫ*/
#define VIDEO_TYPE_EGAC		0x21	/* EGA/VGA in Color Mode	*/		/* EGA/VGA ��ɫ*/

#define NPAR 16

extern void keyboard_interrupt(void);	// �����жϴ������(keyboard.S)��

static unsigned char	video_type;		/* Type of display being used	*/	/* ʹ�õ���ʾ���� */
static unsigned long	video_num_columns;	/* Number of text columns	*/	/* ��Ļ�ı����� */
static unsigned long	video_size_row;		/* Bytes per row		*/		/* ÿ��ʹ�õ��ֽ��� */
static unsigned long	video_num_lines;	/* Number of test lines		*/	/* ��Ļ�ı����� */
static unsigned char	video_page;		/* Initial video page		*/		/* ��ʼ��ʾҳ�� */
static unsigned long	video_mem_start;	/* Start of video RAM		*/	/* ��ʾ�ڴ���ʼ��ַ */
static unsigned long	video_mem_end;		/* End of video RAM (sort of)	*/	/* ��ʾ�ڴ����(ĩ��)��ַ */
static unsigned short	video_port_reg;		/* Video register select port	*/	/* ��ʾ���������Ĵ����˿� */
static unsigned short	video_port_val;		/* Video register value port	*/	/* ��ʾ�������ݼĴ����˿� */
static unsigned short	video_erase_char;	/* Char+Attrib to erase with	*/	/* �����ַ��������ַ�(0x0720) */

// ������Щ����������Ļ��������
static unsigned long	origin;		/* Used for EGA/VGA fast scroll	*/	 // scr_start
									/* ���� EGA/VGA ���ٹ��� */ // ������ʼ�ڴ��ַ��
static unsigned long	scr_end;	/* Used for EGA/VGA fast scroll	*/
									/* ���� EGA/VGA ���ٹ��� */ // ����ĩ���ڴ��ַ��
static unsigned long	pos;		// ��ǰ����Ӧ����ʾ�ڴ�λ�á�
static unsigned long	x,y;		// ��ǰ���λ�á�
static unsigned long	top,bottom;	// ����ʱ�����кţ������кš�
// state ���ڱ������� ESC ת������ʱ�ĵ�ǰ���衣npar,par[]���ڴ�� ESC ���е��м䴦�������
static unsigned long	state=0;		// ANSI ת���ַ����д���״̬��
static unsigned long	npar,par[NPAR];	// ANSI ת���ַ����в��������Ͳ�������
static unsigned long	ques=0;
static unsigned char	attr=0x07;		// �ַ�����(�ڵװ���)��

static void sysbeep(void);				// ϵͳ����������

/*
 * this is what the terminal answers to a ESC-Z or csi0c
 * query (= vt100 response).
 */ /* �������ն˻�Ӧ ESC-Z �� csi0c �����Ӧ��(=vt100 ��Ӧ)��*/
#define RESPONSE "\033[?1;2c"

/* NOTE! gotoxy thinks x==video_num_columns is ok */
/* ע�⣡gotoxy ������Ϊ x==video_num_columns��������ȷ�� */
// ���ٹ�굱ǰλ�á�
// ������new_x - ��������кţ�new_y - ��������кš�
// ���µ�ǰ���λ�ñ��� x,y�������� pos ָ��������ʾ�ڴ��еĶ�Ӧλ�á�
static inline void gotoxy(unsigned int new_x,unsigned int new_y)
{
	// �������Ĺ���кų�����ʾ�����������߹���кų�����ʾ��������������˳�
	if (new_x > video_num_columns || new_y >= video_num_lines)
		return;
	// ���µ�ǰ�����������¹��λ�ö�Ӧ������ʾ�ڴ���λ�ñ��� pos��
	x=new_x;
	y=new_y;
	pos=origin + y*video_size_row + (x<<1);//��xy��ά���껻��һλ���±�
}

// ���ù�����ʼ��ʾ�ڴ��ַ��
static inline void set_origin(void)
{
	cli();
	// ����ѡ����ʾ�������ݼĴ��� r12��Ȼ��д�������ʼ��ַ���ֽڡ������ƶ� 9 λ��
	// ��ʾ�����ƶ� 8 λ���ٳ��� 2(2 �ֽڴ�����Ļ�� 1 �ַ�)��
	// �������Ĭ����ʾ�ڴ�����ġ�
	outb_p(12, video_port_reg);
	outb_p(0xff&((origin-video_mem_start)>>9), video_port_val);
	// ��ѡ����ʾ�������ݼĴ��� r13��Ȼ��д�������ʼ��ַ���ֽڡ������ƶ� 1 λ��ʾ���� 2��
	outb_p(13, video_port_reg);
	outb_p(0xff&((origin-video_mem_start)>>1), video_port_val);
	sti();
}

// ���Ͼ�һ�У���Ļ���������ƶ�����
// ����Ļ���������ƶ�һ�С��μ������б��˵����
static void scrup(void)
{
	// �����ʾ������ EGA����ִ�����²�����
	if (video_type == VIDEO_TYPE_EGAC || video_type == VIDEO_TYPE_EGAM)
	{
		if (!top && bottom == video_num_lines) {
			origin += video_size_row;
			pos += video_size_row;
			scr_end += video_size_row;
			if (scr_end > video_mem_end) {
				__asm__("cld\n\t"
					"rep\n\t"
					"movsl\n\t"
					"movl _video_num_columns,%1\n\t"
					"rep\n\t"
					"stosw"
					::"a" (video_erase_char),
					"c" ((video_num_lines-1)*video_num_columns>>1),
					"D" (video_mem_start),
					"S" (origin)
					:"cx","di","si");
				scr_end -= origin-video_mem_start;
				pos -= origin-video_mem_start;
				origin = video_mem_start;
			} else {
				__asm__("cld\n\t"
					"rep\n\t"
					"stosw"
					::"a" (video_erase_char),
					"c" (video_num_columns),
					"D" (scr_end-video_size_row)
					:"cx","di");
			}
			set_origin();
		} else {
			__asm__("cld\n\t"
				"rep\n\t"
				"movsl\n\t"
				"movl _video_num_columns,%%ecx\n\t"
				"rep\n\t"
				"stosw"
				::"a" (video_erase_char),
				"c" ((bottom-top-1)*video_num_columns>>1),
				"D" (origin+video_size_row*top),
				"S" (origin+video_size_row*(top+1))
				:"cx","di","si");
		}
	}
	else		/* Not EGA/VGA */
	{
		__asm__("cld\n\t"
			"rep\n\t"
			"movsl\n\t"
			"movl _video_num_columns,%%ecx\n\t"
			"rep\n\t"
			"stosw"
			::"a" (video_erase_char),
			"c" ((bottom-top-1)*video_num_columns>>1),
			"D" (origin+video_size_row*top),
			"S" (origin+video_size_row*(top+1))
			:"cx","di","si");
	}
}

// ���¾�һ�У���Ļ���������ƶ���������Ļ���������ƶ�һ�У���Ļ��ʾ�����������ƶ� 1 �У�
// �ڱ��ƶ���ʼ�е��Ϸ�����һ���С� �μ������б��˵����
// �������� scrup()���ƣ�ֻ��Ϊ�����ƶ���ʾ�ڴ�����ʱ���������ݸ��Ǵ��������
// �������Է�������еģ�Ҳ������Ļ������ 2 �е����һ���ַ���ʼ���ơ�
static void scrdown(void)
{
	if (video_type == VIDEO_TYPE_EGAC || video_type == VIDEO_TYPE_EGAM)
	{
		__asm__("std\n\t"
			"rep\n\t"
			"movsl\n\t"
			"addl $2,%%edi\n\t"	/* %edi has been decremented by 4 */
			"movl _video_num_columns,%%ecx\n\t"
			"rep\n\t"
			"stosw"
			::"a" (video_erase_char),
			"c" ((bottom-top-1)*video_num_columns>>1),
			"D" (origin+video_size_row*bottom-4),
			"S" (origin+video_size_row*(bottom-1)-4)
			:"ax","cx","di","si");
	}
	else		/* Not EGA/VGA */
	{
		__asm__("std\n\t"
			"rep\n\t"
			"movsl\n\t"
			"addl $2,%%edi\n\t"	/* %edi has been decremented by 4 */
			"movl _video_num_columns,%%ecx\n\t"
			"rep\n\t"
			"stosw"
			::"a" (video_erase_char),
			"c" ((bottom-top-1)*video_num_columns>>1),
			"D" (origin+video_size_row*bottom-4),
			"S" (origin+video_size_row*(bottom-1)-4)
			:"ax","cx","di","si");
	}
}

// ���λ������һ��(lf - line feed ����)��
static void lf(void)
{
	if (y+1<bottom) {
		y++;
		pos += video_size_row;
		return;
	}
	scrup();
}

// �������һ��(ri - reverse line feed ������)��
static void ri(void)
{
	if (y>top) {
		y--;
		pos -= video_size_row;
		return;
	}
	scrdown();
}

// ���ص��� 1 ��(0 ��)���(cr - carriage return �س�)��
static void cr(void)
{
	pos -= x<<1;
	x=0;
}

// �������ǰһ�ַ�(�ÿո����)(del - delete ɾ��)��
static void del(void)
{
	if (x) {
		pos -= 2;
		x--;
		*(unsigned short *)pos = video_erase_char;
	}
}

// ɾ����Ļ������λ����صĲ��֣�����ĻΪ��λ��
// csi - ��������������(Control Sequence Introducer)��
// ANSI ת�����У�'ESC [sJ'(s = 0 ɾ����굽��Ļ�׶ˣ�1 ɾ����Ļ��ʼ����괦��2 ����ɾ��)��
// ������par - ��Ӧ���� s��
static void csi_J(int par)
{
	long count __asm__("cx");
	long start __asm__("di");

	switch (par) {
		case 0:	/* erase from cursor to end of display */
			count = (scr_end-pos)>>1;
			start = pos;
			break;
		case 1:	/* erase from start to cursor */
			count = (pos-origin)>>1;
			start = origin;
			break;
		case 2: /* erase whole display */
			count = video_num_columns * video_num_lines;
			start = origin;
			break;
		default:
			return;
	}
	__asm__("cld\n\t"
		"rep\n\t"
		"stosw\n\t"
		::"c" (count),
		"D" (start),"a" (video_erase_char)
		:"cx","di");
}

// ɾ����������λ����صĲ��֣���һ��Ϊ��λ��
// ANSI ת���ַ����У�'ESC [sK'(s = 0 ɾ������β��1 �ӿ�ʼɾ����2 ���ж�ɾ��)��
static void csi_K(int par)
{
	long count __asm__("cx");
	long start __asm__("di");

	switch (par) {
		case 0:	/* erase from cursor to end of line */
			if (x>=video_num_columns)
				return;
			count = video_num_columns-x;
			start = pos;
			break;
		case 1:	/* erase from start of line to cursor */
			start = pos - (x<<1);
			count = (x<video_num_columns)?x:video_num_columns;
			break;
		case 2: /* erase whole line */
			start = pos - (x<<1);
			count = video_num_columns;
			break;
		default:
			return;
	}
	__asm__("cld\n\t"
		"rep\n\t"
		"stosw\n\t"
		::"c" (count),
		"D" (start),"a" (video_erase_char)
		:"cx","di");
}

// ������(����)���������������ַ���ʾ��ʽ������Ӵ֡����»��ߡ���˸�����Եȣ���
// ANSI ת���ַ����У�'ESC [nm'��n = 0 ������ʾ��1 �Ӵ֣�4 ���»��ߣ�7 ���ԣ�27 ������ʾ��
void csi_m(void)
{
	int i;

	for (i=0;i<=npar;i++)
		switch (par[i]) {
			case 0:attr=0x07;break;
			case 1:attr=0x0f;break;
			case 4:attr=0x0f;break;
			case 7:attr=0x70;break;
			case 27:attr=0x07;break;
		}
}

// ����������ʾ��ꡣ
// ������ʾ�ڴ����Ӧλ�� pos��������ʾ������������ʾλ�á�
static inline void set_cursor(void)
{
	cli();
	outb_p(14, video_port_reg);
	outb_p(0xff&((pos-video_mem_start)>>9), video_port_val);
	outb_p(15, video_port_reg);
	outb_p(0xff&((pos-video_mem_start)>>1), video_port_val);
	sti();
}

// ���Ͷ��ն� VT100 ����Ӧ���С�
// ����Ӧ���з������������С�
static void respond(struct tty_struct * tty)
{
	char * p = RESPONSE;

	cli();
	while (*p) {
		PUTCH(*p,tty->read_q);
		p++;
	}
	sti();
	copy_to_cooked(tty);
}

// �ڹ�괦����һ�ո��ַ���
static void insert_char(void)
{
	int i=x;
	unsigned short tmp, old = video_erase_char;
	unsigned short * p = (unsigned short *) pos;

	while (i++<video_num_columns) {
		tmp=*p;
		*p=old;
		old=tmp;
		p++;
	}
}

// �ڹ�괦����һ�У����꽫�����µĿ����ϣ���
// ����Ļ�ӹ�������е���Ļ�����¾�һ�С�
static void insert_line(void)
{
	int oldtop,oldbottom;

	oldtop=top;
	oldbottom=bottom;
	top=y;
	bottom = video_num_lines;
	scrdown();
	top=oldtop;
	bottom=oldbottom;
}

// ɾ����괦��һ���ַ���
static void delete_char(void)
{
	int i;
	unsigned short * p = (unsigned short *) pos;

	if (x>=video_num_columns)
		return;
	i = x;
	while (++i < video_num_columns) {
		*p = *(p+1);
		p++;
	}
	*p = video_erase_char;
}

// ɾ����������С�
// �ӹ�������п�ʼ��Ļ�����Ͼ�һ�С�
static void delete_line(void)
{
	int oldtop,oldbottom;

	oldtop=top;
	oldbottom=bottom;
	top=y;
	bottom = video_num_lines;
	scrup();
	top=oldtop;
	bottom=oldbottom;
}

// �ڹ�괦���� nr ���ַ���
// ANSI ת���ַ����У�'ESC [n@ '��
// ���� nr = ���� n��
static void csi_at(unsigned int nr)
{
	if (nr > video_num_columns)
		nr = video_num_columns;
	else if (!nr)
		nr = 1;
	while (nr--)
		insert_char();
}

// �ڹ��λ�ô����� nr �С�
// ANSI ת���ַ�����'ESC [nL'��
static void csi_L(unsigned int nr)
{
	if (nr > video_num_lines)
		nr = video_num_lines;
	else if (!nr)
		nr = 1;
	while (nr--)
		insert_line();
}

// ɾ����괦�� nr ���ַ���
// ANSI ת�����У�'ESC [nP'��
static void csi_P(unsigned int nr)
{
	if (nr > video_num_columns)
		nr = video_num_columns;
	else if (!nr)
		nr = 1;
	while (nr--)
		delete_char();
}

// ɾ����괦�� nr �С�
// ANSI ת�����У�'ESC [nM'��
static void csi_M(unsigned int nr)
{
	if (nr > video_num_lines)
		nr = video_num_lines;
	else if (!nr)
		nr=1;
	while (nr--)
		delete_line();
}

static int saved_x=0;	// ����Ĺ���кš�
static int saved_y=0;	// ����Ĺ���кš�

// ���浱ǰ���λ�á�
static void save_cur(void)
{
	saved_x=x;
	saved_y=y;
}

// �ָ�����Ĺ��λ�á�
static void restore_cur(void)
{
	gotoxy(saved_x, saved_y);
}

// ����̨д������
// ���ն˶�Ӧ�� tty д���������ȡ�ַ�������ʾ����Ļ�ϡ�
void con_write(struct tty_struct * tty)
{
	int nr;
	char c;

	nr = CHARS(tty->write_q);
	while (nr--) {
		GETCH(tty->write_q,c);
		switch(state) {
			case 0:
				if (c>31 && c<127) {
					if (x>=video_num_columns) {
						x -= video_num_columns;
						pos -= video_size_row;
						lf();
					}
					__asm__("movb _attr,%%ah\n\t"
						"movw %%ax,%1\n\t"
						::"a" (c),"m" (*(short *)pos)
						:"ax");
					pos += 2;
					x++;
				} else if (c==27)
					state=1;
				else if (c==10 || c==11 || c==12)
					lf();
				else if (c==13)
					cr();
				else if (c==ERASE_CHAR(tty))
					del();
				else if (c==8) {
					if (x) {
						x--;
						pos -= 2;
					}
				} else if (c==9) {
					c=8-(x&7);
					x += c;
					pos += c<<1;
					if (x>video_num_columns) {
						x -= video_num_columns;
						pos -= video_size_row;
						lf();
					}
					c=9;
				} else if (c==7)
					sysbeep();
				break;
			case 1:
				state=0;
				if (c=='[')
					state=2;
				else if (c=='E')
					gotoxy(0,y+1);
				else if (c=='M')
					ri();
				else if (c=='D')
					lf();
				else if (c=='Z')
					respond(tty);
				else if (x=='7')
					save_cur();
				else if (x=='8')
					restore_cur();
				break;
			case 2:
				for(npar=0;npar<NPAR;npar++)
					par[npar]=0;
				npar=0;
				state=3;
				if (ques=(c=='?'))
					break;
			case 3:
				if (c==';' && npar<NPAR-1) {
					npar++;
					break;
				} else if (c>='0' && c<='9') {
					par[npar]=10*par[npar]+c-'0';
					break;
				} else state=4;
			case 4:
				state=0;
				switch(c) {
					case 'G': case '`':
						if (par[0]) par[0]--;
						gotoxy(par[0],y);
						break;
					case 'A':
						if (!par[0]) par[0]++;
						gotoxy(x,y-par[0]);
						break;
					case 'B': case 'e':
						if (!par[0]) par[0]++;
						gotoxy(x,y+par[0]);
						break;
					case 'C': case 'a':
						if (!par[0]) par[0]++;
						gotoxy(x+par[0],y);
						break;
					case 'D':
						if (!par[0]) par[0]++;
						gotoxy(x-par[0],y);
						break;
					case 'E':
						if (!par[0]) par[0]++;
						gotoxy(0,y+par[0]);
						break;
					case 'F':
						if (!par[0]) par[0]++;
						gotoxy(0,y-par[0]);
						break;
					case 'd':
						if (par[0]) par[0]--;
						gotoxy(x,par[0]);
						break;
					case 'H': case 'f':
						if (par[0]) par[0]--;
						if (par[1]) par[1]--;
						gotoxy(par[1],par[0]);
						break;
					case 'J':
						csi_J(par[0]);
						break;
					case 'K':
						csi_K(par[0]);
						break;
					case 'L':
						csi_L(par[0]);
						break;
					case 'M':
						csi_M(par[0]);
						break;
					case 'P':
						csi_P(par[0]);
						break;
					case '@':
						csi_at(par[0]);
						break;
					case 'm':
						csi_m();
						break;
					case 'r':
						if (par[0]) par[0]--;
						if (!par[1]) par[1] = video_num_lines;
						if (par[0] < par[1] &&
						    par[1] <= video_num_lines) {
							top=par[0];
							bottom=par[1];
						}
						break;
					case 's':
						save_cur();
						break;
					case 'u':
						restore_cur();
						break;
				}
		}
	}
	set_cursor();
}

/*
 *  void con_init(void);
 *
 * This routine initalizes console interrupts, and does nothing
 * else. If you want the screen to clear, call tty_write with
 * the appropriate escape-sequece.
 *
 * Reads the information preserved by setup.s to determine the current display
 * type and sets everything accordingly.
 */
/*
 * void con_init(void);
 * ����ӳ����ʼ������̨�жϣ�����ʲô�������������������Ļ�ɾ��Ļ�����ʹ��
 * �ʵ���ת���ַ����е��� tty_write()������
 *
 * ��ȡ setup.s ���򱣴����Ϣ������ȷ����ǰ��ʾ�����ͣ���������������ز�����
 */
void con_init(void)
{
	register unsigned char a;
	char *display_desc = "????";
	char *display_ptr;

	video_num_columns = ORIG_VIDEO_COLS;
	video_size_row = video_num_columns * 2;
	video_num_lines = ORIG_VIDEO_LINES;
	video_page = ORIG_VIDEO_PAGE;
	video_erase_char = 0x0720;
	
	if (ORIG_VIDEO_MODE == 7)			/* Is this a monochrome display? */
	{
		video_mem_start = 0xb0000;
		video_port_reg = 0x3b4;
		video_port_val = 0x3b5;
		if ((ORIG_VIDEO_EGA_BX & 0xff) != 0x10)
		{
			video_type = VIDEO_TYPE_EGAM;
			video_mem_end = 0xb8000;
			display_desc = "EGAm";
		}
		else
		{
			video_type = VIDEO_TYPE_MDA;
			video_mem_end	= 0xb2000;
			display_desc = "*MDA";
		}
	}
	else								/* If not, it is color. */
	{
		video_mem_start = 0xb8000;
		video_port_reg	= 0x3d4;
		video_port_val	= 0x3d5;
		if ((ORIG_VIDEO_EGA_BX & 0xff) != 0x10)
		{
			video_type = VIDEO_TYPE_EGAC;
			video_mem_end = 0xbc000;
			display_desc = "EGAc";
		}
		else
		{
			video_type = VIDEO_TYPE_CGA;
			video_mem_end = 0xba000;
			display_desc = "*CGA";
		}
	}

	/* Let the user known what kind of display driver we are using */
	
	display_ptr = ((char *)video_mem_start) + video_size_row - 8;
	while (*display_desc)
	{
		*display_ptr++ = *display_desc++;
		display_ptr++;
	}
	
	/* Initialize the variables used for scrolling (mostly EGA/VGA)	*/
	
	origin	= video_mem_start;
	scr_end	= video_mem_start + video_num_lines * video_size_row;
	top	= 0;
	bottom	= video_num_lines;

	gotoxy(ORIG_X,ORIG_Y);
	set_trap_gate(0x21,&keyboard_interrupt);
	outb_p(inb_p(0x21)&0xfd,0x21);
	a=inb_p(0x61);
	outb_p(a|0x80,0x61);
	outb(a,0x61);
}
/* from bsd-net-2: */

// ֹͣ������
// ��λ 8255A PB �˿ڵ�λ 1 ��λ 0��
void sysbeepstop(void)
{
	/* disable counter 2 */
	outb(inb_p(0x61)&0xFC, 0x61);
}

int beepcount = 0;

// ��ͨ������
// 8255A оƬ PB �˿ڵ�λ 1 �����������Ŀ����źţ�λ 0 ���� 8253 ��ʱ�� 2 �����źţ��ö�ʱ����
// ���������������������Ϊ������������Ƶ�ʡ����Ҫʹ��������������Ҫ���������ȿ��� PB �˿�
// λ 1 ��λ 0����λ����Ȼ�����ö�ʱ������һ���Ķ�ʱƵ�ʼ��ɡ�
static void sysbeep(void)
{
	/* enable counter 2 */
	outb_p(inb_p(0x61)|3, 0x61);
	/* set command for counter 2, 2 byte write */
	outb_p(0xB6, 0x43);
	/* send 0x637 for 750 HZ */
	outb_p(0x37, 0x42);
	outb(0x06, 0x42);
	/* 1/8 second */
	beepcount = HZ/8;	
}
