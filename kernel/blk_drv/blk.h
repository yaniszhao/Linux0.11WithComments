#ifndef _BLK_H
#define _BLK_H

#define NR_BLK_DEV	7	// ���豸������

/*
 * NR_REQUEST is the number of entries in the request-queue.
 * NOTE that writes may use only the low 2/3 of these: reads
 * take precedence.
 *
 * 32 seems to be a reasonable number: enough to get some benefit
 * from the elevator-mechanism, but not so much as to lock a lot of
 * buffers when they are in the queue. 64 seems to be too many (easily
 * long pauses in reading when heavy writing/syncing is going on)
 */
/*
 * ���涨��� NR_REQUEST �������������������������
 * ע�⣬��������ʹ����Щ��Ͷ˵� 2/3;���������ȴ��� 
 * 32 �������һ�����������:�Ѿ��㹻�ӵ����㷨�л�úô���
 * �����������ڶ����ж���סʱ�ֲ��Ե��Ǻܴ������64 �Ϳ���
 * ȥ̫����(��������д/ͬ����������ʱ����������ʱ�����ͣ)�� */
#define NR_REQUEST	32

/*
 * Ok, this is an expanded form so that we can use the same
 * request for paging requests when that is implemented. In
 * paging, 'bh' is NULL, and 'waiting' is used to wait for
 * read/write completion.
 */
/*
 * OK�������� request �ṹ��һ����չ��ʽ��
 * �����ʵ���Ժ����ǾͿ����ڷ�ҳ������ʹ��ͬ���� request �ṹ��
 * �ڷ�ҳ�����У�'bh'�� NULL����'waiting'�����ڵȴ���/д����ɡ� */
 // �����������������Ľṹ��������� dev=-1�����ʾ����û�б�ʹ�á�
struct request {
	int dev;		/* -1 if no request */	// ʹ�õ��豸��
	int cmd;		/* READ or WRITE */		// ����(READ �� WRITE)
	int errors;								// ����ʱ�����Ĵ������
	unsigned long sector;					// ��ʼ������(1 ��=2 ����)
	unsigned long nr_sectors;				// ��/д������
	char * buffer;							// ���ݻ�����
	struct task_struct * waiting;			// ����ȴ�����ִ����ɵĵط�
	struct buffer_head * bh;				// ������ͷָ��
	struct request * next;					// ָ����һ������
};

/*
 * This is used in the elevator algorithm: Note that
 * reads always go before writes. This is natural: reads
 * are much more time-critical than writes.
 */
/* ����Ķ������ڵ����㷨:ע�������������д����֮ǰ���С�
 * ���Ǻ���Ȼ��:��������ʱ���Ҫ��Ҫ��д�����ϸ�öࡣ */
#define IN_ORDER(s1,s2) \
((s1)->cmd<(s2)->cmd || (s1)->cmd==(s2)->cmd && \
((s1)->dev < (s2)->dev || ((s1)->dev == (s2)->dev && \
(s1)->sector < (s2)->sector)))

// ���豸�ṹ��
struct blk_dev_struct {
	void (*request_fn)(void);			// ��������ĺ���ָ��
	struct request * current_request;	// ������Ϣ�ṹ
};

// ���豸��(����)��ÿ�ֿ��豸ռ��һ�
extern struct blk_dev_struct blk_dev[NR_BLK_DEV];
// ����������顣��ll_rw.c�ļ����塣
extern struct request request[NR_REQUEST];
// �ȴ��������������ṹ����ͷָ�롣��ll_rw.c�ļ����塣
extern struct task_struct * wait_for_request;


// �ڿ��豸��������(�� hd.c)Ҫ������ͷ�ļ�ʱ�������ȶ������������Ӧ�豸�����豸�š�
#ifdef MAJOR_NR		//���豸��

/*
 * Add entries as needed. Currently the only block devices
 * supported are hard-disks and floppies.
 */
/* ��Ҫʱ������Ŀ��Ŀǰ���豸��֧��Ӳ�̺�����(����������)�� */

//RAM�̵����豸����1����������Ķ�����������ڴ�����豸��ҲΪ1
#if (MAJOR_NR == 1)
/* ram disk */
#define DEVICE_NAME "ramdisk"				// �豸���� ramdisk
#define DEVICE_REQUEST do_rd_request		// �豸������ do_rd_request
#define DEVICE_NR(device) ((device) & 7)	// �豸��(0--7)��
#define DEVICE_ON(device) 					// �����豸�����������뿪���͹ر�
#define DEVICE_OFF(device)					// �ر��豸

