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
* 我们需要下面这些内嵌语句 - 从内核空间创建进程(forking)将导致没有写时复制(COPY ONWRITE)!!!
* 直到执行一个 execve 调用。这对堆栈可能带来问题。处理的方法是在 fork()调用之后不让 main()使用任何堆栈。
* 因此就不能有函数调用 - 这意味着 fork 也要使用内嵌的代码，否则我们在从 fork()退出时就要使用堆栈了。
* 实际上只有 pause 和 fork 需要使用内嵌方式，以保证从 main()中不会弄乱堆栈，
* 但是我们同时还定义了其它一些函数。
*/


//由于创建新进程的过程是通过完全复制父进程代码段和数据段的方式实现的，
//因此在首次使用 fork() 创建新进程 init 时，为了确保新进程用户态堆栈没有进程 0 的多余信息，
//要求进程 0 在创建首个新进程 之前不要使用用户态堆栈，也即要求任务 0 不要调用函数。
//因此在 main.c 主程序移动到任务 0 执行后， 任务 0 中的代码 fork()不能以函数形式进行调用。
//但是head.s中其实已经给mian函数传了三个参数，已经用了栈了。
static inline _syscall0(int,fork)
//另外，任务 0 中的 pause()也需要使用函数内嵌形式来定义。
//如果调度程序首先执行新创建的子进程 init，那么 pause()采用函数调用形式不会有什么问题。
//但是内核调度程序执行父进程(进程 0)和子进程 init 的次序是随机的，
//在创建了 init 后有可能首先会调度进程 0 执行。因此 pause()也必须采用宏定义来 实现。
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

static char printbuf[1024]; // 静态字符串数组，用作内核显示信息的缓存。

extern int vsprintf();								// 送格式化输出到一字符串中 
extern void init(void);
extern void blk_dev_init(void);						//块设备初始化子程序
extern void chr_dev_init(void);						// 字符设备初始化
extern void hd_init(void);							// 硬盘初始化程序
extern void floppy_init(void);						// 软驱初始化程序
extern void mem_init(long start, long end);			// 内存管理初始化
extern long rd_init(long mem_start, int length);	// 虚拟盘初始化
extern long kernel_mktime(struct tm * tm);			// 计算系统开机启动时间(秒)
extern long startup_time;							// 内核启动时间(开机时间)

/*
 * This is set up by the setup-routine at boot-time
 */
 //下面的内容是在setup中得到的，放在head的内存位置
#define EXT_MEM_K (*(unsigned short *)0x90002)		// 1M 以后的扩展内存大小(KB)
#define DRIVE_INFO (*(struct drive_info *)0x90080)	// 硬盘参数表基址
#define ORIG_ROOT_DEV (*(unsigned short *)0x901FC)	// 根文件系统所在设备号，1FC=508

/*
 * Yeah, yeah, it's ugly, but I cannot find how to do this correctly
 * and this seems to work. I anybody has more info on the real-time
 * clock I'd be interested. Most of this was trial and error, and some
 * bios-listing reading. Urghh.
 */

// 这段宏读取 CMOS 实时时钟信息。
// 0x70 是写端口号，0x80|addr 是要读取的 CMOS 内存地址。 
// 0x71 是读端口号。
#define CMOS_READ(addr) ({ \
outb_p(0x80|addr,0x70); \
inb_p(0x71); \
})

//将 BCD 码转换成二进制数值。
#define BCD_TO_BIN(val) ((val)=((val)&15) + ((val)>>4)*10)

static void time_init(void)
{
	struct tm time;

	// CMOS 的访问速度很慢。为了减小时间误差，在读取了下面循环中所有数值后，若此时 CMOS 中秒值
	// 发生了变化，那么就重新读取所有值。这样内核就能把与 CMOS 的时间误差控制在 1 秒之内。
	do {
		time.tm_sec = CMOS_READ(0);
		time.tm_min = CMOS_READ(2);
		time.tm_hour = CMOS_READ(4);
		time.tm_mday = CMOS_READ(7);
		time.tm_mon = CMOS_READ(8);// 当前月份(1―12)
		time.tm_year = CMOS_READ(9);
	} while (time.tm_sec != CMOS_READ(0));
	// 转换成二进制数值
	BCD_TO_BIN(time.tm_sec);
	BCD_TO_BIN(time.tm_min);
	BCD_TO_BIN(time.tm_hour);
	BCD_TO_BIN(time.tm_mday);
	BCD_TO_BIN(time.tm_mon);
	BCD_TO_BIN(time.tm_year);
	time.tm_mon--;// tm_mon 中月份范围是 0―11
	//调用 kernel/mktime.c 中函数，计算从 1970 年 1 月 1 日 0 时起到开机当日经过的秒数，
	//作为开机时间。
	startup_time = kernel_mktime(&time);
}

