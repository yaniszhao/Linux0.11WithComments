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
 * 'buffer.c'用于实现缓冲区高速缓存功能。通过不让中断过程改变缓冲区，而是让调用者
 * 来执行，避免了竞争条件（当然除改变数据以外）。注意！由于中断可以唤醒一个调用者，
 * 因此就需要开关中断指令（cli-sti）序列来检测等待调用返回。但需要非常地快(希望是这样)。
 */

/*
 * NOTE! There is one discordant note here: checking floppies for
 * disk change. This is where it fits best, I think, as it should
 * invalidate changed floppy-disk-caches.
 */

/*
 * 注意！这里有一个程序应不属于这里：检测软盘是否更换。但我想这里是
 * 放置该程序最好的地方了，因为它需要使已更换软盘缓冲失效。
 */

#include <stdarg.h>
 
#include <linux/config.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/system.h>
#include <asm/io.h>

extern int end;	// 由连接程序 ld 生成用于表明内核代码末端的变量。
struct buffer_head * start_buffer = (struct buffer_head *) &end;
struct buffer_head * hash_table[NR_HASH];	// 已分配的
static struct buffer_head * free_list;		// 空闲的
static struct task_struct * buffer_wait = NULL;
int NR_BUFFERS = 0;

// 等待指定缓冲区解锁。
static inline void wait_on_buffer(struct buffer_head * bh)
{
	cli();
	while (bh->b_lock)
		sleep_on(&bh->b_wait);
	sti();
}

// 系统调用。同步设备和内存高速缓冲中数据。
int sys_sync(void)
{
	int i;
	struct buffer_head * bh;

	sync_inodes();		/* write out inodes into buffers */	//将 i 节点写入高速缓冲
	// 扫描所有高速缓冲区，对于已被修改的缓冲块产生写盘请求，将缓冲中数据与设备中同步。
	bh = start_buffer;
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
		wait_on_buffer(bh);			// 等待缓冲区解锁（如果已上锁的话）
		if (bh->b_dirt)
			ll_rw_block(WRITE,bh);	// 产生写设备块请求
	}
	return 0;
}

// 对指定设备进行高速缓冲数据与设备上数据的同步操作。
int sync_dev(int dev)
{
	int i;
	struct buffer_head * bh;

	bh = start_buffer;
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
		if (bh->b_dev != dev)	//指定设备
			continue;
		wait_on_buffer(bh);
		if (bh->b_dev == dev && bh->b_dirt)
			ll_rw_block(WRITE,bh);
	}
	sync_inodes();	// 将 i 节点数据写入高速缓冲。

	//为什么又重新写一次? 因为写入了inode?
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