#elif (MAJOR_NR == 2)
/* floppy */
#define DEVICE_NAME "floppy"
#define DEVICE_INTR do_floppy				// �豸�жϴ������ do_floppy
#define DEVICE_REQUEST do_fd_request
#define DEVICE_NR(device) ((device) & 3)	// �豸��(0--3)
#define DEVICE_ON(device) floppy_on(DEVICE_NR(device))
#define DEVICE_OFF(device) floppy_off(DEVICE_NR(device))

#elif (MAJOR_NR == 3)
/* harddisk */
#define DEVICE_NAME "harddisk"
#define DEVICE_INTR do_hd
#define DEVICE_REQUEST do_hd_request
#define DEVICE_NR(device) (MINOR(device)/5)	// �豸��(0--1)��ÿ��Ӳ�̿����� 4 ��������
#define DEVICE_ON(device)					// Ӳ��һֱ�ڹ��������뿪���͹رա�
#define DEVICE_OFF(device)

#elif
/* unknown blk device */
#error "unknown blk device"

#endif

// CURRENT ��ָ�����豸�ŵĵ�ǰ����ṹ��
#define CURRENT (blk_dev[MAJOR_NR].current_request)
// CURRENT_DEV �� CURRENT ���豸��
#define CURRENT_DEV DEVICE_NR(CURRENT->dev)

#ifdef DEVICE_INTR
void (*DEVICE_INTR)(void) = NULL;	//�������һ��ȫ�ֵĺ���ָ��
#endif
static void (DEVICE_REQUEST)(void);	//��������

// ������������֮����Ҫ�ŵ�ͷ�ļ�������Ϊ��ͬ�Ŀ��豸������ķ�ʽ��ͬ��
// ͨ������ԶԲ�ͬ���豸���в�ͬ�Ĵ���

// �ͷ������Ļ�����(��)��
extern inline void unlock_buffer(struct buffer_head * bh)
{
	if (!bh->b_lock)		// ���ָ���Ļ����� bh ��û�б�����������ʾ������Ϣ��
		printk(DEVICE_NAME ": free buffer being unlocked\n");
	bh->b_lock=0;			// ���򽫸û�����������
	wake_up(&bh->b_wait);	// ���ѵȴ��û������Ľ��̡�
}

// ����������
// ���ȹر�ָ�����豸��Ȼ����˴ζ�д�������Ƿ���Ч��
// �����Ч����ݲ���ֵ���û��������ݸ��±�־���������û�������
// ������±�־����ֵ�� 0����ʾ�˴�������Ĳ�����ʧ�ܣ������ʾ��ؿ��豸 IO ������Ϣ��
// ��󣬻��ѵȴ���������Ľ����Լ��ȴ�������������ֵĽ��̣�
// �ͷŲ�������������ɾ���������
extern inline void end_request(int uptodate)
{
	DEVICE_OFF(CURRENT->dev);							//�ر��豸��
	if (CURRENT->bh) {									//��ͬ���豸CURRENT��ֵ��ͬ��
		CURRENT->bh->b_uptodate = uptodate;				//�ø��±�־��
		unlock_buffer(CURRENT->bh);						//������������
	}
	if (!uptodate) {									// ������±�־Ϊ 0 ����ʾ�豸������Ϣ��
		printk(DEVICE_NAME " I/O error\n\r");
		printk("dev %04x, block %d\n\r",CURRENT->dev,
			CURRENT->bh->b_blocknr);
	}
	wake_up(&CURRENT->waiting);							// ���ѵȴ���������Ľ��̡�
	wake_up(&wait_for_request);							// ���ѵȴ�����Ľ��̡�
	CURRENT->dev = -1;									// �ͷŸ������
	CURRENT = CURRENT->next;	// ������������ɾ������������ҵ�ǰ������ָ��ָ����һ�������
}

// �����ʼ������ꡣ��ͬ���豸����CURRENT�Ĳ�ͬ����ͬ��
#define INIT_REQUEST \
repeat: \
	if (!CURRENT) \			// �����ǰ����ṹָ��Ϊ null �򷵻ء�
		return; \			// ��ʾ���豸Ŀǰ������Ҫ����������
	if (MAJOR(CURRENT->dev) != MAJOR_NR) \		//���豸����������
		panic(DEVICE_NAME ": request list destroyed"); \
	if (CURRENT->bh) { \
		if (!CURRENT->bh->b_lock) \		// ����ڽ����������ʱ������û������������
			panic(DEVICE_NAME ": block not locked"); \
	}

#endif

#endif
