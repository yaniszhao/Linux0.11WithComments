/*
 *  linux/kernel/sched.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * 'sched.c' is the main kernel file. It contains scheduling primitives
 * (sleep_on, wakeup, schedule etc) as well as a number of simple system
 * call functions (type getpid(), which just extracts a field from
 * current-task
 */

//sched.c ���ں����й�������Ⱥ����ĳ������а����йص��ȵĻ�������
//(sleep_on��wakeup�� schedule ��)�Լ�һЩ�򵥵�ϵͳ���ú���(���� getpid())��
//���� Linus Ϊ�˱�̵ķ��㣬���ǵ������� ��������ʱ����Ҫ��Ҳ���������̵ļ��������ŵ������

//�⼸�����������Ĵ�����Ȼ����������Щ���󣬱Ƚ�������⡣
//������Ե��Ⱥ��� schedule()��һЩ˵����

//������ֵ��һ��ĺ������Զ�����˯�ߺ��� sleep_on()�ͻ��Ѻ��� wake_up()��������������Ȼ�̣ܶ� 
//ȴҪ�� schedule()��������⡣������ͼʾ�ķ������Խ��͡�
//�򵥵�˵��sleep_on()��������Ҫ�����ǵ� һ������(������)���������Դ��æ�����ڴ���ʱ��ʱ�л���ȥ��
//���ڵȴ������еȴ�һ��ʱ�䡣 ���л��������ټ������С�
//����ȴ����еķ�ʽ�������˺����е� tmp ָ����Ϊ�������ڵȴ�������� ϵ��
//�����й�ǣ�浽����������ָ�����:*p��tmp �� current��*p �ǵȴ�����ͷָ�룬
//���ļ�ϵͳ�ڴ� i �ڵ�� i_wait ָ�롢�ڴ滺������е� buffer_wait ָ���;
//tmp ����ʱָ��;current �ǵ�ǰ����ָ�롣 
//������Щָ�����ڴ��еı仯������ǿ�����ͼ 5-5 ��ʾ��ͼ˵����ͼ�еĳ�����ʾ�ڴ��ֽ����С�


#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/sys.h>
#include <linux/fdreg.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/segment.h>

#include <signal.h>

//ȡ�ź�nr���ź�λͼ�ж�Ӧλ�Ķ�������ֵ���źű��1-32��
//�����ź� 5 ��λͼ��ֵ = 1<<(5-1) = 16 = 00010000b
#define _S(nr) (1<<((nr)-1))

// ���� SIGKILL �� SIGSTOP �ź�������������
// ��������(...10111111111011111111b)
#define _BLOCKABLE (~(_S(SIGKILL) | _S(SIGSTOP)))

// ��ʾ����� nr �Ľ��̺š�����״̬���ں˶�ջ�����ֽ���(��Լ)
void show_task(int nr,struct task_struct * p)
{
	int i,j = 4096-sizeof(struct task_struct);

	printk("%d: pid=%d, state=%d, ",nr,p->pid,p->state);
	i=0;
	//p+1������PCB֮��Ϊ0���ڴ漴Ϊ���еģ�˵����4Kһ��ʼ�Ǳ���ʼ���˵�
	while (i<j && !((char *)(p+1))[i])
		i++;
	printk("%d (of %d) chars free in kernel stack\n\r",i,j);
}

// ��ʾ�������������š����̺š�����״̬���ں˶�ջ�����ֽ���(��Լ)
// ����źͽ��̺ŵ�����?
void show_stat(void)
{
	int i;

	//NR_TASKS ��ϵͳ�����ɵ�������(����)����(64 ��)
	for (i=0;i<NR_TASKS;i++)
		if (task[i])
			show_task(i,task[i]);
}

// ����ÿ��ʱ��Ƭ�ĵδ���
#define LATCH (1193180/HZ)

extern void mem_use(void);

extern int timer_interrupt(void);
extern int system_call(void);

//��Ϊһ����������ݽṹ�����ں�̬��ջ����ͬһ�ڴ�ҳ�У�
//���ԴӶ�ջ�μĴ��� ss ���Ի�������ݶ�ѡ���
union task_union {
	struct task_struct task;
	char stack[PAGE_SIZE];
};

