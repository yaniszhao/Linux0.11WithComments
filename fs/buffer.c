/*
 *  linux/fs/buffer.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *  'buffer.c' implements the buffer-cache functions. Race-conditions have
 * been avoided by NEVER letting a interrupt change a buffer (except for the
 * data, of course), but instead letting the caller do it. NOTE! As interrupts
 * can wake up a caller, some cli-sti sequences are needed to check for
 * sleep-on-calls. These should be extremely quick, though (I hope).
 */

/*
 * 'buffer.c'����ʵ�ֻ��������ٻ��湦�ܡ�ͨ�������жϹ��̸ı仺�����������õ�����
 * ��ִ�У������˾�����������Ȼ���ı��������⣩��ע�⣡�����жϿ��Ի���һ�������ߣ�
 * ��˾���Ҫ�����ж�ָ�cli-sti�����������ȴ����÷��ء�����Ҫ�ǳ��ؿ�(ϣ��������)��
 */

/*
 * NOTE! There is one discordant note here: checking floppies for
 * disk change. This is where it fits best, I think, as it should
 * invalidate changed floppy-disk-caches.
 */

/*
 * ע�⣡������һ������Ӧ�����������������Ƿ������������������
 * ���øó�����õĵط��ˣ���Ϊ����Ҫʹ�Ѹ������̻���ʧЧ��
 */

#include <stdarg.h>
 
#include <linux/config.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/system.h>
#include <asm/io.h>

extern int end;	// �����ӳ��� ld �������ڱ����ں˴���ĩ�˵ı�����
struct buffer_head * start_buffer = (struct buffer_head *) &end;
struct buffer_head * hash_table[NR_HASH];	// �ѷ����
static struct buffer_head * free_list;		// ���е�
static struct task_struct * buffer_wait = NULL;
int NR_BUFFERS = 0;

// �ȴ�ָ��������������
static inline void wait_on_buffer(struct buffer_head * bh)
{
	cli();
	while (bh->b_lock)
		sleep_on(&bh->b_wait);
	sti();
}

// ϵͳ���á�ͬ���豸���ڴ���ٻ��������ݡ�
int sys_sync(void)
{
	int i;
	struct buffer_head * bh;

	sync_inodes();		/* write out inodes into buffers */	//�� i �ڵ�д����ٻ���
	// ɨ�����и��ٻ������������ѱ��޸ĵĻ�������д�����󣬽��������������豸��ͬ����
	bh = start_buffer;
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
		wait_on_buffer(bh);			// �ȴ�����������������������Ļ���
		if (bh->b_dirt)
			ll_rw_block(WRITE,bh);	// ����д�豸������
	}
	return 0;
}

// ��ָ���豸���и��ٻ����������豸�����ݵ�ͬ��������
int sync_dev(int dev)
{
	int i;
	struct buffer_head * bh;

	bh = start_buffer;
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
		if (bh->b_dev != dev)	//ָ���豸
			continue;
		wait_on_buffer(bh);
		if (bh->b_dev == dev && bh->b_dirt)
			ll_rw_block(WRITE,bh);
	}
	sync_inodes();	// �� i �ڵ�����д����ٻ��塣

	//Ϊʲô������дһ��? ��Ϊд����inode?
	bh = start_buffer;
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
		if (bh->b_dev != dev)
			continue;
		wait_on_buffer(bh);
		if (bh->b_dev == dev && bh->b_dirt)
			ll_rw_block(WRITE,bh);
	}
	return 0;
}

// ʹָ���豸�ڸ��ٻ������е�������Ч��
// ɨ����ٻ����е����л���飬����ָ���豸�Ļ���������λ����Ч(����)��־�����޸ı�־��
void inline invalidate_buffers(int dev)
{
	int i;
	struct buffer_head * bh;

	bh = start_buffer;
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
		if (bh->b_dev != dev)
			continue;
		wait_on_buffer(bh);
		// ���ڽ���ִ�й�˯�ߵȴ���������Ҫ���ж�һ�»������Ƿ���ָ���豸�ġ�
		if (bh->b_dev == dev)	
			bh->b_uptodate = bh->b_dirt = 0;
	}
}

/*
 * This routine checks whether a floppy has been changed, and
 * invalidates all buffer-cache-entries in that case. This
 * is a relatively slow routine, so we have to try to minimize using
 * it. Thus it is called only upon a 'mount' or 'open'. This
 * is the best way of combining speed and utility, I think.
 * People changing diskettes in the middle of an operation deserve
 * to loose :-)
 *
 * NOTE! Although currently this is only for floppies, the idea is
 * that any additional removable block-device will use this routine,
 * and that mount/open needn't know that floppies/whatever are
 * special.
 */
