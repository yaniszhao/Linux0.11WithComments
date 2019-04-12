/*
 *  linux/fs/super.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * super.c contains code to handle the super-block tables.
 */
#include <linux/config.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/system.h>

#include <errno.h>
#include <sys/stat.h>

int sync_dev(int dev);
void wait_for_keypress(void);

/* set_bit uses setb, as gas doesn't recognize setc */
/* set_bit()ʹ���� setb ָ���Ϊ�������� gas ����ʶ��ָ�� setc */
// ����ָ��λƫ�ƴ�����λ��ֵ(0 �� 1)�������ظñ���λֵ��(Ӧ��ȡ��Ϊ test_bit()������)
// Ƕ��ʽ���ꡣ���� bitnr �Ǳ���λƫ��ֵ��addr �ǲ��Ա���λ��������ʼ��ַ��
// %0 - ax(__res)��%1 - 0��%2 - bitnr��%3 - addr
#define set_bit(bitnr,addr) ({ \
register int __res __asm__("ax"); \
__asm__("bt %2,%3;setb %%al":"=a" (__res):"a" (0),"r" (bitnr),"m" (*(addr))); \
__res; })

struct super_block super_block[NR_SUPER];			// ������ṹ���飨�� 8 �
/* this is initialized in init/main.c */
/* ROOT_DEV ���� init/main.c �б���ʼ�� */
int ROOT_DEV = 0;

// ����ָ���ĳ����顣
static void lock_super(struct super_block * sb)
{
	cli();
	while (sb->s_lock)
		sleep_on(&(sb->s_wait));
	sb->s_lock = 1;
	sti();
}

// ��ָ������������������ʹ�� unlock_super ������������������
static void free_super(struct super_block * sb)
{
	cli();
	sb->s_lock = 0;
	wake_up(&(sb->s_wait));
	sti();
}

// ˯�ߵȴ������������
static void wait_on_super(struct super_block * sb)
{
	cli();
	while (sb->s_lock)
		sleep_on(&(sb->s_wait));
	sti();
}

// ȡָ���豸�ĳ����顣���ظó�����ṹָ�롣
struct super_block * get_super(int dev)
{
	struct super_block * s;

	if (!dev)
		return NULL;
	s = 0+super_block;
	while (s < NR_SUPER+super_block)
		// �����ǰ��������ָ���豸�ĳ����飬�����ȵȴ��ó�������������Ѿ����������������Ļ�����
		// �ڵȴ��ڼ䣬�ó������п��ܱ������豸ʹ�ã���˴�ʱ�����ж�һ���Ƿ���ָ���豸�ĳ����飬
		// ������򷵻ظó������ָ�롣��������¶Գ���������������һ�飬��� s ����ָ�򳬼�������
		// ��ʼ����
		if (s->s_dev == dev) {
			wait_on_super(s);
			if (s->s_dev == dev)
				return s;
			s = 0+super_block;
		} else
			s++;
	return NULL;
}

// �ͷ�ָ���豸�ĳ����顣
// �ͷ��豸��ʹ�õĳ������������ s_dev=0�������ͷŸ��豸 i �ڵ�λͼ���߼���λͼ��ռ��
// �ĸ��ٻ���顣����������Ӧ���ļ�ϵͳ�Ǹ��ļ�ϵͳ�������� i �ڵ����Ѿ���װ���������ļ�
// ϵͳ�������ͷŸó����顣
void put_super(int dev)
{
	struct super_block * sb;
	struct m_inode * inode;
	int i;
	// ���ָ���豸�Ǹ��ļ�ϵͳ�豸������ʾ������Ϣ����ϵͳ�̸ı��ˣ�׼��������ս�ɡ��������ء�
	if (dev == ROOT_DEV) {
		printk("root diskette changed: prepare for armageddon\n\r");
		return;
	}
	// ����Ҳ���ָ���豸�ĳ����飬�򷵻ء�
	if (!(sb = get_super(dev)))
		return;
	// ����ó�����ָ�����ļ�ϵͳ i �ڵ��ϰ�װ���������ļ�ϵͳ������ʾ������Ϣ�����ء�
	if (sb->s_imount) {
		printk("Mounted disk changed - tssk, tssk\n\r");
		return;
	}
	// �ҵ�ָ���豸�ĳ���������������ó����飬Ȼ���øó������Ӧ���豸���ֶ�Ϊ 0��
	// Ҳ�����������ó����顣
	lock_super(sb);
	sb->s_dev = 0;
	// Ȼ���ͷŸ��豸 i �ڵ�λͼ���߼���λͼ�ڻ���������ռ�õĻ���顣
	for(i=0;i<I_MAP_SLOTS;i++)
		brelse(sb->s_imap[i]);
	for(i=0;i<Z_MAP_SLOTS;i++)
		brelse(sb->s_zmap[i]);
	// ���Ըó���������������ء�
	free_super(sb);
	return;
}

// ���豸�϶�ȡ�����鵽�������С�
// ������豸�ĳ������Ѿ��ڸ��ٻ����в�����Ч����ֱ�ӷ��ظó������ָ�롣
static struct super_block * read_super(int dev)
{
	struct super_block * s;
	struct buffer_head * bh;
	int i,block;

