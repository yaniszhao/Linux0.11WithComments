/*
 *  linux/kernel/fork.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *  'fork.c' contains the help-routines for the 'fork' system call
 * (see also system_call.s), and some misc functions ('verify_area').
 * Fork is rather simple, once you get the hang of it, but the memory
 * management can be a bitch. See 'mm/mm.c': 'copy_page_tables()'
 */
/*
 * 'fork.c'�к���ϵͳ����'fork'�ĸ����ӳ���(�μ� system_call.s)���Լ�һЩ��������
 * ('verify_area')��һ�����˽��� fork���ͻᷢ�����Ƿǳ��򵥵ģ����ڴ����ȴ��Щ�Ѷȡ�
 * �μ�'mm/mm.c'�е�'copy_page_tables()'��
 */

 
#include <errno.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>
#include <asm/system.h>

extern void write_verify(unsigned long address);

// ���½��̺ţ���ֵ�� find_empty_process()���ɡ�ͨ�������õ���
// last_pid����fork�ӽ���ʱ��pid�������������½��̵�pid�������õ�ȫ�ֱ�����
long last_pid=0;

// ���̿ռ�����дǰ��֤������
// �Ե�ǰ���̵�ַ�� addr �� addr + size ��һ�οռ���ҳΪ��λִ��д����ǰ�ļ�������
// ���ڼ���ж�����ҳ��Ϊ��λ���в�������˳���������Ҫ�ҳ� addr ����ҳ�濪ʼ��ַ start�� 
// Ȼ�� start ���Ͻ������ݶλ�ַ��ʹ��� start �任�� CPU 4G ���Կռ��еĵ�ַ��
// ���ѭ������ write_verify()��ָ����С���ڴ�ռ����дǰ��֤��
// ��ҳ����ֻ���ģ���ִ�й������͸���ҳ�����(дʱ����)��
void verify_area(void * addr,int size)
{
	unsigned long start;

	//Ҫ��֤����addr��ҳ��ʼstart��addr+size��λ�ã�
	//Ҫ����֤start��addr��ô���λ��
	start = (unsigned long) addr;
	size += start & 0xfff;
	start &= 0xfffff000;
	//�û�̬�ĵ�ַ���߼���ַ�����϶λ�ַ�������Ե�ַ
	start += get_base(current->ldt[2]);//ds/ss��

	//һҳһҳ����֤�Ƿ���Ҫдʱ����
	while (size>0) {
		size -= 4096;
		write_verify(start);
		start += 4096;
	}
}

// ����������Ĵ�������ݶλ�ַ���޳�������ҳ��
// nr Ϊ�������;p �����������ݽṹ��ָ�롣
int copy_mem(int nr,struct task_struct * p)
{
	unsigned long old_data_base,new_data_base,data_limit;
	unsigned long old_code_base,new_code_base,code_limit;

	code_limit=get_limit(0x0f);//cs
	data_limit=get_limit(0x17);//ds
	old_code_base = get_base(current->ldt[1]);//cs
	old_data_base = get_base(current->ldt[2]);//ds
	if (old_data_base != old_code_base)// 0.11 �治֧�ִ�������ݶη��������
		panic("We don't support separate I&D");
	if (data_limit < code_limit)//Ӧ��һ����
		panic("Bad data_limit");
	//�������½��������Ե�ַ�ռ��еĻ���ַ���� 64MB * �������
	new_data_base = new_code_base = nr * 0x4000000;
	p->start_code = new_code_base;
	//�����½��ֲ̾����������ж��������еĻ���ַ
	set_base(p->ldt[1],new_code_base);
	set_base(p->ldt[2],new_data_base);
	//�����½��̵�ҳĿ¼�����ҳ��������½��̵����Ե�ַ�ڴ�ҳ��Ӧ��ʵ�������ַ�ڴ�ҳ���ϡ�
	if (copy_page_tables(old_data_base,new_data_base,data_limit)) {
		free_page_tables(new_data_base,data_limit);
		return -ENOMEM;
	}
	return 0;
}

/*
 *  Ok, this is the main fork-routine. It copies the system process
 * information (task[nr]) and sets up the necessary registers. It
 * also copies the data segment in it's entirety.
 */
/* OK����������Ҫ�� fork �ӳ���������ϵͳ������Ϣ(task[n])�������ñ�Ҫ�ļĴ�����
 * ���������ظ������ݶΡ� */
