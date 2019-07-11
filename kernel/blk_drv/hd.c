/*
 *  linux/kernel/hd.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * This is the low-level hd interrupt support. It traverses the
 * request-list, using interrupts to jump between functions. As
 * all the functions are called within interrupts, we may not
 * sleep. Special care is recommended.
 * 
 *  modified by Drew Eckhardt to check nr of hd's from the CMOS.
 */
/*
 * �������ǵײ�Ӳ���жϸ���������Ҫ����ɨ�������б�ʹ���ж��ں���֮����ת��
 * �������еĺ����������ж�����õģ�������Щ����������˯�ߡ����ر�ע�⡣
 * �� Drew Eckhardt �޸ģ����� CMOS ��Ϣ���Ӳ������
 */

#include <linux/config.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/hdreg.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/segment.h>

// ������ blk.h �ļ�֮ǰ������������豸�ų�������Ϊ blk.h �ļ���Ҫ�õ��ó�����
#define MAJOR_NR 3	// Ӳ�����豸���� 3��
#include "blk.h"

//��CMOS�����꺯����
#define CMOS_READ(addr) ({ \
outb_p(0x80|addr,0x70); \
inb_p(0x71); \
})

/* Max read/write errors/sector */
#define MAX_ERRORS	7			// ��/дһ������ʱ����������������
#define MAX_HD		2			// ϵͳ֧�ֵ����Ӳ������

static void recal_intr(void);	// Ӳ���жϳ����ڸ�λ����ʱ����õ�����У������

static int recalibrate = 1;		// ����У����־������ͷ�ƶ��� 0 ���档
static int reset = 1;			// ��λ��־����������д����ʱ�����øñ�־���Ը�λӲ�̺Ϳ�������

/*
 *  This struct defines the HD's and their types.
 */
/* ����ṹ������Ӳ�̲��������� */
// ���ֶηֱ��Ǵ�ͷ����ÿ�ŵ�����������������дǰԤ��������š���ͷ��½������š������ֽڡ� 
// ���ǵĺ�����μ������б���˵����
struct hd_i_struct {
	int head,sect,cyl,wpcom,lzone,ctl;
	};

// ����Ѿ��� include/linux/config.h ͷ�ļ��ж����� HD_TYPE��
// ��ȡ���ж���õĲ�����Ϊhd_info[]�����ݡ�
// ������Ĭ�϶���Ϊ 0 ֵ���� setup()�����л�������á�
#ifdef HD_TYPE
struct hd_i_struct hd_info[] = { HD_TYPE };
#define NR_HD ((sizeof (hd_info))/(sizeof (struct hd_i_struct)))	// ����Ӳ�̸�����
#else
struct hd_i_struct hd_info[] = { {0,0,0,0,0,0},{0,0,0,0,0,0} };
static int NR_HD = 0;	//������2������������0�����
#endif

// ����Ӳ�̷����ṹ������ÿ��������������ʼ�����š���������������
// ���� 5 �ı���������(���� hd[0]�� hd[5]��)��������Ӳ���еĲ�����
static struct hd_struct {
	long start_sect;	//������ʼ������
	long nr_sects;		//������������
} hd[5*MAX_HD]={{0,0},};

// ���˿� port������ nr �֣������� buf �С�
// INSָ��ɴ�DXָ��������˿�����һ���ֽڻ��ֵ���ES: DIָ���Ĵ洢���С�
#define port_read(port,buf,nr) \
__asm__("cld;rep;insw"::"d" (port),"D" (buf),"c" (nr):"cx","di")

// д�˿� port����д nr �֣��� buf ��ȡ���ݡ�
#define port_write(port,buf,nr) \
__asm__("cld;rep;outsw"::"d" (port),"S" (buf),"c" (nr):"cx","si")

extern void hd_interrupt(void);	// Ӳ���жϹ���
extern void rd_load(void);		// �����̴������غ���

