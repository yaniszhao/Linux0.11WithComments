/*
 *  linux/kernel/mktime.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <time.h>

/*
 * This isn't the library routine, it is only used in the kernel.
 * as such, we don't care about years<1970 etc, but assume everything
 * is ok. Similarly, TZ etc is happily ignored. We just do everything
 * as easily as possible. Let's find something public for the library
 * routines (although I think minix times is public).
 */
/*
 * PS. I hate whoever though up the year 1970 - couldn't they have gotten
 * a leap-year instead? I also hate Gregorius, pope or no. I'm grumpy.
 */

/*
* �ⲻ�ǿ⺯�����������ں�ʹ�á�������ǲ�����С�� 1970 �����ݵȣ����ٶ�һ�о���������
* ͬ����ʱ������ TZ ����Ҳ�Ⱥ��ԡ�����ֻ�Ǿ����ܼ򵥵ش������⡣������ҵ�һЩ�����Ŀ⺯��
* (��������Ϊ minix ��ʱ�亯���ǹ�����)��
* ���⣬�Һ��Ǹ����� 1970 �꿪ʼ���� - �ѵ����ǾͲ���ѡ���һ�����꿪ʼ?�Һ޸����������
* ����̻ʡ����̣���ʲô�����ں������Ǹ�Ƣ��������ˡ� */

//��������
#define MINUTE 60
#define HOUR (60*MINUTE)
#define DAY (24*HOUR)
#define YEAR (365*DAY)

/* interestingly, we assume leap-years */
/* ��Ȥ�������ǿ��ǽ������� */

// ��������Ϊ���ޣ�������ÿ���¿�ʼʱ������ʱ�����顣
static int month[12] = {
	0,
	DAY*(31),
	DAY*(31+29),
	DAY*(31+29+31),
	DAY*(31+29+31+30),
	DAY*(31+29+31+30+31),
	DAY*(31+29+31+30+31+30),
	DAY*(31+29+31+30+31+30+31),
	DAY*(31+29+31+30+31+30+31+31),
	DAY*(31+29+31+30+31+30+31+31+30),
	DAY*(31+29+31+30+31+30+31+31+30+31),
	DAY*(31+29+31+30+31+30+31+31+30+31+30)
};

//�øó���ֻ��һ������ mktime()�������ں�ʹ�á�
//����� 1970 �� 1 �� 1 �� 0 ʱ�𵽿������վ�������������Ϊ����ʱ��
long kernel_mktime(struct tm * tm)
{
	long res;
	int year;

	year = tm->tm_year - 70;//tm_year�Ǵ�1900��ʼ����
/* magic offsets (y+1) needed to get leapyears right.*/
/* Ϊ�˻����ȷ����������������Ҫ����һ��ħ��ƫֵ(y+1) */
	res = YEAR*year + DAY*((year+1)/4);
	res += month[tm->tm_mon];
/* and (y+2) here. If it wasn't a leap-year, we have to adjust */
/* �Լ�(y+2)�����(y+2)�������꣬��ô���Ǿͱ�����е���(��ȥһ�������ʱ��)��*/
	if (tm->tm_mon>1 && ((year+2)%4))
		res -= DAY;
	res += DAY*(tm->tm_mday-1);
	res += HOUR*tm->tm_hour;
	res += MINUTE*tm->tm_min;
	res += tm->tm_sec;
	return res;
}
