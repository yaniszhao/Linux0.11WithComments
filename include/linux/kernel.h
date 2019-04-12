//������һЩ�ں˳��õĺ���ԭ�͵ȡ�

/*
 * 'kernel.h' contains some often-used function prototypes etc
 */
/*
 * 'kernel.h'������һЩ���ú�����ԭ�͵ȡ�
 */

 // ��֤������ַ��ʼ���ڴ���Ƿ��ޡ���������׷���ڴ档
void verify_area(void * addr,int count);
// ��ʾ�ں˳�����Ϣ��Ȼ�������ѭ����
volatile void panic(const char * str);
// ��׼��ӡ����ʾ��������
int printf(const char * fmt, ...);
// �ں�ר�õĴ�ӡ��Ϣ������������ printf()��ͬ��
int printk(const char * fmt, ...);
// �� tty ��дָ�����ȵ��ַ�����
int tty_write(unsigned ch,char * buf,int count);
// ͨ���ں��ڴ���亯����
void * malloc(unsigned int size);
// �ͷ�ָ������ռ�õ��ڴ档
void free_s(void * obj, int size);

#define free(x) free_s((x), 0)

/*
 * This is defined as a macro, but at some point this might become a
 * real subroutine that sets a flag if it returns true (to do
 * BSD-style accounting where the process is flagged if it uses root
 * privs).  The implication of this is that you should do normal
 * permissions checks first, and check suser() last.
 */
/*
 * ���溯�����Ժ����ʽ����ģ�������ĳ�������������Գ�Ϊһ���������ӳ���
 * ��������� true ʱ�������ñ�־�����ʹ�� root �û�Ȩ�޵Ľ��������˱�־������
 * ��ִ�� BSD ��ʽ�ļ��ʴ���������ζ����Ӧ������ִ�г���Ȩ�޼�飬�����
 * ��� suser()��
 */
#define suser() (current->euid == 0)

