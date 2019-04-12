//��ͷ�ļ�����Ҫ�������ļ��������޸�ʱ��ṹ tms ��
//������ times() �������ء����� time_t ����sys/types.h �ж���ġ�
//��������һ������ԭ�� times() ��


#ifndef _TIMES_H
#define _TIMES_H

#include <sys/types.h>

struct tms {
	time_t tms_utime;	// �û�ʹ�õ� CPU ʱ�䡣
	time_t tms_stime;	// ϵͳ���ںˣ�CPU ʱ�䡣
	time_t tms_cutime;	// ����ֹ���ӽ���ʹ�õ��û� CPU ʱ�䡣
	time_t tms_cstime;	// ����ֹ���ӽ���ʹ�õ�ϵͳ CPU ʱ�䡣
};

extern time_t times(struct tms * tp);

#endif
