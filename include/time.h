/*
该头文件用于涉及处理时间的函数。在 MINIX 中有一段对时间的描述很有趣：
时间的处理较为复杂，比如什么是 GMT （格林威治标准时间）、本地时间或其它时间等。
尽管主教 Ussher(1581-1656 年 ) 曾经计算过，根据圣经，
世界开始之日是公元前 4004 年 10 月 12 日上午 9 点，但在 UNIX 世界里，
时间是从GMT 1970 年 1 月 1 日午夜开始的，在这之前，所有均是空无的和 ( 无效的 ) 。
*/


#ifndef _TIME_H
#define _TIME_H

#ifndef _TIME_T
#define _TIME_T
typedef long time_t;	// 从 GMT 1970 年 1 月 1 日开始的以秒计数的时间（日历时间）。
#endif

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int size_t;
#endif

#define CLOCKS_PER_SEC 100	// 系统时钟滴答频率，100HZ。

typedef long clock_t;		// 从进程开始系统经过的时钟滴答数。

struct tm {
	int tm_sec; // 秒数 [0，59]。
	int tm_min; // 分钟数 [ 0，59]。
	int tm_hour; // 小时数 [0，59]。
	int tm_mday; // 1 个月的天数 [0，31]。
	int tm_mon; // 1 年中月份 [0，11]。
	int tm_year; // 从 1900 年开始的年数。
	int tm_wday; // 1 星期中的某天 [0，6]（星期天 =0）。
	int tm_yday; // 1 年中的某天 [0，365]。
	int tm_isdst; // 夏令时标志。
};

// 以下是有关时间操作的函数原型。

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
