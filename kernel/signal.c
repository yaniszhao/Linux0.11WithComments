/*
 *  linux/kernel/signal.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>

#include <signal.h>

volatile void do_exit(int error_code);

// ��ȡ��ǰ�����ź�����λͼ(������)
int sys_sgetmask()
{
	return current->blocked;
}

// �����µ��ź�����λͼ��SIGKILL ���ܱ����Ρ�����ֵ��ԭ�ź�����λͼ
int sys_ssetmask(int newmask)
{
	int old=current->blocked;

	//1��ʾ���Σ�0�ű�ʾ��Ч��kill��stop�������Σ�Ϊ�˳����û�����ɱͣ���̡�
	current->blocked = newmask & ~(1<<(SIGKILL-1));
	return old;
}

// ���� sigaction ���ݵ� fs ���ݶ� to ����fsһ��ָ���û�̬���ݶ�
// from ==> fs:to
static inline void save_old(char * from,char * to)
{
	int i;

	verify_area(to, sizeof(struct sigaction));//�жϴ�С�Ƿ񹻣��Լ�дʱ���ơ�
	for (i=0 ; i< sizeof(struct sigaction) ; i++) {
		put_fs_byte(*from,to);
		from++;
		to++;
	}
}

// �� sigaction ���ݴ� fs ���ݶ� from λ�ø��Ƶ� to ��
// fs:from ==> to
static inline void get_new(char * from,char * to)
{
	int i;

	for (i=0 ; i< sizeof(struct sigaction) ; i++)
		*(to++) = get_fs_byte(from++);
}

// signal()ϵͳ���á������� sigaction()��Ϊָ�����źŰ�װ�µ��źž��(�źŴ������)��
// �źž���������û�ָ���ĺ�����Ҳ������ SIG_DFL(Ĭ�Ͼ��)�� SIG_IGN(����)��
// ���� signum --ָ�����ź�;handler -- ָ���ľ��;restorer �C�ָ�����ָ�룬
// �ú����� Libc���ṩ��
// �������źŴ�����������ָ�ϵͳ���÷���ʱ�����Ĵ�����ԭ��ֵ�Լ�ϵͳ���õķ���ֵ��
// �ͺ���ϵͳ����û��ִ�й��źŴ�������ֱ�ӷ��ص��û�����һ����
// ��������ԭ�źž����
int sys_signal(int signum, long handler, long restorer)
{
	struct sigaction tmp;

	if (signum<1 || signum>32 || signum==SIGKILL)
		return -1;
	tmp.sa_handler = (void (*)(int)) handler;
	tmp.sa_mask = 0;
	// �þ��ֻʹ�� 1 �κ�ͻָ���Ĭ��ֵ���������ź����Լ��Ĵ��������յ�
	tmp.sa_flags = SA_ONESHOT | SA_NOMASK;//�൱��sys_sigaction��һ�����������
	tmp.sa_restorer = (void (*)(void)) restorer;
	handler = (long) current->sigaction[signum-1].sa_handler;//����ԭ���Ĵ�������ַ
	current->sigaction[signum-1] = tmp;
	return handler;//handler��long������int ?
}

// sigaction()ϵͳ���á��ı�������յ�һ���ź�ʱ�Ĳ�����
// signum �ǳ��� SIGKILL ������κ��źš�����²���(action)��Ϊ�����²�������װ��
// ��� oldaction ָ�벻Ϊ�գ���ԭ������������ oldaction���ɹ��򷵻� 0������Ϊ-1��
int sys_sigaction(int signum, const struct sigaction * action,
	struct sigaction * oldaction)
{
	struct sigaction tmp;

	if (signum<1 || signum>32 || signum==SIGKILL)
		return -1;

	tmp = current->sigaction[signum-1];

	//Ϊsignum�ź������µĲ�����ע��action���û�̬�ĵ�ַ��
	get_new((char *) action,
		(char *) (signum-1+current->sigaction));
	if (oldaction)
		save_old((char *) &tmp,(char *) oldaction);
	if (current->sigaction[signum-1].sa_flags & SA_NOMASK)
		current->sigaction[signum-1].sa_mask = 0;
	else	//�����������ڴ����ź�ʱ���յ�ͬ�����ź�
		current->sigaction[signum-1].sa_mask |= (1<<(signum-1));
	return 0;
}


// ϵͳ�����жϴ���������������źŴ������(�� kernel/system_call.s,119 ��)�� 
// �öδ������Ҫ�����ǽ��źŵĴ��������뵽�û������ջ�У�
// ���ڱ�ϵͳ���ý������غ�����ִ���źž������Ȼ�����ִ���û��ĳ���
// �źŴ������ŵ��û�̬ȥִ�ж����Ƿŵ��ں�ִ̬�У����Լ����ں˴���ʱ�䣬���Ҹ���ȫ��
void do_signal(long signr,long eax, long ebx, long ecx, long edx,
	long fs, long es, long ds,
	long eip, long cs, long eflags,
	unsigned long * esp, long ss)
{
	unsigned long sa_handler;
	long old_eip=eip;//�û�̬eip
	struct sigaction * sa = current->sigaction + signr - 1;
	int longs;
	unsigned long * tmp_esp;

	sa_handler = (unsigned long) sa->sa_handler;
	if (sa_handler==1)	//�źž��Ϊ SIG_IGN(����)���򷵻�
		return;
	if (!sa_handler) {//������Ϊ SIG_DFL(Ĭ�ϴ���)
		if (signr==SIGCHLD)	//�ź���SIGCHLD �򷵻ء�Ӧ��Ҫ�����ӽ���ʬ��ɣ���ûʵ��?
			return;
		else				//������ֹ���̵�ִ��
			do_exit(1<<(signr-1));
	}
	//������źž��ֻ��ʹ��һ�Σ��򽫸þ���ÿգ��´�ʹ�õ���Ĭ�ϴ���
	if (sa->sa_flags & SA_ONESHOT)
		sa->sa_handler = NULL;

	// ������δ��뽫�źŴ��������뵽�û���ջ�У�ͬʱҲ�� sa_restorer,signr,
	// ����������(���SA_NOMASK û��λ),eax,ecx,edx ��Ϊ�����Լ�ԭ����ϵͳ����
	// �ĳ��򷵻�ָ�뼰��־�Ĵ���ֵѹ���ջ��
	// ����ڱ���ϵͳ�����ж�(0x80)�����û�����ʱ������ִ���û����źž������
	// Ȼ���ټ���ִ���û�����
	
	*(&eip) = sa_handler;//���ص��û�̬ȥִ�д�����
	// ��������ź��Լ��Ĵ������յ��ź��Լ�����Ҳ��Ҫ�����̵�������ѹ���ջ��
	// ע�⣬���� longs �Ľ��Ӧ��ѡ��(7*4):(8*4)����Ϊ��ջ���� 4 �ֽ�Ϊ��λ������
	longs = (sa->sa_flags & SA_NOMASK)?7:8;
	*(&esp) -= longs;//esp-=longs����long*�ļ�ÿ�μ��ľ���ֵΪ4
	verify_area(esp,longs*4);// ������ڴ�ʹ�����(��������ڴ泬���������ҳ��)
	tmp_esp=esp;
	put_fs_long((long) sa->sa_restorer,tmp_esp++);	//��restorer��������ڣ������Ǵ������Ĳ���
	put_fs_long(signr,tmp_esp++);					//���ź�ֵ��ͬʱ��sa_handler�����Ĳ���
	if (!(sa->sa_flags & SA_NOMASK))				//��������
		put_fs_long(current->blocked,tmp_esp++);
	put_fs_long(eax,tmp_esp++);						//��eax
	put_fs_long(ecx,tmp_esp++);						//��ecx
	put_fs_long(edx,tmp_esp++);						//��edx
	put_fs_long(eflags,tmp_esp++);					//��eflags
	put_fs_long(old_eip,tmp_esp++);					//��ԭ����eip
	current->blocked |= sa->sa_mask;				//����������(������)���� sa_mask �е���λ
}