// �����ʼ��������ݣ�������0�Ž��̡�����1��init����
static union task_union init_task = {INIT_TASK,};

// ǰ����޶��� volatile��Ӣ�Ľ������ױ䡢���ȶ�����˼��
// ������Ҫ�� gcc ��Ҫ�Ըñ��������Ż�����Ҳ��ҪŲ��λ�ã�
// ��ΪҲ���ĳ�������޸�����ֵ��
long volatile jiffies=0;	// �ӿ�����ʼ����ĵδ���ʱ��ֵ(10ms/�δ�)

//ע�����ﵥλ����
long startup_time=0;		// ����ʱ�䡣�� 1970:0:0:0 ��ʼ��ʱ������

struct task_struct *current = &(init_task.task);	// ��ǰ����ָ��(��ʼ��Ϊ��ʼ����)
struct task_struct *last_task_used_math = NULL;		// ʹ�ù�Э�����������ָ��

//task����װ��ֻ��ָ�룬����Ŀռ�Ҫ��������
struct task_struct * task[NR_TASKS] = {&(init_task.task), };	// ��������ָ������

// �����û���ջ���û�̬ʹ�ã�4K��ָ��ָ�����һ�
// ��ʼһ��ʼ0����û�е��û�̬ʱ��ջ�õ�Ҳ������ط���
long user_stack [ PAGE_SIZE>>2 ] ;	//���ֽ�Ϊ��λ����Ҫ����4

struct {	//����ͨ��lss����ss:sp
	long * a;	//ָ��ջ��Ԫ�ص���͵�һ���ֽ�
	short b;	//00010 0 00
					//�����PAGE_SIZE>>2 �Ƕ���ռ��С����������Ϊ�˰ѵ�ַָ�����
	} stack_start = { & user_stack [PAGE_SIZE>>2] , 0x10 };

/*
 *  'math_state_restore()' saves the current math information in the
 * old math state array, and gets the new ones from the current task
 */
 /* ����ǰЭ���������ݱ��浽��Э������״̬�����У�������ǰ�����Э������
  * ���ݼ��ؽ�Э�������� */
void math_state_restore()
{
	if (last_task_used_math == current)
		return;
	__asm__("fwait");
	if (last_task_used_math) {
		__asm__("fnsave %0"::"m" (last_task_used_math->tss.i387));
	}
	last_task_used_math=current;
	if (current->used_math) {
		__asm__("frstor %0"::"m" (current->tss.i387));
	} else {
		__asm__("fninit"::);
		current->used_math=1;
	}
}

/*
 *  'schedule()' is the scheduler function. This is GOOD CODE! There
 * probably won't be any reason to change this, as it should work well
 * in all circumstances (ie gives IO-bound processes good response etc).
 * The one thing you might take a look at is the signal-handler code here.
 *
 *   NOTE!!  Task 0 is the 'idle' task, which gets called when no other
 * tasks can run. It can not be killed, and it cannot sleep. The 'state'
 * information in task[0] is never used.
 */
	
