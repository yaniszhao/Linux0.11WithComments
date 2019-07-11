/*
 *  linux/init/main.c
 *
 *  (C) 1991  Linus Torvalds
 */

#define __LIBRARY__
#include <unistd.h>
#include <time.h>

/*
 * we need this inline - forking from kernel space will result
 * in NO COPY ON WRITE (!!!), until an execve is executed. This
 * is no problem, but for the stack. This is handled by not letting
 * main() use the stack at all after fork(). Thus, no function
 * calls - which means inline code for fork too, as otherwise we
 * would use the stack upon exit from 'fork()'.
 *
 * Actually only pause and fork are needed inline, so that there
 * won't be any messing with the stack from main(), but we define
 * some others too.
 */
 /*
* ������Ҫ������Щ��Ƕ��� - ���ں˿ռ䴴������(forking)������û��дʱ����(COPY ONWRITE)!!!
* ֱ��ִ��һ�� execve ���á���Զ�ջ���ܴ������⡣����ķ������� fork()����֮���� main()ʹ���κζ�ջ��
* ��˾Ͳ����к������� - ����ζ�� fork ҲҪʹ����Ƕ�Ĵ��룬���������ڴ� fork()�˳�ʱ��Ҫʹ�ö�ջ�ˡ�
* ʵ����ֻ�� pause �� fork ��Ҫʹ����Ƕ��ʽ���Ա�֤�� main()�в���Ū�Ҷ�ջ��
* ��������ͬʱ������������һЩ������
*/


//���ڴ����½��̵Ĺ�����ͨ����ȫ���Ƹ����̴���κ����ݶεķ�ʽʵ�ֵģ�
//������״�ʹ�� fork() �����½��� init ʱ��Ϊ��ȷ���½����û�̬��ջû�н��� 0 �Ķ�����Ϣ��
//Ҫ����� 0 �ڴ����׸��½��� ֮ǰ��Ҫʹ���û�̬��ջ��Ҳ��Ҫ������ 0 ��Ҫ���ú�����
//����� main.c �������ƶ������� 0 ִ�к� ���� 0 �еĴ��� fork()�����Ժ�����ʽ���е��á�
//����head.s����ʵ�Ѿ���mian�������������������Ѿ�����ջ�ˡ�
static inline _syscall0(int,fork)
//���⣬���� 0 �е� pause()Ҳ��Ҫʹ�ú�����Ƕ��ʽ�����塣
//������ȳ�������ִ���´������ӽ��� init����ô pause()���ú���������ʽ������ʲô���⡣
//�����ں˵��ȳ���ִ�и�����(���� 0)���ӽ��� init �Ĵ���������ģ�
//�ڴ����� init ���п������Ȼ���Ƚ��� 0 ִ�С���� pause()Ҳ������ú궨���� ʵ�֡�
static inline _syscall0(int,pause)
static inline _syscall1(int,setup,void *,BIOS)
static inline _syscall0(int,sync)

#include <linux/tty.h>
#include <linux/sched.h>
#include <linux/head.h>
#include <asm/system.h>
#include <asm/io.h>

#include <stddef.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include <linux/fs.h>

static char printbuf[1024]; // ��̬�ַ������飬�����ں���ʾ��Ϣ�Ļ��档

extern int vsprintf();								// �͸�ʽ�������һ�ַ����� 
extern void init(void);
extern void blk_dev_init(void);						//���豸��ʼ���ӳ���
extern void chr_dev_init(void);						// �ַ��豸��ʼ��
extern void hd_init(void);							// Ӳ�̳�ʼ������
extern void floppy_init(void);						// ������ʼ������
extern void mem_init(long start, long end);			// �ڴ�����ʼ��
extern long rd_init(long mem_start, int length);	// �����̳�ʼ��
extern long kernel_mktime(struct tm * tm);			// ����ϵͳ��������ʱ��(��)
extern long startup_time;							// �ں�����ʱ��(����ʱ��)

/*
 * This is set up by the setup-routine at boot-time
 */
 //�������������setup�еõ��ģ�����head���ڴ�λ��
#define EXT_MEM_K (*(unsigned short *)0x90002)		// 1M �Ժ����չ�ڴ��С(KB)
#define DRIVE_INFO (*(struct drive_info *)0x90080)	// Ӳ�̲������ַ
#define ORIG_ROOT_DEV (*(unsigned short *)0x901FC)	// ���ļ�ϵͳ�����豸�ţ�1FC=508