static long memory_end = 0;				// 机器具有的物理内存容量(字节数)
static long buffer_memory_end = 0;		// 高速缓冲区末端地址。
static long main_memory_start = 0;		// 主内存(将用于分页)开始的位置。

struct drive_info { char dummy[32]; } drive_info;// 用于存放硬盘参数表信息

void main(void)		/* This really IS void, no error here. */
{			/* The startup routine assumes (well, ...) this */
//这里确实是 void，并没错。在 startup 程序(head.s)中就是这样假设的
/* 此时中断仍被禁止着，做完必要的设置后就将其开启
 * Interrupts are still disabled. Do necessary setups, then
 * enable them
 */

	//下面三个是把原来放在bootsect的数据放到内核里的对应位置，
	//因为后面bootsect的内存要被用做他用
 	ROOT_DEV = ORIG_ROOT_DEV;
 	drive_info = DRIVE_INFO;
	memory_end = (1<<20) + (EXT_MEM_K<<10);// 内存大小=1Mb 字节+扩展内存(k)*1024 字节
	memory_end &= 0xfffff000;// 忽略不到 4Kb(1 页)的内存数
	if (memory_end > 16*1024*1024)// 如果内存超过 16Mb，则按 16Mb 计
		memory_end = 16*1024*1024;
	if (memory_end > 12*1024*1024) // 如果内存>12Mb，则设置缓冲区末端=4Mb
		buffer_memory_end = 4*1024*1024;
	else if (memory_end > 6*1024*1024)// 否则如果内存>6Mb，则设置缓冲区末端=2Mb
		buffer_memory_end = 2*1024*1024;
	else							// 否则则设置缓冲区末端=1Mb
		buffer_memory_end = 1*1024*1024;
	main_memory_start = buffer_memory_end;// 主内存起始位置=缓冲区末端;

// 如果定义了内存虚拟盘，则初始化虚拟盘。此时主内存将减少
#ifdef RAMDISK
	main_memory_start += rd_init(main_memory_start, RAMDISK*1024);//确认起始/长度，清零
#endif
	mem_init(main_memory_start,memory_end);	// 主内存的初始化，主要是设置map
	trap_init();							// 陷阱门(软中断)初始化
	blk_dev_init();							// 块设备初始化，主要是请求队列
	chr_dev_init();							// 字符设备初始化
	tty_init();								// tty 初始化，rs和con
	time_init();							// 设置开机启动时间
	sched_init();							// 调度程序初始化(加载了任务 0 的 tr,ldtr)
	buffer_init(buffer_memory_end);			// 缓冲管理初始化，建内存链表等
	hd_init();								// 硬盘初始化，请求队列和硬盘中断地址
	floppy_init();							// 软驱初始化，请求队列和软盘中断地址
	sti(); 									// 所有初始化工作都做完了，开启中断。中断在setup就关了
	move_to_user_mode();					// 从内核态切换到用户态
	if (!fork()) {		/* we count on this going ok */
		init();	// 在新建的子进程(任务 1)中执行
	}
/*
 *   NOTE!!   For any other task 'pause()' would mean we have to get a
 * signal to awaken, but task0 is the sole exception (see 'schedule()')
 * as task 0 gets activated at every idle moment (when no other tasks
 * can run). For task0 'pause()' just means we go check if some other
 * task can run, and if not we return here.
 */
/* 注意!! 对于任何其它的任务，pause将意味着我们必须等待收到一个信号才会返回就绪运行态，
* 但任务 0(task0)是唯一的例外情况(参见schedule())，因为任务 0 在任何空闲时间里都会被激活
* (当没有其它任务在运行时)，因此对于任务 0 pause()仅意味着我们返回来查看是否有其它任务可以运行，
* 如果没有的话我们就回到这里，一直循环执行pause()。 */。
	for(;;) pause();
}

// 产生格式化信息并输出到标准输出设备 stdout(1)，这里是指屏幕上显示。
// 参数'*fmt'指定输出将采用的格式，参见各种标准 C 语言书籍。
// 该子程序正好是 vsprintf 如何使用的一个例子。
// 该程序使用 vsprintf()将格式化的字符串放入 printbuf 缓冲区，
// 然后用 write()将缓冲区的内容输出到标准设备(1--stdout)。
// vsprintf()函数的实现见 kernel/vsprintf.c。
static int printf(const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	write(1,printbuf,i=vsprintf(printbuf, fmt, args));
	va_end(args);
	return i;
}

//普通shell
static char * argv_rc[] = { "/bin/sh", NULL };	// 调用执行程序时参数的字符串数组
static char * envp_rc[] = { "HOME=/", NULL };	// 调用执行程序时的环境字符串数组