/*
* 'schedule()'�ǵ��Ⱥ��������Ǹ��ܺõĴ���!û���κ����ɶ��������޸ģ�
* ��Ϊ�����������еĻ����¹���(�����ܹ��� IO-�߽紦��ܺõ���Ӧ��)��
* ֻ��һ����ֵ�����⣬�Ǿ���������źŴ�����롣
* ע��!!���� 0 �Ǹ�����('idle')����ֻ�е�û�����������������ʱ�ŵ�������
* �����ܱ�ɱ����Ҳ����˯�ߡ����� 0 �е�״̬��Ϣ'state'�Ǵ������õġ�
*/
void schedule(void)
{
	int i,next,c;
	//ע��taskװ��ֻ��ָ�룬����������ָ���ָ��
	struct task_struct ** p;

/* check alarm, wake up any interruptible tasks that have got a signal */
/* ��� alarm(���̵ı�����ʱֵ)�������κ��ѵõ��źŵĿ��ж����� */
	for(p = &LAST_TASK ; p > &FIRST_TASK ; --p)
		if (*p) {
			// ������ù�����Ķ�ʱֵ alarm�������Ѿ�����(alarm<jiffies),
			// �����ź�λͼ���� SIGALRM �źš�Ȼ���� alarm�����źŵ�Ĭ�ϲ�������ֹ���̡�
			if ((*p)->alarm && (*p)->alarm < jiffies) {
					(*p)->signal |= (1<<(SIGALRM-1));
					(*p)->alarm = 0;
				}
			// ����ź�λͼ�г����������ź��⻹�������źţ���һ������alarm����ģ�
			// ���������ڿ��ж�״̬����������Ϊ����״̬��
			if (((*p)->signal & ~(_BLOCKABLE & (*p)->blocked)) &&
			(*p)->state==TASK_INTERRUPTIBLE)
				(*p)->state=TASK_RUNNING;
		}

/* this is the scheduler proper: */
/* �����ǵ��ȳ������Ҫ���� */
	while (1) {
		c = -1;
		next = 0;
		i = NR_TASKS;
		p = &task[NR_TASKS];
		// ��δ���Ҳ�Ǵ�������������һ������ʼѭ�������������������������ۡ�
		// �Ƚ�ÿ������״̬����� counter(��������ʱ��ĵݼ��δ����)ֵ����һ��ֵ��
		// ����ʱ�仹������next ��ָ���ĸ�������š�
		while (--i) {
			if (!*--p)
				continue;
			//ע��TASK_RUNNING�Ǿ���̬
			if ((*p)->state == TASK_RUNNING && (*p)->counter > c)
				c = (*p)->counter, next = i;
		}

		//����н���counter����0��ȡ��ֵ�����Ǹ����̵��ȡ�
		//�����Ҷ����������ߵ���cΪ-1���ǲ�Ϊ0��nextΪ0���ȥ�л�0�Ž��̡�
		if (c) break;
		
		//�������̬�Ľ���counter�������ˣ������¸������ȼ�����counter��
		//counter ֵ�ļ��㷽ʽΪ counter = counter /2 + priority��
		//���������̲����ǽ��̵�״̬�����������Ľ���counterû���껹�ǻ��ܵ�Ӱ�졣		
		for(p = &LAST_TASK ; p > &FIRST_TASK ; --p)
			if (*p)
				(*p)->counter = ((*p)->counter >> 1) +
						(*p)->priority;
	}

	//���Ⱥ�������ϵͳ����ʱȥִ������ 0��
	//��ʱ���� 0 ��ִ��pause()ϵͳ���ã����ֻ����schedule������
	switch_to(next);
}

// pause()ϵͳ���á�ת����ǰ�����״̬Ϊ���жϵĵȴ�״̬�������µ��ȡ�
// ��ϵͳ���ý����½��̽���˯��״̬��ֱ���յ�һ���źš�
// ���ź�������ֹ���̻���ʹ���̵���һ���źŲ�������
// ֻ�е�������һ���źţ������źŲ����������أ�pause()�Ż᷵�ء�
// ��ʱ pause()����ֵӦ����-1������ errno ����Ϊ EINTR�����ﻹû����ȫʵ��(ֱ�� 0.95 ��)��
int sys_pause(void)
{
	current->state = TASK_INTERRUPTIBLE;
	schedule();
	return 0;
}

// �ѵ�ǰ������Ϊ�����жϵĵȴ�״̬������˯�߶���ͷ��ָ��ָ��ǰ����
// ֻ����ȷ�ػ���ʱ�Ż᷵�ء��ú����ṩ�˽������жϴ������֮���ͬ�����ơ�
// ��������*p �Ƿ��õȴ�����Ķ���ͷָ�롣
void sleep_on(struct task_struct **p)
{
	struct task_struct *tmp;

	if (!p)
		return;
	if (current == &(init_task.task)) //0���̲���˯
		panic("task[0] trying to sleep");
	//������Ȼֻ������ָ�룬һ��ָ��ǰһ������Ľ��̣�һ��ָ��ǰ����Ľ��̡�
	//���Ƕ�����̵ĵ��ã��͹�����һ������
	tmp = *p;
	*p = current;
	current->state = TASK_UNINTERRUPTIBLE;
	schedule();
	// ֻ�е�����ȴ����񱻻���ʱ�����ȳ�����ַ��ص�������ʾ�����ѱ���ȷ�ػ��ѡ�
	// ��Ȼ��Ҷ��ڵȴ�ͬ������Դ����ô����Դ����ʱ�����б�Ҫ�������еȴ�����Դ�Ľ��̡�
	// �ú���Ƕ�׵��ã�Ҳ��Ƕ�׻������еȴ�����Դ�Ľ��̡�
	// ע���ǻ������Խ��̣����Դ����ȥ������Դ?
	if (tmp)
		tmp->state=0;//����̬
}