	if (!dev)
		return NULL;
	check_disk_change(dev);
	if (s = get_super(dev))
		return s;
	for (s = 0+super_block ;; s++) {
		if (s >= NR_SUPER+super_block)
			return NULL;
		if (!s->s_dev)
			break;
	}
	s->s_dev = dev;
	s->s_isup = NULL;
	s->s_imount = NULL;
	s->s_time = 0;
	s->s_rd_only = 0;
	s->s_dirt = 0;
	lock_super(s);
	if (!(bh = bread(dev,1))) {
		s->s_dev=0;
		free_super(s);
		return NULL;
	}
	*((struct d_super_block *) s) =
		*((struct d_super_block *) bh->b_data);
	brelse(bh);
	if (s->s_magic != SUPER_MAGIC) {
		s->s_dev = 0;
		free_super(s);
		return NULL;
	}
	for (i=0;i<I_MAP_SLOTS;i++)
		s->s_imap[i] = NULL;
	for (i=0;i<Z_MAP_SLOTS;i++)
		s->s_zmap[i] = NULL;
	block=2;
	for (i=0 ; i < s->s_imap_blocks ; i++)
		if (s->s_imap[i]=bread(dev,block))
			block++;
		else
			break;
	for (i=0 ; i < s->s_zmap_blocks ; i++)
		if (s->s_zmap[i]=bread(dev,block))
			block++;
		else
			break;
	if (block != 2+s->s_imap_blocks+s->s_zmap_blocks) {
		for(i=0;i<I_MAP_SLOTS;i++)
			brelse(s->s_imap[i]);
		for(i=0;i<Z_MAP_SLOTS;i++)
			brelse(s->s_zmap[i]);
		s->s_dev=0;
		free_super(s);
		return NULL;
	}
	s->s_imap[0]->b_data[0] |= 1;
	s->s_zmap[0]->b_data[0] |= 1;
	free_super(s);
	return s;
}

// ж���ļ�ϵͳ��ϵͳ���ú�����
// ���� dev_name ���豸�ļ�����
int sys_umount(char * dev_name)
{
	struct m_inode * inode;
	struct super_block * sb;
	int dev;

	if (!(inode=namei(dev_name)))
		return -ENOENT;
	dev = inode->i_zone[0];
	if (!S_ISBLK(inode->i_mode)) {
		iput(inode);
		return -ENOTBLK;
	}
	iput(inode);
	if (dev==ROOT_DEV)
		return -EBUSY;
	if (!(sb=get_super(dev)) || !(sb->s_imount))
		return -ENOENT;
	if (!sb->s_imount->i_mount)
		printk("Mounted inode has i_mount=0\n");
	for (inode=inode_table+0 ; inode<inode_table+NR_INODE ; inode++)
		if (inode->i_dev==dev && inode->i_count)
				return -EBUSY;
	sb->s_imount->i_mount=0;
	iput(sb->s_imount);
	sb->s_imount = NULL;
	iput(sb->s_isup);
	sb->s_isup = NULL;
	put_super(dev);
	sync_dev(dev);
	return 0;
}

// ��װ�ļ�ϵͳ���ú�����
// ���� dev_name ���豸�ļ�����dir_name �ǰ�װ����Ŀ¼����rw_flag ����װ�ļ��Ķ�д��־��
// �������صĵط�������һ��Ŀ¼�������Ҷ�Ӧ�� i �ڵ�û�б���������ռ�á�
int sys_mount(char * dev_name, char * dir_name, int rw_flag)
{
	struct m_inode * dev_i, * dir_i;
	struct super_block * sb;
	int dev;

	if (!(dev_i=namei(dev_name)))
		return -ENOENT;
	dev = dev_i->i_zone[0];
	if (!S_ISBLK(dev_i->i_mode)) {
		iput(dev_i);
		return -EPERM;
	}
	iput(dev_i);
	if (!(dir_i=namei(dir_name)))
		return -ENOENT;
	if (dir_i->i_count != 1 || dir_i->i_num == ROOT_INO) {
		iput(dir_i);
		return -EBUSY;
	}
	if (!S_ISDIR(dir_i->i_mode)) {
		iput(dir_i);
		return -EPERM;
	}
	if (!(sb=read_super(dev))) {
		iput(dir_i);
		return -EBUSY;
	}
	if (sb->s_imount) {
		iput(dir_i);
		return -EBUSY;
	}
	if (dir_i->i_mount) {
		iput(dir_i);
		return -EPERM;
	}
	sb->s_imount=dir_i;
	dir_i->i_mount=1;
	dir_i->i_dirt=1;		/* NOTE! we don't iput(dir_i) */
	return 0;			/* we do that in umount */
}

// ��װ���ļ�ϵͳ��
// �ú�������ϵͳ������ʼ������ʱ(sys_setup())���õġ�( kernel/blk_drv/hd.c, 157 )
void mount_root(void)
{
	int i,free;
	struct super_block * p;
	struct m_inode * mi;

	if (32 != sizeof (struct d_inode))
		panic("bad i-node size");
	for(i=0;i<NR_FILE;i++)
		file_table[i].f_count=0;
	if (MAJOR(ROOT_DEV) == 2) {
		printk("Insert root floppy and press ENTER");
		wait_for_keypress();
	}
	for(p = &super_block[0] ; p < &super_block[NR_SUPER] ; p++) {
		p->s_dev = 0;
		p->s_lock = 0;
		p->s_wait = NULL;
	}
	if (!(p=read_super(ROOT_DEV)))
		panic("Unable to mount root");
	if (!(mi=iget(ROOT_DEV,ROOT_INO)))
		panic("Unable to read root i-node");
	mi->i_count += 3 ;	/* NOTE! it is logically used 4 times, not 1 */
	p->s_isup = p->s_imount = mi;
	current->pwd = mi;
	current->root = mi;
	free=0;
	i=p->s_nzones;
	while (-- i >= 0)
		if (!set_bit(i&8191,p->s_zmap[i>>13]->b_data))
			free++;
	printk("%d/%d free blocks\n\r",free,p->s_nzones);
	free=0;
	i=p->s_ninodes+1;
	while (-- i >= 0)
		if (!set_bit(i&8191,p->s_imap[i>>13]->b_data))
			free++;
	printk("%d/%d free inodes\n\r",free,p->s_ninodes);
}
