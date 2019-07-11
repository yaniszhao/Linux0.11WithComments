/*
 *  linux/kernel/exit.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/tty.h>
#include <asm/segment.h>

int sys_pause(void);		// �ѽ�����Ϊ˯��״̬��ϵͳ����
int sys_close(int fd);		// �ر�ָ���ļ���ϵͳ����

// �ͷ�ָ������ռ�õ�����ۼ����������ݽṹ��ռ�õ��ڴ档
// ���� p ���������ݽṹ��ָ�롣�ú����ں���� sys_kill()�� sys_waitpid()�б����á�
// ɨ������ָ������� task[]��Ѱ��ָ������������ҵ�����������ո�����ۣ�
// Ȼ���ͷŸ��������ݽṹ��ռ�õ��ڴ�ҳ�棬���ִ�е��Ⱥ������ڷ���ʱ�����˳���
// ����������������û���ҵ�ָ�������Ӧ������ں� panic
void release(struct task_struct * p)
{
	int i;

	if (!p)
		return;
	for (i=1 ; i<NR_TASKS ; i++)
		if (task[i]==p) {
			task[i]=NULL;
			free_page((long)p);//��ΪPCB�Ƕ�̬������ڴ棬����Ҫfree
			schedule();//ע���ͷ���֮������
			return;
		}
	// ָ��������������������
	panic("trying to release non-existent task");
}

// ��ָ������(*p)�����ź�(sig)��Ȩ��Ϊ priv��
// ����:sig -- �ź�ֵ;
// p -- ָ�������ָ��;
// priv -- ǿ�Ʒ����źŵı�־��������Ҫ���ǽ����û����Ի򼶱���ܷ����źŵ�Ȩ����
// �ú����������жϲ�������ȷ�ԣ�Ȼ���ж������Ƿ����㡣
// ����������ָ�����̷����ź� sig ���˳������򷵻�δ��ɴ���š�
static inline int send_sig(long sig,struct task_struct * p,int priv)
{//�ں�̬����ֱ�ӷ����źţ��û�̬��Ҫ����ϵͳ����sys_kill�������ź�
	if (!p || sig<1 || sig>32)
		return -EINVAL;
	// �ж�Ȩ��:
	// ���ǿ�Ʒ��ͱ�־��λ�����ߵ�ǰ���̵���Ч�û���ʶ��(euid)����ָ�����̵� euid(Ҳ�����Լ�)��
	// ���ߵ�ǰ�����ǳ����û������ڽ���λͼ����Ӹ��źţ���������˳���
	if (priv || (current->euid==p->euid) || suser())
		p->signal |= (1<<(sig-1));
	else
		return -EPERM;
	return 0;
}

// ��ֹ�Ự(session)��
static void kill_session(void)
{
	struct task_struct **p = NR_TASKS + task;
	
	while (--p > &FIRST_TASK) {//��0��������ͬ session id �Ľ��̣�ȫ���Ҷ�
		if (*p && (*p)->session == current->session)
			(*p)->signal |= 1<<(SIGHUP-1);
	}
}

/*
 * XXX need to check permissions needed to send signals to process
 * groups, etc. etc.  kill() permissions semantics are tricky!
 */
 /* Ϊ���������ȷ����źţ�XXX ��Ҫ�����ɡ�kill()����ɻ��Ʒǳ�����! */
// ϵͳ���� kill()���������κν��̻�����鷢���κ��źţ�������ֻ��ɱ�����̡� 
// ���� pid �ǽ��̺�;sig ����Ҫ���͵��źš�
// ��� pid ֵ>0�����źű����͸����̺��� pid �Ľ��̡�
// ��� pid=0����ô�źžͻᱻ���͸���ǰ���̵Ľ������е����н��̡�
// ��� pid=-1�����ź� sig �ͻᷢ�͸�����һ������������н��̡�
// ��� pid < -1�����ź� sig �����͸�������-pid �����н��̡�
// ����ź� sig Ϊ 0���򲻷����źţ����Ի���д����顣����ɹ��򷵻� 0��
// �ú���ɨ����������������� pid ��ֵ�����������Ľ��̷���ָ�����ź� sig���� pid ���� 0��
// ������ǰ�����ǽ������鳤�������Ҫ���������ڵĽ���ǿ�Ʒ����ź� sig��
int sys_kill(int pid,int sig)
{
	struct task_struct **p = NR_TASKS + task;
	int err, retval = 0;

	if (!pid) while (--p > &FIRST_TASK) {			//pidΪ0�������鳤ʹ��pid=0��������
		if (*p && (*p)->pgrp == current->pid) 
			if (err=send_sig(sig,*p,1))
				retval = err;
	} else if (pid>0) while (--p > &FIRST_TASK) {	//pid����0��ָ��pid
		if (*p && (*p)->pid == pid) 
			if (err=send_sig(sig,*p,0))
				retval = err;
	} else if (pid == -1) while (--p > &FIRST_TASK)	//pid=-1����0�Ž���������Ȩ�޵Ľ���
		if (err = send_sig(sig,*p,0))
			retval = err;
	else while (--p > &FIRST_TASK)					//С��-1����Ӧ-pid�����飬ǰ��Ҳ����Ȩ��
		if (*p && (*p)->pgrp == -pid)
			if (err = send_sig(sig,*p,0))
				retval = err;
	return retval;
}

