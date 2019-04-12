/*
 *  linux/kernel/vsprintf.c
 *
 *  (C) 1991  Linus Torvalds
 */

/* vsprintf.c -- Lars Wirzenius & Linus Torvalds. */
/*
 * Wirzenius wrote this portably, Torvalds fucked it up :-)
 */
 // Lars Wirzenius �� Linus �ĺ��ѣ��� Helsinki ��ѧʱ��ͬ��һ��칫�ҡ�
 // �� 1991 ���ļ����� Linux ʱ��Linus ��ʱ�� C ���Ի����Ǻ���Ϥ��
 // ������ʹ�ÿɱ�����б������ܡ�
 // ��� Lars Wirzenius ��Ϊ����д����������ں���ʾ��Ϣ�Ĵ��롣
 // ������(1998 ��)��������δ�������һ�� bug��ֱ�� 1994 ������˷��֣������Ծ�����
 // ��� bug ����ʹ��*��Ϊ�������ʱ�����ǵ���ָ����������Ǻ��ˡ�
 // �ڱ���������� bug ����Ȼ����(130 ��)�� 
 // ���ĸ�����ҳ�� http://liw.iki.fi/liw/


#include <stdarg.h>
#include <string.h>

/* we use this so that we can do without the ctype library */
/* ����ʹ������Ķ��壬�������ǾͿ��Բ�ʹ�� ctype ���� */
#define is_digit(c)	((c) >= '0' && (c) <= '9')	// �ж��ַ��Ƿ������ַ���

// �ú������ַ����ִ�ת�������������������ִ�ָ���ָ�룬�����ǽ����ֵ��
// ����ָ�뽫ǰ�ơ�
static int skip_atoi(const char **s)
{
	int i=0;

	//sָ����ǵ�ַ���飬ÿ����ַ��Ӧһ���ַ�
	while (is_digit(**s))
		i = i*10 + *((*s)++) - '0';//ע��������*s��ֵ
	return i;
}

#define ZEROPAD	1		/* pad with zero */			/* ����� */
#define SIGN	2		/* unsigned/signed long */	/* �޷���/���ų����� */
#define PLUS	4		/* show plus */				/* ��ʾ�� */
#define SPACE	8		/* space if plus */			/* ���Ǽӣ����ÿո� */
#define LEFT	16		/* left justified */		/* ����� */
#define SPECIAL	32		/* 0x */
#define SMALL	64		/* use 'abcdef' instead of 'ABCDEF' */ /* ʹ��Сд��ĸ */

// ������������:n Ϊ��������base Ϊ����;���:n Ϊ�̣���������ֵΪ������
// ע��n�ᱻ���£����ȼ���t=n%base; n=n/base; return t;
#define do_div(n,base) ({ \
int __res; \
__asm__("divl %4":"=a" (n),"=d" (__res):"0" (n),"1" (0),"r" (base)); \
__res; })

// ������ת��Ϊָ�����Ƶ��ַ�����
// ����:num-����;base-����;size-�ַ�������;precision-���ֳ���(����);type-����ѡ� 
// ���:str �ַ���ָ�롣
static char * number(char * str, int num, int base, int size, int precision
	,int type)
{
	char c,sign,tmp[36];
	const char *digits="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i;

	// ������� type ָ����Сд��ĸ������Сд��ĸ����
	if (type&SMALL) digits="0123456789abcdefghijklmnopqrstuvwxyz";
	// �������ָ��Ҫ�����(����߽�)�������������е������־��
	if (type&LEFT) type &= ~ZEROPAD;
	// ������ƻ���С�� 2 ����� 36�����˳�����Ҳ��������ֻ�ܴ�������� 2-32 ֮�������
	if (base<2 || base>36)
		return 0;
	// �������߿ո����
	c = (type & ZEROPAD) ? '0' : ' ' ;

	// �������ָ���Ǵ�������������ֵ num С�� 0�����÷��ű��� sign=���ţ���ʹ num ȡ����ֵ��
	if (type&SIGN && num<0) {//ֻ�����з��Ÿ���������Ϊ�޷��Ÿ���
		sign='-';
		num = -num;//��Ϊ����
	} else// ����Ҫô���޷��� ���� �з��ŵ�Ϊ���������ܷ���λ��Ҫд��'+'���߿ո�
		sign=(type&PLUS) ? '+' : ((type&SPACE) ? ' ' : 0);//0��ʾ���÷���λ
	//����λҪռһ���ֽڣ���д�Ŀռ������һ��
	if (sign) size--;
	if (type&SPECIAL)//��Ҫ��ʾ���Ƶı�ʶ
		if (base==16) size -= 2;	//0x
		else if (base==8) size--;	//0

	i=0;
	if (num==0)	//num�������ҪôΪ0Ҫô����0
		tmp[i++]='0';
	else while (num!=0)	//���д��ͦ����˼��
		tmp[i++]=digits[do_div(num,base)];	//ע��num�ᱻ����

	//���ʵ�ʴ�С���ھ���λ����ʹ��ʵ�ʵ�λ�����������Ⱦ���λ���٣�����䵽���㾫��λ��
	if (i>precision) precision=i;
	size -= precision;
	
	// ������������ʼ�γ�����Ҫ��ת�����������ʱ�����ַ��� str �С�
	// ��������û������(ZEROPAD)������(�����)��־��
	// ���� str ���������ʣ����ֵָ���Ŀո�����
	if (!(type&(ZEROPAD+LEFT)))
		while(size-->0)
			*str++ = ' ';
	if (sign)	// ���������λ���������š�	
		*str++ = sign;
	if (type&SPECIAL)
		if (base==8)
			*str++ = '0';
		else if (base==16) {
			*str++ = '0';
			*str++ = digits[33];//ֱ��д'x'����'X'Ҳ���Բ���Ҫ�жϴ�Сд
		}
	if (!(type&LEFT))//û������뵫��0���ſ���Ϊ��
		while(size-->0)
			*str++ = c;
	// ��ʱ i ������ֵ num �����ָ�����
	// �����ָ���С�ھ���ֵ���� str �з���(����ֵ-i)��'0'��
	// ע�����д������ͦ�ɵ�
	while(i<precision--)
		*str++ = '0';
	//�������
	while(i-->0)
		*str++ = tmp[i];
	// �����ֵ�Դ����㣬���ʾ���ͱ�־���������־��־������ʣ�����з���ո�
	while(size-->0)
		*str++ = ' ';
	return str;//���ص�ַ
}