/*
 * Yeah, yeah, it's ugly, but I cannot find how to do this correctly
 * and this seems to work. I anybody has more info on the real-time
 * clock I'd be interested. Most of this was trial and error, and some
 * bios-listing reading. Urghh.
 */

// ��κ��ȡ CMOS ʵʱʱ����Ϣ��
// 0x70 ��д�˿ںţ�0x80|addr ��Ҫ��ȡ�� CMOS �ڴ��ַ�� 
// 0x71 �Ƕ��˿ںš�
#define CMOS_READ(addr) ({ \
outb_p(0x80|addr,0x70); \
inb_p(0x71); \
})

//�� BCD ��ת���ɶ�������ֵ��
#define BCD_TO_BIN(val) ((val)=((val)&15) + ((val)>>4)*10)

static void time_init(void)
{
	struct tm time;

	// CMOS �ķ����ٶȺ�����Ϊ�˼�Сʱ�����ڶ�ȡ������ѭ����������ֵ������ʱ CMOS ����ֵ
	// �����˱仯����ô�����¶�ȡ����ֵ�������ں˾��ܰ��� CMOS ��ʱ���������� 1 ��֮�ڡ�
	do {
		time.tm_sec = CMOS_READ(0);
		time.tm_min = CMOS_READ(2);
		time.tm_hour = CMOS_READ(4);
		time.tm_mday = CMOS_READ(7);
		time.tm_mon = CMOS_READ(8);// ��ǰ�·�(1��12)
		time.tm_year = CMOS_READ(9);
	} while (time.tm_sec != CMOS_READ(0));
	// ת���ɶ�������ֵ
	BCD_TO_BIN(time.tm_sec);
	BCD_TO_BIN(time.tm_min);
	BCD_TO_BIN(time.tm_hour);
	BCD_TO_BIN(time.tm_mday);
	BCD_TO_BIN(time.tm_mon);
	BCD_TO_BIN(time.tm_year);
	time.tm_mon--;// tm_mon ���·ݷ�Χ�� 0��11
	//���� kernel/mktime.c �к���������� 1970 �� 1 �� 1 �� 0 ʱ�𵽿������վ�����������
	//��Ϊ����ʱ�䡣
	startup_time = kernel_mktime(&time);
}

static long memory_end = 0;				// �������е������ڴ�����(�ֽ���)
static long buffer_memory_end = 0;		// ���ٻ�����ĩ�˵�ַ��
static long main_memory_start = 0;		// ���ڴ�(�����ڷ�ҳ)��ʼ��λ�á�

struct drive_info { char dummy[32]; } drive_info;// ���ڴ��Ӳ�̲�������Ϣ

