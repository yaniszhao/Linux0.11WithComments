/*
 *  linux/kernel/blk_dev/ll_rw.c
 *
 * (C) 1991 Linus Torvalds
 */

/*
 * This handles all read/write requests to block devices
 */
/* �ó�������豸�����ж�/д������*/
// �ó�����Ҫ����ִ�еͲ���豸�� / д�������Ǳ������п��豸��ϵͳ�������ֵĽӿڳ���
// ��������ͨ�����øó���ĵͼ����д���� ll_rw_block() ����д���豸�е����ݡ�
 
#include <errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/system.h>

#include "blk.h" // ���豸ͷ�ļ��������������ݽṹ�����豸���ݽṹ�ͺ꺯������Ϣ��

/*
 * The request-struct contains all necessary data
 * to load a nr of sectors into memory
 */
 /* ����ṹ�к��м��� nr �������ݵ��ڴ��е����б������Ϣ��*/
struct request request[NR_REQUEST];

/*
 * used to wait on when there are no free requests
 */
 /* ����������������û�п�����ʱ���̵���ʱ�ȴ��� */
struct task_struct * wait_for_request = NULL;

/* blk_dev_struct is:
 *	do_request-address
 *	next-request
 */
/* blk_dev_struct ���豸�ṹ�ǣ�(kernel/blk_drv/blk.h,23)
 * do_request-address 	// ��Ӧ���豸�ŵ����������ָ�롣
 * current-request 		// ���豸����һ������
 */
// ������ʹ�����豸����Ϊ������ʵ�����ݽ��ڸ��ֿ��豸���������ʼ��ʱ���롣���磬Ӳ��
// ����������г�ʼ��ʱ��hd.c��343 �У�����һ����伴�������� blk_dev[3]�����ݡ�
struct blk_dev_struct blk_dev[NR_BLK_DEV] = {
	{ NULL, NULL },		/* no_dev */
	{ NULL, NULL },		/* dev mem */
	{ NULL, NULL },		/* dev fd */
	{ NULL, NULL },		/* dev hd */
	{ NULL, NULL },		/* dev ttyx */
	{ NULL, NULL },		/* dev tty */
	{ NULL, NULL }		/* dev lp */
};

// ����ָ���Ļ����� bh�����ָ���Ļ������Ѿ�������������������ʹ�Լ�˯�ߣ������жϵصȴ�����
// ֱ����ִ�н�����������������ȷ�ػ��ѡ�
static inline void lock_buffer(struct buffer_head * bh)
{
	cli();
	while (bh->b_lock)			// ����������ѱ���������˯�ߣ�ֱ��������������
		sleep_on(&bh->b_wait);
	bh->b_lock=1;				// ���������û�������
	sti();
}

// �ͷţ������������Ļ�������
static inline void unlock_buffer(struct buffer_head * bh)
{
	if (!bh->b_lock)
		printk("ll_rw_block.c: buffer not locked\n\r");
	bh->b_lock = 0;
	wake_up(&bh->b_wait);// ���ѵȴ��û�����������
}

/*
 * add-request adds a request to the linked list.
 * It disables interrupts so that it can muck with the
 * request-lists in peace.
 */
/*
 * add-request()�������м���һ����������ر��жϣ�
 * �������ܰ�ȫ�ش������������� */
 */
// �������м������������ dev ָ�����豸��req ��������ṹ��Ϣָ�롣
static void add_request(struct blk_dev_struct * dev, struct request * req)
{
	struct request * tmp;

	req->next = NULL;
	cli();
	if (req->bh)
		req->bh->b_dirt = 0;				// �建�������ࡱ��־��
	// ��� dev �ĵ�ǰ����(current_request)�Ӷ�Ϊ�գ����ʾĿǰ���豸û�������
	// �����ǵ� 1 ���������˿ɽ����豸��ǰ����ָ��ֱ��ָ��������
	// ������ִ����Ӧ�豸����������
	if (!(tmp = dev->current_request)) {
		dev->current_request = req;
		sti();
		(dev->request_fn)();
		return;
	}
	// ���Ŀǰ���豸�Ѿ����������ڵȴ������������õ����㷨������Ѳ���λ�ã�
	// Ȼ�󽫵�ǰ������뵽���������С������㷨���������ô��̴�ͷ���ƶ�������С��
	// �Ӷ�����Ӳ�̷���ʱ�䡣
	for ( ; tmp->next ; tmp=tmp->next)
		if ((IN_ORDER(tmp,req) ||
		    !IN_ORDER(tmp,tmp->next)) &&
		    IN_ORDER(req,tmp->next))
			break;
	req->next=tmp->next;
	tmp->next=req;
	sti();
}

