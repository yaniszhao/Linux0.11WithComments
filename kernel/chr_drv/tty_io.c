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
 * 'tty_io.c'�� tty һ�ַ���صĸо����ǿ���̨���Ǵ���ͨ�����ó���ͬ��
 * ʵ���˻��ԡ��淶(��)ģʽ�ȡ�
 *
 * Kill-line��лл John T Kahl��
 */
#include <ctype.h>
#include <errno.h>
#include <signal.h>

// ���������Ӧ�ź����ź�λͼ�еĶ�Ӧ����λ��
#define ALRMMASK (1<<(SIGALRM-1))		// ����(alarm)�ź�����λ��
#define KILLMASK (1<<(SIGKILL-1))		// ��ֹ(kill)�ź�����λ��
#define INTMASK (1<<(SIGINT-1))			// �����ж�(int)�ź�����λ��
#define QUITMASK (1<<(SIGQUIT-1))		// �����˳�(quit)�ź�����λ��
#define TSTPMASK (1<<(SIGTSTP-1))		// tty ������ֹͣ����(tty stop)�ź�����λ��

#include <linux/sched.h>
#include <linux/tty.h>
#include <asm/segment.h>
#include <asm/system.h>

#define _L_FLAG(tty,f)	((tty)->termios.c_lflag & f)	// ȡ termios �ṹ�еı���ģʽ��־��
#define _I_FLAG(tty,f)	((tty)->termios.c_iflag & f)	// ȡ termios �ṹ�е�����ģʽ��־��
#define _O_FLAG(tty,f)	((tty)->termios.c_oflag & f)	// ȡ termios �ṹ�е����ģʽ��־��

// ȡ termios �ṹ�б���ģʽ��־���е�һ����־λ��
#define L_CANON(tty)	_L_FLAG((tty),ICANON)	// ȡ����ģʽ��־���й淶���죩ģʽ��־λ��
#define L_ISIG(tty)	_L_FLAG((tty),ISIG)			// ȡ�źű�־λ��
#define L_ECHO(tty)	_L_FLAG((tty),ECHO)			// ȡ�����ַ���־λ��
#define L_ECHOE(tty)	_L_FLAG((tty),ECHOE)	// �淶ģʽʱ��ȡ���Բ�����־λ��
#define L_ECHOK(tty)	_L_FLAG((tty),ECHOK)	// �淶ģʽʱ��ȡ KILL ������ǰ�б�־λ��
#define L_ECHOCTL(tty)	_L_FLAG((tty),ECHOCTL)	// ȡ���Կ����ַ���־λ��
#define L_ECHOKE(tty)	_L_FLAG((tty),ECHOKE)	// �淶ģʽʱ��ȡ KILL �����в����Ա�־λ��

// ȡ termios �ṹ������ģʽ��־�е�һ����־λ��
#define I_UCLC(tty)	_I_FLAG((tty),IUCLC)		// ȡ����ģʽ��־���д�д��Сдת����־λ��
#define I_NLCR(tty)	_I_FLAG((tty),INLCR)		// ȡ���з� NL ת�س��� CR ��־λ��
#define I_CRNL(tty)	_I_FLAG((tty),ICRNL)		// ȡ�س��� CR ת���з� NL ��־λ��
#define I_NOCR(tty)	_I_FLAG((tty),IGNCR)		// ȡ���Իس��� CR ��־λ��

// ȡ termios �ṹ�����ģʽ��־�е�һ����־λ��
#define O_POST(tty)	_O_FLAG((tty),OPOST)		// ȡ���ģʽ��־����ִ����������־��
#define O_NLCR(tty)	_O_FLAG((tty),ONLCR)		// ȡ���з� NL ת�س����з� CR-NL ��־��
#define O_CRNL(tty)	_O_FLAG((tty),OCRNL)		// ȡ�س��� CR ת���з� NL ��־��
#define O_NLRET(tty)	_O_FLAG((tty),ONLRET)	// ȡ���з� NL ִ�лس����ܵı�־��
#define O_LCUC(tty)	_O_FLAG((tty),OLCUC)		// ȡСдת��д�ַ���־��

// tty ���ݽṹ�� tty_table ���顣���а���������ʼ�������ݣ��ֱ��Ӧ����̨�������ն� 1 ��
// �����ն� 2 �ĳ�ʼ�����ݡ�
struct tty_struct tty_table[] = {
	{
		{ICRNL,		/* change incoming CR to NL */			/* ������� CR ת��Ϊ NL */
		OPOST|ONLCR,	/* change outgoing NL to CRNL */	/* ������� NL ת CRNL */
		0,													// ����ģʽ��־��ʼ��Ϊ 0��
		ISIG | ICANON | ECHO | ECHOCTL | ECHOKE,			// ����ģʽ��־��
		0,		/* console termio */						// ����̨ termio��
		INIT_C_CC},											// �����ַ����顣
		0,			/* initial pgrp */						// ������ʼ�����顣
		0,			/* initial stopped */					// ��ʼֹͣ��־��
		con_write,											// tty д����ָ�롣
		{0,0,0,0,""},		/* console read-queue */		// tty ����̨�����С�
		{0,0,0,0,""},		/* console write-queue */		// tty ����̨д���С�
		{0,0,0,0,""}		/* console secondary queue */	// tty ����̨����(�ڶ�)���С�
	},{
		{0, /* no translation */					// ����ģʽ��־��0������ת����
		0,  /* no translation */					// ���ģʽ��־��0������ת����
		B2400 | CS8,								// ����ģʽ��־�������� 2400bps��8 λ����λ��
		0,											// ����ģʽ��־ 0��
		0,											// �й�� 0��
		INIT_C_CC},									// �����ַ����顣
		0,											// ������ʼ�����顣
		0,											// ��ʼֹͣ��־��
		rs_write,									// ���� 1 tty д����ָ�롣
		{0x3f8,0,0,0,""},		/* rs 1 */			// �����ն� 1 ��������С�
		{0x3f8,0,0,0,""},							// �����ն� 1 д������С�
		{0,0,0,0,""}								// �����ն� 1 ����������С�
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
 * �����ǻ�����ʹ�õĻ�����е�ַ��ͨ���޸������ʵ��
 * α tty �ն˻������ն����͡�Ŀǰ��û����������
 */
// tty ������е�ַ��rs_io.s ������ʹ�ã�����ȡ�ö�д������е�ַ��
struct tty_queue * table_list[]={
	&tty_table[0].read_q, &tty_table[0].write_q,
	&tty_table[1].read_q, &tty_table[1].write_q,
	&tty_table[2].read_q, &tty_table[2].write_q
	};

// tty �ն˳�ʼ��������
// ��ʼ�������ն˺Ϳ���̨�նˡ�
void tty_init(void)
{
	rs_init();
	con_init();
}

// tty �����жϣ�^C���ַ���������
// �� tty �ṹ��ָ���ģ�ǰ̨�������������еĽ��̷���ָ�����ź� mask��ͨ�����ź��� SIGINT��
void tty_intr(struct tty_struct * tty, int mask)
{
	int i;

	if (tty->pgrp <= 0)
		return;
	for (i=0;i<NR_TASKS;i++)
		if (task[i] && task[i]->pgrp==tty->pgrp)
			task[i]->signal |= mask;
}

// ������л����������ý��̽�����жϵ�˯��״̬��
// ������queue - ָ�����е�ָ�롣
// ������ȡ���л��������ַ�ʱ���ô˺�����
static void sleep_if_empty(struct tty_queue * queue)
{
	cli();
	while (!current->signal && EMPTY(*queue))
		interruptible_sleep_on(&queue->proc_list);
	sti();
}

// �����л����������ý��̽�����жϵ�˯��״̬��
// ������queue - ָ�����е�ָ�롣
// �����������л�������д��ʱ���ô˺�����
static void sleep_if_full(struct tty_queue * queue)
{
	if (!FULL(*queue))
		return;
	cli();
	while (!current->signal && LEFT(*queue)<128)
		interruptible_sleep_on(&queue->proc_list);
	sti();
}

// �ȴ�������
// �������̨�Ķ����л����������ý��̽�����жϵ�˯��״̬��
void wait_for_keypress(void)
{
	sleep_if_empty(&tty_table[0].secondary);
}

// ���Ƴɹ淶ģʽ�ַ����С�
// ��ָ�� tty �ն˶��л������е��ַ����Ƴɹ淶(��)ģʽ�ַ�������ڸ�������(�淶ģʽ����)�С�
// ������tty - ָ���ն˵� tty �ṹ��
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

// tty �����������ն˸�����������ж�ȡָ���������ַ����ŵ��û�ָ���Ļ������С�
// ������channel - ���豸�ţ�buf �C �û�������ָ�룻nr - �����ֽ�����
// �����Ѷ��ֽ�����
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

// tty д���������û��������е��ַ�д�� tty ��д�����С�
// ������channel - ���豸�ţ�buf - ������ָ�룻nr - д�ֽ�����
// ������д�ֽ�����
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
 * �ǣ���ʱ������ú�ϲ�� 386�����ӳ����Ǵ�һ���жϴ�������е��õģ���ʹ��
 * �жϴ��������˯��ҲӦ�þ���û������(��ϣ�����)����Ȼ���������֤������
 * ��ģ���ô�ҽ����� intel һ����?���������Ǳ���С�ģ��ڵ��ø��ӳ���֮ǰ��
 * Ҫ�ָ��жϡ�
 *
 * �Ҳ���Ϊ��ͨ�������»ᴦ������˯�ߣ������ܺã���Ϊ����˯������ȫ����ġ�
 */
// tty �жϴ�����ú��� - ִ�� tty �жϴ���
// ������tty - ָ���� tty �ն˺ţ�0��1 �� 2����
// ��ָ�� tty �ն˶��л������е��ַ����Ƴɹ淶(��)ģʽ�ַ�������ڸ�������(�淶ģʽ����)�С�
// �ڴ��ڶ��ַ��ж�(rs_io.s, 109)�ͼ����ж�(kerboard.S, 69)�е��á�
void do_tty_interrupt(int tty)
{
	copy_to_cooked(tty_table+tty);
}

// �ַ��豸��ʼ���������գ�Ϊ�Ժ���չ��׼����
void chr_dev_init(void)
{
}