void main(void)		/* This really IS void, no error here. */
{			/* The startup routine assumes (well, ...) this */
//����ȷʵ�� void����û���� startup ����(head.s)�о������������
/* ��ʱ�ж��Ա���ֹ�ţ������Ҫ�����ú�ͽ��俪��
 * Interrupts are still disabled. Do necessary setups, then
 * enable them
 */

	//���������ǰ�ԭ������bootsect�����ݷŵ��ں���Ķ�Ӧλ�ã�
	//��Ϊ����bootsect���ڴ�Ҫ����������
 	ROOT_DEV = ORIG_ROOT_DEV;
 	drive_info = DRIVE_INFO;
	memory_end = (1<<20) + (EXT_MEM_K<<10);// �ڴ��С=1Mb �ֽ�+��չ�ڴ�(k)*1024 �ֽ�
	memory_end &= 0xfffff000;// ���Բ��� 4Kb(1 ҳ)���ڴ���
	if (memory_end > 16*1024*1024)// ����ڴ泬�� 16Mb���� 16Mb ��
		memory_end = 16*1024*1024;
	if (memory_end > 12*1024*1024) // ����ڴ�>12Mb�������û�����ĩ��=4Mb
		buffer_memory_end = 4*1024*1024;
	else if (memory_end > 6*1024*1024)// ��������ڴ�>6Mb�������û�����ĩ��=2Mb
		buffer_memory_end = 2*1024*1024;
	else							// ���������û�����ĩ��=1Mb
		buffer_memory_end = 1*1024*1024;
	main_memory_start = buffer_memory_end;// ���ڴ���ʼλ��=������ĩ��;

// ����������ڴ������̣����ʼ�������̡���ʱ���ڴ潫����
#ifdef RAMDISK
	main_memory_start += rd_init(main_memory_start, RAMDISK*1024);//ȷ����ʼ/���ȣ�����
#endif
	mem_init(main_memory_start,memory_end);	// ���ڴ�ĳ�ʼ������Ҫ������map
	trap_init();							// ������(���ж�)��ʼ��
	blk_dev_init();							// ���豸��ʼ������Ҫ���������
	chr_dev_init();							// �ַ��豸��ʼ��
	tty_init();								// tty ��ʼ����rs��con
	time_init();							// ���ÿ�������ʱ��
	sched_init();							// ���ȳ����ʼ��(���������� 0 �� tr,ldtr)
	buffer_init(buffer_memory_end);			// ��������ʼ�������ڴ������
	hd_init();								// Ӳ�̳�ʼ����������к�Ӳ���жϵ�ַ
	floppy_init();							// ������ʼ����������к������жϵ�ַ
	sti(); 									// ���г�ʼ�������������ˣ������жϡ��ж���setup�͹���
	move_to_user_mode();					// ���ں�̬�л����û�̬
	if (!fork()) {		/* we count on this going ok */
		init();	// ���½����ӽ���(���� 1)��ִ��
	}
/*
 *   NOTE!!   For any other task 'pause()' would mean we have to get a
 * signal to awaken, but task0 is the sole exception (see 'schedule()')
 * as task 0 gets activated at every idle moment (when no other tasks
 * can run). For task0 'pause()' just means we go check if some other
 * task can run, and if not we return here.
 */
/* ע��!! �����κ�����������pause����ζ�����Ǳ���ȴ��յ�һ���źŲŻ᷵�ؾ�������̬��
* ������ 0(task0)��Ψһ���������(�μ�schedule())����Ϊ���� 0 ���κο���ʱ���ﶼ�ᱻ����
* (��û����������������ʱ)����˶������� 0 pause()����ζ�����Ƿ������鿴�Ƿ�����������������У�
* ���û�еĻ����Ǿͻص����һֱѭ��ִ��pause()�� */��
	for(;;) pause();
}

// ������ʽ����Ϣ���������׼����豸 stdout(1)��������ָ��Ļ����ʾ��
// ����'*fmt'ָ����������õĸ�ʽ���μ����ֱ�׼ C �����鼮��
// ���ӳ��������� vsprintf ���ʹ�õ�һ�����ӡ�
// �ó���ʹ�� vsprintf()����ʽ�����ַ������� printbuf ��������
// Ȼ���� write()���������������������׼�豸(1--stdout)��
// vsprintf()������ʵ�ּ� kernel/vsprintf.c��
static int printf(const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	write(1,printbuf,i=vsprintf(printbuf, fmt, args));
	va_end(args);
	return i;
}

//��ͨshell
static char * argv_rc[] = { "/bin/sh", NULL };	// ����ִ�г���ʱ�������ַ�������
static char * envp_rc[] = { "HOME=/", NULL };	// ����ִ�г���ʱ�Ļ����ַ�������

//��¼shell
// ��-���Ǵ��ݸ� shell ���� sh ��һ����־��
// ͨ��ʶ��ñ�־��sh�������Ϊ��¼ shell ִ�С�
// ��ִ�й������� shell ��ʾ����ִ�� sh ��̫һ����
static char * argv[] = { "-/bin/sh",NULL };
static char * envp[] = { "HOME=/usr/root", NULL };

