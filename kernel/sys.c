/*
 *  linux/kernel/sys.c
 *
 *  (C) 1991  Linus Torvalds
 */

// sys.c ������Ҫ�����кܶ�ϵͳ���ù��ܵ�ʵ�ֺ�����
// ���У�������ֵΪ-ENOSYS�����ʾ����� Linux ��û��ʵ�ָù��ܡ�


// ����ļ����ϵͳ���ô���漰�û���Ȩ�ޡ�
// �û�Ȩ�ް���uid gid / ruid rgid / euid egid / suid sgid

#include <errno.h>

#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/kernel.h>
#include <asm/segment.h>
#include <sys/times.h>
#include <sys/utsname.h>

// �������ں�ʱ��
int sys_ftime()
{
	return -ENOSYS;
}

int sys_break()
{
	return -ENOSYS;
}

// ���ڵ�ǰ���̶��ӽ��̽��е���(degugging)
int sys_ptrace()
{
	return -ENOSYS;
}

// �ı䲢��ӡ�ն�������
int sys_stty()
{
	return -ENOSYS;
}

// ȡ�ն���������Ϣ
int sys_gtty()
{
	return -ENOSYS;
}

// �޸��ļ���
int sys_rename()
{
	return -ENOSYS;
}

int sys_prof()
{
	return -ENOSYS;
}

// ���õ�ǰ�����ʵ���Լ�/������Ч�� ID(gid)��
// �������û�г����û���Ȩ����ôֻ�ܻ�����ʵ���� ID ����Ч�� ID��
// ���������г����û���Ȩ����������������Ч�ĺ�ʵ�ʵ��� ID��
// ������ gid(saved gid)�����ó�����Ч gid ֵͬ��
int sys_setregid(int rgid, int egid)
{
	if (rgid>0) {//���ý��̵�gid
		if ((current->gid == rgid) || 
		    suser())
			current->gid = rgid;
		else
			return(-EPERM);
	}
	if (egid>0) {//���ý��̵�egid
		if ((current->gid == egid) ||
		    (current->egid == egid) ||
		    (current->sgid == egid) ||
		    suser())
			current->egid = egid;
		else
			return(-EPERM);
	}
	return 0;
}

// ���ý������(gid)���������û�г����û���Ȩ��������ʹ�� setgid()������Ч gid
// (effective gid)����Ϊ���䱣�� gid(saved gid)����ʵ�� gid(real gid)��
// ��������г����û���Ȩ����ʵ�� gid����Ч gid �ͱ��� gid �������óɲ���ָ���� gid��
int sys_setgid(int gid)
{
	return(sys_setregid(gid, gid));
}

// �򿪻�رս��̼��ʹ���
int sys_acct()
{
	return -ENOSYS;
}

// ӳ�����������ڴ浽���̵������ַ�ռ�
int sys_phys()
{
	return -ENOSYS;
}

int sys_lock()
{
	return -ENOSYS;
}

int sys_mpx()
{
	return -ENOSYS;
}

int sys_ulimit()
{
	return -ENOSYS;
}

// ���ش� 1970 �� 1 �� 1 �� 00:00:00 GMT ��ʼ��ʱ��ʱ��ֵ(��)��
// ��� tloc ��Ϊ null����ʱ��ֵҲ�洢�����
int sys_time(long * tloc)
{
	int i;

	i = CURRENT_TIME;
	if (tloc) {
		verify_area(tloc,4);//�жϴ�С�Ƿ񹻣��Լ�дʱ���ơ�
		put_fs_long(i,(unsigned long *)tloc);
	}
	return i;
}

/*
 * Unprivileged users may change the real user id to the effective uid
 * or vice versa.
 */
 /* ����Ȩ���û����Լ�ʵ���û���ʶ��(real uid)�ĳ���Ч�û���ʶ��(effective uid)����֮ҲȻ�� */
// ���������ʵ���Լ�/������Ч�û� ID(uid)��
// �������û�г����û���Ȩ����ôֻ�ܻ�����ʵ���û� ID ����Ч�û� ID��
// ���������г����û���Ȩ����������������Ч�ĺ�ʵ�ʵ��û� ID�� 
// ������ uid(saved uid)�����ó�����Ч uid ֵͬ��
int sys_setreuid(int ruid, int euid)
{
	int old_ruid = current->uid;
	
	if (ruid>0) {//�ı���̵�uid
		if ((current->euid==ruid) ||
			(old_ruid == ruid) ||
		    suser())
			current->uid = ruid;
		else
			return(-EPERM);
	}
	if (euid>0) {//�ı���̵�euid
		if ((old_ruid == euid) ||
			(current->euid == euid) ||
		    suser())
			current->euid = euid;
		else {
			current->uid = old_ruid;
			return(-EPERM);
		}
	}
	return 0;
}

// ���������û���(uid)��
// �������û�г����û���Ȩ��������ʹ�� setuid()������Ч uid(effective uid)���ó�
// �䱣�� uid(saved uid)����ʵ�� uid(real uid)��
// ��������г����û���Ȩ����ʵ�� uid����Ч uid �ͱ��� uid �������óɲ���ָ���� uid��
int sys_setuid(int uid)
{
	return(sys_setreuid(uid, uid));
}

