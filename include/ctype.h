
// ���ļ��ǹ����ַ����Ժʹ����ͷ�ļ���Ҳ�Ǳ�׼ C ���ͷ�ļ�֮һ��
// ���ж�����һЩ�й��ַ������жϺ�ת���ĺꡣ

#ifndef _CTYPE_H
#define _CTYPE_H

#define _U	0x01	/* upper */							// �ñ���λ���ڴ�д�ַ�[A-Z]��
#define _L	0x02	/* lower */							// �ñ���λ����Сд�ַ�[a-z]��
#define _D	0x04	/* digit */							// �ñ���λ��������[0-9]��
#define _C	0x08	/* cntrl */							// �ñ���λ���ڿ����ַ���
#define _P	0x10	/* punct */							// �ñ���λ���ڱ���ַ���
#define _S	0x20	/* white space (space/lf/tab) */	// ���ڿհ��ַ�����ո�\t��\n �ȡ�
#define _X	0x40	/* hex digit */						// �ñ���λ����ʮ���������֡�
#define _SP	0x80	/* hard space (0x20) */				// �ñ���λ���ڿո��ַ�(0x20)��

extern unsigned char _ctype[];	// �ַ���������(��)�������˸����ַ���Ӧ��������ԡ�
extern char _ctmp;				// һ����ʱ�ַ�����(�� lib/ctype.c �ж���)��

// ������һЩȷ���ַ����͵ĺꡣ
#define isalnum(c) ((_ctype+1)[c]&(_U|_L|_D))
#define isalpha(c) ((_ctype+1)[c]&(_U|_L))
#define iscntrl(c) ((_ctype+1)[c]&(_C))					// �ǿ����ַ���
#define isdigit(c) ((_ctype+1)[c]&(_D))
#define isgraph(c) ((_ctype+1)[c]&(_P|_U|_L|_D))		// ��ͼ���ַ���
#define islower(c) ((_ctype+1)[c]&(_L))	
#define isprint(c) ((_ctype+1)[c]&(_P|_U|_L|_D|_SP))	// �ǿɴ�ӡ�ַ���
#define ispunct(c) ((_ctype+1)[c]&(_P))					// �Ǳ����š�
#define isspace(c) ((_ctype+1)[c]&(_S))					// �ǿհ��ַ���ո�,\f,\n,\r,\t,\v��
#define isupper(c) ((_ctype+1)[c]&(_U))
#define isxdigit(c) ((_ctype+1)[c]&(_D|_X))				// ��ʮ���������֡�

#define isascii(c) (((unsigned) c)<=0x7f)				// �� ASCII �ַ���
#define toascii(c) (((unsigned) c)&0x7f)				// ת���� ASCII �ַ���

// �������������У������ǰʹ����ǰ׺(unsigned)����� c Ӧ�ü����ţ�����ʾ��(c)��
// ��Ϊ�ڳ����� c ������һ�����ӵı��ʽ�����磬��������� a + b�����������ţ����ں궨����
// ����ˣ�(unsigned) a + b������Ȼ���ԡ��������ž�����ȷ��ʾ��(unsigned)(a + b)��
#define tolower(c) (_ctmp=c,isupper(_ctmp)?_ctmp-('A'-'a'):_ctmp)	// ת����Сд
#define toupper(c) (_ctmp=c,islower(_ctmp)?_ctmp-('a'-'A'):_ctmp)	// ת���ɴ�д

#endif
