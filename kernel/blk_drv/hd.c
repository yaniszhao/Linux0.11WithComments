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
 * ±¾³ÌĞòÊÇµ×²ãÓ²ÅÌÖĞ¶Ï¸¨Öú³ÌĞò¡£Ö÷ÒªÓÃÓÚÉ¨ÃèÇëÇóÁĞ±í£¬Ê¹ÓÃÖĞ¶ÏÔÚº¯ÊıÖ®¼äÌø×ª¡£
 * ÓÉÓÚËùÓĞµÄº¯Êı¶¼ÊÇÔÚÖĞ¶ÏÀïµ÷ÓÃµÄ£¬ËùÒÔÕâĞ©º¯Êı²»¿ÉÒÔË¯Ãß¡£ÇëÌØ±ğ×¢Òâ¡£
 * ÓÉ Drew Eckhardt ĞŞ¸Ä£¬ÀûÓÃ CMOS ĞÅÏ¢¼ì²âÓ²ÅÌÊı¡£
 */

#include <linux/config.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/hdreg.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/segment.h>

// ±ØĞëÔÚ blk.h ÎÄ¼şÖ®Ç°¶¨ÒåÏÂÃæµÄÖ÷Éè±¸ºÅ³£Êı£¬ÒòÎª blk.h ÎÄ¼şÖĞÒªÓÃµ½¸Ã³£Êı¡£
#define MAJOR_NR 3	// Ó²ÅÌÖ÷Éè±¸ºÅÊÇ 3¡£
#include "blk.h"

//¶ÁCMOS²ÎÊıºêº¯Êı¡£
#define CMOS_READ(addr) ({ \
outb_p(0x80|addr,0x70); \
inb_p(0x71); \
})

/* Max read/write errors/sector */
#define MAX_ERRORS	7			// ¶Á/Ğ´Ò»¸öÉÈÇøÊ±ÔÊĞíµÄ×î¶à³ö´í´ÎÊı¡£
#define MAX_HD		2			// ÏµÍ³Ö§³ÖµÄ×î¶àÓ²ÅÌÊı¡£

static void recal_intr(void);	// Ó²ÅÌÖĞ¶Ï³ÌĞòÔÚ¸´Î»²Ù×÷Ê±»áµ÷ÓÃµÄÖØĞÂĞ£Õıº¯Êı

static int recalibrate = 1;		// ÖØĞÂĞ£Õı±êÖ¾¡£½«´ÅÍ·ÒÆ¶¯µ½ 0 ÖùÃæ¡£
static int reset = 1;			// ¸´Î»±êÖ¾¡£µ±·¢Éú¶ÁĞ´´íÎóÊ±»áÉèÖÃ¸Ã±êÖ¾£¬ÒÔ¸´Î»Ó²ÅÌºÍ¿ØÖÆÆ÷¡£

/*
 *  This struct defines the HD's and their types.
 */
/* ÏÂÃæ½á¹¹¶¨ÒåÁËÓ²ÅÌ²ÎÊı¼°ÀàĞÍ */
// ¸÷×Ö¶Î·Ö±ğÊÇ´ÅÍ·Êı¡¢Ã¿´ÅµÀÉÈÇøÊı¡¢ÖùÃæÊı¡¢Ğ´Ç°Ô¤²¹³¥ÖùÃæºÅ¡¢´ÅÍ·×ÅÂ½ÇøÖùÃæºÅ¡¢¿ØÖÆ×Ö½Ú¡£ 
// ËüÃÇµÄº¬ÒåÇë²Î¼û³ÌĞòÁĞ±íºóµÄËµÃ÷¡£
struct hd_i_struct {
	int head,sect,cyl,wpcom,lzone,ctl;
	};

// Èç¹ûÒÑ¾­ÔÚ include/linux/config.h Í·ÎÄ¼şÖĞ¶¨ÒåÁË HD_TYPE£¬
// ¾ÍÈ¡ÆäÖĞ¶¨ÒåºÃµÄ²ÎÊı×÷Îªhd_info[]µÄÊı¾İ¡£
// ·ñÔò£¬ÏÈÄ¬ÈÏ¶¼ÉèÎª 0 Öµ£¬ÔÚ setup()º¯ÊıÖĞ»á½øĞĞÉèÖÃ¡£
#ifdef HD_TYPE
struct hd_i_struct hd_info[] = { HD_TYPE };
#define NR_HD ((sizeof (hd_info))/(sizeof (struct hd_i_struct)))	// ¼ÆËãÓ²ÅÌ¸öÊı¡£
#else
struct hd_i_struct hd_info[] = { {0,0,0,0,0,0},{0,0,0,0,0,0} };
static int NR_HD = 0;	//ÉÏÃæÊÇ2¸ö£¬ÕâÀïÓÖÊÇ0£¬Ææ¹Ö
#endif

// ¶¨ÒåÓ²ÅÌ·ÖÇø½á¹¹¡£¸ø³öÃ¿¸ö·ÖÇøµÄÎïÀíÆğÊ¼ÉÈÇøºÅ¡¢·ÖÇøÉÈÇø×ÜÊı¡£
// ÆäÖĞ 5 µÄ±¶Êı´¦µÄÏî(ÀıÈç hd[0]ºÍ hd[5]µÈ)´ú±íÕû¸öÓ²ÅÌÖĞµÄ²ÎÊı¡£
static struct hd_struct {
	long start_sect;	//·ÖÇøÆğÊ¼ÉÈÇøºÅ
	long nr_sects;		//·ÖÇøÉÈÇø×ÜÊı
} hd[5*MAX_HD]={{0,0},};

// ¶Á¶Ë¿Ú port£¬¹²¶Á nr ×Ö£¬±£´æÔÚ buf ÖĞ¡£
// INSÖ¸Áî¿É´ÓDXÖ¸³öµÄÍâÉè¶Ë¿ÚÊäÈëÒ»¸ö×Ö½Ú»ò×Öµ½ÓÉES: DIÖ¸¶¨µÄ´æ´¢Æ÷ÖĞ¡£
#define port_read(port,buf,nr) \
__asm__("cld;rep;insw"::"d" (port),"D" (buf),"c" (nr):"cx","di")