/* This may be used only once, enforced by 'static int callable' */
/* ����ú���ֻ�ڳ�ʼ��ʱ������һ�Ρ��þ�̬���� callable ��Ϊ�ɵ��ñ�־��*/
// �ú����Ĳ����ɳ�ʼ������ init/main.c �� init �ӳ�������Ϊָ�� 0x90080 ����
// �˴������ setup.s ����� BIOS ȡ�õ� 2 ��Ӳ�̵Ļ���������(32 �ֽ�)��
// Ӳ�̲�������Ϣ�μ������б���˵����
// ��������Ҫ�����Ƕ�ȡ CMOS ��Ӳ�̲�������Ϣ����������Ӳ�̷����ṹ hd��
// ������ RAM �����̺͸��ļ�ϵͳ��
int sys_setup(void * BIOS)
{
	static int callable = 1;
	int i,drive;
	unsigned char cmos_disks;
	struct partition *p;
	struct buffer_head * bh;

	if (!callable)//��֤�������ֻ�ܱ�ִ��һ��
		return -1;
	callable = 0;

// ���û���� config.h �ж���Ӳ�̲������ʹ� 0x90080 �����롣	
#ifndef HD_TYPE
	for (drive=0 ; drive<2 ; drive++) {
		hd_info[drive].cyl = *(unsigned short *) BIOS;			//������
		hd_info[drive].head = *(unsigned char *) (2+BIOS);		//��ͷ��
		hd_info[drive].wpcom = *(unsigned short *) (5+BIOS);	//дǰԤ���������
		hd_info[drive].ctl = *(unsigned char *) (8+BIOS);		//�����ֽ�
		hd_info[drive].lzone = *(unsigned short *) (12+BIOS);	//��ͷ��½�������
		hd_info[drive].sect = *(unsigned char *) (14+BIOS);		//ÿ�ŵ�������
		BIOS += 16;	// ÿ��Ӳ�̵Ĳ����� 16 �ֽڣ����� BIOS ָ����һ����
	}

	// setup.s ������ȡ BIOS �е�Ӳ�̲�������Ϣʱ�����ֻ�� 1 ��Ӳ�̣�
	// �ͻὫ��Ӧ�� 2 ��Ӳ�̵� 16 �ֽ�ȫ�����㡣
	// �������ֻҪ�жϵ� 2 ��Ӳ���������Ƿ�Ϊ 0 �Ϳ���֪����û�е� 2 ��Ӳ���ˡ�
	if (hd_info[1].cyl)
		NR_HD=2;
	else
		NR_HD=1;
#endif

	for (i=0 ; i<NR_HD ; i++) {	//��ʼ����������Ӳ���Ǹ�����
		hd[i*5].start_sect = 0;						// Ӳ����ʼ������
		hd[i*5].nr_sects = hd_info[i].head*			// Ӳ������������
				hd_info[i].sect*hd_info[i].cyl;
	}

	/*
		We querry CMOS about hard disks : it could be that 
		we have a SCSI/ESDI/etc controller that is BIOS
		compatable with ST-506, and thus showing up in our
		BIOS table, but not register compatable, and therefore
		not present in CMOS.

		Furthurmore, we will assume that our ST-506 drives
		<if any> are the primary drives in the system, and 
		the ones reflected as drive 1 or 2.

		The first drive is stored in the high nibble of CMOS
		byte 0x12, the second in the low nibble.  This will be
		either a 4 bit drive type or 0xf indicating use byte 0x19 
		for an 8 bit type, drive 1, 0x1a for drive 2 in CMOS.

		Needless to say, a non-zero value means we have 
		an AT controller hard disk for that drive.

		
	*/

	/*
	 * ���Ƕ� CMOS �й�Ӳ�̵���Ϣ��Щ����:���ܻ���������������������һ�� SCSI/ESDI/�ȵ� ��������
	 * ������ ST-506 ��ʽ�� BIOS ���ݵģ��������������ǵ� BIOS �������У���ȴ�ֲ� �ǼĴ������ݵģ�
	 * �����Щ������ CMOS ���ֲ����ڡ�
	 * ���⣬���Ǽ��� ST-506 ������(����еĻ�)��ϵͳ�еĻ�����������Ҳ���������� 1 �� 2 ���ֵ���������
	 * �� 1 ����������������� CMOS �ֽ� 0x12 �ĸ߰��ֽ��У��� 2 ������ڵͰ��ֽ��С�
	 * �� 4 λ�ֽ� ��Ϣ���������������ͣ�Ҳ���ܽ��� 0xf��
	 * 0xf ��ʾʹ�� CMOS �� 0x19 �ֽ���Ϊ������ 1 �� 8 λ �����ֽڣ�
	 * ʹ�� CMOS �� 0x1A �ֽ���Ϊ������ 2 �������ֽڡ� 
	 * ��֮��һ������ֵ��ζ��������һ�� AT ������Ӳ�̼��ݵ���������
	 */

	// �����������ԭ�������Ӳ�̵����Ƿ��� AT ���������ݵġ�
	// �й� CMOS ��Ϣ��μ� 4.2.3.1 �ڡ�
	if ((cmos_disks = CMOS_READ(0x12)) & 0xf0)
		if (cmos_disks & 0x0f)
			NR_HD = 2;
		else
			NR_HD = 1;
	else
		NR_HD = 0;

	// �� NR_HD=0��������Ӳ�̶����� AT ���������ݵģ�Ӳ�����ݽṹ���㡣 
	// �� NR_HD=1���򽫵� 2 ��Ӳ�̵Ĳ������㡣
	for (i = NR_HD ; i < 2 ; i++) {//��Ȥ��д����ʡȥ���ж�
		hd[i*5].start_sect = 0;
		hd[i*5].nr_sects = 0;
	}

	// ��ȡÿһ��Ӳ���ϵ� 1 �����ݣ���ȡ��������Ϣ��
	for (drive=0 ; drive<NR_HD ; drive++) {
		if (!(bh = bread(0x300 + drive*5,0))) {//��ȡ��һ�����ݿ鵽������
			printk("Unable to read partition table of drive %d\n\r",
				drive);
			panic("");
		}
		if (bh->b_data[510] != 0x55 || (unsigned char)
		    bh->b_data[511] != 0xAA) {	// �ж�Ӳ����Ϣ��Ч��־'55AA'
			printk("Bad partition table on drive %d\n\r",drive);
			panic("");
		}
		p = 0x1BE + (void *)bh->b_data;	// ������λ��Ӳ�̵� 1 ������ 0x1BE ����
		for (i=1;i<5;i++,p++) {//ÿ��Ӳ�̶�4��������
			hd[i+5*drive].start_sect = p->start_sect;
			hd[i+5*drive].nr_sects = p->nr_sects;
		}
		brelse(bh);	// ������λ��Ӳ�̵� 1 ������ 0x1BE ����
	}
	if (NR_HD)		//��Ӳ�����Ѷ��룬���ӡ��Ϣ��
		printk("Partition table%s ok.\n\r",(NR_HD>1)?"s":"");//��ӡ��Ϣ�����ǼӸ���������
	rd_load();		// ���ظ��ļ�ϵͳ��RAMDISK
	mount_root();	// ���س�����͸�Ŀ¼�����ٻ�����
	return (0);
}