// ֪ͨ������ -- ����� pid �����ź� SIGCHLD:Ĭ��������ӽ��̽�ֹͣ����ֹ��
// ���û���ҵ������̣����Լ��ͷš������� POSIX.1 Ҫ������������������ֹ��
// ���ӽ���Ӧ�ñ���ʼ���� 1 ���ݡ�
static void tell_father(int pid)
{
	int i;

	if (pid)//pid��ֵ�Ǹ����̵�pid
		for (i=0;i<NR_TASKS;i++) {
			if (!task[i])
				continue;
			if (task[i]->pid != pid)
				continue;
			task[i]->signal |= (1<<(SIGCHLD-1));
			return;
		}
/* if we don't find any fathers, we just release ourselves */
/* This is not really OK. Must change it to make father 1 */
	printk("BAD BAD - no father found\n\r");
	release(current);//�Ҳ��������̲Ű��Լ���PCB���գ�һ�㶼����û�и����̵�
}

// �����˳��������code �Ǵ����롣
int do_exit(long code)
{
	int i;

	free_page_tables(get_base(current->ldt[1]),get_limit(0x0f));//�ͷ�cs��
	free_page_tables(get_base(current->ldt[2]),get_limit(0x17));//�ͷ�ds/ss��

	//���ӽ��̽���1�Ž��̴����ƺ������������ӽ��̲�������
	for (i=0 ; i<NR_TASKS ; i++)		
		if (task[i] && task[i]->father == current->pid) {	//����ǰ���̵��ӽ���
			task[i]->father = 1;							//��1�Ž���ȥ�����ӽ���
			if (task[i]->state == TASK_ZOMBIE)				//���ӽ�����������1�Ž��̴���ʬ��
				/* assumption task[1] is always init */
				(void) send_sig(SIGCHLD, task[1], 1);
		}
		
	// �رյ�ǰ���̴��ŵ������ļ�
	for (i=0 ; i<NR_OPEN ; i++)
		if (current->filp[i])
			sys_close(i);
		
	// �Ե�ǰ���̵Ĺ���Ŀ¼ pwd����Ŀ¼ root �Լ������ i �ڵ����ͬ�����������ֱ��ÿ�(�ͷ�)
	iput(current->pwd); 	//ͬ��pwd
	current->pwd=NULL;
	iput(current->root);	//ͬ��root
	current->root=NULL;
	iput(current->executable);	//ͬ������i���
	current->executable=NULL;

	// �����ǰ�����ǻỰͷ��(leader)���̲������п����նˣ����ͷŸ��ն�
	if (current->leader && current->tty >= 0)
		tty_table[current->tty].pgrp = 0;

	// �����ǰ�����ϴ�ʹ�ù�Э���������� last_task_used_math �ÿ�
	if (last_task_used_math == current)
		last_task_used_math = NULL;

	// �����ǰ������ leader ���̣�����ֹ�ûỰ��������ؽ��̡�
	// �����ɱ������leader���̣���ͬһsession�Ľ��̶�Ҫ��ɱ����
	// �����Ϊʲô�ػ�����Ҫ�ؽ�session��ԭ��
	if (current->leader)
		kill_session();

	// �ѵ�ǰ������Ϊ����״̬��������ǰ�����Ѿ��ͷ�����Դ�������潫�ɸ����̶�ȡ���˳��롣
	// ��0�Ž����⡣�����붼��һ�������̣����Ҳ��1�Ž���������
	// �ҽ����˳�ʱ���Ҫ�������̷ŵ�1�Ž���ȥ���������н��̶��Ǳ�1�Ž��̴���ʬ�塣
	current->state = TASK_ZOMBIE;
	current->exit_code = code;		//�����������ר�ŷ��˳��Ĵ������
	tell_father(current->father);	//���߸������Լ��ȹ���
	schedule();						//���µ��Ƚ������У����ø����̴����������������ƺ�����
	return (-1);	/* just to suppress warnings */
}

// ϵͳ���� exit()����ֹ����
int sys_exit(int error_code)
{
	return do_exit((error_code&0xff)<<8);
}