// Ğ´¶Ë¿Ú port£¬¹²Ğ´ nr ×Ö£¬´Ó buf ÖĞÈ¡Êı¾İ¡£
#define port_write(port,buf,nr) \
__asm__("cld;rep;outsw"::"d" (port),"S" (buf),"c" (nr):"cx","si")

extern void hd_interrupt(void);	// Ó²ÅÌÖĞ¶Ï¹ı³Ì
extern void rd_load(void);		// ĞéÄâÅÌ´´½¨¼ÓÔØº¯Êı

/* This may be used only once, enforced by 'static int callable' */
/* ÏÂÃæ¸Ãº¯ÊıÖ»ÔÚ³õÊ¼»¯Ê±±»µ÷ÓÃÒ»´Î¡£ÓÃ¾²Ì¬±äÁ¿ callable ×÷Îª¿Éµ÷ÓÃ±êÖ¾¡£*/
// ¸Ãº¯ÊıµÄ²ÎÊıÓÉ³õÊ¼»¯³ÌĞò init/main.c µÄ init ×Ó³ÌĞòÉèÖÃÎªÖ¸Ïò 0x90080 ´¦£¬
// ´Ë´¦´æ·Å×Å setup.s ³ÌĞò´Ó BIOS È¡µÃµÄ 2 ¸öÓ²ÅÌµÄ»ù±¾²ÎÊı±í(32 ×Ö½Ú)¡£
// Ó²ÅÌ²ÎÊı±íĞÅÏ¢²Î¼ûÏÂÃæÁĞ±íºóµÄËµÃ÷¡£
// ±¾º¯ÊıÖ÷Òª¹¦ÄÜÊÇ¶ÁÈ¡ CMOS ºÍÓ²ÅÌ²ÎÊı±íĞÅÏ¢£¬ÓÃÓÚÉèÖÃÓ²ÅÌ·ÖÇø½á¹¹ hd£¬
// ²¢¼ÓÔØ RAM ĞéÄâÅÌºÍ¸ùÎÄ¼şÏµÍ³¡£
int sys_setup(void * BIOS)
{
	static int callable = 1;
	int i,drive;
	unsigned char cmos_disks;
	struct partition *p;
	struct buffer_head * bh;

	if (!callable)//±£Ö¤Õâ¸öº¯ÊıÖ»ÄÜ±»Ö´ĞĞÒ»´Î
		return -1;
	callable = 0;

// Èç¹ûÃ»ÓĞÔÚ config.h ÖĞ¶¨ÒåÓ²ÅÌ²ÎÊı£¬¾Í´Ó 0x90080 ´¦¶ÁÈë¡£	
#ifndef HD_TYPE
	for (drive=0 ; drive<2 ; drive++) {
		hd_info[drive].cyl = *(unsigned short *) BIOS;			//ÖùÃæÊı
		hd_info[drive].head = *(unsigned char *) (2+BIOS);		//´ÅÍ·Êı
		hd_info[drive].wpcom = *(unsigned short *) (5+BIOS);	//Ğ´Ç°Ô¤²¹³¥ÖùÃæºÅ
		hd_info[drive].ctl = *(unsigned char *) (8+BIOS);		//¿ØÖÆ×Ö½Ú
		hd_info[drive].lzone = *(unsigned short *) (12+BIOS);	//´ÅÍ·×ÅÂ½ÇøÖùÃæºÅ
		hd_info[drive].sect = *(unsigned char *) (14+BIOS);		//Ã¿´ÅµÀÉÈÇøÊı
		BIOS += 16;	// Ã¿¸öÓ²ÅÌµÄ²ÎÊı±í³¤ 16 ×Ö½Ú£¬ÕâÀï BIOS Ö¸ÏòÏÂÒ»¸ö±í
	}

	// setup.s ³ÌĞòÔÚÈ¡ BIOS ÖĞµÄÓ²ÅÌ²ÎÊı±íĞÅÏ¢Ê±£¬Èç¹ûÖ»ÓĞ 1 ¸öÓ²ÅÌ£¬
	// ¾Í»á½«¶ÔÓ¦µÚ 2 ¸öÓ²ÅÌµÄ 16 ×Ö½ÚÈ«²¿ÇåÁã¡£
	// Òò´ËÕâÀïÖ»ÒªÅĞ¶ÏµÚ 2 ¸öÓ²ÅÌÖùÃæÊıÊÇ·ñÎª 0 ¾Í¿ÉÒÔÖªµÀÓĞÃ»ÓĞµÚ 2 ¸öÓ²ÅÌÁË¡£
	if (hd_info[1].cyl)
		NR_HD=2;
	else
		NR_HD=1;
#endif

	for (i=0 ; i<NR_HD ; i++) {	//³õÊ¼»¯´ú±íÕû¿éÓ²ÅÌÄÇ¸ö·ÖÇø
		hd[i*5].start_sect = 0;						// Ó²ÅÌÆğÊ¼ÉÈÇøºÅ
		hd[i*5].nr_sects = hd_info[i].head*			// Ó²ÅÌ×ÜÉÈÇøÊı¡£
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
	 * ÎÒÃÇ¶Ô CMOS ÓĞ¹ØÓ²ÅÌµÄĞÅÏ¢ÓĞĞ©»³ÒÉ:¿ÉÄÜ»á³öÏÖÕâÑùµÄÇé¿ö£¬ÎÒÃÇÓĞÒ»¿é SCSI/ESDI/µÈµÄ ¿ØÖÆÆ÷£¬
	 * ËüÊÇÒÔ ST-506 ·½Ê½Óë BIOS ¼æÈİµÄ£¬Òò¶ø»á³öÏÖÔÚÎÒÃÇµÄ BIOS ²ÎÊı±íÖĞ£¬µ«È´ÓÖ²» ÊÇ¼Ä´æÆ÷¼æÈİµÄ£¬
	 * Òò´ËÕâĞ©²ÎÊıÔÚ CMOS ÖĞÓÖ²»´æÔÚ¡£
	 * ÁíÍâ£¬ÎÒÃÇ¼ÙÉè ST-506 Çı¶¯Æ÷(Èç¹ûÓĞµÄ»°)ÊÇÏµÍ³ÖĞµÄ»ù±¾Çı¶¯Æ÷£¬Ò²¼´ÒÔÇı¶¯Æ÷ 1 »ò 2 ³öÏÖµÄÇı¶¯Æ÷¡£
	 * µÚ 1 ¸öÇı¶¯Æ÷²ÎÊı´æ·ÅÔÚ CMOS ×Ö½Ú 0x12 µÄ¸ß°ë×Ö½ÚÖĞ£¬µÚ 2 ¸ö´æ·ÅÔÚµÍ°ë×Ö½ÚÖĞ¡£
	 * ¸Ã 4 Î»×Ö½Ú ĞÅÏ¢¿ÉÒÔÊÇÇı¶¯Æ÷ÀàĞÍ£¬Ò²¿ÉÄÜ½öÊÇ 0xf¡£
	 * 0xf ±íÊ¾Ê¹ÓÃ CMOS ÖĞ 0x19 ×Ö½Ú×÷ÎªÇı¶¯Æ÷ 1 µÄ 8 Î» ÀàĞÍ×Ö½Ú£¬
	 * Ê¹ÓÃ CMOS ÖĞ 0x1A ×Ö½Ú×÷ÎªÇı¶¯Æ÷ 2 µÄÀàĞÍ×Ö½Ú¡£ 
	 * ×ÜÖ®£¬Ò»¸ö·ÇÁãÖµÒâÎ¶×ÅÎÒÃÇÓĞÒ»¸ö AT ¿ØÖÆÆ÷Ó²ÅÌ¼æÈİµÄÇı¶¯Æ÷¡£
	 */

	// ÕâÀï¸ù¾İÉÏÊöÔ­ÀíÀ´¼ì²âÓ²ÅÌµ½µ×ÊÇ·ñÊÇ AT ¿ØÖÆÆ÷¼æÈİµÄ¡£
	// ÓĞ¹Ø CMOS ĞÅÏ¢Çë²Î¼û 4.2.3.1 ½Ú¡£
	if ((cmos_disks = CMOS_READ(0x12)) & 0xf0)
		if (cmos_disks & 0x0f)
			NR_HD = 2;
		else
			NR_HD = 1;
	else
		NR_HD = 0;

	// Èô NR_HD=0£¬ÔòÁ½¸öÓ²ÅÌ¶¼²»ÊÇ AT ¿ØÖÆÆ÷¼æÈİµÄ£¬Ó²ÅÌÊı¾İ½á¹¹ÇåÁã¡£ 
	// Èô NR_HD=1£¬Ôò½«µÚ 2 ¸öÓ²ÅÌµÄ²ÎÊıÇåÁã¡£
	for (i = NR_HD ; i < 2 ; i++) {//ÓĞÈ¤µÄĞ´·¨£¬Ê¡È¥ÁËÅĞ¶Ï
		hd[i*5].start_sect = 0;
		hd[i*5].nr_sects = 0;
	}

	// ¶ÁÈ¡Ã¿Ò»¸öÓ²ÅÌÉÏµÚ 1 ¿éÊı¾İ£¬»ñÈ¡·ÖÇø±íĞÅÏ¢¡£
	for (drive=0 ; drive<NR_HD ; drive++) {
		if (!(bh = bread(0x300 + drive*5,0))) {//¶ÁÈ¡µÚÒ»¸öÊı¾İ¿éµ½»º³åÇø
			printk("Unable to read partition table of drive %d\n\r",
				drive);
			panic("");
		}
		if (bh->b_data[510] != 0x55 || (unsigned char)
		    bh->b_data[511] != 0xAA) {	// ÅĞ¶ÏÓ²ÅÌĞÅÏ¢ÓĞĞ§±êÖ¾'55AA'
			printk("Bad partition table on drive %d\n\r",drive);
			panic("");
		}
		p = 0x1BE + (void *)bh->b_data;	// ·ÖÇø±íÎ»ÓÚÓ²ÅÌµÚ 1 ÉÈÇøµÄ 0x1BE ´¦¡£
		for (i=1;i<5;i++,p++) {//Ã¿¿éÓ²ÅÌ¶Á4¸ö·ÖÇø±í
			hd[i+5*drive].start_sect = p->start_sect;
			hd[i+5*drive].nr_sects = p->nr_sects;
		}
		brelse(bh);	// ·ÖÇø±íÎ»ÓÚÓ²ÅÌµÚ 1 ÉÈÇøµÄ 0x1BE ´¦¡£
	}
	if (NR_HD)		//ÓĞÓ²ÅÌÇÒÒÑ¶ÁÈë£¬Ôò´òÓ¡ĞÅÏ¢¡£
		printk("Partition table%s ok.\n\r",(NR_HD>1)?"s":"");//´òÓ¡ĞÅÏ¢»¹¿¼ÂÇ¼Ó¸´Êı£¬¹ş¹ş
	rd_load();		// ¼ÓÔØ¸ùÎÄ¼şÏµÍ³µ½RAMDISK
	mount_root();	// ¼ÓÔØ³¬¼¶¿éºÍ¸ùÄ¿Â¼µ½¸ßËÙ»º³åÇø
	return (0);
}

// ÅĞ¶Ï²¢Ñ­»·µÈ´ıÇı¶¯Æ÷¾ÍĞ÷¡£
// ¶ÁÓ²ÅÌ¿ØÖÆÆ÷×´Ì¬¼Ä´æÆ÷¶Ë¿Ú HD_STATUS(0x1f7)£¬²¢Ñ­»·¼ì²âÇı¶¯Æ÷¾ÍĞ÷±ÈÌØÎ»ºÍ¿ØÖÆÆ÷Ã¦Î»¡£ 
// Èç¹û·µ»ØÖµÎª 0£¬Ôò±íÊ¾µÈ´ı³¬Ê±³ö´í£¬·ñÔò OK¡£
static int controller_ready(void)
{
	int retries=10000;

	while (--retries && (inb_p(HD_STATUS)&0xc0)!=0x40);
	return (retries);
}

// ¼ì²âÓ²ÅÌÖ´ĞĞÃüÁîºóµÄ×´Ì¬¡£(win_±íÊ¾ÎÂÇĞË¹ÌØÓ²ÅÌµÄËõĞ´)
// ¶ÁÈ¡×´Ì¬¼Ä´æÆ÷ÖĞµÄÃüÁîÖ´ĞĞ½á¹û×´Ì¬¡£·µ»Ø 0 ±íÊ¾Õı³££¬1 ³ö´í¡£
// Èç¹ûÖ´ĞĞÃüÁî´í£¬ÔòÔÙ¶Á´íÎó¼Ä´æÆ÷ HD_ERROR(0x1f1)¡£
static int win_result(void)
{
	int i=inb_p(HD_STATUS);

	if ((i & (BUSY_STAT | READY_STAT | WRERR_STAT | SEEK_STAT | ERR_STAT))
		== (READY_STAT | SEEK_STAT))
		return(0); /* ok */
	if (i&1) i=inb(HD_ERROR);
	return (1);
}

// ÏòÓ²ÅÌ¿ØÖÆÆ÷·¢ËÍÃüÁî¿é¡£
// µ÷ÓÃ²ÎÊı:drive - Ó²ÅÌºÅ(0-1); nsect - ¶ÁĞ´ÉÈÇøÊı;
// sect - ÆğÊ¼ÉÈÇø; head - ´ÅÍ·ºÅ;
// cyl - ÖùÃæºÅ; cmd - ÃüÁîÂë(²Î¼û¿ØÖÆÆ÷ÃüÁîÁĞ±í£¬±í 6.3);
// *intr_addr() - Ó²ÅÌÖĞ¶Ï·¢ÉúÊ±´¦Àí³ÌĞòÖĞ½«µ÷ÓÃµÄ C ´¦Àíº¯Êı¡£
static void hd_out(unsigned int drive,unsigned int nsect,unsigned int sect,
		unsigned int head,unsigned int cyl,unsigned int cmd,
		void (*intr_addr)(void))
{
	register int port asm("dx"); 	//port ±äÁ¿¶ÔÓ¦¼Ä´æÆ÷ dx£»ÕâĞ´·¨£¬Ö®Ç°Ã»Óöµ½¹ı¡£

	if (drive>1 || head>15)			// Èç¹ûÇı¶¯Æ÷ºÅ(0,1)>1 »ò´ÅÍ·ºÅ>15£¬Ôò³ÌĞò²»Ö§³Ö¡£
		panic("Trying to write bad sector");
	if (!controller_ready())		// Èç¹ûµÈ´ıÒ»¶ÎÊ±¼äºóÈÔÎ´¾ÍĞ÷Ôò³ö´í£¬ËÀ»ú¡£
		panic("HD controller not ready");

	//ÕâÊÇ¸öº¯ÊıÖ¸Õë£¬ÓÖDEVICE_INTRºê¶¨Òåºó·ÖÅä
	// do_hd º¯ÊıÖ¸Õë½«ÔÚÓ²ÅÌÖĞ¶Ï³ÌĞòÖĞ±»µ÷ÓÃ¡£
	do_hd = intr_addr;		
	
	outb_p(hd_info[drive].ctl,HD_CMD);		// Ïò¿ØÖÆ¼Ä´æÆ÷(0x3f6)Êä³ö¿ØÖÆ×Ö½Ú¡£
	port=HD_DATA;							// ÖÃ dx ÎªÊı¾İ¼Ä´æÆ÷¶Ë¿Ú(0x1f0)¡£
	outb_p(hd_info[drive].wpcom>>2,++port);	// ²ÎÊı:Ğ´Ô¤²¹³¥ÖùÃæºÅ(Ğè³ı 4)¡£
	outb_p(nsect,++port);					// ²ÎÊı:¶Á/Ğ´ÉÈÇø×ÜÊı¡££
	outb_p(sect,++port);					// ²ÎÊı:ÆğÊ¼ÉÈÇø¡£
	outb_p(cyl,++port);						// ²ÎÊı:ÖùÃæºÅµÍ 8 Î»¡£
	outb_p(cyl>>8,++port);					// ²ÎÊı:ÖùÃæºÅ¸ß 8 Î»¡£
	outb_p(0xA0|(drive<<4)|head,++port);	// ²ÎÊı:Çı¶¯Æ÷ºÅ+´ÅÍ·ºÅ¡£
	outb(cmd,++port);						// ÃüÁî:Ó²ÅÌ¿ØÖÆÃüÁî¡£
}

// µÈ´ıÓ²ÅÌ¾ÍĞ÷¡£Ò²¼´Ñ­»·µÈ´ıÖ÷×´Ì¬¿ØÖÆÆ÷Ã¦±êÖ¾Î»¸´Î»¡£
// Èô½öÓĞ¾ÍĞ÷»òÑ°µÀ½áÊø±êÖ¾ÖÃÎ»£¬Ôò³É¹¦£¬·µ»Ø 0¡£
// Èô¾­¹ıÒ»¶ÎÊ±¼äÈÔÎªÃ¦£¬Ôò·µ»Ø 1¡£
static int drive_busy(void)
{
	unsigned int i;

	for (i = 0; i < 10000; i++)					// Ñ­»·µÈ´ı¾ÍĞ÷±êÖ¾Î»ÖÃÎ»¡£
		if (READY_STAT == (inb_p(HD_STATUS) & (BUSY_STAT|READY_STAT)))
			break;
	i = inb(HD_STATUS);							// ÔÙÈ¡Ö÷¿ØÖÆÆ÷×´Ì¬×Ö½Ú¡£
	i &= BUSY_STAT | READY_STAT | SEEK_STAT;	// ¼ì²âÃ¦Î»¡¢¾ÍĞ÷Î»ºÍÑ°µÀ½áÊøÎ»
	if (i == READY_STAT | SEEK_STAT)			// Èô½öÓĞ¾ÍĞ÷»òÑ°µÀ½áÊø±êÖ¾£¬Ôò·µ»Ø 0¡£
		return(0);
	printk("HD controller times out\n\r");		// ·ñÔòµÈ´ı³¬Ê±£¬ÏÔÊ¾ĞÅÏ¢¡£²¢·µ»Ø 1¡£
	return(1);
}

// Õï¶Ï¸´Î»(ÖØĞÂĞ£Õı)Ó²ÅÌ¿ØÖÆÆ÷¡£
static void reset_controller(void)
{
	int	i;

	outb(4,HD_CMD);							// Ïò¿ØÖÆ¼Ä´æÆ÷¶Ë¿Ú·¢ËÍ¿ØÖÆ×Ö½Ú(4-¸´Î»)
	for(i = 0; i < 100; i++) nop();			// µÈ´ıÒ»¶ÎÊ±¼ä(Ñ­»·¿Õ²Ù×÷)
	outb(hd_info[0].ctl & 0x0f ,HD_CMD);	// ÔÙ·¢ËÍÕı³£µÄ¿ØÖÆ×Ö½Ú(²»½ûÖ¹ÖØÊÔ¡¢ÖØ¶Á)¡£
	if (drive_busy())						// ÈôµÈ´ıÓ²ÅÌ¾ÍĞ÷³¬Ê±£¬ÔòÏÔÊ¾³ö´íĞÅÏ¢¡£
		printk("HD-controller still busy\n\r");
	if ((i = inb(HD_ERROR)) != 1)			// È¡´íÎó¼Ä´æÆ÷£¬Èô²»µÈÓÚ 1(ÎŞ´íÎó)Ôò³ö´í¡£
		printk("HD-controller reset failed: %02x\n\r",i);
}

// ¸´Î»Ó²ÅÌ nr¡£Ê×ÏÈ¸´Î»(ÖØĞÂĞ£Õı)Ó²ÅÌ¿ØÖÆÆ÷¡£È»ºó·¢ËÍÓ²ÅÌ¿ØÖÆÆ÷ÃüÁî¡°½¨Á¢Çı¶¯Æ÷²ÎÊı¡±£¬
// ÆäÖĞ recal_intr()ÊÇÔÚÓ²ÅÌÖĞ¶Ï´¦Àí³ÌĞòÖĞµ÷ÓÃµÄÖØĞÂĞ£Õı´¦Àíº¯Êı¡£
static void reset_hd(int nr)
{
	//¸´Î»(ÖØĞÂĞ£Õı)Ó²ÅÌ¿ØÖÆÆ÷
	reset_controller();	
	//·¢ËÍÓ²ÅÌ¿ØÖÆÆ÷ÃüÁî¡°½¨Á¢Çı¶¯Æ÷²ÎÊı¡±
	hd_out(nr,hd_info[nr].sect,hd_info[nr].sect,hd_info[nr].head-1,
		hd_info[nr].cyl,WIN_SPECIFY,&recal_intr);
}

// ÒâÍâÓ²ÅÌÖĞ¶Ïµ÷ÓÃº¯Êı¡£
// ·¢ÉúÒâÍâÓ²ÅÌÖĞ¶ÏÊ±£¬Ó²ÅÌÖĞ¶Ï´¦Àí³ÌĞòÖĞµ÷ÓÃµÄÄ¬ÈÏ C ´¦Àíº¯Êı¡£
// ÔÚ±»µ÷ÓÃº¯ÊıÖ¸ÕëÎª¿ÕÊ±µ÷ÓÃ¸Ãº¯Êı¡£
void unexpected_hd_interrupt(void)
{
	printk("Unexpected HD interrupt\n\r");
}

// ¶ÁĞ´Ó²ÅÌÊ§°Ü´¦Àíµ÷ÓÃº¯Êı¡£
static void bad_rw_intr(void)
{
	// Èç¹û¶ÁÉÈÇøÊ±µÄ³ö´í´ÎÊı´óÓÚ»òµÈÓÚ 7 ´ÎÊ±£¬Ôò½áÊøÇëÇó²¢»½ĞÑµÈ´ı¸ÃÇëÇóµÄ½ø³Ì£¬
	// ¶øÇÒ¶ÔÓ¦»º³åÇø¸üĞÂ±êÖ¾¸´Î»(Ã»ÓĞ¸üĞÂ)¡£
	if (++CURRENT->errors >= MAX_ERRORS)
		end_request(0);
	// Èç¹û¶ÁÒ»ÉÈÇøÊ±µÄ³ö´í´ÎÊıÒÑ¾­´óÓÚ 3 ´Î£¬ ÔòÒªÇóÖ´ĞĞ¸´Î»Ó²ÅÌ¿ØÖÆÆ÷²Ù×÷¡£
	if (CURRENT->errors > MAX_ERRORS/2)
		reset = 1;
}

// ¶Á²Ù×÷ÖĞ¶Ïµ÷ÓÃº¯Êı¡£½«ÔÚÓ²ÅÌ¶ÁÃüÁî½áÊøÊ±Òı·¢µÄÖĞ¶Ï¹ı³ÌÖĞ±»µ÷ÓÃ¡£
// ¸Ãº¯ÊıÊ×ÏÈÅĞ¶Ï´Ë´Î¶ÁÃüÁî²Ù×÷ÊÇ·ñ³ö´í¡£ÈôÃüÁî½áÊøºó¿ØÖÆÆ÷»¹´¦ÓÚÃ¦×´Ì¬£¬»òÕßÃüÁîÖ´ĞĞ´íÎó£¬ 
// Ôò´¦ÀíÓ²ÅÌ²Ù×÷Ê§°ÜÎÊÌâ£¬½Ó×ÅÇëÇóÓ²ÅÌ×÷¸´Î»´¦Àí²¢Ö´ĞĞÆäËüÇëÇóÏî¡£
// Èç¹û¶ÁÃüÁîÃ»ÓĞ³ö´í£¬Ôò´ÓÊı¾İ¼Ä´æÆ÷¶Ë¿Ú°ÑÒ»¸öÉÈÇøµÄÊı¾İ¶Áµ½ÇëÇóÏîµÄ»º³åÇøÖĞ£¬²¢µİ¼õÇëÇóÏî 
// ËùĞè¶ÁÈ¡µÄÉÈÇøÊıÖµ¡£Èôµİ¼õºó²»µÈÓÚ 0£¬±íÊ¾±¾ÏîÇëÇó»¹ÓĞÊı¾İÃ»È¡Íê£¬ÓÚÊÇÖ±½Ó·µ»Ø£¬µÈ´ıÓ²ÅÌ 
// ÔÚ¶Á³öÁíÒ»¸öÉÈÇøÊı¾İºóµÄÖĞ¶Ï¡£·ñÔò±íÃ÷±¾ÇëÇóÏîËùĞèµÄËùÓĞÉÈÇø¶¼ÒÑ¶ÁÍê£¬ÓÚÊÇ´¦Àí±¾´ÎÇëÇóÏî 
// ½áÊøÊÂÒË¡£×îºóÔÙ´Îµ÷ÓÃ do_hd_request()£¬È¥´¦ÀíÆäËüÓ²ÅÌÇëÇóÏî¡£
static void read_intr(void)
{
	if (win_result()) {			// Èô¿ØÖÆÆ÷Ã¦¡¢¶ÁĞ´´í»òÃüÁîÖ´ĞĞ´í£¬
		bad_rw_intr();			// Ôò½øĞĞ¶ÁĞ´Ó²ÅÌÊ§°Ü´¦Àí£¬
		do_hd_request();		// È»ºóÔÙ´ÎÇëÇóÓ²ÅÌ×÷ÏàÓ¦(¸´Î»)´¦Àí¡£
		return;
	}

	// ½«Êı¾İ´ÓÊı¾İ¼Ä´æÆ÷¿Ú¶Áµ½ÇëÇó½á¹¹»º³åÇø¡£
	port_read(HD_DATA,CURRENT->buffer,256);// ×¢Òâ:256 ÊÇÖ¸ÄÚ´æ×Ö£¬Ò²¼´ 512 ×Ö½Ú¡£

	CURRENT->errors = 0;			// Çå³ö´í´ÎÊı¡£
	CURRENT->buffer += 512;			// µ÷Õû»º³åÇøÖ¸Õë£¬Ö¸ÏòĞÂµÄ¿ÕÇø¡£
	CURRENT->sector++;				// ÆğÊ¼ÉÈÇøºÅ¼Ó 1£¬

	if (--CURRENT->nr_sectors) {	// Èç¹ûËùĞè¶Á³öµÄÉÈÇøÊı»¹Ã»ÓĞ¶ÁÍê£¬
		do_hd = &read_intr;			// ÔòÔÙ´ÎÖÃÓ²ÅÌµ÷ÓÃ C º¯ÊıÖ¸ÕëÎª read_intr()£¬
		return;// ÒòÎªÓ²ÅÌÖĞ¶Ï´¦Àí³ÌĞòÃ¿´Îµ÷ÓÃ do_hd Ê±¶¼»á½«¸Ãº¯ÊıÖ¸ÕëÖÃ¿Õ¡£²Î¼û system_call.s
	}
	end_request(1);					// ÈôÈ«²¿ÉÈÇøÊı¾İÒÑ¾­¶ÁÍê£¬Ôò´¦ÀíÇëÇó½áÊøÊÂÒË¡£
	do_hd_request();				//Ö´ĞĞÆäËüÓ²ÅÌÇëÇó²Ù×÷¡£
}

// Ğ´ÉÈÇøÖĞ¶Ïµ÷ÓÃº¯Êı¡£ÔÚÓ²ÅÌÖĞ¶Ï´¦Àí³ÌĞòÖĞ±»µ÷ÓÃ¡£
// ÔÚĞ´ÃüÁîÖ´ĞĞºó£¬»á²úÉúÓ²ÅÌÖĞ¶ÏĞÅºÅ£¬Ö´ĞĞÓ²ÅÌÖĞ¶Ï´¦Àí³ÌĞò£¬
// ´ËÊ±ÔÚÓ²ÅÌÖĞ¶Ï´¦Àí³ÌĞòÖĞµ÷ÓÃµÄ C º¯ÊıÖ¸Õë do_hd()ÒÑ¾­Ö¸Ïò write_intr()£¬
// Òò´Ë»áÔÚĞ´²Ù×÷Íê³É(»ò³ö´í)ºó£¬Ö´ĞĞ¸Ãº¯Êı¡£
static void write_intr(void)
{
	if (win_result()) {				// Èç¹ûÓ²ÅÌ¿ØÖÆÆ÷·µ»Ø´íÎóĞÅÏ¢£¬
		bad_rw_intr();				// ÔòÊ×ÏÈ½øĞĞÓ²ÅÌ¶ÁĞ´Ê§°Ü´¦Àí£¬
		do_hd_request();			// È»ºóÔÙ´ÎÇëÇóÓ²ÅÌ×÷ÏàÓ¦(¸´Î»)´¦Àí£¬
		return;						// È»ºó·µ»Ø(Ò²ÍË³öÁË´Ë´ÎÓ²ÅÌÖĞ¶Ï)¡£
	}
	if (--CURRENT->nr_sectors) {	// ·ñÔò½«ÓûĞ´ÉÈÇøÊı¼õ 1£¬Èô»¹ÓĞÉÈÇøÒªĞ´£¬Ôò
		CURRENT->sector++;			// µ±Ç°ÇëÇóÆğÊ¼ÉÈÇøºÅ+1£¬
		CURRENT->buffer += 512;		// µ÷ÕûÇëÇó»º³åÇøÖ¸Õë£¬
		do_hd = &write_intr;		// ÖÃÓ²ÅÌÖĞ¶Ï³ÌĞòµ÷ÓÃº¯ÊıÖ¸ÕëÎª write_intr()£¬
		port_write(HD_DATA,CURRENT->buffer,256);// ÔÙÏòÊı¾İ¼Ä´æÆ÷¶Ë¿ÚĞ´ 256 ×Ö¡£
		return;									// ·µ»ØµÈ´ıÓ²ÅÌÔÙ´ÎÍê³ÉĞ´²Ù×÷ºóµÄÖĞ¶Ï´¦Àí¡£
	}
	end_request(1);					// ÈôÈ«²¿ÉÈÇøÊı¾İÒÑ¾­Ğ´Íê£¬Ôò´¦ÀíÇëÇó½áÊøÊÂÒË£¬
	do_hd_request();				// Ö´ĞĞÆäËüÓ²ÅÌÇëÇó²Ù×÷¡£
}

// Ó²ÅÌÖØĞÂĞ£Õı£¨¸´Î»£©ÖĞ¶Ïµ÷ÓÃº¯Êı¡£ÔÚÓ²ÅÌÖĞ¶Ï´¦Àí³ÌĞòÖĞ±»µ÷ÓÃ¡£
// Èç¹ûÓ²ÅÌ¿ØÖÆÆ÷·µ»Ø´íÎóĞÅÏ¢£¬ÔòÊ×ÏÈ½øĞĞÓ²ÅÌ¶ÁĞ´Ê§°Ü´¦Àí£¬È»ºóÇëÇóÓ²ÅÌ×÷ÏàÓ¦(¸´Î»)´¦Àí¡£
static void recal_intr(void)
{
	if (win_result())
		bad_rw_intr();
	do_hd_request();
}

// Ö´ĞĞÓ²ÅÌ¶ÁĞ´ÇëÇó²Ù×÷¡£
// ÈôÇëÇóÏîÊÇ¿éÉè±¸µÄµÚ 1 ¸ö£¬Ôò¿éÉè±¸µ±Ç°ÇëÇóÏîÖ¸Õë»áÖ±½ÓÖ¸Ïò¸ÃÇëÇóÏî£¬²¢»áÁ¢¿Ìµ÷ÓÃ±¾º¯ÊıÖ´ĞĞ¶ÁĞ´²Ù×÷¡£
// ·ñÔòÔÚÒ»¸ö¶ÁĞ´²Ù×÷Íê³É¶øÒı·¢µÄÓ²ÅÌÖĞ¶Ï¹ı³ÌÖĞ£¬Èô»¹ÓĞÇëÇóÏîĞèÒª´¦Àí£¬ÔòÒ²»áÔÚÖĞ¶Ï¹ı³ÌÖĞµ÷ÓÃ±¾º¯Êı¡£
void do_hd_request(void)
{
	int i,r;
	unsigned int block,dev;
	unsigned int sec,head,cyl;
	unsigned int nsect;

	// ¼ì²âÇëÇóÏîµÄºÏ·¨ĞÔ£¬ÈôÒÑÃ»ÓĞÇëÇóÏîÔòÍË³ö¡£
	INIT_REQUEST;
	// È¡Éè±¸ºÅÖĞµÄ×ÓÉè±¸ºÅ(¼ûÁĞ±íºó¶ÔÓ²ÅÌÉè±¸ºÅµÄËµÃ÷)¡£×ÓÉè±¸ºÅ¼´ÊÇÓ²ÅÌÉÏµÄ·ÖÇøºÅ¡£
	dev = MINOR(CURRENT->dev);
	block = CURRENT->sector;
	// Èç¹û×ÓÉè±¸ºÅ²»´æÔÚ»òÕßÆğÊ¼ÉÈÇø´óÓÚ¸Ã·ÖÇøÉÈÇøÊı-2£¬Ôò½áÊø¸ÃÇëÇó£¬²¢Ìø×ªµ½±êºÅ repeat ´¦¡£
	// ÒòÎªÒ»´ÎÒªÇó¶ÁĞ´ 2 ¸öÉÈÇø£¨512*2 ×Ö½Ú£©£¬ËùÒÔÇëÇóµÄÉÈÇøºÅ²»ÄÜ´óÓÚ·ÖÇøÖĞ×îºóµ¹ÊıµÚ¶ş¸öÉÈÇøºÅ¡£
	if (dev >= 5*NR_HD || block+2 > hd[dev].nr_sects) {
		end_request(0);
		goto repeat;//repeatÔÚINIT_REQUESTÖĞ¶¨ÒåµÄ¡£
	}
	// Í¨¹ı¼ÓÉÏ±¾·ÖÇøµÄÆğÊ¼ÉÈÇøºÅ£¬°Ñ½«ËùĞè¶ÁĞ´µÄ¿é¶ÔÓ¦µ½Õû¸öÓ²ÅÌµÄ¾ø¶ÔÉÈÇøºÅÉÏ¡£
	block += hd[dev].start_sect;
	dev /= 5;// ´ËÊ± dev ´ú±íÓ²ÅÌºÅ£¨ÊÇµÚ 1 ¸öÓ²ÅÌ(0)»¹ÊÇµÚ 2 ¸ö(1)£©¡£
	// ÏÂÃæÇ¶Èë»ã±à´úÂëÓÃÀ´´ÓÓ²ÅÌĞÅÏ¢½á¹¹ÖĞ¸ù¾İÆğÊ¼ÉÈÇøºÅºÍÃ¿´ÅµÀÉÈÇøÊı¼ÆËãÔÚ´ÅµÀÖĞµÄ
	// ÉÈÇøºÅ(sec)¡¢ËùÔÚÖùÃæºÅ(cyl)ºÍ´ÅÍ·ºÅ(head)¡£
	__asm__("divl %4":"=a" (block),"=d" (sec):"0" (block),"1" (0),
		"r" (hd_info[dev].sect));
	__asm__("divl %4":"=a" (cyl),"=d" (head):"0" (block),"1" (0),
		"r" (hd_info[dev].head));
	sec++;
	nsect = CURRENT->nr_sectors;
	// Èç¹û reset ±êÖ¾ÊÇÖÃÎ»µÄ£¬ÔòÖ´ĞĞ¸´Î»²Ù×÷¡£¸´Î»Ó²ÅÌºÍ¿ØÖÆÆ÷£¬²¢ÖÃĞèÒªÖØĞÂĞ£Õı±êÖ¾£¬·µ»Ø¡£
	if (reset) {
		reset = 0;
		recalibrate = 1;
		reset_hd(CURRENT_DEV);
		return;
	}
	// Èç¹ûÖØĞÂĞ£Õı±êÖ¾(recalibrate)ÖÃÎ»£¬ÔòÊ×ÏÈ¸´Î»¸Ã±êÖ¾£¬È»ºóÏòÓ²ÅÌ¿ØÖÆÆ÷·¢ËÍÖØĞÂĞ£ÕıÃüÁî¡£
	// ¸ÃÃüÁî»áÖ´ĞĞÑ°µÀ²Ù×÷£¬ÈÃ´¦ÓÚÈÎºÎµØ·½µÄ´ÅÍ·ÒÆ¶¯µ½ 0 ÖùÃæ¡£
	if (recalibrate) {
		recalibrate = 0;
		hd_out(dev,hd_info[CURRENT_DEV].sect,0,0,0,
			WIN_RESTORE,&recal_intr);
		return;
	}	
	// Èç¹ûµ±Ç°ÇëÇóÊÇĞ´ÉÈÇø²Ù×÷£¬Ôò·¢ËÍĞ´ÃüÁî£¬Ñ­»·¶ÁÈ¡×´Ì¬¼Ä´æÆ÷ĞÅÏ¢²¢ÅĞ¶ÏÇëÇó·şÎñ±êÖ¾
	// DRQ_STAT ÊÇ·ñÖÃÎ»¡£DRQ_STAT ÊÇÓ²ÅÌ×´Ì¬¼Ä´æÆ÷µÄÇëÇó·şÎñÎ»£¬±íÊ¾Çı¶¯Æ÷ÒÑ¾­×¼±¸ºÃÔÚÖ÷»úºÍ
	// Êı¾İ¶Ë¿ÚÖ®¼ä´«ÊäÒ»¸ö×Ö»òÒ»¸ö×Ö½ÚµÄÊı¾İ¡£
	if (CURRENT->cmd == WRITE) {
		hd_out(dev,nsect,sec,head,cyl,WIN_WRITE,&write_intr);
		for(i=0 ; i<3000 && !(r=inb_p(HD_STATUS)&DRQ_STAT) ; i++)
			/* nothing */ ;
		// Èç¹ûÇëÇó·şÎñ DRQ ÖÃÎ»ÔòÍË³öÑ­»·¡£ÈôµÈµ½Ñ­»·½áÊøÒ²Ã»ÓĞÖÃÎ»£¬
		// Ôò±íÊ¾´Ë´ÎĞ´Ó²ÅÌ²Ù×÷Ê§°Ü£¬È¥´¦ÀíÏÂÒ»¸öÓ²ÅÌÇëÇó¡£
		// ·ñÔòÏòÓ²ÅÌ¿ØÖÆÆ÷Êı¾İ¼Ä´æÆ÷¶Ë¿Ú HD_DATA Ğ´Èë 1 ¸öÉÈÇøµÄÊı¾İ¡£
		if (!r) {
			bad_rw_intr();
			goto repeat;//repeatÔÚINIT_REQUESTÖĞ¶¨ÒåµÄ¡£
		}
		port_write(HD_DATA,CURRENT->buffer,256);
	// Èç¹ûµ±Ç°ÇëÇóÊÇ¶ÁÓ²ÅÌÉÈÇø£¬ÔòÏòÓ²ÅÌ¿ØÖÆÆ÷·¢ËÍ¶ÁÉÈÇøÃüÁî¡£
	} else if (CURRENT->cmd == READ) {
		hd_out(dev,nsect,sec,head,cyl,WIN_READ,&read_intr);
	} else
		panic("unknown hd-command");
}

// Ó²ÅÌÏµÍ³³õÊ¼»¯¡£
void hd_init(void)
{
	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;// do_hd_request()¡£
	// ÉèÖÃÓ²ÅÌÖĞ¶ÏÃÅÏòÁ¿ int 0x2E(46)¡£
	set_intr_gate(0x2E,&hd_interrupt);
	// ¸´Î»½ÓÁªµÄÖ÷ 8259A int2 µÄÆÁ±ÎÎ»£¬ÔÊĞí´ÓÆ¬·¢³öÖĞ¶ÏÇëÇóĞÅºÅ¡£
	outb_p(inb_p(0x21)&0xfb,0x21);
	// ¸´Î»Ó²ÅÌµÄÖĞ¶ÏÇëÇóÆÁ±ÎÎ»£¨ÔÚ´ÓÆ¬ÉÏ£©£¬ÔÊĞíÓ²ÅÌ¿ØÖÆÆ÷·¢ËÍÖĞ¶ÏÇëÇóĞÅºÅ¡£
	outb(inb_p(0xA1)&0xbf,0xA1);
}