// �жϲ�ѭ���ȴ�������������
// ��Ӳ�̿�����״̬�Ĵ����˿� HD_STATUS(0x1f7)����ѭ�������������������λ�Ϳ�����æλ�� 
// �������ֵΪ 0�����ʾ�ȴ���ʱ�������� OK��
static int controller_ready(void)
{
	int retries=10000;

	while (--retries && (inb_p(HD_STATUS)&0xc0)!=0x40);
	return (retries);
}

// ���Ӳ��ִ��������״̬��(win_��ʾ����˹��Ӳ�̵���д)
// ��ȡ״̬�Ĵ����е�����ִ�н��״̬������ 0 ��ʾ������1 ����
// ���ִ����������ٶ�����Ĵ��� HD_ERROR(0x1f1)��
static int win_result(void)
{
	int i=inb_p(HD_STATUS);

	if ((i & (BUSY_STAT | READY_STAT | WRERR_STAT | SEEK_STAT | ERR_STAT))
		== (READY_STAT | SEEK_STAT))
		return(0); /* ok */
	if (i&1) i=inb(HD_ERROR);
	return (1);
}

// ��Ӳ�̿�������������顣
// ���ò���:drive - Ӳ�̺�(0-1); nsect - ��д������;
// sect - ��ʼ����; head - ��ͷ��;
// cyl - �����; cmd - ������(�μ������������б��� 6.3);
// *intr_addr() - Ӳ���жϷ���ʱ��������н����õ� C ��������
static void hd_out(unsigned int drive,unsigned int nsect,unsigned int sect,
		unsigned int head,unsigned int cyl,unsigned int cmd,
		void (*intr_addr)(void))
{
	register int port asm("dx"); 	//port ������Ӧ�Ĵ��� dx����д����֮ǰû��������

	if (drive>1 || head>15)			// �����������(0,1)>1 ���ͷ��>15�������֧�֡�
		panic("Trying to write bad sector");
	if (!controller_ready())		// ����ȴ�һ��ʱ�����δ���������������
		panic("HD controller not ready");

	//���Ǹ�����ָ�룬��DEVICE_INTR�궨������
	// do_hd ����ָ�뽫��Ӳ���жϳ����б����á�
	do_hd = intr_addr;		
	
	outb_p(hd_info[drive].ctl,HD_CMD);		// ����ƼĴ���(0x3f6)��������ֽڡ�
	port=HD_DATA;							// �� dx Ϊ���ݼĴ����˿�(0x1f0)��
	outb_p(hd_info[drive].wpcom>>2,++port);	// ����:дԤ���������(��� 4)��
	outb_p(nsect,++port);					// ����:��/д�����������
	outb_p(sect,++port);					// ����:��ʼ������
	outb_p(cyl,++port);						// ����:����ŵ� 8 λ��
	outb_p(cyl>>8,++port);					// ����:����Ÿ� 8 λ��
	outb_p(0xA0|(drive<<4)|head,++port);	// ����:��������+��ͷ�š�
	outb(cmd,++port);						// ����:Ӳ�̿������
}