// ����ϵͳʱ������ڡ����� tptr �Ǵ� 1970 �� 1 �� 1 �� 00:00:00 GMT ��ʼ��ʱ��ʱ��ֵ(��)��
// ���ý��̱�����г����û�Ȩ�ޡ�
int sys_stime(long * tptr)
{
	if (!suser())
		return -EPERM;
	//������ʱ�����? ���ֻ�Ǹĵ�ǰʱ��Ļ�����jiffies�ƺ���һ��ɡ�
	startup_time = get_fs_long((unsigned long *)tptr) - jiffies/HZ;
	return 0;
}

// ��ȡ��ǰ����ʱ�䡣
// tms �ṹ�а����û�ʱ�䡢ϵͳʱ�䡢�ӽ����û�ʱ�䡢�ӽ���ϵͳʱ�䡣
int sys_times(struct tms * tbuf)
{
	if (tbuf) {
		verify_area(tbuf,sizeof *tbuf);
		put_fs_long(current->utime,(unsigned long *)&tbuf->tms_utime);
		put_fs_long(current->stime,(unsigned long *)&tbuf->tms_stime);
		put_fs_long(current->cutime,(unsigned long *)&tbuf->tms_cutime);
		put_fs_long(current->cstime,(unsigned long *)&tbuf->tms_cstime);
	}
	return jiffies;//���صδ���
}

// ������ end_data_seg ��ֵ��������ϵͳȷʵ���㹻���ڴ棬���ҽ���û�г�Խ��������ݶδ�Сʱ��
// �ú����������ݶ�ĩβΪ end_data_seg ָ����ֵ����ֵ������ڴ����β����ҪС�ڶ�ջ��β 16KB��
// ����ֵ�����ݶε��½�βֵ(�������ֵ��Ҫ��ֵ��ͬ��������д���)��
// �ú����������û�ֱ�ӵ��ã����� libc �⺯�����а�װ�����ҷ���ֵҲ��һ����
int sys_brk(unsigned long end_data_seg)
{
	if (end_data_seg >= current->end_code &&
	    end_data_seg < current->start_stack - 16384)//16386==0x4000==64k
		current->brk = end_data_seg;//brkΪ���ݶ��ܳ���(��βֵ)�������޸ĶѵĴ�С
	return current->brk;
}

/*
 * This needs some heave checking ...
 * I just haven't get the stomach for it. I also don't fully
 * understand sessions/pgrp etc. Let somebody who does explain it.
 */
/* ���������ҪĳЩ�ϸ�ļ��...
 * ��ֻ��û��θ��������Щ����Ҳ����ȫ���� sessions/pgrp �ȡ��������˽����ǵ��������ɡ� */
// ���ý��̵Ľ����� ID Ϊ pgid��
// ������� pid=0����ʹ�õ�ǰ���̺š���� pgid Ϊ 0����ʹ�ò��� pid ָ���Ľ��̵��� ID ��Ϊpgid��
// ����ú������ڽ����̴�һ���������Ƶ���һ�������飬���������������������ͬһ���Ự(session)��
// ����������£����� pgid ָ����Ҫ��������н����� ID��
// ��ʱ����ĻỰ ID �����뽫Ҫ������̵���ͬ(193 ��)��
int sys_setpgid(int pid, int pgid)
{//����pid���̵������id
	int i;

	if (!pid)//pidΪ0��Ŀ�����Ϊ��ǰ����
		pid = current->pid;
	if (!pgid)//pgidΪ0��Ŀ������ֵΪpid
		pgid = current->pid;
	for (i=0 ; i<NR_TASKS ; i++)
		if (task[i] && task[i]->pid==pid) {
			if (task[i]->leader)	//��������ǻỰ���첻�ܸĽ�����
				return -EPERM;		//������ POSIX �������г���??
			if (task[i]->session != current->session)
				return -EPERM;//���Ŀ����̵�session�͵�ǰ���̲���һ����ҲûȨ�޿��Ը�
			task[i]->pgrp = pgid;
			return 0;
		}
	return -ESRCH;
}

// ���ص�ǰ���̵���š��� getpgid(0)��ͬ
int sys_getpgrp(void)
{
	return current->pgrp;
}

// ����һ���Ự(session)(�������� leader=1)������������Ự��=�����=����̺š�
// setsid -- SET Session ID��
int sys_setsid(void)
{
	//�Ѿ��ǻỰ���������Ȩ�ޣ����˳�
	if (current->leader && !suser())
		return -EPERM;
	current->leader = 1;
	current->session = current->pgrp = current->pid;//�������Ҫע��
	current->tty = -1;	// ��ʾ��ǰ����û�п����ն�
	return current->pgrp;	// ���ػỰ ID
}

// ��ȡϵͳ��Ϣ������ utsname �ṹ���� 5 ���ֶΣ��ֱ���:
// ���汾����ϵͳ�����ơ�����ڵ����ơ���ǰ���м��𡢰汾�����Ӳ���������ơ�
int sys_uname(struct utsname * name)
{
	static struct utsname thisname = {
		"linux .0","nodename","release ","version ","machine "
	};
	int i;

	if (!name) return -ERROR;
	verify_area(name,sizeof *name);//�жϴ�С�Ƿ񹻣��Լ�дʱ���ơ�
	for(i=0;i<sizeof *name;i++)
		put_fs_byte(((char *) &thisname)[i],i+(char *) name);
	return 0;
}

// ���õ�ǰ���̴����ļ�����������Ϊ mask & 0777��������ԭ�����롣
int sys_umask(int mask)
{
	int old = current->umask;

	current->umask = mask & 0777;//0777�ǰ˽��Ƶ�
	return (old);
}