// �� main()���Ѿ�������ϵͳ��ʼ���������ڴ��������Ӳ���豸����������
// init()�������������� 0 �� 1 �δ������ӽ���(���� 1)�С�
// �����ȶԵ�һ����Ҫִ�еĳ���(shell)�Ļ������г�ʼ����Ȼ����ظó���ִ��֮��
void init(void)
{
	int pid,i;

	// ���ڶ�ȡӲ�̲���������������Ϣ������������(�����ڵĻ�)�Ͱ�װ���ļ�ϵͳ�豸��
	setup((void *) &drive_info);//��Ӧ������ sys_setup()

	// �����Զ�д���ʷ�ʽ���豸��/dev/tty0��������Ӧ�ն˿���̨��
	// �������ǵ�һ�δ��ļ���������˲������ļ������(�ļ�������)�϶��� 0��
	// �þ���� UNIX �����ϵͳĬ�ϵĿ���̨��׼������ stdin��
	// ��������Զ���д�ķ�ʽ����Ϊ�˸��Ʋ�����׼���(д)��� stdout �ͱ�׼���������� stderr��
	(void) open("/dev/tty0",O_RDWR,0);
	(void) dup(0);// ���ƾ����������� 1 �� -- stdout ��׼����豸
	(void) dup(0);// ���ƾ����������� 2 �� -- stderr ��׼��������豸

	//�������û�̬�����õ���printf���ں���ֻ���������1��0/2�������û�̬�ģ�
	//����printf�Ķ���Ϊstaticֻ�����ļ���ʹ�ã������Ĵ�ӡ���붼�������ں�̬��printk
	//�����ӡ���������������ֽ�����ÿ�� 1024 �ֽڣ��Լ����ڴ��������ڴ��ֽ���
	printf("%d buffers = %d bytes buffer space\n\r",NR_BUFFERS,
		NR_BUFFERS*BLOCK_SIZE);
	printf("Free mem: %d bytes\n\r",memory_end-main_memory_start);

	// ���� fork()���ڴ���һ���ӽ���(���� 2)��
	// ���ڱ��������ӽ��̣�fork()������ 0 ֵ������ԭ����(������)�򷵻��ӽ��̵Ľ��̺� pid��
	// ���ӽ��̹ر��˾��0(stdin)����ֻ����ʽ��/etc/rc �ļ���
	// ��ʹ�� execve()���������������滻��/bin/sh ����(�� shell ����)��Ȼ��ִ��/bin/sh ����
	// ���������ͻ��������ֱ��� argv_rc �� envp_rc ���������
	// ���� execve()��μ� fs/exec.c��
	// ����_exit()�˳�ʱ�ĳ����� 1 �C ����δ���;2 -- �ļ���Ŀ¼�����ڡ�
	if (!(pid=fork())) {//�ӽ��̴��룬�Ҳ����߳����������
		close(0);
		//֮ǰ��tty0����Ϊ�˴���stdout��stderr������Ϊ�˴�rc�ļ�������
		if (open("/etc/rc",O_RDONLY,0))
			_exit(1);
		execve("/bin/sh",argv_rc,envp_rc);//ִ��shell����
		_exit(2);
	}

	//�����̴����￪ʼ
	if (pid>0)
		while (pid != wait(&i))//�ȴ�2�Ž����˳�
			/* nothing */;

	// ���ִ�е����˵���մ������ӽ��̵�ִ����ֹͣ����ֹ�ˡ�
	// ����ѭ���������ٴ���һ���ӽ��̣������������ʾ����ʼ�����򴴽��ӽ���ʧ�ܡ���Ϣ������ִ�С�
	// �������������ӽ��̽��ر�������ǰ�������ľ��(stdin, stdout, stderr)��
	// �´���һ���Ự�����ý�����ţ�Ȼ�����´�/dev/tty0 ��Ϊ stdin�������Ƴ� stdout �� stderr��
	// �ٴ�ִ��ϵͳ���ͳ���/bin/sh��
	// �����ִ����ѡ�õĲ����ͻ���������ѡ��һ�ס�
	// Ȼ�󸸽����ٴ����� wait()�ȴ���
	// ����ӽ�����ֹͣ��ִ�У����ڱ�׼�������ʾ������Ϣ���ӽ��� pid ֹͣ�����У�
	// �������� i����Ȼ�����������ȥ...���γɡ�����ѭ����
	while (1) {
		if ((pid=fork())<0) {
			printf("Fork failed in init\r\n");
			continue;
		}
		if (!pid) {	//�ӽ���
			close(0);close(1);close(2);			//ΪʲôҪ�������´���--�����Ự��Ҫ��
			setsid();							//�ѵ�ǰ������ɻỰ���졣
			(void) open("/dev/tty0",O_RDWR,0);
			(void) dup(0);
			(void) dup(0);
			_exit(execve("/bin/sh",argv,envp));	//��һ�ײ���ִ��
		}
		//�����̴��⿪ʼ
		while (1)//��while��if��������if��while��Ч����һ���ģ�ֻ��д����һ��
			if (pid == wait(&i))
				break;
		printf("\n\rchild %d died with code %04x\n\r",pid,i);
		sync();//ˢ�»���������ʱ��ʾ��Ϣ������Ҫ�Ȼ��������Ż��ӡ����
	}
	_exit(0);	/* NOTE! _exit, not exit() */
}
