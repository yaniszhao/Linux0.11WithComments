/*
��ͷ�ļ������漰����ʱ��ĺ������� MINIX ����һ�ζ�ʱ�����������Ȥ��
ʱ��Ĵ����Ϊ���ӣ�����ʲô�� GMT ���������α�׼ʱ�䣩������ʱ�������ʱ��ȡ�
�������� Ussher(1581-1656 �� ) ���������������ʥ����
���翪ʼ֮���ǹ�Ԫǰ 4004 �� 10 �� 12 ������ 9 �㣬���� UNIX �����
ʱ���Ǵ�GMT 1970 �� 1 �� 1 ����ҹ��ʼ�ģ�����֮ǰ�����о��ǿ��޵ĺ� ( ��Ч�� ) ��
*/


#ifndef _TIME_H
#define _TIME_H

#ifndef _TIME_T
#define _TIME_T
typedef long time_t;	// �� GMT 1970 �� 1 �� 1 �տ�ʼ�����������ʱ�䣨����ʱ�䣩��
#endif

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int size_t;
#endif

#define CLOCKS_PER_SEC 100	// ϵͳʱ�ӵδ�Ƶ�ʣ�100HZ��

typedef long clock_t;		// �ӽ��̿�ʼϵͳ������ʱ�ӵδ�����

struct tm {
	int tm_sec; // ���� [0��59]��
	int tm_min; // ������ [ 0��59]��
	int tm_hour; // Сʱ�� [0��59]��
	int tm_mday; // 1 ���µ����� [0��31]��
	int tm_mon; // 1 �����·� [0��11]��
	int tm_year; // �� 1900 �꿪ʼ��������
	int tm_wday; // 1 �����е�ĳ�� [0��6]�������� =0����
	int tm_yday; // 1 ���е�ĳ�� [0��365]��
	int tm_isdst; // ����ʱ��־��
};

// �������й�ʱ������ĺ���ԭ�͡�

clock_t clock(void);
time_t time(time_t * tp);
double difftime(time_t time2, time_t time1);
time_t mktime(struct tm * tp);

char * asctime(const struct tm * tp);
char * ctime(const time_t * tp);
struct tm * gmtime(const time_t *tp);
struct tm *localtime(const time_t * tp);
size_t strftime(char * s, size_t smax, const char * fmt, const struct tm * tp);
void tzset(void);

#endif