// �ȴ�Ӳ�̾�����Ҳ��ѭ���ȴ���״̬������æ��־λ��λ��
// �����о�����Ѱ��������־��λ����ɹ������� 0��
// ������һ��ʱ����Ϊæ���򷵻� 1��
static int drive_busy(void)
{
	unsigned int i;

	for (i = 0; i < 10000; i++)					// ѭ���ȴ�������־λ��λ��
		if (READY_STAT == (inb_p(HD_STATUS) & (BUSY_STAT|READY_STAT)))
			break;
	i = inb(HD_STATUS);							// ��ȡ��������״̬�ֽڡ�
	i &= BUSY_STAT | READY_STAT | SEEK_STAT;	// ���æλ������λ��Ѱ������λ
	if (i == READY_STAT | SEEK_STAT)			// �����о�����Ѱ��������־���򷵻� 0��
		return(0);
	printk("HD controller times out\n\r");		// ����ȴ���ʱ����ʾ��Ϣ�������� 1��
	return(1);
}

// ��ϸ�λ(����У��)Ӳ�̿�������
static void reset_controller(void)
{
	int	i;

	outb(4,HD_CMD);							// ����ƼĴ����˿ڷ��Ϳ����ֽ�(4-��λ)
	for(i = 0; i < 100; i++) nop();			// �ȴ�һ��ʱ��(ѭ���ղ���)
	outb(hd_info[0].ctl & 0x0f ,HD_CMD);	// �ٷ��������Ŀ����ֽ�(����ֹ���ԡ��ض�)��
	if (drive_busy())						// ���ȴ�Ӳ�̾�����ʱ������ʾ������Ϣ��
		printk("HD-controller still busy\n\r");
	if ((i = inb(HD_ERROR)) != 1)			// ȡ����Ĵ������������� 1(�޴���)�����
		printk("HD-controller reset failed: %02x\n\r",i);
}

// ��λӲ�� nr�����ȸ�λ(����У��)Ӳ�̿�������Ȼ����Ӳ�̿��������������������������
// ���� recal_intr()����Ӳ���жϴ�������е��õ�����У����������
static void reset_hd(int nr)
{
	//��λ(����У��)Ӳ�̿�����
	reset_controller();	
	//����Ӳ�̿������������������������
	hd_out(nr,hd_info[nr].sect,hd_info[nr].sect,hd_info[nr].head-1,
		hd_info[nr].cyl,WIN_SPECIFY,&recal_intr);
}

// ����Ӳ���жϵ��ú�����
// ��������Ӳ���ж�ʱ��Ӳ���жϴ�������е��õ�Ĭ�� C ��������
// �ڱ����ú���ָ��Ϊ��ʱ���øú�����
void unexpected_hd_interrupt(void)
{
	printk("Unexpected HD interrupt\n\r");
}

// ��дӲ��ʧ�ܴ�����ú�����
static void bad_rw_intr(void)
{
	// ���������ʱ�ĳ���������ڻ���� 7 ��ʱ����������󲢻��ѵȴ�������Ľ��̣�
	// ���Ҷ�Ӧ���������±�־��λ(û�и���)��
	if (++CURRENT->errors >= MAX_ERRORS)
		end_request(0);
	// �����һ����ʱ�ĳ�������Ѿ����� 3 �Σ� ��Ҫ��ִ�и�λӲ�̿�����������
	if (CURRENT->errors > MAX_ERRORS/2)
		reset = 1;
}