// 使指定设备在高速缓冲区中的数据无效。
// 扫描高速缓冲中的所有缓冲块，对于指定设备的缓冲区，复位其有效(更新)标志和已修改标志。
void inline invalidate_buffers(int dev)
{
	int i;
	struct buffer_head * bh;

	bh = start_buffer;
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
		if (bh->b_dev != dev)
			continue;
		wait_on_buffer(bh);
		// 由于进程执行过睡眠等待，所以需要再判断一下缓冲区是否是指定设备的。
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
 * 该子程序检查一个软盘是否已经被更换，如果已经更换就使高速缓冲中与该软驱
 * 对应的所有缓冲区无效。该子程序相对来说较慢，所以我们要尽量少使用它。
 * 所以仅在执行'mount'或'open'时才调用它。我想这是将速度和实用性相结合的
 * 最好方法。若在操作过程当中更换软盘，会导致数据的丢失，这是咎由自取。
 *
 * 注意！尽管目前该子程序仅用于软盘，以后任何可移动介质的块设备都将使用该
 * 程序，mount/open 操作是不需要知道是否是软盘或其它什么特殊介质的。
 */
// 检查磁盘是否更换，如果已更换就使对应高速缓冲区无效。
void check_disk_change(int dev)
{
	int i;

	if (MAJOR(dev) != 2)			// 是软盘设备吗？如果不是则退出。
		return;	
	if (!floppy_change(dev & 0x03))	// 测试对应软盘是否已更换，如果没有则退出。
		return;
	// 软盘已经更换，所以释放对应设备的 i 节点位图和逻辑块位图所占的高速缓冲区；
	// 并使该设备的 i 节点和数据块信息所占的高速缓冲区无效。
	for (i=0 ; i<NR_SUPER ; i++)
		if (super_block[i].s_dev == dev)
			put_super(super_block[i].s_dev);
	invalidate_inodes(dev);
	invalidate_buffers(dev);
}

// hash 函数和 hash 表项的计算宏定义。
#define _hashfn(dev,block) (((unsigned)(dev^block))%NR_HASH)
#define hash(dev,block) hash_table[_hashfn(dev,block)]

// 从 hash 队列和空闲缓冲队列中移走指定的缓冲块。
static inline void remove_from_queues(struct buffer_head * bh)
{
/* remove from hash-queue */
	/* 从 hash 队列中移除缓冲块 */
	if (bh->b_next)
		bh->b_next->b_prev = bh->b_prev;
	if (bh->b_prev)
		bh->b_prev->b_next = bh->b_next;
	// 如果该缓冲区是该队列的头一个块，则让 hash 表的对应项指向本队列中的下一个缓冲区。
	if (hash(bh->b_dev,bh->b_blocknr) == bh)
		hash(bh->b_dev,bh->b_blocknr) = bh->b_next;
/* remove from free list */	
	/* 从空闲缓冲区表中移除缓冲块 */
	if (!(bh->b_prev_free) || !(bh->b_next_free))
		panic("Free block list corrupted");
	bh->b_prev_free->b_next_free = bh->b_next_free;
	bh->b_next_free->b_prev_free = bh->b_prev_free;
	if (free_list == bh)	// 如果空闲链表头指向本缓冲区，则让其指向下一缓冲区。
		free_list = bh->b_next_free;
}

// 将指定缓冲区插入空闲链表尾并放入 hash 队列中。
static inline void insert_into_queues(struct buffer_head * bh)
{
/* put at end of free list */		// 头插到空闲表头
	bh->b_next_free = free_list;
	bh->b_prev_free = free_list->b_prev_free;
	free_list->b_prev_free->b_next_free = bh;
	free_list->b_prev_free = bh;
/* put the buffer in new hash-queue if it has a device */
/* 如果该缓冲块对应一个设备，则将其插入新 hash 队列中 */
	bh->b_prev = NULL;
	bh->b_next = NULL;
	if (!bh->b_dev)
		return;
	bh->b_next = hash(bh->b_dev,bh->b_blocknr);	//头插到对应的hash表头
	hash(bh->b_dev,bh->b_blocknr) = bh;
	bh->b_next->b_prev = bh;
}

// 在高速缓冲中寻找给定设备和指定块的缓冲区块。
// 如果找到则返回缓冲区块的指针，否则返回 NULL。
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
 * 代码为什么会是这样子的？我听见你问... 原因是竞争条件。由于我们没有对
 * 缓冲区上锁（除非我们正在读取它们中的数据），那么当我们（进程）睡眠时
 * 缓冲区可能会发生一些问题（例如一个读错误将导致该缓冲区出错）。目前
 * 这种情况实际上是不会发生的，但处理的代码已经准备好了。
 */
// 循环查找，得到指定设备的指定块在缓冲区中的位置。
struct buffer_head * get_hash_table(int dev, int block)
{
	struct buffer_head * bh;

	for (;;) {
		if (!(bh=find_buffer(dev,block)))
			return NULL;
		// 对该缓冲区增加引用计数，并等待该缓冲区解锁（如果已被上锁）。
		bh->b_count++;
		wait_on_buffer(bh);
		if (bh->b_dev == dev && bh->b_blocknr == block)
			return bh;
		bh->b_count--;//走到这是wait前这个块还是对应设备的，回来就不是了。
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
 * OK，下面是 getblk 函数，该函数的逻辑并不是很清晰，同样也是因为要考虑
 * 竞争条件问题。其中大部分代码很少用到，(例如重复操作语句)，因此它应该
 * 比看上去的样子有效得多。
 *
 * 算法已经作了改变：希望能更好，而且一个难以琢磨的错误已经去除。
 */
// 下面宏定义用于同时判断缓冲区的修改标志和锁定标志，并且定义修改标志的权重要比锁定标志大。
#define BADNESS(bh) (((bh)->b_dirt<<1)+(bh)->b_lock)
// 取指定设备的指定块的缓冲区。
// 已经在高速缓冲中直接返回，此时缓冲区中带有指定块的数据；
// 否则就需要在高速缓冲中建立一个对应的新项，此时缓冲区中还没有对应的数据。
struct buffer_head * getblk(int dev,int block)
{
	struct buffer_head * tmp, * bh;

repeat:
	if (bh = get_hash_table(dev,block))			//已经在高速缓存中，直接返回
		return bh;
	tmp = free_list;							//没在缓存中，需要新建
	do {
		if (tmp->b_count)						//跳过被使用的空闲块
			continue;
		if (!bh || BADNESS(tmp)<BADNESS(bh)) {	//类似Clock算法?
			bh = tmp;
			if (!BADNESS(tmp))					//即没被修改又没被锁，可以直接返回了
				break;
		}
/* and repeat until we find something good */
	} while ((tmp = tmp->b_next_free) != free_list);
	if (!bh) {			//没有找到合适空闲块
		sleep_on(&buffer_wait);
		goto repeat;
	}
	wait_on_buffer(bh);	// 有合适的块，等待该缓冲区解锁（如果已被上锁的话）。
	if (bh->b_count)	// 如果该缓冲区又被其它任务使用的话，只好重复上述过程。
		goto repeat;
	// 如果该缓冲区已被修改，则将数据写盘，并再次等待缓冲区解锁。
	// 如果该缓冲区又被其它任务使用的话，只好再重复上述过程。
	while (bh->b_dirt) {
		sync_dev(bh->b_dev);//回写到设备
		wait_on_buffer(bh);
		if (bh->b_count)	//等待回来还是要判断下被别人用没
			goto repeat;
	}
/* NOTE!! While we slept waiting for this block, somebody else might */
/* already have added "this" block to the cache. check it */
	/* 注意！！当进程为了等待该缓冲块而睡眠时，其它进程可能已经将该缓冲块 */
	/* 加入进高速缓冲中，所以要对此进行检查。*/
	// 在高速缓冲 hash 表中检查指定设备和块的缓冲区是否已经被加入进去。
	// 如果是的话，就再次重复上述过程。
	if (find_buffer(dev,block))
		goto repeat;
/* OK, FINALLY we know that this buffer is the only one of it's kind, */
/* and that it's unused (b_count=0), unlocked (b_lock=0), and clean */
	/* OK，最终我们知道该缓冲区是指定参数的唯一一块，*/
	/* 而且还没有被使用(b_count=0)，未被上锁(b_lock=0)，并且是干净的（未被修改的）*/
	// 于是让我们占用此缓冲区。置引用计数为 1，复位修改标志和有效(更新)标志。
	bh->b_count=1;
	bh->b_dirt=0;
	bh->b_uptodate=0;
	remove_from_queues(bh);//从空闲表中和hash表中取出结点
	bh->b_dev=dev;
	bh->b_blocknr=block;
	insert_into_queues(bh);//将结点头插到空闲表和hash表中
	return bh;
}

// 释放指定的缓冲区。唤醒等待空闲缓冲区的进程。
// 只需要要引用计数递减 1。即使减到0也不马上处理。
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
 * 从设备上读取指定的数据块并返回含有数据的缓冲区。
 */
 // 从设备上读取指定的数据块并返回含有数据的缓冲区。
struct buffer_head * bread(int dev,int block)
{
	struct buffer_head * bh;

	if (!(bh=getblk(dev,block)))
		panic("bread: getblk returned NULL\n");
	if (bh->b_uptodate)	// 如果该缓冲区中的数据是有效的（已更新的）可以直接使用，则返回。
		return bh;
	ll_rw_block(READ,bh);//从对应设备读取数据进buffer
	wait_on_buffer(bh);
	if (bh->b_uptodate)	//数据已经更新进来了
		return bh;
	brelse(bh);	// 否则表明读设备操作失败，释放该缓冲区
	return NULL;
}

// 复制内存块。
// 从 from 地址复制一块数据到 to 位置。注意from和to都是内核的地址偏移，而不是用户的。
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
 * bread_page 一次读四个缓冲块内容读到内存指定的地址。它是一个完整的函数，
 * 因为同时读取四块可以获得速度上的好处，不用等着读一块，再读一块了。
 */
 // 读设备上一个页面（4 个缓冲块）的内容到内存指定的地址。
void bread_page(unsigned long address,int dev,int b[4])
{
	struct buffer_head * bh[4];//一页，四个块
	int i;

	for (i=0 ; i<4 ; i++)
		if (b[i]) {							//b[i]是数据的在设备的块号
			if (bh[i] = getblk(dev,b[i]))	//没读进buffer则读进来
				if (!bh[i]->b_uptodate)
					ll_rw_block(READ,bh[i]);
		} else
			bh[i] = NULL;
	for (i=0 ; i<4 ; i++,address += BLOCK_SIZE)
		if (bh[i]) {
			wait_on_buffer(bh[i]);
			if (bh[i]->b_uptodate)
				COPYBLK((unsigned long) bh[i]->b_data,address);
			brelse(bh[i]);// 读完就可以释放该缓冲区了。
		}
}

/*
 * Ok, breada can be used as bread, but additionally to mark other
 * blocks for reading as well. End the argument list with a negative
 * number.
 */
/*
 * OK，breada 可以象 bread 一样使用，但会另外预读一些块。
 * 该函数参数列表需要使用一个负数来表明参数列表的结束。
 */
// 从指定设备读取指定的一些块。
// 成功时返回第 1 块的缓冲区头指针，否则返回 NULL。
struct buffer_head * breada(int dev,int first, ...)
{
	va_list args;
	struct buffer_head * bh, *tmp;

	va_start(args,first);						//得到可变参数的第一个参数
	// 读第一个块
	if (!(bh=getblk(dev,first)))
		panic("bread: getblk returned NULL\n");
	if (!bh->b_uptodate)
		ll_rw_block(READ,bh);
	//在等待读取设备块的时候，不切换进程，而是先干点其他的事
	while ((first=va_arg(args,int))>=0) {	//最后一个参数是-1表示结尾
		tmp=getblk(dev,first);
		if (tmp) {
			if (!tmp->b_uptodate)
				ll_rw_block(READA,bh);
			tmp->b_count--;
		}
	}
	va_end(args);
	wait_on_buffer(bh);	//这个时候才考虑把CPU让出去
	if (bh->b_uptodate)	//已经加载好了
		return bh;
	brelse(bh);//加载失败，释放
	return (NULL);
}

// 缓冲区初始化函数。
// 参数 buffer_end 是指定的缓冲区内存的末端。对于系统有 16MB 内存，则缓冲区末端设置为 4MB。
// 对于系统有 8MB 内存，缓冲区末端设置为 2MB。
void buffer_init(long buffer_end)
{
	struct buffer_head * h = start_buffer;
	void * b; //缓冲区真正的end位置
	int i;

	// 如果缓冲区高端等于 1Mb，则由于从 640KB-1MB 被显示内存和 BIOS 占用，
	// 因此实际可用缓冲区内存高端应该是 640KB。否则内存高端一定大于 1MB。
	if (buffer_end == 1<<20)
		b = (void *) (640*1024);
	else
		b = (void *) buffer_end;

	// 这段代码用于初始化缓冲区，建立空闲缓冲区环链表，并获取系统中缓冲块的数目。
	// 操作的过程是从缓冲区高端开始划分 1K 大小的缓冲块，与此同时在缓冲区低端建立描述该缓冲块
	// 的结构 buffer_head，并将这些 buffer_head 组成双向链表。
	// h 是指向缓冲头结构的指针，而 h+1 是指向内存地址连续的下一个缓冲头地址，也可以说是指向 h
	// 缓冲头的末端外。为了保证有足够长度的内存来存储一个缓冲头结构，需要 b 所指向的内存块
	// 地址 >= h 缓冲头的末端，也即要>=h+1。
	while ( (b -= BLOCK_SIZE) >= ((void *) (h+1)) ) {//实际块的地址要大于头部结点的地址
		h->b_dev = 0;			//free
		h->b_dirt = 0;
		h->b_count = 0;
		h->b_lock = 0;
		h->b_uptodate = 0;		//无数据
		h->b_wait = NULL;
		h->b_next = NULL;
		h->b_prev = NULL;
		h->b_data = (char *) b;	//实际的缓冲块
		h->b_prev_free = h-1;	//因为是第一次，所以直接指向相邻项就可以了
		h->b_next_free = h+1;
		h++;
		NR_BUFFERS++;
		if (b == (void *) 0x100000)	//要除开640KB到1MB的空间
			b = (void *) 0xA0000;
	}
	h--;
	free_list = start_buffer;
	free_list->b_prev_free = h; //成一个环
	h->b_next_free = free_list;
	for (i=0;i<NR_HASH;i++)		//初始hash表
		hash_table[i]=NULL;
}	