// ϵͳ���� waitpid()������ǰ���̣�ֱ�� pid ָ�����ӽ����˳�(��ֹ)�����յ�Ҫ����ֹ 
// �ý��̵��źţ���������Ҫ����һ���źž��(�źŴ������)����� pid ��ָ���ӽ������� 
// �˳�(�ѳ���ν�Ľ�������)���򱾵��ý����̷��ء��ӽ���ʹ�õ�������Դ���ͷš�
// ��� pid > 0, ��ʾ�ȴ����̺ŵ��� pid ���ӽ��̡�
// ��� pid = 0, ��ʾ�ȴ�������ŵ��ڵ�ǰ������ŵ��κ��ӽ��̡�
// ��� pid < -1, ��ʾ�ȴ�������ŵ��� pid ����ֵ���κ��ӽ��̡�
// ��� pid = -1, ��ʾ�ȴ��κ��ӽ��̡�
// �� options = WUNTRACED����ʾ����ӽ�����ֹͣ�ģ�Ҳ���Ϸ���(�������)�� 
// �� options = WNOHANG����ʾ���û���ӽ����˳�����ֹ�����Ϸ��ء�
// �������״ָ̬�� stat_addr ��Ϊ�գ���ͽ�״̬��Ϣ���浽���
int sys_waitpid(pid_t pid,unsigned long * stat_addr, int options)
{
	int flag, code;
	struct task_struct ** p;

	//�жϴ�С�Ƿ񹻣��Լ�дʱ���ơ�
	verify_area(stat_addr,4);//ע��stat_addr���û�̬�ĵ�ַ
repeat:
	flag=0;
	for(p = &LAST_TASK ; p > &FIRST_TASK ; --p) {
		if (!*p || *p == current)//����Ϊ�գ������Ǳ�����
			continue;
		if ((*p)->father != current->pid)//�����Ǳ����̵��ӽ���
			continue;
		if (pid>0) {//����0����Ϊ��Ӧ�ӽ���
			if ((*p)->pid != pid)
				continue;
		} else if (!pid) {//����0��������̵��ӽ�����һ����
			if ((*p)->pgrp != current->pgrp)
				continue;
		} else if (pid != -1) {//С��-1����Ϊ-pid������̵���һ���̣�ҲҪ���ӽ���
			if ((*p)->pgrp != -pid)
				continue;
		}

		//����һ���������pid=-1�������ߵ������ж���״̬
		switch ((*p)->state) {
			case TASK_STOPPED://��WUNTRACED��־����ʹΪֹͣ״̬Ҳ����
				if (!(options & WUNTRACED))
					continue;
				put_fs_long(0x7f,stat_addr);//���ӽ����˳�������Ϊ127
				//���ﷵ�صĶ�Ӧ�ӽ��̵�pid������ϵͳ���÷��غ��û�̬������eax�еõ�
				return (*p)->pid;
			case TASK_ZOMBIE://���̴���ʬ��Ӧ�þ�������ط���
				//ע�⣬����1�Ž��̻����sys_waitpid�⣬�����̴����ӽ��̺�Ҳ���ܻ���ã�
				//����һ��ĸ�����Ҳ���Եõ��ӽ��̵�����ʱ�䣬Ҳ���������ӽ��̵�ʬ�塣
				current->cutime += (*p)->utime;//��¼�ӽ��̵�utime
				current->cstime += (*p)->stime;//��¼�ӽ��̵�stime
				flag = (*p)->pid;
				code = (*p)->exit_code;
				release(*p);	//PCB�������Ѿ�����Ҫ�ˣ�����free�ˣ���һ������Ҫ
				put_fs_long(code,stat_addr);//�����ӽ��̵��˳�������
				return flag;//�����ӽ���pid
			default:
				flag=1;
				continue;
		}
	}

	//�ߵ�������û���ҵ�һ�������ӽ��̡�
	//flagΪ0��Ҫ���ӽ��̲����ʣ�Ҳ����pid���������̫ǡ����
	//flagΪ1��Ҫ��״̬�����ʣ������з��ϵ�pid���ӽ��̣�����Ҫ���ж��£�
	//�����WNOHANG��ֱ�ӷ��أ����û�о������ȴ������źŷ��͹�����
	if (flag) {
		if (options & WNOHANG)//ֱ�ӷ���
			return 0;
		current->state=TASK_INTERRUPTIBLE;//������������źſ��Ի��ѵ�
		schedule();//ȥ������������

		//�ߵ����ʱ��˵�������յ����źš�
		//����SIGCHLD�ź�˵�������ӽ��̹��ˣ�ֱ�ӿ���goto�������ӽ��̵Ĵ��봦��
		//�����յ��������ź�(������ʱ��Ӧ���յ������źŵĵ�)�������˳���������
		if (!(current->signal &= ~(1<<(SIGCHLD-1))))
			goto repeat;
		else
			return -EINTR;
	}
	return -ECHILD;
}


