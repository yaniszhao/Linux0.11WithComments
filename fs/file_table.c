/*
 *  linux/fs/file_table.c
 *
 *  (C) 1991  Linus Torvalds
 */

 //�ó���Ŀǰ�ǿյģ����������ļ������顣

#include <linux/fs.h>

struct file file_table[NR_FILE];	// �ļ�������(64 ��)