// �������жϵ��ú���������Ӳ�̶��������ʱ�������жϹ����б����á�
// �ú��������жϴ˴ζ���������Ƿ��������������������������æ״̬����������ִ�д��� 
// ����Ӳ�̲���ʧ�����⣬��������Ӳ������λ����ִ�����������
// ���������û�г���������ݼĴ����˿ڰ�һ�����������ݶ���������Ļ������У����ݼ������� 
// �����ȡ��������ֵ�����ݼ��󲻵��� 0����ʾ��������������ûȡ�꣬����ֱ�ӷ��أ��ȴ�Ӳ�� 
// �ڶ�����һ���������ݺ���жϡ��������������������������������Ѷ��꣬���Ǵ����������� 
// �������ˡ�����ٴε��� do_hd_request()��ȥ��������Ӳ�������
static void read_intr(void)
{
	if (win_result()) {			// ��������æ����д�������ִ�д�
		bad_rw_intr();			// ����ж�дӲ��ʧ�ܴ���
		do_hd_request();		// Ȼ���ٴ�����Ӳ������Ӧ(��λ)����
		return;
	}

	// �����ݴ����ݼĴ����ڶ�������ṹ��������
	port_read(HD_DATA,CURRENT->buffer,256);// ע��:256 ��ָ�ڴ��֣�Ҳ�� 512 �ֽڡ�

	CURRENT->errors = 0;			// ����������
	CURRENT->buffer += 512;			// ����������ָ�룬ָ���µĿ�����
	CURRENT->sector++;				// ��ʼ�����ż� 1��

	if (--CURRENT->nr_sectors) {	// ��������������������û�ж��꣬
		do_hd = &read_intr;			// ���ٴ���Ӳ�̵��� C ����ָ��Ϊ read_intr()��
		return;// ��ΪӲ���жϴ������ÿ�ε��� do_hd ʱ���Ὣ�ú���ָ���ÿա��μ� system_call.s
	}
	end_request(1);					// ��ȫ�����������Ѿ����꣬��������������ˡ�
	do_hd_request();				//ִ������Ӳ�����������
}

// д�����жϵ��ú�������Ӳ���жϴ�������б����á�
// ��д����ִ�к󣬻����Ӳ���ж��źţ�ִ��Ӳ���жϴ������
// ��ʱ��Ӳ���жϴ�������е��õ� C ����ָ�� do_hd()�Ѿ�ָ�� write_intr()��
// ��˻���д�������(�����)��ִ�иú�����
static void write_intr(void)
{
	if (win_result()) {				// ���Ӳ�̿��������ش�����Ϣ��
		bad_rw_intr();				// �����Ƚ���Ӳ�̶�дʧ�ܴ���
		do_hd_request();			// Ȼ���ٴ�����Ӳ������Ӧ(��λ)����
		return;						// Ȼ�󷵻�(Ҳ�˳��˴˴�Ӳ���ж�)��
	}
	if (--CURRENT->nr_sectors) {	// ������д�������� 1������������Ҫд����
		CURRENT->sector++;			// ��ǰ������ʼ������+1��
		CURRENT->buffer += 512;		// �������󻺳���ָ�룬
		do_hd = &write_intr;		// ��Ӳ���жϳ�����ú���ָ��Ϊ write_intr()��
		port_write(HD_DATA,CURRENT->buffer,256);// �������ݼĴ����˿�д 256 �֡�
		return;									// ���صȴ�Ӳ���ٴ����д��������жϴ���
	}
	end_request(1);					// ��ȫ�����������Ѿ�д�꣬��������������ˣ�
	do_hd_request();				// ִ������Ӳ�����������
}

// Ӳ������У������λ���жϵ��ú�������Ӳ���жϴ�������б����á�
// ���Ӳ�̿��������ش�����Ϣ�������Ƚ���Ӳ�̶�дʧ�ܴ���Ȼ������Ӳ������Ӧ(��λ)����
static void recal_intr(void)
{
	if (win_result())
		bad_rw_intr();
	do_hd_request();
}