/*
 * ���ӳ�����һ�������Ƿ��Ѿ�������������Ѿ�������ʹ���ٻ������������
 * ��Ӧ�����л�������Ч�����ӳ��������˵��������������Ҫ������ʹ������
 * ���Խ���ִ��'mount'��'open'ʱ�ŵ��������������ǽ��ٶȺ�ʵ�������ϵ�
 * ��÷��������ڲ������̵��и������̣��ᵼ�����ݵĶ�ʧ�����Ǿ�����ȡ��
 *
 * ע�⣡����Ŀǰ���ӳ�����������̣��Ժ��κο��ƶ����ʵĿ��豸����ʹ�ø�
 * ����mount/open �����ǲ���Ҫ֪���Ƿ������̻�����ʲô������ʵġ�
 */
// �������Ƿ����������Ѹ�����ʹ��Ӧ���ٻ�������Ч��
void check_disk_change(int dev)
{
	int i;

	if (MAJOR(dev) != 2)			// �������豸������������˳���
		return;	
	if (!floppy_change(dev & 0x03))	// ���Զ�Ӧ�����Ƿ��Ѹ��������û�����˳���
		return;
	// �����Ѿ������������ͷŶ�Ӧ�豸�� i �ڵ�λͼ���߼���λͼ��ռ�ĸ��ٻ�������
	// ��ʹ���豸�� i �ڵ�����ݿ���Ϣ��ռ�ĸ��ٻ�������Ч��
	for (i=0 ; i<NR_SUPER ; i++)
		if (super_block[i].s_dev == dev)
			put_super(super_block[i].s_dev);
	invalidate_inodes(dev);
	invalidate_buffers(dev);
}

// hash ������ hash ����ļ���궨�塣
#define _hashfn(dev,block) (((unsigned)(dev^block))%NR_HASH)
#define hash(dev,block) hash_table[_hashfn(dev,block)]

// �� hash ���кͿ��л������������ָ���Ļ���顣
static inline void remove_from_queues(struct buffer_head * bh)
{
/* remove from hash-queue */
	/* �� hash �������Ƴ������ */
	if (bh->b_next)
		bh->b_next->b_prev = bh->b_prev;
	if (bh->b_prev)
		bh->b_prev->b_next = bh->b_next;
	// ����û������Ǹö��е�ͷһ���飬���� hash ��Ķ�Ӧ��ָ�򱾶����е���һ����������
	if (hash(bh->b_dev,bh->b_blocknr) == bh)
		hash(bh->b_dev,bh->b_blocknr) = bh->b_next;
/* remove from free list */	
	/* �ӿ��л����������Ƴ������ */
	if (!(bh->b_prev_free) || !(bh->b_next_free))
		panic("Free block list corrupted");
	bh->b_prev_free->b_next_free = bh->b_next_free;
	bh->b_next_free->b_prev_free = bh->b_prev_free;
	if (free_list == bh)	// �����������ͷָ�򱾻�������������ָ����һ��������
		free_list = bh->b_next_free;
}

// ��ָ�������������������β������ hash �����С�
static inline void insert_into_queues(struct buffer_head * bh)
{
/* put at end of free list */		// ͷ�嵽���б�ͷ
	bh->b_next_free = free_list;
	bh->b_prev_free = free_list->b_prev_free;
	free_list->b_prev_free->b_next_free = bh;
	free_list->b_prev_free = bh;
/* put the buffer in new hash-queue if it has a device */
/* ����û�����Ӧһ���豸����������� hash ������ */
	bh->b_prev = NULL;
	bh->b_next = NULL;
	if (!bh->b_dev)
		return;
	bh->b_next = hash(bh->b_dev,bh->b_blocknr);	//ͷ�嵽��Ӧ��hash��ͷ
	hash(bh->b_dev,bh->b_blocknr) = bh;
	bh->b_next->b_prev = bh;
}

// �ڸ��ٻ�����Ѱ�Ҹ����豸��ָ����Ļ������顣
// ����ҵ��򷵻ػ��������ָ�룬���򷵻� NULL��
static struct buffer_head * find_buffer(int dev, int block)
{		
	struct buffer_head * tmp;

	for (tmp = hash(dev,block) ; tmp != NULL ; tmp = tmp->b_next)
		if (tmp->b_dev==dev && tmp->b_blocknr==block)
			return tmp;
	return NULL;
}

