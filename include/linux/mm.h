//mm.h ���ڴ����ͷ�ļ���������Ҫ�������ڴ�ҳ��Ĵ�С�ͼ���ҳ���ͷź���ԭ�͡�


#ifndef _MM_H
#define _MM_H

#define PAGE_SIZE 4096		// �����ڴ�ҳ��Ĵ�С(�ֽ���)��

extern unsigned long get_free_page(void);
extern unsigned long put_page(unsigned long page,unsigned long address);
extern void free_page(unsigned long addr);

#endif