// ����ǰ������Ϊ���жϵĵȴ�״̬��������*p ָ���ĵȴ������С�
// ���ж�˯�ߺͲ����ж�˯��������Ҫ�����ܷ���Ӧ�źš�
void interruptible_sleep_on(struct task_struct **p)
{
	struct task_struct *tmp;

	if (!p)
		return;
	if (current == &(init_task.task))
		panic("task[0] trying to sleep");
	tmp=*p;
	*p=current;
repeat:	current->state = TASK_INTERRUPTIBLE;
	schedule();
	
	// ����ȴ������л��еȴ����񣬲��Ҷ���ͷָ����ָ��������ǵ�ǰ����ʱ��
	// �򽫸õȴ�������Ϊ�����еľ���״̬��������ִ�е��ȳ���
	// ��ָ��*p ��ָ��Ĳ��ǵ�ǰ����ʱ����ʾ�ڵ�ǰ���񱻷�����к�
	// �����µ����񱻲���ȴ������У���ˣ���Ӧ��ͬʱҲ�����������ĵȴ�������Ϊ������״̬��
	if (*p && *p != current) {//��ͷ����ͷ��Ϊ��ǰ����
		(**p).state=0;
		goto repeat;
	}
	// ����һ���������Ӧ����*p = tmp���ö���ͷָ��ָ������ȴ�����
	// �����ڵ�ǰ����֮ǰ����ȴ����е��������Ĩ���ˡ�
	
	*p=NULL;
	if (tmp)
		tmp->state=0;
}

void wake_up(struct task_struct **p)
{
	if (p && *p) {//��ͷ��㻽�ѣ������̻����ΰѶ����еĺ�����㻽�ѡ�
		(**p).state=0;
		*p=NULL;
	}
}

/*
 * OK, here are some floppy things that shouldn't be in the kernel
 * proper. They are here because the floppy needs a timer, and this
 * was the easiest way of doing it.
 */
 /*
  * ���ˣ������￪ʼ��һЩ�й����̵��ӳ��򣬱���Ӧ�÷����ں˵���Ҫ�����еġ������Ƿ�������
  * ����Ϊ������Ҫһ��ʱ�ӣ����������������İ취�� */

// ��������������ʱ����Ĵ��롣
// ���Ķ���δ���֮ǰ���ȿ�һ�¿��豸һ�����й�������������(floppy.c)�����˵����
// ���ߵ��Ķ����̿��豸��������ʱ��������δ��롣
// ����ʱ�䵥λ:1 ���δ� = 1/100 �롣
// ���������ŵȴ������������������ת�ٵĽ���ָ�롣�������� 0-3 �ֱ��Ӧ���� A-D��
static struct task_struct * wait_motor[4] = {NULL,NULL,NULL,NULL};//�ĸ��ȴ�����
// ��������ֱ��Ÿ����������������Ҫ�ĵδ�����������Ĭ������ʱ��Ϊ 50 ���δ�(0.5 ��)��
static int  mon_timer[4]={0,0,0,0};
// ��������ֱ��Ÿ����������ͣת֮ǰ��ά�ֵ�ʱ�䡣�������趨Ϊ 10000 ���δ�(100 ��)��
static int moff_timer[4]={0,0,0,0};
// ��Ӧ�����������е�ǰ��������Ĵ������üĴ���ÿλ�Ķ�������: 
// λ 7-4:�ֱ���������� D-A ����������1 - ����;0 - �رա�
// λ 3 :1 - ����DMA���ж�����;0 - ��ֹDMA���ж����� 
// λ 2 :1 - �������̿�����;0 - ��λ���̿�������
// λ 1-0:00 - 11������ѡ����Ƶ����� A-D��
unsigned char current_DOR = 0x0C;//0000 1 1 00

