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
 * ąžłĚĐňĘÇľ×˛ăÓ˛ĹĚÖĐśĎ¸¨ÖúłĚĐňĄŁÖ÷ŇŞÓĂÓÚÉ¨ĂčÇëÇóÁĐąíŁŹĘšÓĂÖĐśĎÔÚşŻĘýÖŽźäĚř×ŞĄŁ
 * ÓÉÓÚËůÓĐľÄşŻĘýśźĘÇÔÚÖĐśĎŔďľ÷ÓĂľÄŁŹËůŇÔŐâĐŠşŻĘý˛ťżÉŇÔËŻĂßĄŁÇëĚŘąđ×˘ŇâĄŁ
 * ÓÉ Drew Eckhardt ĐŢ¸ÄŁŹŔűÓĂ CMOS ĐĹĎ˘źě˛âÓ˛ĹĚĘýĄŁ
 */

#include <linux/config.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/hdreg.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/segment.h>

// ąŘĐëÔÚ blk.h ÎÄźţÖŽÇ°ś¨ŇĺĎÂĂćľÄÖ÷Éčą¸şĹłŁĘýŁŹŇňÎŞ blk.h ÎÄźţÖĐŇŞÓĂľ˝¸ĂłŁĘýĄŁ
#define MAJOR_NR 3	// Ó˛ĹĚÖ÷Éčą¸şĹĘÇ 3ĄŁ
#include "blk.h"

//śÁCMOS˛ÎĘýşęşŻĘýĄŁ
#define CMOS_READ(addr) ({ \
outb_p(0x80|addr,0x70); \
inb_p(0x71); \
})

/* Max read/write errors/sector */
#define MAX_ERRORS	7			// śÁ/Đ´Ňť¸öÉČÇřĘąÔĘĐíľÄ×îśŕłö´í´ÎĘýĄŁ
#define MAX_HD		2			// ĎľÍłÖ§łÖľÄ×îśŕÓ˛ĹĚĘýĄŁ

static void recal_intr(void);	// Ó˛ĹĚÖĐśĎłĚĐňÔÚ¸´Îť˛Ů×÷Ęąťáľ÷ÓĂľÄÖŘĐÂĐŁŐýşŻĘý

static int recalibrate = 1;		// ÖŘĐÂĐŁŐýąęÖžĄŁ˝Ť´ĹÍˇŇĆśŻľ˝ 0 ÖůĂćĄŁ
static int reset = 1;			// ¸´ÎťąęÖžĄŁľąˇ˘ÉúśÁĐ´´íÎóĘąťáÉčÖĂ¸ĂąęÖžŁŹŇÔ¸´ÎťÓ˛ĹĚşÍżŘÖĆĆ÷ĄŁ

/*
 *  This struct defines the HD's and their types.
 */
/* ĎÂĂć˝áššś¨ŇĺÁËÓ˛ĹĚ˛ÎĘýź°ŔŕĐÍ */
// ¸÷×ÖśÎˇÖąđĘÇ´ĹÍˇĘýĄ˘Ăż´ĹľŔÉČÇřĘýĄ˘ÖůĂćĘýĄ˘Đ´Ç°Ô¤˛šłĽÖůĂćşĹĄ˘´ĹÍˇ×ĹÂ˝ÇřÖůĂćşĹĄ˘żŘÖĆ×Ö˝ÚĄŁ 
// ËüĂÇľÄşŹŇĺÇë˛ÎźűłĚĐňÁĐąíşóľÄËľĂ÷ĄŁ
struct hd_i_struct {
	int head,sect,cyl,wpcom,lzone,ctl;
	};

// ČçšűŇŃž­ÔÚ include/linux/config.h ÍˇÎÄźţÖĐś¨ŇĺÁË HD_TYPEŁŹ
// žÍČĄĆäÖĐś¨ŇĺşĂľÄ˛ÎĘý×÷ÎŞhd_info[]ľÄĘýžÝĄŁ
// ˇńÔňŁŹĎČÄŹČĎśźÉčÎŞ 0 ÖľŁŹÔÚ setup()şŻĘýÖĐťá˝řĐĐÉčÖĂĄŁ
#ifdef HD_TYPE
struct hd_i_struct hd_info[] = { HD_TYPE };
#define NR_HD ((sizeof (hd_info))/(sizeof (struct hd_i_struct)))	// źĆËăÓ˛ĹĚ¸öĘýĄŁ
#else
struct hd_i_struct hd_info[] = { {0,0,0,0,0,0},{0,0,0,0,0,0} };
static int NR_HD = 0;	//ÉĎĂćĘÇ2¸öŁŹŐâŔďÓÖĘÇ0ŁŹĆćšÖ
#endif

// ś¨ŇĺÓ˛ĹĚˇÖÇř˝áššĄŁ¸řłöĂż¸öˇÖÇřľÄÎďŔíĆđĘźÉČÇřşĹĄ˘ˇÖÇřÉČÇř×ÜĘýĄŁ
// ĆäÖĐ 5 ľÄąśĘý´ŚľÄĎî(ŔýČç hd[0]şÍ hd[5]ľČ)´úąíŐű¸öÓ˛ĹĚÖĐľÄ˛ÎĘýĄŁ
static struct hd_struct {
	long start_sect;	//ˇÖÇřĆđĘźÉČÇřşĹ
	long nr_sects;		//ˇÖÇřÉČÇř×ÜĘý
} hd[5*MAX_HD]={{0,0},};

// śÁśËżÚ portŁŹš˛śÁ nr ×ÖŁŹąŁ´ćÔÚ buf ÖĐĄŁ
// INSÖ¸ÁîżÉ´ÓDXÖ¸łöľÄÍâÉčśËżÚĘäČëŇť¸ö×Ö˝Úťň×Öľ˝ÓÉES: DIÖ¸ś¨ľÄ´ć´˘Ć÷ÖĐĄŁ
#define port_read(port,buf,nr) \
__asm__("cld;rep;insw"::"d" (port),"D" (buf),"c" (nr):"cx","di")

