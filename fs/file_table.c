/*
 *  linux/fs/file_table.c
 *
 *  (C) 1991  Linus Torvalds
 */

 //该程序目前是空的，仅定义了文件表数组。

#include <linux/fs.h>

struct file file_table[NR_FILE];	// 文件表数组(64 项)