// ָ������������������ת״̬����ȴ�ʱ�䡣
// nr -- ������(0-3)������ֵΪ�δ�����
int ticks_to_floppy_on(unsigned int nr)
{
	// ѡ��������־(kernel/blk_drv/floppy.c,122)��
	// ��ѡ������Ӧ��������Ĵ���������������λ��
	// mask �� 4 λ�Ǹ�������������־��
	extern unsigned char selected;
	unsigned char mask = 0x10 << nr;

	if (nr>3)	// ϵͳ����� 4 ��������
		panic("floppy_on: nr>3");
	moff_timer[nr]=10000;		/* 100 s = very big :-) */
	cli();				/* use floppy_off to turn it off */
	mask |= current_DOR;
	if (!selected) {
		mask &= 0xFC;
		mask |= nr;
	}
	if (mask != current_DOR) {
		outb(mask,FD_DOR);
		if ((mask ^ current_DOR) & 0xf0)
			mon_timer[nr] = HZ/2;
		else if (mon_timer[nr] < 2)
			mon_timer[nr] = 2;
		current_DOR = mask;
	}
	sti();
	return mon_timer[nr];
}

// �ȴ�ָ������������������һ��ʱ�䣬Ȼ�󷵻ء�
// ����ָ���������������������ת���������ʱ��Ȼ��˯�ߵȴ���
// �ڶ�ʱ�жϹ����л�һֱ�ݼ��ж������趨����ʱֵ��
// ����ʱ���ڣ��ͻỽ������ĵȴ����̡�
void floppy_on(unsigned int nr)
{
	cli();
	// ������������ʱ��û������һֱ�ѵ�ǰ������Ϊ�����ж�˯��״̬������ȴ�������еĶ����С�
	while (ticks_to_floppy_on(nr))
		sleep_on(nr+wait_motor);
	sti();
}

// �ùر���Ӧ�������ͣת��ʱ��(3 ��)��
// ����ʹ�øú�����ȷ�ر�ָ����������������￪�� 100 ��֮��Ҳ�ᱻ�رա�
void floppy_off(unsigned int nr)
{
	moff_timer[nr]=3*HZ;
}

// ���̶�ʱ�����ӳ��򡣸������������ʱֵ�����ر�ͣת��ʱֵ��
// ���ӳ������ʱ�Ӷ�ʱ�жϹ����б����ã����ϵͳÿ����һ���δ�(10ms)�ͻᱻ����һ�Σ�
// ��ʱ������￪����ͣת��ʱ����ֵ��
// ���ĳһ�����ͣת��ʱ��������������Ĵ����������λ��λ��
void do_floppy_timer(void)
{
	int i;
	unsigned char mask = 0x10;

	for (i=0 ; i<4 ; i++,mask <<= 1) {
		if (!(mask & current_DOR))	// ������� DOR ָ�������������
			continue;
		if (mon_timer[i]) {
			if (!--mon_timer[i])	// ������������ʱ�����ѽ���
				wake_up(i+wait_motor);
		} else if (!moff_timer[i]) {// ������ͣת��ʱ����λ��Ӧ�������λ��
			current_DOR &= ~mask;	// ��������������Ĵ�����
			outb(current_DOR,FD_DOR);
		} else
			moff_timer[i]--;	// ���ͣת��ʱ�ݼ�
	}
}

#define TIME_REQUESTS 64 // ������ 64 ����ʱ������(64 ������)

// ��ʱ������ṹ�Ͷ�ʱ�����顣
// �������ݽṹ���ƾ�̬����
static struct timer_list {
	long jiffies;				// ��ʱ�δ�����
	void (*fn)();				// ��ʱ�������
	struct timer_list * next;	// ��һ����ʱ����
} timer_list[TIME_REQUESTS], * next_timer = NULL;