/*
 * Why like this, I hear you say... The reason is race-conditions.
 * As we don't lock buffers (unless we are readint them, that is),
 * something might happen to it while we sleep (ie a read-error
 * will force it bad). This shouldn't really happen currently, but
 * the code is ready.
 */
/*
 * ����Ϊʲô���������ӵģ�����������... ԭ���Ǿ�����������������û�ж�
 * �����������������������ڶ�ȡ�����е����ݣ�����ô�����ǣ����̣�˯��ʱ
 * ���������ܻᷢ��һЩ���⣨����һ�������󽫵��¸û�����������Ŀǰ
 * �������ʵ�����ǲ��ᷢ���ģ�������Ĵ����Ѿ�׼�����ˡ�
 */
// ѭ�����ң��õ�ָ���豸��ָ�����ڻ������е�λ�á�
struct buffer_head * get_hash_table(int dev, int block)
{
	struct buffer_head * bh;

	for (;;) {
		if (!(bh=find_buffer(dev,block)))
			return NULL;
		// �Ըû������������ü��������ȴ��û���������������ѱ���������
		bh->b_count++;
		wait_on_buffer(bh);
		if (bh->b_dev == dev && bh->b_blocknr == block)
			return bh;
		bh->b_count--;//�ߵ�����waitǰ����黹�Ƕ�Ӧ�豸�ģ������Ͳ����ˡ�
	}
}

/*
 * Ok, this is getblk, and it isn't very clear, again to hinder
 * race-conditions. Most of the code is seldom used, (ie repeating),
 * so it should be much more efficient than it looks.
 *
 * The algoritm is changed: hopefully better, and an elusive bug removed.
 */
/*
 * OK�������� getblk �������ú������߼������Ǻ�������ͬ��Ҳ����ΪҪ����
 * �����������⡣���д󲿷ִ�������õ���(�����ظ��������)�������Ӧ��
 * �ȿ���ȥ��������Ч�öࡣ
 *
 * �㷨�Ѿ����˸ı䣺ϣ���ܸ��ã�����һ��������ĥ�Ĵ����Ѿ�ȥ����
 */
// ����궨������ͬʱ�жϻ��������޸ı�־��������־�����Ҷ����޸ı�־��Ȩ��Ҫ��������־��
#define BADNESS(bh) (((bh)->b_dirt<<1)+(bh)->b_lock)
// ȡָ���豸��ָ����Ļ�������
// �Ѿ��ڸ��ٻ�����ֱ�ӷ��أ���ʱ�������д���ָ��������ݣ�
// �������Ҫ�ڸ��ٻ����н���һ����Ӧ�������ʱ�������л�û�ж�Ӧ�����ݡ�
struct buffer_head * getblk(int dev,int block)
{
	struct buffer_head * tmp, * bh;

repeat:
	if (bh = get_hash_table(dev,block))			//�Ѿ��ڸ��ٻ����У�ֱ�ӷ���
		return bh;
	tmp = free_list;							//û�ڻ����У���Ҫ�½�
	do {
		if (tmp->b_count)						//������ʹ�õĿ��п�
			continue;
		if (!bh || BADNESS(tmp)<BADNESS(bh)) {	//����Clock�㷨?
			bh = tmp;
			if (!BADNESS(tmp))					//��û���޸���û����������ֱ�ӷ�����
				break;
		}
/* and repeat until we find something good */
	} while ((tmp = tmp->b_next_free) != free_list);
	if (!bh) {			//û���ҵ����ʿ��п�
		sleep_on(&buffer_wait);
		goto repeat;
	}
	wait_on_buffer(bh);	// �к��ʵĿ飬�ȴ��û���������������ѱ������Ļ�����
	if (bh->b_count)	// ����û������ֱ���������ʹ�õĻ���ֻ���ظ��������̡�
		goto repeat;
	// ����û������ѱ��޸ģ�������д�̣����ٴεȴ�������������
	// ����û������ֱ���������ʹ�õĻ���ֻ�����ظ��������̡�
	while (bh->b_dirt) {
		sync_dev(bh->b_dev);//��д���豸
		wait_on_buffer(bh);
		if (bh->b_count)	//�ȴ���������Ҫ�ж��±�������û
			goto repeat;
	}
/* NOTE!! While we slept waiting for this block, somebody else might */
/* already have added "this" block to the cache. check it */
	/* ע�⣡��������Ϊ�˵ȴ��û�����˯��ʱ���������̿����Ѿ����û���� */
	/* ��������ٻ����У�����Ҫ�Դ˽��м�顣*/
	// �ڸ��ٻ��� hash ���м��ָ���豸�Ϳ�Ļ������Ƿ��Ѿ��������ȥ��
	// ����ǵĻ������ٴ��ظ��������̡�
	if (find_buffer(dev,block))
		goto repeat;
/* OK, FINALLY we know that this buffer is the only one of it's kind, */
/* and that it's unused (b_count=0), unlocked (b_lock=0), and clean */
	/* OK����������֪���û�������ָ��������Ψһһ�飬*/
	/* ���һ�û�б�ʹ��(b_count=0)��δ������(b_lock=0)�������Ǹɾ��ģ�δ���޸ĵģ�*/
	// ����������ռ�ô˻������������ü���Ϊ 1����λ�޸ı�־����Ч(����)��־��
	bh->b_count=1;
	bh->b_dirt=0;
	bh->b_uptodate=0;
	remove_from_queues(bh);//�ӿ��б��к�hash����ȡ�����
	bh->b_dev=dev;
	bh->b_blocknr=block;
	insert_into_queues(bh);//�����ͷ�嵽���б��hash����
	return bh;
}