// Đ´śËżÚ portŁŹš˛Đ´ nr ×ÖŁŹ´Ó buf ÖĐČĄĘýžÝĄŁ
#define port_write(port,buf,nr) \
__asm__("cld;rep;outsw"::"d" (port),"S" (buf),"c" (nr):"cx","si")

extern void hd_interrupt(void);	// Ó˛ĹĚÖĐśĎšýłĚ
extern void rd_load(void);		// ĐéÄâĹĚ´´˝¨źÓÔŘşŻĘý

/* This may be used only once, enforced by 'static int callable' */
/* ĎÂĂć¸ĂşŻĘýÖťÔÚłőĘźťŻĘąąťľ÷ÓĂŇť´ÎĄŁÓĂž˛ĚŹąäÁż callable ×÷ÎŞżÉľ÷ÓĂąęÖžĄŁ*/
// ¸ĂşŻĘýľÄ˛ÎĘýÓÉłőĘźťŻłĚĐň init/main.c ľÄ init ×ÓłĚĐňÉčÖĂÎŞÖ¸Ďň 0x90080 ´ŚŁŹ
// ´Ë´Ś´ćˇĹ×Ĺ setup.s łĚĐň´Ó BIOS ČĄľĂľÄ 2 ¸öÓ˛ĹĚľÄťůąž˛ÎĘýąí(32 ×Ö˝Ú)ĄŁ
// Ó˛ĹĚ˛ÎĘýąíĐĹĎ˘˛ÎźűĎÂĂćÁĐąíşóľÄËľĂ÷ĄŁ
// ąžşŻĘýÖ÷ŇŞšŚÄÜĘÇśÁČĄ CMOS şÍÓ˛ĹĚ˛ÎĘýąíĐĹĎ˘ŁŹÓĂÓÚÉčÖĂÓ˛ĹĚˇÖÇř˝ášš hdŁŹ
// ˛˘źÓÔŘ RAM ĐéÄâĹĚşÍ¸ůÎÄźţĎľÍłĄŁ
int sys_setup(void * BIOS)
{
	static int callable = 1;
	int i,drive;
	unsigned char cmos_disks;
	struct partition *p;
	struct buffer_head * bh;

	if (!callable)//ąŁÖ¤Őâ¸öşŻĘýÖťÄÜąťÖ´ĐĐŇť´Î
		return -1;
	callable = 0;

// ČçšűĂťÓĐÔÚ config.h ÖĐś¨ŇĺÓ˛ĹĚ˛ÎĘýŁŹžÍ´Ó 0x90080 ´ŚśÁČëĄŁ	
#ifndef HD_TYPE
	for (drive=0 ; drive<2 ; drive++) {
		hd_info[drive].cyl = *(unsigned short *) BIOS;			//ÖůĂćĘý
		hd_info[drive].head = *(unsigned char *) (2+BIOS);		//´ĹÍˇĘý
		hd_info[drive].wpcom = *(unsigned short *) (5+BIOS);	//Đ´Ç°Ô¤˛šłĽÖůĂćşĹ
		hd_info[drive].ctl = *(unsigned char *) (8+BIOS);		//żŘÖĆ×Ö˝Ú
		hd_info[drive].lzone = *(unsigned short *) (12+BIOS);	//´ĹÍˇ×ĹÂ˝ÇřÖůĂćşĹ
		hd_info[drive].sect = *(unsigned char *) (14+BIOS);		//Ăż´ĹľŔÉČÇřĘý
		BIOS += 16;	// Ăż¸öÓ˛ĹĚľÄ˛ÎĘýąíł¤ 16 ×Ö˝ÚŁŹŐâŔď BIOS Ö¸ĎňĎÂŇť¸öąí
	}

	// setup.s łĚĐňÔÚČĄ BIOS ÖĐľÄÓ˛ĹĚ˛ÎĘýąíĐĹĎ˘ĘąŁŹČçšűÖťÓĐ 1 ¸öÓ˛ĹĚŁŹ
	// žÍťá˝ŤśÔÓŚľÚ 2 ¸öÓ˛ĹĚľÄ 16 ×Ö˝ÚČŤ˛żÇĺÁăĄŁ
	// Ňň´ËŐâŔďÖťŇŞĹĐśĎľÚ 2 ¸öÓ˛ĹĚÖůĂćĘýĘÇˇńÎŞ 0 žÍżÉŇÔÖŞľŔÓĐĂťÓĐľÚ 2 ¸öÓ˛ĹĚÁËĄŁ
	if (hd_info[1].cyl)
		NR_HD=2;
	else
		NR_HD=1;
#endif

	for (i=0 ; i<NR_HD ; i++) {	//łőĘźťŻ´úąíŐűżéÓ˛ĹĚÄÇ¸öˇÖÇř
		hd[i*5].start_sect = 0;						// Ó˛ĹĚĆđĘźÉČÇřşĹ
		hd[i*5].nr_sects = hd_info[i].head*			// Ó˛ĹĚ×ÜÉČÇřĘýĄŁ
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
	 * ÎŇĂÇśÔ CMOS ÓĐšŘÓ˛ĹĚľÄĐĹĎ˘ÓĐĐŠťłŇÉ:żÉÄÜťáłöĎÖŐâŃůľÄÇéżöŁŹÎŇĂÇÓĐŇťżé SCSI/ESDI/ľČľÄ żŘÖĆĆ÷ŁŹ
	 * ËüĘÇŇÔ ST-506 ˇ˝Ę˝Óë BIOS źćČÝľÄŁŹŇňśřťáłöĎÖÔÚÎŇĂÇľÄ BIOS ˛ÎĘýąíÖĐŁŹľŤČ´ÓÖ˛ť ĘÇźÄ´ćĆ÷źćČÝľÄŁŹ
	 * Ňň´ËŐâĐŠ˛ÎĘýÔÚ CMOS ÖĐÓÖ˛ť´ćÔÚĄŁ
	 * ÁíÍâŁŹÎŇĂÇźŮÉč ST-506 ÇýśŻĆ÷(ČçšűÓĐľÄť°)ĘÇĎľÍłÖĐľÄťůąžÇýśŻĆ÷ŁŹŇ˛ź´ŇÔÇýśŻĆ÷ 1 ťň 2 łöĎÖľÄÇýśŻĆ÷ĄŁ
	 * ľÚ 1 ¸öÇýśŻĆ÷˛ÎĘý´ćˇĹÔÚ CMOS ×Ö˝Ú 0x12 ľÄ¸ß°ë×Ö˝ÚÖĐŁŹľÚ 2 ¸ö´ćˇĹÔÚľÍ°ë×Ö˝ÚÖĐĄŁ
	 * ¸Ă 4 Îť×Ö˝Ú ĐĹĎ˘żÉŇÔĘÇÇýśŻĆ÷ŔŕĐÍŁŹŇ˛żÉÄÜ˝öĘÇ 0xfĄŁ
	 * 0xf ąíĘžĘšÓĂ CMOS ÖĐ 0x19 ×Ö˝Ú×÷ÎŞÇýśŻĆ÷ 1 ľÄ 8 Îť ŔŕĐÍ×Ö˝ÚŁŹ
	 * ĘšÓĂ CMOS ÖĐ 0x1A ×Ö˝Ú×÷ÎŞÇýśŻĆ÷ 2 ľÄŔŕĐÍ×Ö˝ÚĄŁ 
	 * ×ÜÖŽŁŹŇť¸öˇÇÁăÖľŇâÎś×ĹÎŇĂÇÓĐŇť¸ö AT żŘÖĆĆ÷Ó˛ĹĚźćČÝľÄÇýśŻĆ÷ĄŁ
	 */

	// ŐâŔď¸ůžÝÉĎĘöÔ­ŔíŔ´źě˛âÓ˛ĹĚľ˝ľ×ĘÇˇńĘÇ AT żŘÖĆĆ÷źćČÝľÄĄŁ
	// ÓĐšŘ CMOS ĐĹĎ˘Çë˛Îźű 4.2.3.1 ˝ÚĄŁ
	if ((cmos_disks = CMOS_READ(0x12)) & 0xf0)
		if (cmos_disks & 0x0f)
			NR_HD = 2;
		else
			NR_HD = 1;
	else
		NR_HD = 0;

	// Čô NR_HD=0ŁŹÔňÁ˝¸öÓ˛ĹĚśź˛ťĘÇ AT żŘÖĆĆ÷źćČÝľÄŁŹÓ˛ĹĚĘýžÝ˝áššÇĺÁăĄŁ 
	// Čô NR_HD=1ŁŹÔň˝ŤľÚ 2 ¸öÓ˛ĹĚľÄ˛ÎĘýÇĺÁăĄŁ
	for (i = NR_HD ; i < 2 ; i++) {//ÓĐČ¤ľÄĐ´ˇ¨ŁŹĘĄČĽÁËĹĐśĎ
		hd[i*5].start_sect = 0;
		hd[i*5].nr_sects = 0;
	}

	// śÁČĄĂżŇť¸öÓ˛ĹĚÉĎľÚ 1 żéĘýžÝŁŹťńČĄˇÖÇřąíĐĹĎ˘ĄŁ
	for (drive=0 ; drive<NR_HD ; drive++) {
		if (!(bh = bread(0x300 + drive*5,0))) {//śÁČĄľÚŇť¸öĘýžÝżéľ˝ťşłĺÇř
			printk("Unable to read partition table of drive %d\n\r",
				drive);
			panic("");
		}
		if (bh->b_data[510] != 0x55 || (unsigned char)
		    bh->b_data[511] != 0xAA) {	// ĹĐśĎÓ˛ĹĚĐĹĎ˘ÓĐĐ§ąęÖž'55AA'
			printk("Bad partition table on drive %d\n\r",drive);
			panic("");
		}
		p = 0x1BE + (void *)bh->b_data;	// ˇÖÇřąíÎťÓÚÓ˛ĹĚľÚ 1 ÉČÇřľÄ 0x1BE ´ŚĄŁ
		for (i=1;i<5;i++,p++) {//ĂżżéÓ˛ĹĚśÁ4¸öˇÖÇřąí
			hd[i+5*drive].start_sect = p->start_sect;
			hd[i+5*drive].nr_sects = p->nr_sects;
		}
		brelse(bh);	// ˇÖÇřąíÎťÓÚÓ˛ĹĚľÚ 1 ÉČÇřľÄ 0x1BE ´ŚĄŁ
	}
	if (NR_HD)		//ÓĐÓ˛ĹĚÇŇŇŃśÁČëŁŹÔň´ňÓĄĐĹĎ˘ĄŁ
		printk("Partition table%s ok.\n\r",(NR_HD>1)?"s":"");//´ňÓĄĐĹĎ˘ťšżźÂÇźÓ¸´ĘýŁŹšţšţ
	rd_load();		// źÓÔŘ¸ůÎÄźţĎľÍłľ˝RAMDISK
	mount_root();	// źÓÔŘłŹźśżéşÍ¸ůÄżÂźľ˝¸ßËŮťşłĺÇř
	return (0);
}

// ĹĐśĎ˛˘Ń­ťˇľČ´ýÇýśŻĆ÷žÍĐ÷ĄŁ
// śÁÓ˛ĹĚżŘÖĆĆ÷×´ĚŹźÄ´ćĆ÷śËżÚ HD_STATUS(0x1f7)ŁŹ˛˘Ń­ťˇźě˛âÇýśŻĆ÷žÍĐ÷ąČĚŘÎťşÍżŘÖĆĆ÷ĂŚÎťĄŁ 
// ČçšűˇľťŘÖľÎŞ 0ŁŹÔňąíĘžľČ´ýłŹĘąłö´íŁŹˇńÔň OKĄŁ
static int controller_ready(void)
{
	int retries=10000;

	while (--retries && (inb_p(HD_STATUS)&0xc0)!=0x40);
	return (retries);
}

// źě˛âÓ˛ĹĚÖ´ĐĐĂüÁîşóľÄ×´ĚŹĄŁ(win_ąíĘžÎÂÇĐËšĚŘÓ˛ĹĚľÄËőĐ´)
// śÁČĄ×´ĚŹźÄ´ćĆ÷ÖĐľÄĂüÁîÖ´ĐĐ˝ášű×´ĚŹĄŁˇľťŘ 0 ąíĘžŐýłŁŁŹ1 łö´íĄŁ
// ČçšűÖ´ĐĐĂüÁî´íŁŹÔňÔŮśÁ´íÎóźÄ´ćĆ÷ HD_ERROR(0x1f1)ĄŁ
static int win_result(void)
{
	int i=inb_p(HD_STATUS);

	if ((i & (BUSY_STAT | READY_STAT | WRERR_STAT | SEEK_STAT | ERR_STAT))
		== (READY_STAT | SEEK_STAT))
		return(0); /* ok */
	if (i&1) i=inb(HD_ERROR);
	return (1);
}

// ĎňÓ˛ĹĚżŘÖĆĆ÷ˇ˘ËÍĂüÁîżéĄŁ
// ľ÷ÓĂ˛ÎĘý:drive - Ó˛ĹĚşĹ(0-1); nsect - śÁĐ´ÉČÇřĘý;
// sect - ĆđĘźÉČÇř; head - ´ĹÍˇşĹ;
// cyl - ÖůĂćşĹ; cmd - ĂüÁîÂë(˛ÎźűżŘÖĆĆ÷ĂüÁîÁĐąíŁŹąí 6.3);
// *intr_addr() - Ó˛ĹĚÖĐśĎˇ˘ÉúĘą´ŚŔíłĚĐňÖĐ˝Ťľ÷ÓĂľÄ C ´ŚŔíşŻĘýĄŁ
static void hd_out(unsigned int drive,unsigned int nsect,unsigned int sect,
		unsigned int head,unsigned int cyl,unsigned int cmd,
		void (*intr_addr)(void))
{
	register int port asm("dx"); 	//port ąäÁżśÔÓŚźÄ´ćĆ÷ dxŁťŐâĐ´ˇ¨ŁŹÖŽÇ°ĂťÓöľ˝šýĄŁ

	if (drive>1 || head>15)			// ČçšűÇýśŻĆ÷şĹ(0,1)>1 ťň´ĹÍˇşĹ>15ŁŹÔňłĚĐň˛ťÖ§łÖĄŁ
		panic("Trying to write bad sector");
	if (!controller_ready())		// ČçšűľČ´ýŇťśÎĘąźäşóČÔÎ´žÍĐ÷Ôňłö´íŁŹËŔťúĄŁ
		panic("HD controller not ready");

	//ŐâĘÇ¸öşŻĘýÖ¸ŐëŁŹÓÖDEVICE_INTRşęś¨ŇĺşóˇÖĹä
	// do_hd şŻĘýÖ¸Őë˝ŤÔÚÓ˛ĹĚÖĐśĎłĚĐňÖĐąťľ÷ÓĂĄŁ
	do_hd = intr_addr;		
	
	outb_p(hd_info[drive].ctl,HD_CMD);		// ĎňżŘÖĆźÄ´ćĆ÷(0x3f6)ĘäłöżŘÖĆ×Ö˝ÚĄŁ
	port=HD_DATA;							// ÖĂ dx ÎŞĘýžÝźÄ´ćĆ÷śËżÚ(0x1f0)ĄŁ
	outb_p(hd_info[drive].wpcom>>2,++port);	// ˛ÎĘý:Đ´Ô¤˛šłĽÖůĂćşĹ(Đčłý 4)ĄŁ
	outb_p(nsect,++port);					// ˛ÎĘý:śÁ/Đ´ÉČÇř×ÜĘýĄŁŁ
	outb_p(sect,++port);					// ˛ÎĘý:ĆđĘźÉČÇřĄŁ
	outb_p(cyl,++port);						// ˛ÎĘý:ÖůĂćşĹľÍ 8 ÎťĄŁ
	outb_p(cyl>>8,++port);					// ˛ÎĘý:ÖůĂćşĹ¸ß 8 ÎťĄŁ
	outb_p(0xA0|(drive<<4)|head,++port);	// ˛ÎĘý:ÇýśŻĆ÷şĹ+´ĹÍˇşĹĄŁ
	outb(cmd,++port);						// ĂüÁî:Ó˛ĹĚżŘÖĆĂüÁîĄŁ
}

// ľČ´ýÓ˛ĹĚžÍĐ÷ĄŁŇ˛ź´Ń­ťˇľČ´ýÖ÷×´ĚŹżŘÖĆĆ÷ĂŚąęÖžÎť¸´ÎťĄŁ
// Čô˝öÓĐžÍĐ÷ťňŃ°ľŔ˝áĘřąęÖžÖĂÎťŁŹÔňłÉšŚŁŹˇľťŘ 0ĄŁ
// Čôž­šýŇťśÎĘąźäČÔÎŞĂŚŁŹÔňˇľťŘ 1ĄŁ
static int drive_busy(void)
{
	unsigned int i;

	for (i = 0; i < 10000; i++)					// Ń­ťˇľČ´ýžÍĐ÷ąęÖžÎťÖĂÎťĄŁ
		if (READY_STAT == (inb_p(HD_STATUS) & (BUSY_STAT|READY_STAT)))
			break;
	i = inb(HD_STATUS);							// ÔŮČĄÖ÷żŘÖĆĆ÷×´ĚŹ×Ö˝ÚĄŁ
	i &= BUSY_STAT | READY_STAT | SEEK_STAT;	// źě˛âĂŚÎťĄ˘žÍĐ÷ÎťşÍŃ°ľŔ˝áĘřÎť
	if (i == READY_STAT | SEEK_STAT)			// Čô˝öÓĐžÍĐ÷ťňŃ°ľŔ˝áĘřąęÖžŁŹÔňˇľťŘ 0ĄŁ
		return(0);
	printk("HD controller times out\n\r");		// ˇńÔňľČ´ýłŹĘąŁŹĎÔĘžĐĹĎ˘ĄŁ˛˘ˇľťŘ 1ĄŁ
	return(1);
}

// ŐďśĎ¸´Îť(ÖŘĐÂĐŁŐý)Ó˛ĹĚżŘÖĆĆ÷ĄŁ
static void reset_controller(void)
{
	int	i;

	outb(4,HD_CMD);							// ĎňżŘÖĆźÄ´ćĆ÷śËżÚˇ˘ËÍżŘÖĆ×Ö˝Ú(4-¸´Îť)
	for(i = 0; i < 100; i++) nop();			// ľČ´ýŇťśÎĘąźä(Ń­ťˇżŐ˛Ů×÷)
	outb(hd_info[0].ctl & 0x0f ,HD_CMD);	// ÔŮˇ˘ËÍŐýłŁľÄżŘÖĆ×Ö˝Ú(˛ť˝űÖšÖŘĘÔĄ˘ÖŘśÁ)ĄŁ
	if (drive_busy())						// ČôľČ´ýÓ˛ĹĚžÍĐ÷łŹĘąŁŹÔňĎÔĘžłö´íĐĹĎ˘ĄŁ
		printk("HD-controller still busy\n\r");
	if ((i = inb(HD_ERROR)) != 1)			// ČĄ´íÎóźÄ´ćĆ÷ŁŹČô˛ťľČÓÚ 1(ÎŢ´íÎó)Ôňłö´íĄŁ
		printk("HD-controller reset failed: %02x\n\r",i);
}

// ¸´ÎťÓ˛ĹĚ nrĄŁĘ×ĎČ¸´Îť(ÖŘĐÂĐŁŐý)Ó˛ĹĚżŘÖĆĆ÷ĄŁČťşóˇ˘ËÍÓ˛ĹĚżŘÖĆĆ÷ĂüÁîĄ°˝¨Á˘ÇýśŻĆ÷˛ÎĘýĄąŁŹ
// ĆäÖĐ recal_intr()ĘÇÔÚÓ˛ĹĚÖĐśĎ´ŚŔíłĚĐňÖĐľ÷ÓĂľÄÖŘĐÂĐŁŐý´ŚŔíşŻĘýĄŁ
static void reset_hd(int nr)
{
	//¸´Îť(ÖŘĐÂĐŁŐý)Ó˛ĹĚżŘÖĆĆ÷
	reset_controller();	
	//ˇ˘ËÍÓ˛ĹĚżŘÖĆĆ÷ĂüÁîĄ°˝¨Á˘ÇýśŻĆ÷˛ÎĘýĄą
	hd_out(nr,hd_info[nr].sect,hd_info[nr].sect,hd_info[nr].head-1,
		hd_info[nr].cyl,WIN_SPECIFY,&recal_intr);
}

// ŇâÍâÓ˛ĹĚÖĐśĎľ÷ÓĂşŻĘýĄŁ
// ˇ˘ÉúŇâÍâÓ˛ĹĚÖĐśĎĘąŁŹÓ˛ĹĚÖĐśĎ´ŚŔíłĚĐňÖĐľ÷ÓĂľÄÄŹČĎ C ´ŚŔíşŻĘýĄŁ
// ÔÚąťľ÷ÓĂşŻĘýÖ¸ŐëÎŞżŐĘąľ÷ÓĂ¸ĂşŻĘýĄŁ
void unexpected_hd_interrupt(void)
{
	printk("Unexpected HD interrupt\n\r");
}

// śÁĐ´Ó˛ĹĚĘ§°Ü´ŚŔíľ÷ÓĂşŻĘýĄŁ
static void bad_rw_intr(void)
{
	// ČçšűśÁÉČÇřĘąľÄłö´í´ÎĘý´óÓÚťňľČÓÚ 7 ´ÎĘąŁŹÔň˝áĘřÇëÇó˛˘ť˝ĐŃľČ´ý¸ĂÇëÇóľÄ˝řłĚŁŹ
	// śřÇŇśÔÓŚťşłĺÇř¸üĐÂąęÖž¸´Îť(ĂťÓĐ¸üĐÂ)ĄŁ
	if (++CURRENT->errors >= MAX_ERRORS)
		end_request(0);
	// ČçšűśÁŇťÉČÇřĘąľÄłö´í´ÎĘýŇŃž­´óÓÚ 3 ´ÎŁŹ ÔňŇŞÇóÖ´ĐĐ¸´ÎťÓ˛ĹĚżŘÖĆĆ÷˛Ů×÷ĄŁ
	if (CURRENT->errors > MAX_ERRORS/2)
		reset = 1;
}

// śÁ˛Ů×÷ÖĐśĎľ÷ÓĂşŻĘýĄŁ˝ŤÔÚÓ˛ĹĚśÁĂüÁî˝áĘřĘąŇýˇ˘ľÄÖĐśĎšýłĚÖĐąťľ÷ÓĂĄŁ
// ¸ĂşŻĘýĘ×ĎČĹĐśĎ´Ë´ÎśÁĂüÁî˛Ů×÷ĘÇˇńłö´íĄŁČôĂüÁî˝áĘřşóżŘÖĆĆ÷ťš´ŚÓÚĂŚ×´ĚŹŁŹťňŐßĂüÁîÖ´ĐĐ´íÎóŁŹ 
// Ôň´ŚŔíÓ˛ĹĚ˛Ů×÷Ę§°ÜÎĘĚâŁŹ˝Ó×ĹÇëÇóÓ˛ĹĚ×÷¸´Îť´ŚŔí˛˘Ö´ĐĐĆäËüÇëÇóĎîĄŁ
// ČçšűśÁĂüÁîĂťÓĐłö´íŁŹÔň´ÓĘýžÝźÄ´ćĆ÷śËżÚ°ŃŇť¸öÉČÇřľÄĘýžÝśÁľ˝ÇëÇóĎîľÄťşłĺÇřÖĐŁŹ˛˘ľÝźőÇëÇóĎî 
// ËůĐčśÁČĄľÄÉČÇřĘýÖľĄŁČôľÝźőşó˛ťľČÓÚ 0ŁŹąíĘžąžĎîÇëÇóťšÓĐĘýžÝĂťČĄÍęŁŹÓÚĘÇÖą˝ÓˇľťŘŁŹľČ´ýÓ˛ĹĚ 
// ÔÚśÁłöÁíŇť¸öÉČÇřĘýžÝşóľÄÖĐśĎĄŁˇńÔňąíĂ÷ąžÇëÇóĎîËůĐčľÄËůÓĐÉČÇřśźŇŃśÁÍęŁŹÓÚĘÇ´ŚŔíąž´ÎÇëÇóĎî 
// ˝áĘřĘÂŇËĄŁ×îşóÔŮ´Îľ÷ÓĂ do_hd_request()ŁŹČĽ´ŚŔíĆäËüÓ˛ĹĚÇëÇóĎîĄŁ
static void read_intr(void)
{
	if (win_result()) {			// ČôżŘÖĆĆ÷ĂŚĄ˘śÁĐ´´íťňĂüÁîÖ´ĐĐ´íŁŹ
		bad_rw_intr();			// Ôň˝řĐĐśÁĐ´Ó˛ĹĚĘ§°Ü´ŚŔíŁŹ
		do_hd_request();		// ČťşóÔŮ´ÎÇëÇóÓ˛ĹĚ×÷ĎŕÓŚ(¸´Îť)´ŚŔíĄŁ
		return;
	}

	// ˝ŤĘýžÝ´ÓĘýžÝźÄ´ćĆ÷żÚśÁľ˝ÇëÇó˝áššťşłĺÇřĄŁ
	port_read(HD_DATA,CURRENT->buffer,256);// ×˘Ňâ:256 ĘÇÖ¸ÄÚ´ć×ÖŁŹŇ˛ź´ 512 ×Ö˝ÚĄŁ

	CURRENT->errors = 0;			// Çĺłö´í´ÎĘýĄŁ
	CURRENT->buffer += 512;			// ľ÷ŐűťşłĺÇřÖ¸ŐëŁŹÖ¸ĎňĐÂľÄżŐÇřĄŁ
	CURRENT->sector++;				// ĆđĘźÉČÇřşĹźÓ 1ŁŹ

	if (--CURRENT->nr_sectors) {	// ČçšűËůĐčśÁłöľÄÉČÇřĘýťšĂťÓĐśÁÍęŁŹ
		do_hd = &read_intr;			// ÔňÔŮ´ÎÖĂÓ˛ĹĚľ÷ÓĂ C şŻĘýÖ¸ŐëÎŞ read_intr()ŁŹ
		return;// ŇňÎŞÓ˛ĹĚÖĐśĎ´ŚŔíłĚĐňĂż´Îľ÷ÓĂ do_hd Ęąśźťá˝Ť¸ĂşŻĘýÖ¸ŐëÖĂżŐĄŁ˛Îźű system_call.s
	}
	end_request(1);					// ČôČŤ˛żÉČÇřĘýžÝŇŃž­śÁÍęŁŹÔň´ŚŔíÇëÇó˝áĘřĘÂŇËĄŁ
	do_hd_request();				//Ö´ĐĐĆäËüÓ˛ĹĚÇëÇó˛Ů×÷ĄŁ
}

// Đ´ÉČÇřÖĐśĎľ÷ÓĂşŻĘýĄŁÔÚÓ˛ĹĚÖĐśĎ´ŚŔíłĚĐňÖĐąťľ÷ÓĂĄŁ
// ÔÚĐ´ĂüÁîÖ´ĐĐşóŁŹťá˛úÉúÓ˛ĹĚÖĐśĎĐĹşĹŁŹÖ´ĐĐÓ˛ĹĚÖĐśĎ´ŚŔíłĚĐňŁŹ
// ´ËĘąÔÚÓ˛ĹĚÖĐśĎ´ŚŔíłĚĐňÖĐľ÷ÓĂľÄ C şŻĘýÖ¸Őë do_hd()ŇŃž­Ö¸Ďň write_intr()ŁŹ
// Ňň´ËťáÔÚĐ´˛Ů×÷ÍęłÉ(ťňłö´í)şóŁŹÖ´ĐĐ¸ĂşŻĘýĄŁ
static void write_intr(void)
{
	if (win_result()) {				// ČçšűÓ˛ĹĚżŘÖĆĆ÷ˇľťŘ´íÎóĐĹĎ˘ŁŹ
		bad_rw_intr();				// ÔňĘ×ĎČ˝řĐĐÓ˛ĹĚśÁĐ´Ę§°Ü´ŚŔíŁŹ
		do_hd_request();			// ČťşóÔŮ´ÎÇëÇóÓ˛ĹĚ×÷ĎŕÓŚ(¸´Îť)´ŚŔíŁŹ
		return;						// ČťşóˇľťŘ(Ň˛ÍËłöÁË´Ë´ÎÓ˛ĹĚÖĐśĎ)ĄŁ
	}
	if (--CURRENT->nr_sectors) {	// ˇńÔň˝ŤÓűĐ´ÉČÇřĘýźő 1ŁŹČôťšÓĐÉČÇřŇŞĐ´ŁŹÔň
		CURRENT->sector++;			// ľąÇ°ÇëÇóĆđĘźÉČÇřşĹ+1ŁŹ
		CURRENT->buffer += 512;		// ľ÷ŐűÇëÇóťşłĺÇřÖ¸ŐëŁŹ
		do_hd = &write_intr;		// ÖĂÓ˛ĹĚÖĐśĎłĚĐňľ÷ÓĂşŻĘýÖ¸ŐëÎŞ write_intr()ŁŹ
		port_write(HD_DATA,CURRENT->buffer,256);// ÔŮĎňĘýžÝźÄ´ćĆ÷śËżÚĐ´ 256 ×ÖĄŁ
		return;									// ˇľťŘľČ´ýÓ˛ĹĚÔŮ´ÎÍęłÉĐ´˛Ů×÷şóľÄÖĐśĎ´ŚŔíĄŁ
	}
	end_request(1);					// ČôČŤ˛żÉČÇřĘýžÝŇŃž­Đ´ÍęŁŹÔň´ŚŔíÇëÇó˝áĘřĘÂŇËŁŹ
	do_hd_request();				// Ö´ĐĐĆäËüÓ˛ĹĚÇëÇó˛Ů×÷ĄŁ
}

// Ó˛ĹĚÖŘĐÂĐŁŐýŁ¨¸´ÎťŁŠÖĐśĎľ÷ÓĂşŻĘýĄŁÔÚÓ˛ĹĚÖĐśĎ´ŚŔíłĚĐňÖĐąťľ÷ÓĂĄŁ
// ČçšűÓ˛ĹĚżŘÖĆĆ÷ˇľťŘ´íÎóĐĹĎ˘ŁŹÔňĘ×ĎČ˝řĐĐÓ˛ĹĚśÁĐ´Ę§°Ü´ŚŔíŁŹČťşóÇëÇóÓ˛ĹĚ×÷ĎŕÓŚ(¸´Îť)´ŚŔíĄŁ
static void recal_intr(void)
{
	if (win_result())
		bad_rw_intr();
	do_hd_request();
}

// Ö´ĐĐÓ˛ĹĚśÁĐ´ÇëÇó˛Ů×÷ĄŁ
// ČôÇëÇóĎîĘÇżéÉčą¸ľÄľÚ 1 ¸öŁŹÔňżéÉčą¸ľąÇ°ÇëÇóĎîÖ¸ŐëťáÖą˝ÓÖ¸Ďň¸ĂÇëÇóĎîŁŹ˛˘ťáÁ˘żĚľ÷ÓĂąžşŻĘýÖ´ĐĐśÁĐ´˛Ů×÷ĄŁ
// ˇńÔňÔÚŇť¸öśÁĐ´˛Ů×÷ÍęłÉśřŇýˇ˘ľÄÓ˛ĹĚÖĐśĎšýłĚÖĐŁŹČôťšÓĐÇëÇóĎîĐčŇŞ´ŚŔíŁŹÔňŇ˛ťáÔÚÖĐśĎšýłĚÖĐľ÷ÓĂąžşŻĘýĄŁ
void do_hd_request(void)
{
	int i,r;
	unsigned int block,dev;
	unsigned int sec,head,cyl;
	unsigned int nsect;

	// źě˛âÇëÇóĎîľÄşĎˇ¨ĐÔŁŹČôŇŃĂťÓĐÇëÇóĎîÔňÍËłöĄŁ
	INIT_REQUEST;
	// ČĄÉčą¸şĹÖĐľÄ×ÓÉčą¸şĹ(źűÁĐąíşóśÔÓ˛ĹĚÉčą¸şĹľÄËľĂ÷)ĄŁ×ÓÉčą¸şĹź´ĘÇÓ˛ĹĚÉĎľÄˇÖÇřşĹĄŁ
	dev = MINOR(CURRENT->dev);
	block = CURRENT->sector;
	// Čçšű×ÓÉčą¸şĹ˛ť´ćÔÚťňŐßĆđĘźÉČÇř´óÓÚ¸ĂˇÖÇřÉČÇřĘý-2ŁŹÔň˝áĘř¸ĂÇëÇóŁŹ˛˘Ěř×Şľ˝ąęşĹ repeat ´ŚĄŁ
	// ŇňÎŞŇť´ÎŇŞÇóśÁĐ´ 2 ¸öÉČÇřŁ¨512*2 ×Ö˝ÚŁŠŁŹËůŇÔÇëÇóľÄÉČÇřşĹ˛ťÄÜ´óÓÚˇÖÇřÖĐ×îşóľšĘýľÚśţ¸öÉČÇřşĹĄŁ
	if (dev >= 5*NR_HD || block+2 > hd[dev].nr_sects) {
		end_request(0);
		goto repeat;//repeatÔÚINIT_REQUESTÖĐś¨ŇĺľÄĄŁ
	}
	// Í¨šýźÓÉĎąžˇÖÇřľÄĆđĘźÉČÇřşĹŁŹ°Ń˝ŤËůĐčśÁĐ´ľÄżéśÔÓŚľ˝Őű¸öÓ˛ĹĚľÄžřśÔÉČÇřşĹÉĎĄŁ
	block += hd[dev].start_sect;
	dev /= 5;// ´ËĘą dev ´úąíÓ˛ĹĚşĹŁ¨ĘÇľÚ 1 ¸öÓ˛ĹĚ(0)ťšĘÇľÚ 2 ¸ö(1)ŁŠĄŁ
	// ĎÂĂćÇśČëťăąŕ´úÂëÓĂŔ´´ÓÓ˛ĹĚĐĹĎ˘˝áššÖĐ¸ůžÝĆđĘźÉČÇřşĹşÍĂż´ĹľŔÉČÇřĘýźĆËăÔÚ´ĹľŔÖĐľÄ
	// ÉČÇřşĹ(sec)Ą˘ËůÔÚÖůĂćşĹ(cyl)şÍ´ĹÍˇşĹ(head)ĄŁ
	__asm__("divl %4":"=a" (block),"=d" (sec):"0" (block),"1" (0),
		"r" (hd_info[dev].sect));
	__asm__("divl %4":"=a" (cyl),"=d" (head):"0" (block),"1" (0),
		"r" (hd_info[dev].head));
	sec++;
	nsect = CURRENT->nr_sectors;
	// Čçšű reset ąęÖžĘÇÖĂÎťľÄŁŹÔňÖ´ĐĐ¸´Îť˛Ů×÷ĄŁ¸´ÎťÓ˛ĹĚşÍżŘÖĆĆ÷ŁŹ˛˘ÖĂĐčŇŞÖŘĐÂĐŁŐýąęÖžŁŹˇľťŘĄŁ
	if (reset) {
		reset = 0;
		recalibrate = 1;
		reset_hd(CURRENT_DEV);
		return;
	}
	// ČçšűÖŘĐÂĐŁŐýąęÖž(recalibrate)ÖĂÎťŁŹÔňĘ×ĎČ¸´Îť¸ĂąęÖžŁŹČťşóĎňÓ˛ĹĚżŘÖĆĆ÷ˇ˘ËÍÖŘĐÂĐŁŐýĂüÁîĄŁ
	// ¸ĂĂüÁîťáÖ´ĐĐŃ°ľŔ˛Ů×÷ŁŹČĂ´ŚÓÚČÎşÎľŘˇ˝ľÄ´ĹÍˇŇĆśŻľ˝ 0 ÖůĂćĄŁ
	if (recalibrate) {
		recalibrate = 0;
		hd_out(dev,hd_info[CURRENT_DEV].sect,0,0,0,
			WIN_RESTORE,&recal_intr);
		return;
	}	
	// ČçšűľąÇ°ÇëÇóĘÇĐ´ÉČÇř˛Ů×÷ŁŹÔňˇ˘ËÍĐ´ĂüÁîŁŹŃ­ťˇśÁČĄ×´ĚŹźÄ´ćĆ÷ĐĹĎ˘˛˘ĹĐśĎÇëÇóˇţÎńąęÖž
	// DRQ_STAT ĘÇˇńÖĂÎťĄŁDRQ_STAT ĘÇÓ˛ĹĚ×´ĚŹźÄ´ćĆ÷ľÄÇëÇóˇţÎńÎťŁŹąíĘžÇýśŻĆ÷ŇŃž­×źą¸şĂÔÚÖ÷ťúşÍ
	// ĘýžÝśËżÚÖŽźä´ŤĘäŇť¸ö×ÖťňŇť¸ö×Ö˝ÚľÄĘýžÝĄŁ
	if (CURRENT->cmd == WRITE) {
		hd_out(dev,nsect,sec,head,cyl,WIN_WRITE,&write_intr);
		for(i=0 ; i<3000 && !(r=inb_p(HD_STATUS)&DRQ_STAT) ; i++)
			/* nothing */ ;
		// ČçšűÇëÇóˇţÎń DRQ ÖĂÎťÔňÍËłöŃ­ťˇĄŁČôľČľ˝Ń­ťˇ˝áĘřŇ˛ĂťÓĐÖĂÎťŁŹ
		// ÔňąíĘž´Ë´ÎĐ´Ó˛ĹĚ˛Ů×÷Ę§°ÜŁŹČĽ´ŚŔíĎÂŇť¸öÓ˛ĹĚÇëÇóĄŁ
		// ˇńÔňĎňÓ˛ĹĚżŘÖĆĆ÷ĘýžÝźÄ´ćĆ÷śËżÚ HD_DATA Đ´Čë 1 ¸öÉČÇřľÄĘýžÝĄŁ
		if (!r) {
			bad_rw_intr();
			goto repeat;//repeatÔÚINIT_REQUESTÖĐś¨ŇĺľÄĄŁ
		}
		port_write(HD_DATA,CURRENT->buffer,256);
	// ČçšűľąÇ°ÇëÇóĘÇśÁÓ˛ĹĚÉČÇřŁŹÔňĎňÓ˛ĹĚżŘÖĆĆ÷ˇ˘ËÍśÁÉČÇřĂüÁîĄŁ
	} else if (CURRENT->cmd == READ) {
		hd_out(dev,nsect,sec,head,cyl,WIN_READ,&read_intr);
	} else
		panic("unknown hd-command");
}

// Ó˛ĹĚĎľÍłłőĘźťŻĄŁ
void hd_init(void)
{
	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;// do_hd_request()ĄŁ
	// ÉčÖĂÓ˛ĹĚÖĐśĎĂĹĎňÁż int 0x2E(46)ĄŁ
	set_intr_gate(0x2E,&hd_interrupt);
	// ¸´Îť˝ÓÁŞľÄÖ÷ 8259A int2 ľÄĆÁąÎÎťŁŹÔĘĐí´ÓĆŹˇ˘łöÖĐśĎÇëÇóĐĹşĹĄŁ
	outb_p(inb_p(0x21)&0xfb,0x21);
	// ¸´ÎťÓ˛ĹĚľÄÖĐśĎÇëÇóĆÁąÎÎťŁ¨ÔÚ´ÓĆŹÉĎŁŠŁŹÔĘĐíÓ˛ĹĚżŘÖĆĆ÷ˇ˘ËÍÖĐśĎÇëÇóĐĹşĹĄŁ
	outb(inb_p(0xA1)&0xbf,0xA1);
}