// ��Ӷ�ʱ�����������Ϊָ���Ķ�ʱֵ(�δ���)����Ӧ�Ĵ������ָ�롣 
// ������������(floppy.c)���øú���ִ��������ر�������ʱ������ 
// jiffies �C �� 10 ����Ƶĵδ���;*fn()- ��ʱʱ�䵽ʱִ�еĺ�����
// ע�������ʱ�����ں˶�ʱ������Ҫ�Ǹ������õģ��û������á�
void add_timer(long jiffies, void (*fn)(void))
{
	struct timer_list * p;

	if (!fn)
		return;
	cli();
	
	// �����ʱֵ<=0�������̵����䴦����򡣲��Ҹö�ʱ�������������С�
	if (jiffies <= 0)
		(fn)();
	else {
		// �Ӷ�ʱ�������У���һ�������
		for (p = timer_list ; p < timer_list + TIME_REQUESTS ; p++)
			if (!p->fn)
				break;
		// ����Ѿ������˶�ʱ�����飬��ϵͳ����
		if (p >= timer_list + TIME_REQUESTS)
			panic("No more time requests free");
		// ��ʱ�����ݽṹ������Ӧ��Ϣ������������ͷ��
		p->fn = fn;
		p->jiffies = jiffies;
		p->next = next_timer;//����ͷ�巨
		next_timer = p;
		// �������ʱֵ��С��������������ʱ��ȥ����ǰ����Ҫ�ĵδ�����
		// �����ڴ���ʱ��ʱֻҪ�鿴����ͷ�ĵ�һ��Ķ�ʱ�Ƿ��ڼ��ɡ�
		// ��γ������û�п�����ȫ������²���Ķ�ʱ��ֵ < ԭ��ͷһ����ʱ��ֵʱ��
		// ҲӦ�ý����к���Ķ�ʱֵ����ȥ�µĵ� 1 ���Ķ�ʱֵ��
		while (p->next && p->next->jiffies < p->jiffies) {
			p->jiffies -= p->next->jiffies;
			fn = p->fn;
			p->fn = p->next->fn;
			p->next->fn = fn;
			jiffies = p->jiffies;
			p->jiffies = p->next->jiffies;
			p->next->jiffies = jiffies;
			p = p->next;
		}
	}
	sti();
}

// ʱ���ж� C ����������� 
// ���� cpl �ǵ�ǰ��Ȩ�� 0 �� 3��0 ��ʾ�ں˴�����ִ�С�
// ����һ����������ִ��ʱ��Ƭ����ʱ������������л�����ִ��һ����ʱ���¹�����
void do_timer(long cpl)
{
	extern int beepcount;			// ����������ʱ��δ���
	extern void sysbeepstop(void);	// �ر�������

	// ���������������������رշ�����
	//(�� 0x61 �ڷ��������λλ 0 �� 1��λ 0 ���� 8253 ������ 2 �Ĺ�����λ 1 ����������)��
	if (beepcount)
		if (!--beepcount)
			sysbeepstop();

	//��Ȩ�����ں�ջ�б���Ľ����ں˵�CS��ѡ������õ���
	if (cpl)				//��Ȩ��0����3
		current->utime++;	//�û�ʹ��ʱ���1���δ�
	else
		current->stime++;//�ں�ʹ��ʱ���1���δ�

	// ������û��Ķ�ʱ�����ڣ�������� 1 ����ʱ����ֵ�� 1��
	// ����ѵ��� 0���������Ӧ�Ĵ�����򣬲����ô������ָ����Ϊ�ա�
	// Ȼ��ȥ�����ʱ����
	if (next_timer) {
		next_timer->jiffies--;
		while (next_timer && next_timer->jiffies <= 0) {
			void (*fn)(void);//�ֲ�����������ָ���������ʽ
			
			fn = next_timer->fn;
			next_timer->fn = NULL;// ȥ�����ʱ����
			next_timer = next_timer->next;
			(fn)();//������
		}
	}
	
	// �����ǰ���̿����� FDC ����������Ĵ������������λ����λ�ģ���ִ�����̶�ʱ����(245 ��)
	if (current_DOR & 0xf0)
		do_floppy_timer();
	// �����������ʱ�仹û�꣬���˳����������жϺ����ִ�б����̵Ĵ��롣
	if ((--current->counter)>0) return;
	current->counter=0;
	// �����ں�̬���򣬲����� counter ֵ���е��ȡ�
	// ������Ҫ����Ϊ�ں˿϶�Ҫ�������ꡣ
	if (!cpl) return;
	//ʱ��Ƭ������Ҫ���µ���
	schedule();
}