// �ͷ�ָ���Ļ����������ѵȴ����л������Ľ��̡�
// ֻ��ҪҪ���ü����ݼ� 1����ʹ����0Ҳ�����ϴ���
void brelse(struct buffer_head * buf)
{
	if (!buf)
		return;
	wait_on_buffer(buf);
	if (!(buf->b_count--))
		panic("Trying to free free buffer");
	wake_up(&buffer_wait);
}

/*
 * bread() reads a specified block and returns the buffer that contains
 * it. It returns NULL if the block was unreadable.
 */
/*
 * ���豸�϶�ȡָ�������ݿ鲢���غ������ݵĻ�������
 */
 // ���豸�϶�ȡָ�������ݿ鲢���غ������ݵĻ�������
struct buffer_head * bread(int dev,int block)
{
	struct buffer_head * bh;

	if (!(bh=getblk(dev,block)))
		panic("bread: getblk returned NULL\n");
	if (bh->b_uptodate)	// ����û������е���������Ч�ģ��Ѹ��µģ�����ֱ��ʹ�ã��򷵻ء�
		return bh;
	ll_rw_block(READ,bh);//�Ӷ�Ӧ�豸��ȡ���ݽ�buffer
	wait_on_buffer(bh);
	if (bh->b_uptodate)	//�����Ѿ����½�����
		return bh;
	brelse(bh);	// ����������豸����ʧ�ܣ��ͷŸû�����
	return NULL;
}

// �����ڴ�顣
// �� from ��ַ����һ�����ݵ� to λ�á�ע��from��to�����ں˵ĵ�ַƫ�ƣ��������û��ġ�
#define COPYBLK(from,to) \
__asm__("cld\n\t" \
	"rep\n\t" \
	"movsl\n\t" \
	::"c" (BLOCK_SIZE/4),"S" (from),"D" (to) \
	:"cx","di","si")

/*
 * bread_page reads four buffers into memory at the desired address. It's
 * a function of its own, as there is some speed to be got by reading them
 * all at the same time, not waiting for one to be read, and then another
 * etc.
 */
/*
 * bread_page һ�ζ��ĸ���������ݶ����ڴ�ָ���ĵ�ַ������һ�������ĺ�����
 * ��Ϊͬʱ��ȡ�Ŀ���Ի���ٶ��ϵĺô������õ��Ŷ�һ�飬�ٶ�һ���ˡ�
 */
 // ���豸��һ��ҳ�棨4 ������飩�����ݵ��ڴ�ָ���ĵ�ַ��
void bread_page(unsigned long address,int dev,int b[4])
{
	struct buffer_head * bh[4];//һҳ���ĸ���
	int i;

	for (i=0 ; i<4 ; i++)
		if (b[i]) {							//b[i]�����ݵ����豸�Ŀ��
			if (bh[i] = getblk(dev,b[i]))	//û����buffer�������
				if (!bh[i]->b_uptodate)
					ll_rw_block(READ,bh[i]);
		} else
			bh[i] = NULL;
	for (i=0 ; i<4 ; i++,address += BLOCK_SIZE)
		if (bh[i]) {
			wait_on_buffer(bh[i]);
			if (bh[i]->b_uptodate)
				COPYBLK((unsigned long) bh[i]->b_data,address);
			brelse(bh[i]);// ����Ϳ����ͷŸû������ˡ�
		}
}