// ִ��Ӳ�̶�д���������
// ���������ǿ��豸�ĵ� 1 ��������豸��ǰ������ָ���ֱ��ָ���������������̵��ñ�����ִ�ж�д������
// ������һ����д������ɶ�������Ӳ���жϹ����У���������������Ҫ������Ҳ�����жϹ����е��ñ�������
void do_hd_request(void)
{
	int i,r;
	unsigned int block,dev;
	unsigned int sec,head,cyl;
	unsigned int nsect;

	// ���������ĺϷ��ԣ�����û�����������˳���
	INIT_REQUEST;
	// ȡ�豸���е����豸��(���б���Ӳ���豸�ŵ�˵��)�����豸�ż���Ӳ���ϵķ����š�
	dev = MINOR(CURRENT->dev);
	block = CURRENT->sector;
	// ������豸�Ų����ڻ�����ʼ�������ڸ÷���������-2������������󣬲���ת����� repeat ����
	// ��Ϊһ��Ҫ���д 2 ��������512*2 �ֽڣ�����������������Ų��ܴ��ڷ�����������ڶ��������š�
	if (dev >= 5*NR_HD || block+2 > hd[dev].nr_sects) {
		end_request(0);
		goto repeat;//repeat��INIT_REQUEST�ж���ġ�
	}
	// ͨ�����ϱ���������ʼ�����ţ��ѽ������д�Ŀ��Ӧ������Ӳ�̵ľ����������ϡ�
	block += hd[dev].start_sect;
	dev /= 5;// ��ʱ dev ����Ӳ�̺ţ��ǵ� 1 ��Ӳ��(0)���ǵ� 2 ��(1)����
	// ����Ƕ�������������Ӳ����Ϣ�ṹ�и�����ʼ�����ź�ÿ�ŵ������������ڴŵ��е�
	// ������(sec)�����������(cyl)�ʹ�ͷ��(head)��
	__asm__("divl %4":"=a" (block),"=d" (sec):"0" (block),"1" (0),
		"r" (hd_info[dev].sect));
	__asm__("divl %4":"=a" (cyl),"=d" (head):"0" (block),"1" (0),
		"r" (hd_info[dev].head));
	sec++;
	nsect = CURRENT->nr_sectors;
	// ��� reset ��־����λ�ģ���ִ�и�λ��������λӲ�̺Ϳ�������������Ҫ����У����־�����ء�
	if (reset) {
		reset = 0;
		recalibrate = 1;
		reset_hd(CURRENT_DEV);
		return;
	}
	// �������У����־(recalibrate)��λ�������ȸ�λ�ñ�־��Ȼ����Ӳ�̿�������������У�����
	// �������ִ��Ѱ���������ô����κεط��Ĵ�ͷ�ƶ��� 0 ���档
	if (recalibrate) {
		recalibrate = 0;
		hd_out(dev,hd_info[CURRENT_DEV].sect,0,0,0,
			WIN_RESTORE,&recal_intr);
		return;
	}	
	// �����ǰ������д��������������д���ѭ����ȡ״̬�Ĵ�����Ϣ���ж���������־
	// DRQ_STAT �Ƿ���λ��DRQ_STAT ��Ӳ��״̬�Ĵ������������λ����ʾ�������Ѿ�׼������������
	// ���ݶ˿�֮�䴫��һ���ֻ�һ���ֽڵ����ݡ�
	if (CURRENT->cmd == WRITE) {
		hd_out(dev,nsect,sec,head,cyl,WIN_WRITE,&write_intr);
		for(i=0 ; i<3000 && !(r=inb_p(HD_STATUS)&DRQ_STAT) ; i++)
			/* nothing */ ;
		// ���������� DRQ ��λ���˳�ѭ�������ȵ�ѭ������Ҳû����λ��
		// ���ʾ�˴�дӲ�̲���ʧ�ܣ�ȥ������һ��Ӳ������
		// ������Ӳ�̿��������ݼĴ����˿� HD_DATA д�� 1 �����������ݡ�
		if (!r) {
			bad_rw_intr();
			goto repeat;//repeat��INIT_REQUEST�ж���ġ�
		}
		port_write(HD_DATA,CURRENT->buffer,256);
	// �����ǰ�����Ƕ�Ӳ������������Ӳ�̿��������Ͷ��������
	} else if (CURRENT->cmd == READ) {
		hd_out(dev,nsect,sec,head,cyl,WIN_READ,&read_intr);
	} else
		panic("unknown hd-command");
}

// Ӳ��ϵͳ��ʼ����
void hd_init(void)
{
	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;// do_hd_request()��
	// ����Ӳ���ж������� int 0x2E(46)��
	set_intr_gate(0x2E,&hd_interrupt);
	// ��λ�������� 8259A int2 ������λ�������Ƭ�����ж������źš�
	outb_p(inb_p(0x21)&0xfb,0x21);
	// ��λӲ�̵��ж���������λ���ڴ�Ƭ�ϣ�������Ӳ�̿����������ж������źš�
	outb(inb_p(0xA1)&0xbf,0xA1);
}
