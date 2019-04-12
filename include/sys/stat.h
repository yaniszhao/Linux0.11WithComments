// ��ͷ�ļ�˵���˺��� stat() ���ص����ݼ���ṹ���ͣ��Լ�һЩ���Բ������Ժꡢ����ԭ�͡�

#ifndef _SYS_STAT_H
#define _SYS_STAT_H

#include <sys/types.h>

struct stat {
	dev_t	st_dev;		// �����ļ����豸�š�
	ino_t	st_ino;		// �ļ� i �ڵ�š�
	umode_t	st_mode;	// �ļ����ͺ����ԡ�
	nlink_t	st_nlink;	// ָ���ļ�����������
	uid_t	st_uid;		// �ļ����û�(��ʶ)�š�
	gid_t	st_gid;		// �ļ�����š�
	dev_t	st_rdev;	// �豸��(����ļ���������ַ��ļ�����ļ�)��
	off_t	st_size;	// �ļ���С���ֽ�����������ļ��ǳ����ļ�����
	time_t	st_atime;	// �ϴΣ���󣩷���ʱ�䡣
	time_t	st_mtime;	// ����޸�ʱ�䡣
	time_t	st_ctime;	// ���ڵ��޸�ʱ�䡣
};

// st_mode ֵ�ķ������ơ�
// �ļ����ͣ�
#define S_IFMT  00170000	// �ļ����ͣ�8 ���Ʊ�ʾ����
#define S_IFREG  0100000	// �����ļ���
#define S_IFBLK  0060000	// �����⣨�豸���ļ�������� dev/fd0��
#define S_IFDIR  0040000	// Ŀ¼�ļ���
#define S_IFCHR  0020000	// �ַ��豸�ļ���
#define S_IFIFO  0010000	// FIFO �����ļ���
// �ļ�����λ��
// S_ISUID ���ڲ����ļ��� set-user-ID ��־�Ƿ���λ�����ñ�־��λ����ִ�и��ļ�ʱ�����̵�
// ��Ч�û� ID ��������Ϊ���ļ��������û� ID��S_ISGID ��������� ID ������ͬ����
#define S_ISUID  0004000	// ִ��ʱ�����û� ID��set-user-ID����
#define S_ISGID  0002000	// ִ��ʱ������ ID��set-group-ID����
#define S_ISVTX  0001000	// ����Ŀ¼������ɾ����־��

#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)	// �Ƿ� FIFO �����ļ���

// �ļ�����Ȩ�ޣ�
#define S_IRWXU 00700	// �������Զ���д��ִ��/������
#define S_IRUSR 00400	// ��������ɡ�
#define S_IWUSR 00200	// ����д��ɡ�
#define S_IXUSR 00100	// ����ִ��/������ɡ�

#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010

#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001

extern int chmod(const char *_path, mode_t mode);
extern int fstat(int fildes, struct stat *stat_buf);
extern int mkdir(const char *_path, mode_t mode);
extern int mkfifo(const char *_path, mode_t mode);
extern int stat(const char *filename, struct stat *stat_buf);
extern mode_t umask(mode_t mask);

#endif