/*
 * Ok, breada can be used as bread, but additionally to mark other
 * blocks for reading as well. End the argument list with a negative
 * number.
 */
/*
 * OK��breada ������ bread һ��ʹ�ã���������Ԥ��һЩ�顣
 * �ú��������б���Ҫʹ��һ�����������������б�Ľ�����
 */
// ��ָ���豸��ȡָ����һЩ�顣
// �ɹ�ʱ���ص� 1 ��Ļ�����ͷָ�룬���򷵻� NULL��
struct buffer_head * breada(int dev,int first, ...)
{
	va_list args;
	struct buffer_head * bh, *tmp;

	va_start(args,first);						//�õ��ɱ�����ĵ�һ������
	// ����һ����
	if (!(bh=getblk(dev,first)))
		panic("bread: getblk returned NULL\n");
	if (!bh->b_uptodate)
		ll_rw_block(READ,bh);
	//�ڵȴ���ȡ�豸���ʱ�򣬲��л����̣������ȸɵ���������
	while ((first=va_arg(args,int))>=0) {	//���һ��������-1��ʾ��β
		tmp=getblk(dev,first);
		if (tmp) {
			if (!tmp->b_uptodate)
				ll_rw_block(READA,bh);
			tmp->b_count--;
		}
	}
	va_end(args);
	wait_on_buffer(bh);	//���ʱ��ſ��ǰ�CPU�ó�ȥ
	if (bh->b_uptodate)	//�Ѿ����غ���
		return bh;
	brelse(bh);//����ʧ�ܣ��ͷ�
	return (NULL);
}

// ��������ʼ��������
// ���� buffer_end ��ָ���Ļ������ڴ��ĩ�ˡ�����ϵͳ�� 16MB �ڴ棬�򻺳���ĩ������Ϊ 4MB��
// ����ϵͳ�� 8MB �ڴ棬������ĩ������Ϊ 2MB��
void buffer_init(long buffer_end)
{
	struct buffer_head * h = start_buffer;
	void * b; //������������endλ��
	int i;

	// ����������߶˵��� 1Mb�������ڴ� 640KB-1MB ����ʾ�ڴ�� BIOS ռ�ã�
	// ���ʵ�ʿ��û������ڴ�߶�Ӧ���� 640KB�������ڴ�߶�һ������ 1MB��
	if (buffer_end == 1<<20)
		b = (void *) (640*1024);
	else
		b = (void *) buffer_end;

	// ��δ������ڳ�ʼ�����������������л���������������ȡϵͳ�л�������Ŀ��
	// �����Ĺ����Ǵӻ������߶˿�ʼ���� 1K ��С�Ļ���飬���ͬʱ�ڻ������Ͷ˽��������û����
	// �Ľṹ buffer_head��������Щ buffer_head ���˫������
	// h ��ָ�򻺳�ͷ�ṹ��ָ�룬�� h+1 ��ָ���ڴ��ַ��������һ������ͷ��ַ��Ҳ����˵��ָ�� h
	// ����ͷ��ĩ���⡣Ϊ�˱�֤���㹻���ȵ��ڴ����洢һ������ͷ�ṹ����Ҫ b ��ָ����ڴ��
	// ��ַ >= h ����ͷ��ĩ�ˣ�Ҳ��Ҫ>=h+1��
	while ( (b -= BLOCK_SIZE) >= ((void *) (h+1)) ) {//ʵ�ʿ�ĵ�ַҪ����ͷ�����ĵ�ַ
		h->b_dev = 0;			//free
		h->b_dirt = 0;
		h->b_count = 0;
		h->b_lock = 0;
		h->b_uptodate = 0;		//������
		h->b_wait = NULL;
		h->b_next = NULL;
		h->b_prev = NULL;
		h->b_data = (char *) b;	//ʵ�ʵĻ����
		h->b_prev_free = h-1;	//��Ϊ�ǵ�һ�Σ�����ֱ��ָ��������Ϳ�����
		h->b_next_free = h+1;
		h++;
		NR_BUFFERS++;
		if (b == (void *) 0x100000)	//Ҫ����640KB��1MB�Ŀռ�
			b = (void *) 0xA0000;
	}
	h--;
	free_list = start_buffer;
	free_list->b_prev_free = h; //��һ����
	h->b_next_free = free_list;
	for (i=0;i<NR_HASH;i++)		//��ʼhash��
		hash_table[i]=NULL;
}	
