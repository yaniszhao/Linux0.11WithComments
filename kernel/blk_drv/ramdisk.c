/*
 *  linux/kernel/blk_drv/ramdisk.c
 *
 *  Written by Theodore Ts'o, 12/2/91
 */
/* �� Theodore Ts'o ���ƣ�12/2/91 */
// Theodore Ts'o (Ted Ts'o)�� linux �����е��������Linux �����緶Χ�ڵ�����Ҳ�����ܴ��
// ���ͣ����� Linux ����ϵͳ������ʱ�����ͻ��ż��������Ϊ linux �ķ�չ�ṩ�� maillist����
// �ڱ����޵������������� linux �� ftp վ�㣨tsx-11.mit.edu��������������ȻΪ��� linux �û�
// �ṩ�������� linux �����������֮һ�������ʵ���� ext2 �ļ�ϵͳ�����ļ�ϵͳ�ѳ�Ϊ
// linux ��������ʵ�ϵ��ļ�ϵͳ��׼����������Ƴ��� ext3 �ļ�ϵͳ�����������ļ�ϵͳ��
// �ȶ��Ժͷ���Ч�ʡ���Ϊ�������Ƴ磬�� 97 �ڣ�2002 �� 5 �£��� linuxjournal �ڿ�������Ϊ
// �˷�����������������˲ɷá�Ŀǰ����Ϊ IBM linux �������Ĺ��������������й� LSB
// (Linux Standard Base)�ȷ���Ĺ�����(������ҳ��http://thunk.org/tytso/)

//���ļ����ڴ������̣� Ram Disk ������������ Theodore Ts'o ���ơ��������豸��һ������������
//����ģ��ʵ�ʴ��̴洢���ݵķ�ʽ����Ŀ����Ҫ��Ϊ����߶ԡ����̡����ݵĶ�д�����ٶȡ�������Ҫ
//ռ��һЩ������ڴ���Դ�⣬����Ҫȱ����һ��ϵͳ������رգ��������е��������ݽ�ȫ����ʧ����
//����������ͨ�����һЩϵͳ����ȳ��ù��߳������ʱ���ݣ�������Ҫ�������ĵ���

#include <string.h>

#include <linux/config.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <asm/system.h>
#include <asm/segment.h>
#include <asm/memory.h>

#define MAJOR_NR 1	// RAM �����豸���� 1�����豸�ű����� blk.h ֮ǰ�����塣
#include "blk.h"

// ���������ڴ��е���ʼλ�á��� 52 �г�ʼ������ rd_init()��
// ȷ�����μ�(init/main.c,124)����д rd_���� ramdisk_��
char	*rd_start;
int	rd_length = 0;	// ��������ռ�ڴ��С���ֽڣ���

// �����̵�ǰ�������������������ṹ�� do_hd_request()����(hd.c,294)��
// �ڵͼ����豸�ӿں��� ll_rw_block()�����������̣�rd�����������ӵ� rd ��������֮��
// �ͻ���øú����� rd ��ǰ��������д����ú������ȼ��㵱ǰ��������ָ������ʼ������Ӧ
// �����������ڴ����ʼλ�� addr ��Ҫ�����������Ӧ���ֽڳ���ֵ len��Ȼ������������е�
// ������в���������д���� WRITE���Ͱ���������ָ�������е�����ֱ�Ӹ��Ƶ��ڴ�λ�� addr
// �������Ƕ�������֮�����ݸ�����ɺ󼴿�ֱ�ӵ��� end_request()�Ա�������������������
// Ȼ����ת��������ʼ����ȥ������һ�������
void do_rd_request(void)
{
	int	len;
	char	*addr;

	// ���������ĺϷ��ԣ�����û�����������˳�(�μ� blk.h,127)��
	INIT_REQUEST;
	// �������ȡ�� ramdisk ����ʼ������Ӧ���ڴ���ʼλ�ú��ڴ泤�ȡ�
	// ���� sector << 9 ��ʾ sector * 512��CURRENT ����Ϊ(blk_dev[MAJOR_NR].current_request)��
	addr = rd_start + (CURRENT->sector << 9);
	len = CURRENT->nr_sectors << 9;
	// ������豸�Ų�Ϊ 1 ���߶�Ӧ�ڴ���ʼλ��>������ĩβ������������󣬲���ת�� repeat ����
	// ��� repeat �����ں� INIT_REQUEST �ڣ�λ�ں�Ŀ�ʼ�����μ� blk.h��127 �С�
	if ((MINOR(CURRENT->dev) != 1) || (addr+len > rd_start+rd_length)) {
		end_request(0);
		goto repeat;
	}
	// �����д����(WRITE)�����������л����������ݸ��Ƶ� addr ��������Ϊ len �ֽڡ�
	if (CURRENT-> cmd == WRITE) {
		(void ) memcpy(addr,
			      CURRENT->buffer,
			      len);
	// ����Ƕ�����(READ)���� addr ��ʼ�����ݸ��Ƶ��������л������У�����Ϊ len �ֽڡ�
	} else if (CURRENT->cmd == READ) {
		(void) memcpy(CURRENT->buffer, 
			      addr,
			      len);
	// ������ʾ������ڣ�������
	} else
		panic("unknown ramdisk-command");
	// ������ɹ������ø��±�־�������������豸����һ�����
	end_request(1);
	goto repeat;
}