int vsprintf(char *buf, const char *fmt, va_list args)
{
	int len;
	int i;
	char * str;
	char *s;
	int *ip;

	int flags;		/* flags to number() */	/* number����ʹ�õ�type���� */

	int field_width;	/* width of output field */	/* ����ֶο��*/
	
						/* min. �������ָ���;max. �ַ������ַ����� */
	int precision;		/* min. # of digits for integers; max
				   number of chars for from string */

						/* 'h', 'l',��'L'���������ֶ� */
	int qualifier;		/* 'h', 'l', or 'L' for integer fields */

	for (str=buf ; *fmt ; ++fmt) {
		if (*fmt != '%') {//�ַ�����û������ֱ�Ӹ��Ƹ�Ŀ���ַ���
			*str++ = *fmt;
			continue;
		}
			
		/* process flags */
		flags = 0;
		repeat:
			++fmt;		/* this also skips first '%' */	//�ٷֺŲ��ô�����
			switch (*fmt) {
				case '-': flags |= LEFT; goto repeat;//�����
				case '+': flags |= PLUS; goto repeat;//�������Ӻ�
				case ' ': flags |= SPACE; goto repeat;//����Ϊ����λ�ո�
				case '#': flags |= SPECIAL; goto repeat;//0x��0
				case '0': flags |= ZEROPAD; goto repeat;//�����
				//ûƥ�䵽�ַ���ֱ��������
				}

		// ȡ��ǰ�����ֶο����ֵ������ field_width �����С�
		// ��������������ֵ��ֱ��ȡ��Ϊ���ֵ�� 
		// �������������ַ�'*'����ʾ��һ������ָ����ȡ���˵��� va_arg ȡ���ֵ��
		// ����ʱ���ֵС�� 0����ø�����ʾ����б�־��'-'��־(����)��
		// ��˻����ڱ�־����������ñ�־�������ֶο��ֵȡΪ�����ֵ��
		/* get field width */
		field_width = -1;
		if (is_digit(*fmt))//�õ����ֵĿ�ȣ���%5d
			//���ﴫ��ȥ��char **�����Կ��Ը�fmt��ֵ������д�ͺ�����˼�ˡ�
			//���������char *���ܸģ��������غ�fmt��Ҫ�����������ȡ�
			field_width = skip_atoi(&fmt);
		else if (*fmt == '*') {//��%*
			/* it's the next argument */	// �����и� bug��Ӧ����++fmt;
			field_width = va_arg(args, int);//�Ӳ����еõ�����
			if (field_width < 0) {
				field_width = -field_width;
				flags |= LEFT;
			}
		}

		// ������δ��룬ȡ��ʽת�����ľ����򣬲����� precision �����С�
		// ������ʼ�ı�־��'.'�� // �䴦�������������������ơ�
		// ���������������ֵ��ֱ��ȡ��Ϊ����ֵ��
		// ��������������ַ�'*'����ʾ��һ������ָ�����ȡ���˵��� va_arg ȡ����ֵ��
		// ����ʱ���ֵС�� 0�����ֶξ���ֵȡΪ�����ֵ��
		/* get the precision */
		precision = -1;
		if (*fmt == '.') {//��%5.2f
			++fmt;	
			if (is_digit(*fmt))
				precision = skip_atoi(&fmt);
			else if (*fmt == '*') {
				/* it's the next argument */
				precision = va_arg(args, int);
			}
			if (precision < 0)
				precision = 0;
		}

		// ������δ�������������η������������ qualifer ������
		/* get the conversion qualifier */
		qualifier = -1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
			qualifier = *fmt;
			++fmt;
		}

		switch (*fmt) {// �������ת��ָʾ��
		// ���ת��ָʾ����'c'�����ʾ��Ӧ����Ӧ���ַ���
		// ��ʱ�����־������������룬����ֶ�ǰ���������ֵ-1 ���ո��ַ���
		// Ȼ���ٷ�������ַ���
		// �������򻹴��� 0�����ʾΪ���룬���ڲ����ַ�������ӿ��ֵ-1 ���ո��ַ���
		case 'c':
			if (!(flags & LEFT))//������룬�����
				while (--field_width > 0)
					*str++ = ' ';
			//������������'0'��'a'�������ʵ�ʵ�ֵΪ48��97��������ת�����ַ�
			*str++ = (unsigned char) va_arg(args, int);
			while (--field_width > 0)//����룬���ұ�
				*str++ = ' ';
			break;
		// ���ת��ָʾ����'s'�����ʾ��Ӧ�������ַ���������ȡ�����ַ����ĳ��ȣ�
		// ���䳬���˾�����ֵ�� ����չ������=�ַ������ȡ�
		// ��ʱ�����־������������룬����ֶ�ǰ����(���ֵ-�ַ�������)���ո��ַ���
		// Ȼ���ٷ�������ַ������������򻹴��� 0�����ʾΪ���룬
		// ���ڲ����ַ����������(���ֵ-�ַ�������)���ո��ַ���
		case 's':
			s = va_arg(args, char *);//�����������ַ������׵�ַ
			len = strlen(s);
			//precision����ûʲô��
			if (precision < 0)//û��'.'��precisionΪ-1
				precision = len;
			else if (len > precision)//��precision������Ϊ׼
				len = precision;

			if (!(flags & LEFT))//������룬�����
				while (len < field_width--)
					*str++ = ' ';
			for (i = 0; i < len; ++i)//��������
				*str++ = *s++;
			while (len < field_width--)//����룬���ұ�
				*str++ = ' ';
			break;
		//8����
		case 'o':
			str = number(str, va_arg(args, unsigned long), 8,
				field_width, precision, flags);
			break;
		// �����ʽת������'p'����ʾ��Ӧ������һ��ָ�����͡�
		// ��ʱ���ò���û�����ÿ������Ĭ�Ͽ��Ϊ 8��������Ҫ���㡣
		// Ȼ����� number()�������д���
		case 'p':
			if (field_width == -1) {
				field_width = 8;//��ַΪ32λ4�ֽڣ�16����ռ8��λ��
				flags |= ZEROPAD;//ǰ��Ҫ��0
			}
			str = number(str,
				(unsigned long) va_arg(args, void *), 16,
				field_width, precision, flags);
			break;
		// ����ʽת��ָʾ��'x'��'X'�����ʾ��Ӧ������Ҫ��ӡ��ʮ�������������
		// 'x'��ʾ��Сд��ĸ��ʾ��
		case 'x':
			flags |= SMALL;
		case 'X':
			str = number(str, va_arg(args, unsigned long), 16,
				field_width, precision, flags);
			break;

		// �����ʽת���ַ���'d','i'��'u'�����ʾ��Ӧ������������'d', 'i'�������������
		// �����Ҫ���ϴ����ű�־��'u'�����޷���������
		case 'd':
		case 'i':
			flags |= SIGN;
		case 'u'://�޷��ź���û���ر���?
			str = number(str, va_arg(args, unsigned long), 10,
				field_width, precision, flags);
			break;
		// ����ʽת��ָʾ����'n'��
		// ���ʾҪ�ѵ�ĿǰΪֹת��������ַ������浽��Ӧ����ָ��ָ����λ���С�
		// �������� va_arg()ȡ�øò���ָ�룬Ȼ���Ѿ�ת���õ��ַ��������ָ����ָ��λ�á�
		case 'n':
			ip = va_arg(args, int *);
			*ip = (str - buf);//str���Ѿ�ת���õĵط���buf����ʼ
			break;

		default:
			if (*fmt != '%')//����%��ʾ��ʽ�д��������%
				*str++ = '%';
			if (*fmt)//�������ַ�����ֱ�����
				*str++ = *fmt;
			else//����β��
				--fmt;
			break;
		}
	}
	*str = '\0';
	return str-buf;//�����ַ�����С
}
