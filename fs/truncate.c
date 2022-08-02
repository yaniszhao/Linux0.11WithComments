/*
 *  linux/fs/truncate.c
 *
 *  (C) 1991  Linus Torvalds
 */

// 本程序用于释放指定 i 节点在设备上占用的所有逻辑块，包括直接块、一次间接块和二次间接块。
// 从而将文件的节点对应的文件长度截为 0 ，并释放占用的设备空间。


#include <linux/sched.h>

#include <sys/stat.h>

// 释放一次间接块。
static void free_ind(int dev,int block)
{
	struct buffer_head * bh;
	unsigned short * p;
	int i;

	if (!block)
		return;
	// 读取一次间接块，并释放其上表明使用的所有逻辑块，然后释放该一次间接块的缓冲区。
	if (bh=bread(dev,block)) {
		p = (unsigned short *) bh->b_data;
		for (i=0;i<512;i++,p++)
			if (*p)
				free_block(dev,*p);
		brelse(bh);
	}
	// 释放设备上的一次间接块。
	free_block(dev,block);
}

// 释放二次间接块。
static void free_dind(int dev,int block)
{
	struct buffer_head * bh;
	unsigned short * p;
	int i;

	if (!block)
		return;
	// 读取二次间接块的一级块，并释放其上表明使用的所有逻辑块，然后释放该一级块的缓冲区。
	if (bh=bread(dev,block)) {
		p = (unsigned short *) bh->b_data;
		for (i=0;i<512;i++,p++)
			if (*p)
				free_ind(dev,*p);
		brelse(bh);
	}
	// 最后释放设备上的二次间接块。
	free_block(dev,block);
}

// 将节点对应的文件长度截为 0，并释放占用的设备空间。
void truncate(struct m_inode * inode)
{
	int i;
	// 如果不是常规文件或者是目录文件，则返回。
	if (!(S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode)))
		return;
	for (i=0;i<7;i++)							// 7个直接块
		if (inode->i_zone[i]) {
			free_block(inode->i_dev,inode->i_zone[i]);
			inode->i_zone[i]=0;
		}
	free_ind(inode->i_dev,inode->i_zone[7]);	// 一次间接
	free_dind(inode->i_dev,inode->i_zone[8]);	// 二次间接
	inode->i_zone[7] = inode->i_zone[8] = 0;	// 逻辑块项 7、8 置零。
	inode->i_size = 0;							// 文件大小置零。
	inode->i_dirt = 1;							// 置节点已修改标志。
	inode->i_mtime = inode->i_ctime = CURRENT_TIME;	 // 重置文件和节点修改时间为当前时间。
}