/*
 * Returns amount of memory which needs to be reserved.
 */
 /* �����ڴ������� ramdisk ������ڴ��� */
// �����̳�ʼ��������ȷ�����������ڴ��е���ʼ��ַ�����ȡ��������������������㡣
long rd_init(long mem_start, int length)
{
	int	i;
	char	*cp;

	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;	// do_rd_request()��
	rd_start = (char *) mem_start;					// ���� 16MB ϵͳ����ֵΪ 4MB��
	rd_length = length;
	cp = rd_start;
	for (i=0; i < length; i++)
		*cp++ = '\0';
	return(length);
}

/*
 * If the root device is the ram disk, try to load it.
 * In order to do this, the root device is originally set to the
 * floppy, and we later change it to be ram disk.
 */
/*
 * ������ļ�ϵͳ�豸(root device)�� ramdisk �Ļ������Լ�������root device ԭ����ָ��
 * ���̵ģ����ǽ����ĳ�ָ�� ramdisk��
 */
// ���԰Ѹ��ļ�ϵͳ���ص��������С�
// �ú��������ں����ú��� setup()��hd.c��156 �У��б����á����⣬1 ���̿� = 1024 �ֽڡ�
void rd_load(void)
{
	struct buffer_head *bh;	// ���ٻ����ͷָ�롣
	struct super_block	s;	// �ļ�������ṹ��
	/* ��ʾ���ļ�ϵͳӳ���ļ��� boot �̵� 256 ���̿鿪ʼ��*/
	int		block = 256;	/* Start at block 256 */ 
	int		i = 1;			
	int		nblocks;
	char		*cp;		/* Move pointer */
	
	if (!rd_length)	// ��� ramdisk �ĳ���Ϊ�㣬���˳���
		return;
	printk("Ram disk: %d bytes, starting at 0x%x\n", rd_length,
		(int) rd_start);		// ��ʾ ramdisk �Ĵ�С�Լ��ڴ���ʼλ�á�
	if (MAJOR(ROOT_DEV) != 2)	// �����ʱ���ļ��豸�������̣����˳���
		return;
	// �����̿� 256+1,256,256+2��breada()���ڶ�ȡָ�������ݿ飬���������Ҫ���Ŀ飬Ȼ�󷵻�
	// �������ݿ�Ļ�����ָ�롣������� NULL�����ʾ���ݿ鲻�ɶ�(fs/buffer.c,322)��
	// ���� block+1 ��ָ�����ϵĳ����顣
	bh = breada(ROOT_DEV,block+1,block,block+2,-1);
	if (!bh) {
		printk("Disk error while looking for ramdisk!\n");
		return;
	}
	// �� s ָ�򻺳����еĴ��̳����顣(d_super_block �����г�����ṹ)��
	*((struct d_super_block *) &s) = *((struct d_super_block *) bh->b_data);
	brelse(bh);
	if (s.s_magic != SUPER_MAGIC)	// �����������ħ�����ԣ���˵������ minix �ļ�ϵͳ��
		/* No ram disk image present, assume normal floppy boot */
		/* ������û�� ramdisk ӳ���ļ����˳�ȥִ��ͨ������������ */
		return;
	// ���� = �߼�����(������) * 2^(ÿ���ο����Ĵη�)��
	// ������ݿ��������ڴ����������������ɵĿ�������Ҳ���ܼ��أ���ʾ������Ϣ�����ء�������ʾ
	// �������ݿ���Ϣ��
	nblocks = s.s_nzones << s.s_log_zone_size;
	if (nblocks > (rd_length >> BLOCK_SIZE_BITS)) {
		printk("Ram disk image too big!  (%d blocks, %d avail)\n", 
			nblocks, rd_length >> BLOCK_SIZE_BITS);
		return;
	}
	printk("Loading %d bytes into ram disk... 0000k", 
		nblocks << BLOCK_SIZE_BITS);
	// cp ָ����������ʼ����Ȼ�󽫴����ϵĸ��ļ�ϵͳӳ���ļ����Ƶ��������ϡ�
	cp = rd_start;
	while (nblocks) {
		if (nblocks > 2) 	// ������ȡ�Ŀ������� 3 ������ó�ǰԤ����ʽ�����ݿ顣
			bh = breada(ROOT_DEV, block, block+1, block+2, -1);
		else				// ����͵����ȡ��
			bh = bread(ROOT_DEV, block);
		if (!bh) {
			printk("I/O error on block %d, aborting load\n", 
				block);
			return;
		}
		(void) memcpy(cp, bh->b_data, BLOCK_SIZE);	// ���������е����ݸ��Ƶ� cp ����
		brelse(bh);									// �ͷŻ�������
		printk("\010\010\010\010\010%4dk",i);		// ��ӡ���ؿ����ֵ��
		cp += BLOCK_SIZE;							// ������ָ��ǰ�ơ�
		block++;
		nblocks--;
		i++;
	}
	printk("\010\010\010\010\010done \n");
	ROOT_DEV=0x0101;								// �޸� ROOT_DEV ʹ��ָ�������� ramdisk��
}
