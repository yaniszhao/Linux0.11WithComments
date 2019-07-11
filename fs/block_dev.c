/*
 *  linux/fs/block_dev.c
 *
 *  (C) 1991  Linus Torvalds
 */

//Ϊwrite��readϵͳ�����ṩ���豸�Ķ���д����

#include <errno.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>
#include <asm/system.h>

// ���ݿ�д���� - ��ָ���豸�Ӹ���ƫ�ƴ�д��ָ�������ֽ����ݡ�
// ������dev - �豸�ţ�pos - �豸�ļ���ƫ����ָ�룻buf - �û���ַ�ռ��л�������ַ��
// count - Ҫ���͵��ֽ�����
// �����ں���˵��д����������ٻ�������д�����ݣ�ʲôʱ����������д���豸���ɸ��ٻ������
// �������������ġ����⣬��Ϊ�豸���Կ�Ϊ��λ���ж�д�ģ���˶���д��ʼλ�ò����ڿ���ʼ
// ��ʱ����Ҫ�Ƚ���ʼ�ֽ����ڵ������������Ȼ����Ҫд�����ݴ�д��ʼ����д���ÿ飬�ٽ���
// ����һ������д�̣������ɸ��ٻ������ȥ������
int block_write(int dev, long * pos, char * buf, int count)
{
	// �� pos ��ַ����ɿ�ʼ��д��Ŀ���� block��
	// ���������� 1 �ֽ��ڸÿ��е�ƫ��λ�� offset��
	int block = *pos >> BLOCK_SIZE_BITS;
	int offset = *pos & (BLOCK_SIZE-1); // �ƺ���Ҫ�������
	int chars;			// һ����ʣ���д���ֽ�����
	int written = 0;	// ��д�ֽ���
	struct buffer_head * bh;
	register char * p;

	// ���Ҫд����ֽ��� count��ѭ��ִ�����²�����ֱ��ȫ��д�롣
	while (count>0) {
		// ����һ����ʣ���д���ֽ�����
		chars = BLOCK_SIZE - offset;	
		// �����Ҫд����ֽ������һ�飬��ֻ��д count �ֽڡ�
		if (chars > count)
			chars=count;
		// �������Ҫд 1 �����ݣ���ֱ������ 1 ����ٻ���顣
		if (chars == BLOCK_SIZE)
			bh = getblk(dev,block);
		// ������Ҫ���뽫���޸ĵ����ݿ飬��Ԥ�����������ݣ�Ȼ�󽫿�ŵ��� 1��
		else
			bh = breada(dev,block,block+1,block+2,-1);
		block++;
		// �����������ʧ�ܣ��򷵻���д�ֽ��������û��д���κ��ֽڣ��򷵻س���ţ�������
		if (!bh)
			return written?written:-EIO;
		// p ָ��������ݿ��п�ʼд��λ�á�
		// �����д������ݲ���һ�飬����ӿ鿪ʼ��д���޸ģ�������ֽڣ������������ offset Ϊ�㡣
		p = offset + bh->b_data;
		offset = 0;
		// ���ļ���ƫ��ָ��ǰ����д�ֽ������ۼ���д�ֽ��� chars�����ͼ���ֵ��ȥ�˴��Ѵ����ֽ�����
		*pos += chars;
		written += chars;
		count -= chars;
		// ���û����������� chars �ֽڵ� p ָ��ĸ��ٻ������п�ʼд���λ�á�
		while (chars-->0)
			*(p++) = get_fs_byte(buf++);
		bh->b_dirt = 1;
		brelse(bh);	// ������д����ֽ����������˳���
	}
	return written;
}

// ���ݿ������ - ��ָ���豸��λ�ö���ָ���ֽ��������ݵ����ٻ����С�
int block_read(int dev, unsigned long * pos, char * buf, int count)
{
	int block = *pos >> BLOCK_SIZE_BITS;
	int offset = *pos & (BLOCK_SIZE-1);
	int chars;
	int read = 0;
	struct buffer_head * bh;
	register char * p;

	// ���Ҫ������ֽ��� count��ѭ��ִ�����²�����ֱ��ȫ�����롣
	while (count>0) {
		// �����ڸÿ����������ֽ����������Ҫ������ֽ�������һ�飬��ֻ��� count �ֽڡ�
		chars = BLOCK_SIZE-offset;
		if (chars > count)
			chars = count;
		// ������Ҫ�����ݿ飬��Ԥ�����������ݣ���������������򷵻��Ѷ��ֽ�����
		// ���û�ж����κ��ֽڣ��򷵻س���š�Ȼ�󽫿�ŵ��� 1��
		if (!(bh = breada(dev,block,block+1,block+2,-1)))
			return read?read:-EIO;
		block++;
		// p ָ����豸�������ݿ�����Ҫ��ȡ�Ŀ�ʼλ�á��������Ҫ��ȡ�����ݲ���һ�飬
		// ����ӿ鿪ʼ��ȡ������ֽڣ���������轫 offset ���㡣
		p = offset + bh->b_data;
		offset = 0;
		// ���ļ���ƫ��ָ��ǰ���Ѷ����ֽ��� chars���ۼ��Ѷ��ֽ�����
		// ���ͼ���ֵ��ȥ�˴��Ѵ����ֽ�����
		*pos += chars;
		read += chars;
		count -= chars;
		// �Ӹ��ٻ������� p ָ��Ŀ�ʼλ�ø��� chars �ֽ����ݵ��û������������ͷŸø��ٻ�������
		while (chars-->0)
			put_fs_byte(*(p++),buf++);
		brelse(bh);
	}
	return read;	// �����Ѷ�ȡ���ֽ����������˳���
}