// ϵͳ���ù��� - ���ñ�����ʱʱ��ֵ(��)��
// ������� seconds>0�������ø��µĶ�ʱֵ������ԭ��ʱֵ�����򷵻� 0��
int sys_alarm(long seconds)
{
	int old = current->alarm;

	if (old)
		old = (old - jiffies) / HZ;//������
	//���ɵδ�����ע������Ҫ���Ͽ��������ڵĵδ���
	current->alarm = (seconds>0)?(jiffies+HZ*seconds):0;
	return (old);
}

// ȡ��ǰ���̺� pid��
int sys_getpid(void)
{
	return current->pid;
}

// ȡ�����̺� ppid��
int sys_getppid(void)
{
	return current->father;
}

// ȡ�û��� uid��
int sys_getuid(void)
{
	return current->uid;
}

// ȡ euid��
int sys_geteuid(void)
{
	return current->euid;
}

// ȡ��� gid��
int sys_getgid(void)
{
	return current->gid;
}

// ȡ egid��
int sys_getegid(void)
{
	return current->egid;
}

// ϵͳ���ù��� -- ���Ͷ� CPU ��ʹ������Ȩ(���˻�����?)��
// Ӧ������ increment ���� 0������Ļ�,��ʹ����Ȩ����!!
int sys_nice(long increment)
{
	if (current->priority-increment>0)
		current->priority -= increment;
	return 0;
}

// ���ȳ���ĳ�ʼ���ӳ���
void sched_init(void)
{
	int i;
	struct desc_struct * p;// ��������ṹָ�롣gdt��ldt

	// sigaction �Ǵ���й��ź�״̬�Ľṹ
	if (sizeof(struct sigaction) != 16)
		panic("Struct sigaction MUST be 16 bytes");

	// ���ó�ʼ����(���� 0)������״̬���������;ֲ����ݱ�������
	set_tss_desc(gdt+FIRST_TSS_ENTRY,&(init_task.task.tss));//tss0
	set_ldt_desc(gdt+FIRST_LDT_ENTRY,&(init_task.task.ldt));//ldt0
	// ���������������������(ע�� i=1 ��ʼ�����Գ�ʼ���������������)
	p = gdt+2+FIRST_TSS_ENTRY;
	for(i=1;i<NR_TASKS;i++) {
		task[i] = NULL;
		p->a=p->b=0;
		p++;
		p->a=p->b=0;
		p++;
	}
/* Clear NT, so that we won't have troubles with that later on */
/* �����־�Ĵ����е�λ NT�������Ժ�Ͳ������鷳 */
// NT ��־���ڿ��Ƴ���ĵݹ����(Nested Task)���� NT ��λʱ����ô��ǰ�ж�����ִ��
// iret ָ��ʱ�ͻ����������л���NT ָ�� TSS �е� back_link �ֶ��Ƿ���Ч��
	__asm__("pushfl ; andl $0xffffbfff,(%esp) ; popfl");

	// ע��!!�ǽ� GDT ����Ӧ LDT ��������ѡ������ص� ldtr��
	// ֻ��ȷ������һ�Σ��Ժ�������LDT �ļ��أ��� CPU ���� TSS �е� LDT ���Զ����ء�
	ltr(0);					// ������ 0 �� TSS ���ص�����Ĵ��� tr��
	lldt(0);				// ���ֲ�����������ص��ֲ���������Ĵ���

	// ����������ڳ�ʼ�� 8253 ��ʱ��
	outb_p(0x36,0x43);		/* binary, mode 3, LSB/MSB, ch 0 */
	outb_p(LATCH & 0xff , 0x40);	/* LSB */		// ��ʱֵ���ֽ�
	outb(LATCH >> 8 , 0x40);	/* MSB */			// ��ʱֵ���ֽ�
	set_intr_gate(0x20,&timer_interrupt);		// ����ʱ���жϴ��������(����ʱ���ж���)
	outb(inb_p(0x21)&~0x01,0x21);				// �޸��жϿ����������룬����ʱ���жϡ�

	// ����ϵͳ�����ж���
	set_system_gate(0x80,&system_call);//ϵͳ�����жϵ�ע��
}