int copy_process(int nr,long ebp,long edi,long esi,long gs,long none,
		long ebx,long ecx,long edx,
		long fs,long es,long ds,
		long eip,long cs,long eflags,long esp,long ss)
{
	struct task_struct *p;
	int i;
	struct file *f;

	p = (struct task_struct *) get_free_page();//����µ�ҳ��PCB
	if (!p)
		return -EAGAIN;

	// ��������ṹָ��������������С�
	// ���� nr Ϊ����ţ���ǰ�� find_empty_process()����
	task[nr] = p;

	// �������нṹ�ڵ�����
	*p = *current;	/* NOTE! this doesn't copy the supervisor stack */
					/* ע��!���������Ḵ�Ƴ����û��Ķ�ջ(ֻ���Ƶ�ǰ��������) */
	// �޸��ر������
	p->state = TASK_UNINTERRUPTIBLE; 	//���½��̵�״̬����Ϊ�����жϵȴ�״̬�����ⱻִ��
	p->pid = last_pid;					//�½��̺š���ǰ����� find_empty_process()�õ�
	p->father = current->pid;			//���ø�����
	p->counter = p->priority;			//��ʼ������counter����Ĭ��ֵΪ15
	p->signal = 0;						//��ʼ���ź�λͼ
	p->alarm = 0;						//��ʼ��������ʱ(�δ���)��0��ʾ�ޱ�����ʱ
	p->leader = 0;		/* process leadership doesn't inherit */ //�쵼Ȩ���ܼ̳�
	p->utime = p->stime = 0;			//��ʼ��utime��stime
	p->cutime = p->cstime = 0;			//��ʼ���ӽ���utime��stime
	p->start_time = jiffies;			//���ÿ�ʼʱ��
	//��������״̬��tss
	p->tss.back_link = 0;
	p->tss.esp0 = PAGE_SIZE + (long) p;	//�ں�̬��esp���պ���PCB��ĩβ
	p->tss.ss0 = 0x10;					//�ں�̬ss
	//ss1:esp1��ss2:esp2û����
	p->tss.eip = eip;					//�û�̬eip���͸�����һ��
	p->tss.eflags = eflags;				//�û�̬eflags���͸�����һ��
	p->tss.eax = 0;						//�ӽ��̵�fork�������ص���0�����Ǹ����̷����ӽ��̵�pid
	p->tss.ecx = ecx;
	p->tss.edx = edx;
	p->tss.ebx = ebx;
	p->tss.esp = esp;					//�½��̵�esp�õĸ����̵ģ�����Ҫ�󸸽��̵�ջҪ�����ɾ�
	p->tss.ebp = ebp;
	p->tss.esi = esi;
	p->tss.edi = edi;
	p->tss.es = es & 0xffff;			//��Ȼ��ѡ�������16λ���������ݽṹ�õ���long��32λ
	p->tss.cs = cs & 0xffff;
	p->tss.ss = ss & 0xffff;
	p->tss.ds = ds & 0xffff;
	p->tss.fs = fs & 0xffff;
	p->tss.gs = gs & 0xffff;
	p->tss.ldt = _LDT(nr);				//��������ldtr
	p->tss.trace_bitmap = 0x80000000;	//�� 16 λ��Ч

	// �����ǰ����ʹ����Э���������ͱ����������ġ�
	if (last_task_used_math == current)
		__asm__("clts ; fnsave %0"::"m" (p->tss.i387));

	// ����������Ĵ�������ݶλ�ַ���޳�������ҳ��
	// �������(����ֵ���� 0)����λ������������Ӧ��ͷ�Ϊ�������������ڴ�ҳ��
	if (copy_mem(nr,p)) {
		task[nr] = NULL;
		free_page((long) p);
		return -EAGAIN;
	}

	// �̳и����̵Ĵ򿪵��ļ���
	// ��������������ļ��Ǵ򿪵ģ��򽫶�Ӧ�ļ��Ĵ򿪴����� 1��
	for (i=0; i<NR_OPEN;i++)
		if (f=p->filp[i])
			f->f_count++;
	if (current->pwd)//�̳�pwd
		current->pwd->i_count++;
	if (current->root)//�̳и�Ŀ¼
		current->root->i_count++;
	if (current->executable)//�̳�ִ���ļ�
		current->executable->i_count++;

	//ע��tss��ldt����ڵ�ַ
	set_tss_desc(gdt+(nr<<1)+FIRST_TSS_ENTRY,&(p->tss));
	set_ldt_desc(gdt+(nr<<1)+FIRST_LDT_ENTRY,&(p->ldt));
	p->state = TASK_RUNNING;	/* do this last, just in case */
								/* ����ٽ����������óɿ�����״̬���Է���һ */
	return last_pid;// �����½��̺�(��������ǲ�ͬ��)
}

// Ϊ�½���ȡ�ò��ظ��Ľ��̺� last_pid�������������������е������(���� index)��
// ���̺���1��MAX_LONG�����������task�����е��±�
int find_empty_process(void)
{
	int i;
	//ѭ���õ�һ��û���ù���������Ϊ�µ�pid
	repeat:
		if ((++last_pid)<0) last_pid=1;//ѭ��������֮��Ҫ�������
		for(i=0 ; i<NR_TASKS ; i++)//����Ѿ��������˾��ж���һ�����������
			if (task[i] && task[i]->pid == last_pid) goto repeat;

	//�ҵ�һ���յ�����鲢�������±꼴�����
	for(i=1 ; i<NR_TASKS ; i++)
		if (!task[i])
			return i;
	return -EAGAIN;
}
