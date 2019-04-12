/*
��ϵͳ���߱�׼ C �������и���Ϊ errno �ı����������� C ��׼���Ƿ���Ҫ���������
�� C ��׼����֯�� X3J11 ���������˺ܴ����ۡ��������۵Ľ����û��ȥ�� errno ��
��������������Ϊ�� errno.h�� ��ͷ�ļ���
��Ϊ��׼����֯ϣ��ÿ���⺯�������ݶ�����Ҫ��һ����Ӧ�ı�׼ͷ�ļ�������������

��Ҫԭ�����ڣ������ں��е�ÿ��ϵͳ���ã�����䷵��ֵ����ָ��ϵͳ���õĽ��ֵ�Ļ���
�ͺ��ѱ����������������ÿ����������һ���� / ��ָʾֵ�������ֵ���з��أ�
�Ͳ��ܷܺ���صõ�ϵͳ���õĽ��ֵ��

����İ취֮һ�ǽ������ַ�ʽ������ϣ�
����һ���ض���ϵͳ���ã�����ָ��һ������Ч���ֵ��Χ������ĳ�����ֵ��
�������ָ����Բ��� null ֵ������ pid ���Է��� -1 ֵ��
�������������£�ֻҪ������ֵ��ͻ�����Բ��� -1 ����ʾ����ֵ��
���Ǳ�׼ C �⺯������ֵ����֪�Ƿ�������������������ط��˽��������ͣ�
��˲����� errno ���������Ϊ�����׼ C �����ƻ��Ƽ��ݣ� 
Linux �ں��еĿ��ļ�Ҳ���������ִ����������Ҳ�����˱�׼ C �����ͷ�ļ���
������ӿɲμ� lib/open.c �����Լ� unistd.h �е�ϵͳ���ú궨�塣
��ĳЩ����£�������Ȼ�ӷ��ص� -1 ֵ֪�������ˣ�����֪������ĳ���ţ�
�Ϳ���ͨ����ȡ errno ��ֵ��ȷ�����һ�δ���ĳ���š�

���ļ���Ȼֻ�Ƕ����� Linux ϵͳ�е�һЩ�����루����ţ��ĳ������ţ�
���� Linus ���ǳ���ļ�����Ҳ�����Щ���Ŷ������ POSIX ��׼�е�һ����
���ǲ�ҪС������򵥵Ĵ��룬���ļ�Ҳ�� SCO��˾ָ�� Linux ����ϵͳ�ַ����Ȩ���г����ļ���֮һ��
Ϊ���о������Ȩ���⣬�� 2003 �� 12 �·ݣ�10 �����ǰ Linux �ں˵Ķ���������Ա���������ֶԲߡ�
���а��� Linus �� Alan Cox �� H.J.Lu �� Mitchell BlankJr ��
���ڵ�ǰ�ں˰汾�� 2.4.x ���е� errno.h �ļ��� 0.96c ���ں˿�ʼ��û�б仯����
���Ǿ�һֱ�����١�����Щ�ϰ汾���ں˴����С�
��� Linus ���ָ��ļ��Ǵ� H.J.Lu ��ʱά���� Libc 2.x �������ó����Զ����ɵģ�
���а�����һЩ�� SCO ӵ�а�Ȩ�� UNIX �ϰ汾�� V6 �� V7 �ȣ���ͬ�ı�������
*/


#ifndef _ERRNO_H
#define _ERRNO_H

/*
 * ok, as I hadn't got any other source of information about
 * possible error numbers, I was forced to use the same numbers
 * as minix.
 * Hopefully these are posix or something. I wouldn't know (and posix
 * isn't telling me - they want $$$ for their f***ing standard).
 *
 * We don't use the _SIGN cludge of minix, so kernel returns must
 * see to the sign by themselves.
 *
 * NOTE! Remember to change strerror() if you change this file!
 */

/*
 * ok��������û�еõ��κ������йس���ŵ����ϣ���ֻ��ʹ���� minix ϵͳ
 * ��ͬ�ĳ�����ˡ�
 * ϣ����Щ�� POSIX ���ݵĻ�����һ���̶����������ģ��Ҳ�֪�������� POSIX
 * û�и����� - Ҫ������ǵĻ쵰��׼��Ҫ��Ǯ����
 *
 * ����û��ʹ�� minix ������_SIGN �أ������ں˵ķ���ֵ�����Լ���������š�
 *
 * ע�⣡�����ı���ļ��Ļ�������ҲҪ�޸� strerror()������
 */

// ϵͳ�����Լ��ܶ�⺯������һ�������ֵ�Ա�ʾ����ʧ�ܻ����
// ���ֵͨ��ѡ��-1 ��������һЩ�ض���ֵ����ʾ�������������ֵ��˵���������ˡ�
// �����Ҫ֪����������ͣ�����Ҫ�鿴��ʾϵͳ����ŵı��� errno��
// �ñ������� errno.h �ļ����������ڳ���ʼִ��ʱ�ñ���ֵ����ʼ��Ϊ 0��
extern int errno;

#define ERROR		99		// һ�����
#define EPERM		 1
#define ENOENT		 2
#define ESRCH		 3
#define EINTR		 4
#define EIO		 5
#define ENXIO		 6
#define E2BIG		 7
#define ENOEXEC		 8
#define EBADF		 9
#define ECHILD		10
#define EAGAIN		11
#define ENOMEM		12
#define EACCES		13
#define EFAULT		14
#define ENOTBLK		15
#define EBUSY		16
#define EEXIST		17
#define EXDEV		18
#define ENODEV		19
#define ENOTDIR		20
#define EISDIR		21
#define EINVAL		22
#define ENFILE		23
#define EMFILE		24
#define ENOTTY		25
#define ETXTBSY		26
#define EFBIG		27
#define ENOSPC		28
#define ESPIPE		29
#define EROFS		30
#define EMLINK		31
#define EPIPE		32
#define EDOM		33
#define ERANGE		34
#define EDEADLK		35
#define ENAMETOOLONG	36
#define ENOLCK		37
#define ENOSYS		38
#define ENOTEMPTY	39

#endif