//登录shell
// “-”是传递给 shell 程序 sh 的一个标志。
// 通过识别该标志，sh程序会作为登录 shell 执行。
// 其执行过程与在 shell 提示符下执行 sh 不太一样。
static char * argv[] = { "-/bin/sh",NULL };
static char * envp[] = { "HOME=/usr/root", NULL };

// 在 main()中已经进行了系统初始化，包括内存管理、各种硬件设备和驱动程序。
// init()函数运行在任务 0 第 1 次创建的子进程(任务 1)中。
// 它首先对第一个将要执行的程序(shell)的环境进行初始化，然后加载该程序并执行之。
void init(void)
{
	int pid,i;

	// 用于读取硬盘参数包括分区表信息并加载虚拟盘(若存在的话)和安装根文件系统设备。
	setup((void *) &drive_info);//对应函数是 sys_setup()

	// 下面以读写访问方式打开设备“/dev/tty0”，它对应终端控制台。
	// 由于这是第一次打开文件操作，因此产生的文件句柄号(文件描述符)肯定是 0。
	// 该句柄是 UNIX 类操作系统默认的控制台标准输入句柄 stdin。
	// 这里把它以读和写的方式打开是为了复制产生标准输出(写)句柄 stdout 和标准出错输出句柄 stderr。
	(void) open("/dev/tty0",O_RDWR,0);
	(void) dup(0);// 复制句柄，产生句柄 1 号 -- stdout 标准输出设备
	(void) dup(0);// 复制句柄，产生句柄 2 号 -- stderr 标准出错输出设备

	//这里是用户态所以用的是printf在内核中只有这个进程1和0/2进程是用户态的，
	//所以printf的定义为static只允许本文件内使用，其他的打印代码都运行在内核态用printk
	//下面打印缓冲区块数和总字节数，每块 1024 字节，以及主内存区空闲内存字节数
	printf("%d buffers = %d bytes buffer space\n\r",NR_BUFFERS,
		NR_BUFFERS*BLOCK_SIZE);
	printf("Free mem: %d bytes\n\r",memory_end-main_memory_start);

	// 下面 fork()用于创建一个子进程(任务 2)。
	// 对于被创建的子进程，fork()将返回 0 值，对于原进程(父进程)则返回子进程的进程号 pid。
	// 该子进程关闭了句柄0(stdin)、以只读方式打开/etc/rc 文件，
	// 并使用 execve()函数将进程自身替换成/bin/sh 程序(即 shell 程序)，然后执行/bin/sh 程序。
	// 所带参数和环境变量分别由 argv_rc 和 envp_rc 数组给出。
	// 关于 execve()请参见 fs/exec.c。
	// 函数_exit()退出时的出错码 1  操作未许可;2 -- 文件或目录不存在。
	if (!(pid=fork())) {//子进程代码，且不会走出这个大括号
		close(0);
		//之前从tty0打开是为了创建stdout和stderr现在是为了从rc文件读内容
		if (open("/etc/rc",O_RDONLY,0))
			_exit(1);
		execve("/bin/sh",argv_rc,envp_rc);//执行shell程序
		_exit(2);
	}

	//父进程从这里开始
	if (pid>0)
		while (pid != wait(&i))//等待2号进程退出
			/* nothing */;

	// 如果执行到这里，说明刚创建的子进程的执行已停止或终止了。
	// 下面循环中首先再创建一个子进程，如果出错，则显示“初始化程序创建子进程失败”信息并继续执行。
	// 对于所创建的子进程将关闭所有以前还遗留的句柄(stdin, stdout, stderr)，
	// 新创建一个会话并设置进程组号，然后重新打开/dev/tty0 作为 stdin，并复制成 stdout 和 stderr。
	// 再次执行系统解释程序/bin/sh。
	// 但这次执行所选用的参数和环境数组另选了一套。
	// 然后父进程再次运行 wait()等待。
	// 如果子进程又停止了执行，则在标准输出上显示出错信息“子进程 pid 停止了运行，
	// 返回码是 i”，然后继续重试下去...，形成“大”死循环。
	while (1) {
		if ((pid=fork())<0) {
			printf("Fork failed in init\r\n");
			continue;
		}
		if (!pid) {	//子进程
			close(0);close(1);close(2);			//为什么要关了重新创建--创建会话的要求
			setsid();							//把当前进程设成会话首领。
			(void) open("/dev/tty0",O_RDWR,0);
			(void) dup(0);
			(void) dup(0);
			_exit(execve("/bin/sh",argv,envp));	//换一套参数执行
		}
		//父进程从这开始
		while (1)//先while在if和上面先if在while的效果是一样的，只是写法不一样
			if (pid == wait(&i))
				break;
		printf("\n\rchild %d died with code %04x\n\r",pid,i);
		sync();//刷新缓冲区，及时显示信息，否则要等缓冲区满才会打印出来
	}
	_exit(0);	/* NOTE! _exit, not exit() */
}