// �������������������С������ǣ����豸�� major������ rw��������ݵĻ�����ͷָ�� bh��
static void make_request(int major,int rw, struct buffer_head * bh)
{
	struct request * req;
	int rw_ahead;

/* WRITEA/READA is special case - it is not really needed, so if the */
/* buffer is locked, we just forget about it, else it's a normal read */
	/* WRITEA/READA ��һ��������� - ���ǲ����Ǳ�Ҫ����������������Ѿ�������*/
	/* ���ǾͲ��������˳�������Ļ���ִ��һ��Ķ�/д������ */
	// ����'READ'��'WRITE'�����'A'�ַ�����Ӣ�ĵ��� Ahead����ʾ��ǰԤ��/д���ݿ����˼��
	// ���������� READA/WRITEA ���������ָ���Ļ���������ʹ�ã��ѱ�����ʱ���ͷ���Ԥ��/д����
	// �������Ϊ��ͨ�� READ/WRITE ������в�����
	if (rw_ahead = (rw == READA || rw == WRITEA)) {
		if (bh->b_lock)
			return;
		if (rw == READA)
			rw = READ;
		else
			rw = WRITE;
	}
	// �������� READ �� WRITE ���ʾ�ں˳����д���ʾ������Ϣ��������
	if (rw!=READ && rw!=WRITE)
		panic("Bad block dev command, must be R/W/RA/WA");
	// ����������������������Ѿ���������ǰ���񣨽��̣��ͻ�˯�ߣ�ֱ������ȷ�ػ��ѡ�
	lock_buffer(bh);
	// ���������д���һ��������ݲ��ࣨû�б��޸Ĺ��������������Ƕ����һ����������Ǹ��¹��ģ�
	// �������������󡣽��������������˳���
	if ((rw == WRITE && !bh->b_dirt) || (rw == READ && bh->b_uptodate)) {
		unlock_buffer(bh);
		return;
	}
repeat:
/* we don't allow the write-requests to fill up the queue completely:
 * we want some room for reads: they take precedence. The last third
 * of the requests are only for reads.
 */
 	/* ���ǲ����ö�����ȫ����д�����������ҪΪ��������һЩ�ռ䣺������
	 * �����ȵġ�������еĺ�����֮һ�ռ���Ϊ��׼���ġ�
	 */
	// �������Ǵ���������ĩβ��ʼ������������ġ���������Ҫ�󣬶��ڶ��������󣬿���ֱ��
	// �Ӷ���ĩβ��ʼ��������д������ֻ�ܴӶ��� 2/3 �������ͷ�������������롣
	if (rw == READ)
		req = request+NR_REQUEST;			// ���ڶ����󣬽�����ָ��ָ�����β����
	else
		req = request+((NR_REQUEST*2)/3);	// ����д���󣬶���ָ��ָ����� 2/3 ����
/* find an empty request */
	/* ����һ���������� */
	// �Ӻ���ǰ������������ṹ request �� dev �ֶ�ֵ=-1 ʱ����ʾ����δ��ռ�á�
	while (--req >= request)
		if (req->dev<0)
			break;
/* if none found, sleep on new requests: check for rw_ahead */
	/* ���û���ҵ���������øô�������˯�ߣ������Ƿ���ǰ��/д */
	// ���û��һ���ǿ��еģ���ʱ request ����ָ���Ѿ�����Խ��ͷ��������鿴�˴������Ƿ���
	// ��ǰ��/д��READA �� WRITEA���������������˴����󡣷����ñ�������˯�ߣ��ȴ��������
	// �ڳ��������һ����������������С�
	if (req < request) {
		if (rw_ahead) {
			unlock_buffer(bh);
			return;
		}
		sleep_on(&wait_for_request);
		goto repeat;
	}
/* fill up the request-info, and add it to the queue */
	/* ���������������д������Ϣ���������������� */
	// ����ִ�е������ʾ���ҵ�һ���������������ṹ�μ���kernel/blk_drv/blk.h,23����
	req->dev = bh->b_dev;
	req->cmd = rw;
	req->errors=0;
	req->sector = bh->b_blocknr<<1;
	req->nr_sectors = 2;
	req->buffer = bh->b_data;
	req->waiting = NULL;
	req->bh = bh;
	req->next = NULL;
	add_request(major+blk_dev,req);
}

// �Ͳ��д���ݿ麯�����ǿ��豸��ϵͳ�������ֵĽӿں�����
// �ú�����fs/buffer.c�б����á���Ҫ�����Ǵ������豸��д��������뵽ָ�����豸��������С�
// ʵ�ʵĶ�д�����������豸�� request_fn()������ɡ�����Ӳ�̲������ú����� do_hd_request()��
// �������̲������ú����� do_fd_request()���������������� do_rd_request()��
// ���⣬��Ҫ��/д���豸����Ϣ�ѱ����ڻ����ͷ�ṹ�У����豸�š���š�
// ������rw �C READ��READA��WRITE �� WRITEA ���bh �C ���ݻ����ͷָ�롣
void ll_rw_block(int rw, struct buffer_head * bh)
{
	unsigned int major;
	// ����豸�����豸�Ų����ڻ��߸��豸�Ķ�д�������������ڣ�����ʾ������Ϣ�������ء�
	if ((major=MAJOR(bh->b_dev)) >= NR_BLK_DEV ||
	!(blk_dev[major].request_fn)) {
		printk("Trying to read nonexistent block-device\n\r");
		return;
	}
	make_request(major,rw,bh);// �������������������С�
}

// ���豸��ʼ���������ɳ�ʼ������ main.c ���ã�init/main.c,128����
// ��ʼ���������飬��������������Ϊ������(dev = -1)���� 32 ��(NR_REQUEST = 32)��
void blk_dev_init(void)
{
	int i;

	for (i=0 ; i<NR_REQUEST ; i++) {
		request[i].dev = -1;
		request[i].next = NULL;
	}
}
