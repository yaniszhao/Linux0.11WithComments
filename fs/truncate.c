/*
 *  linux/fs/truncate.c
 *
 *  (C) 1991  Linus Torvalds
 */

// �����������ͷ�ָ�� i �ڵ����豸��ռ�õ������߼��飬����ֱ�ӿ顢һ�μ�ӿ�Ͷ��μ�ӿ顣
// �Ӷ����ļ��Ľڵ��Ӧ���ļ����Ƚ�Ϊ 0 �����ͷ�ռ�õ��豸�ռ䡣


#include <linux/sched.h>

#include <sys/stat.h>

// �ͷ�һ�μ�ӿ顣
static void free_ind(int dev,int block)
{
	struct buffer_head * bh;
	unsigned short * p;
	int i;

	if (!block)
		return;
	// ��ȡһ�μ�ӿ飬���ͷ����ϱ���ʹ�õ������߼��飬Ȼ���ͷŸ�һ�μ�ӿ�Ļ�������
	if (bh=bread(dev,block)) {
		p = (unsigned short *) bh->b_data;
		for (i=0;i<512;i++,p++)
			if (*p)
				free_block(dev,*p);
		brelse(bh);
	}
	// �ͷ��豸�ϵ�һ�μ�ӿ顣
	free_block(dev,block);
}

// �ͷŶ��μ�ӿ顣
static void free_dind(int dev,int block)
{
	struct buffer_head * bh;
	unsigned short * p;
	int i;

	if (!block)
		return;
	// ��ȡ���μ�ӿ��һ���飬���ͷ����ϱ���ʹ�õ������߼��飬Ȼ���ͷŸ�һ����Ļ�������
	if (bh=bread(dev,block)) {
		p = (unsigned short *) bh->b_data;
		for (i=0;i<512;i++,p++)
			if (*p)
				free_ind(dev,*p);
		brelse(bh);
	}
	// ����ͷ��豸�ϵĶ��μ�ӿ顣
	free_block(dev,block);
}

// ���ڵ��Ӧ���ļ����Ƚ�Ϊ 0�����ͷ�ռ�õ��豸�ռ䡣
void truncate(struct m_inode * inode)
{
	int i;
	// ������ǳ����ļ�������Ŀ¼�ļ����򷵻ء�
	if (!(S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode)))
		return;
	for (i=0;i<7;i++)							// 7��ֱ�ӿ�
		if (inode->i_zone[i]) {
			free_block(inode->i_dev,inode->i_zone[i]);
			inode->i_zone[i]=0;
		}
	free_ind(inode->i_dev,inode->i_zone[7]);	// һ�μ��
	free_dind(inode->i_dev,inode->i_zone[8]);	// ���μ��
	inode->i_zone[7] = inode->i_zone[8] = 0;	// �߼����� 7��8 ���㡣
	inode->i_size = 0;							// �ļ���С���㡣
	inode->i_dirt = 1;							// �ýڵ����޸ı�־��
	inode->i_mtime = inode->i_ctime = CURRENT_TIME;	 // �����ļ��ͽڵ��޸�ʱ��Ϊ��ǰʱ�䡣
}

