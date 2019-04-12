/*
head ͷ�ļ��������� Intel CPU ���������ļ򵥽ṹ����ָ������������š�
*/

#ifndef _HEAD_H
#define _HEAD_H

// �����˶������������ݽṹ���ýṹ��˵��ÿ������
// ������ 8 ���ֽڹ��ɣ�ÿ������������ 256 �
typedef struct desc_struct {
	unsigned long a,b;
} desc_table[256];

extern unsigned long pg_dir[1024];	// �ڴ�ҳĿ¼���顣ÿ��Ŀ¼��Ϊ 4 �ֽڡ��������ַ 0 ��ʼ��
extern desc_table idt,gdt;			// �ж���������ȫ����������

#define GDT_NUL 0			// ȫ����������ĵ� 0 ����á�
#define GDT_CODE 1			// �� 1 ����ں˴�����������
#define GDT_DATA 2			// �� 2 ����ں����ݶ��������
#define GDT_TMP 3			// �� 3 �ϵͳ����������Linux û��ʹ�á�

#define LDT_NUL 0			// ÿ���ֲ���������ĵ� 0 ����á�
#define LDT_CODE 1			// �� 1 ����û����������������
#define LDT_DATA 2			// �� 2 ����û��������ݶ��������

#endif
