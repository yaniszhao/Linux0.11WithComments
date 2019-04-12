/*
 *  linux/kernel/vsprintf.c
 *
 *  (C) 1991  Linus Torvalds
 */

/* vsprintf.c -- Lars Wirzenius & Linus Torvalds. */
/*
 * Wirzenius wrote this portably, Torvalds fucked it up :-)
 */
 // Lars Wirzenius 是 Linus 的好友，在 Helsinki 大学时曾同处一间办公室。
 // 在 1991 年夏季开发 Linux 时，Linus 当时对 C 语言还不是很熟悉，
 // 还不会使用可变参数列表函数功能。
 // 因此 Lars Wirzenius 就为他编写了这段用于内核显示信息的代码。
 // 他后来(1998 年)承认在这段代码中有一个 bug，直到 1994 年才有人发现，并予以纠正。
 // 这个 bug 是在使用*作为输出域宽度时，忘记递增指针跳过这个星号了。
 // 在本代码中这个 bug 还仍然存在(130 行)。 
 // 他的个人主页是 http://liw.iki.fi/liw/


#include <stdarg.h>
#include <string.h>

/* we use this so that we can do without the ctype library */
/* 我们使用下面的定义，这样我们就可以不使用 ctype 库了 */
#define is_digit(c)	((c) >= '0' && (c) <= '9')	// 判断字符是否数字字符。

// 该函数将字符数字串转换成整数。输入是数字串指针的指针，返回是结果数值。
// 另外指针将前移。
static int skip_atoi(const char **s)
{
	int i=0;

	//s指向的是地址数组，每个地址对应一个字符
	while (is_digit(**s))
		i = i*10 + *((*s)++) - '0';//注意这里会改*s的值
	return i;
}

#define ZEROPAD	1		/* pad with zero */			/* 填充零 */
#define SIGN	2		/* unsigned/signed long */	/* 无符号/符号长整数 */
#define PLUS	4		/* show plus */				/* 显示加 */
#define SPACE	8		/* space if plus */			/* 如是加，则置空格 */
#define LEFT	16		/* left justified */		/* 左对齐 */
#define SPECIAL	32		/* 0x */
#define SMALL	64		/* use 'abcdef' instead of 'ABCDEF' */ /* 使用小写字母 */

// 除操作。输入:n 为被除数，base 为除数;结果:n 为商，函数返回值为余数。
// 注意n会被更新，即等价于t=n%base; n=n/base; return t;
#define do_div(n,base) ({ \
int __res; \
__asm__("divl %4":"=a" (n),"=d" (__res):"0" (n),"1" (0),"r" (base)); \
__res; })

// 将整数转换为指定进制的字符串。
// 输入:num-整数;base-进制;size-字符串长度;precision-数字长度(精度);type-类型选项。 
// 输出:str 字符串指针。
static char * number(char * str, int num, int base, int size, int precision
	,int type)
{
	char c,sign,tmp[36];
	const char *digits="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i;

	// 如果类型 type 指出用小写字母，则定义小写字母集。
	if (type&SMALL) digits="0123456789abcdefghijklmnopqrstuvwxyz";
	// 如果类型指出要左调整(靠左边界)，则屏蔽类型中的填零标志。
	if (type&LEFT) type &= ~ZEROPAD;
	// 如果进制基数小于 2 或大于 36，则退出处理，也即本程序只能处理基数在 2-32 之间的数。
	if (base<2 || base>36)
		return 0;
	// 零填充或者空格填充
	c = (type & ZEROPAD) ? '0' : ' ' ;

	// 如果类型指出是带符号数并且数值 num 小于 0，则置符号变量 sign=负号，并使 num 取绝对值。
	if (type&SIGN && num<0) {//只可能有符号负数，不能为无符号负数
		sign='-';
		num = -num;//变为正数
	} else// 否则要么是无符号 或者 有符号但为正数；可能符号位需要写成'+'或者空格
		sign=(type&PLUS) ? '+' : ((type&SPACE) ? ' ' : 0);//0表示不用符号位
	//符号位要占一个字节，可写的空间就少了一个
	if (sign) size--;
	if (type&SPECIAL)//若要显示进制的标识
		if (base==16) size -= 2;	//0x
		else if (base==8) size--;	//0

	i=0;
	if (num==0)	//num被处理过要么为0要么大于0
		tmp[i++]='0';
	else while (num!=0)	//这个写法挺有意思啊
		tmp[i++]=digits[do_div(num,base)];	//注意num会被更新

	//如果实际大小大于精度位数，使用实际的位数；但是若比精度位数少，则填充到满足精度位数
	if (i>precision) precision=i;
	size -= precision;
	
	// 从这里真正开始形成所需要的转换结果，并暂时放在字符串 str 中。
	// 若类型中没有填零(ZEROPAD)和左靠齐(左调整)标志，
	// 则在 str 中首先填放剩余宽度值指出的空格数。
	if (!(type&(ZEROPAD+LEFT)))
		while(size-->0)
			*str++ = ' ';
	if (sign)	// 若需带符号位，则存入符号。	
		*str++ = sign;
	if (type&SPECIAL)
		if (base==8)
			*str++ = '0';
		else if (base==16) {
			*str++ = '0';
			*str++ = digits[33];//直接写'x'或者'X'也可以不过要判断大小写
		}
	if (!(type&LEFT))//没有左对齐但有0填充才可能为真
		while(size-->0)
			*str++ = c;
	// 此时 i 存有数值 num 的数字个数。
	// 若数字个数小于精度值，则 str 中放入(精度值-i)个'0'。
	// 注意这个写法还是挺巧的
	while(i<precision--)
		*str++ = '0';
	//填充数字
	while(i-->0)
		*str++ = tmp[i];
	// 若宽度值仍大于零，则表示类型标志中有左靠齐标志标志。则在剩余宽度中放入空格。
	while(size-->0)
		*str++ = ' ';
	return str;//返回地址
}

int vsprintf(char *buf, const char *fmt, va_list args)
{
	int len;
	int i;
	char * str;
	char *s;
	int *ip;

	int flags;		/* flags to number() */	/* number函数使用的type参数 */

	int field_width;	/* width of output field */	/* 输出字段宽度*/
	
						/* min. 整数数字个数;max. 字符串中字符个数 */
	int precision;		/* min. # of digits for integers; max
				   number of chars for from string */

						/* 'h', 'l',或'L'用于整数字段 */
	int qualifier;		/* 'h', 'l', or 'L' for integer fields */

	for (str=buf ; *fmt ; ++fmt) {
		if (*fmt != '%') {//字符串中没遇到就直接复制给目标字符串
			*str++ = *fmt;
			continue;
		}
			
		/* process flags */
		flags = 0;
		repeat:
			++fmt;		/* this also skips first '%' */	//百分号不用存下来
			switch (*fmt) {
				case '-': flags |= LEFT; goto repeat;//左对齐
				case '+': flags |= PLUS; goto repeat;//正数带加号
				case ' ': flags |= SPACE; goto repeat;//正数为符号位空格
				case '#': flags |= SPECIAL; goto repeat;//0x或0
				case '0': flags |= ZEROPAD; goto repeat;//零填充
				//没匹配到字符就直接往下走
				}

		// 取当前参数字段宽度域值，放入 field_width 变量中。
		// 如果宽度域中是数值则直接取其为宽度值。 
		// 如果宽度域中是字符'*'，表示下一个参数指定宽度。因此调用 va_arg 取宽度值。
		// 若此时宽度值小于 0，则该负数表示其带有标志域'-'标志(左靠齐)，
		// 因此还需在标志变量中添入该标志，并将字段宽度值取为其绝对值。
		/* get field width */
		field_width = -1;
		if (is_digit(*fmt))//得到数字的宽度，如%5d
			//这里传进去是char **，所以可以改fmt的值。这样写就很有意思了。
			//如果传的是char *则不能改，函数返回后fmt还要往后跳过长度。
			field_width = skip_atoi(&fmt);
		else if (*fmt == '*') {//如%*
			/* it's the next argument */	// 这里有个 bug，应插入++fmt;
			field_width = va_arg(args, int);//从参数中得到长度
			if (field_width < 0) {
				field_width = -field_width;
				flags |= LEFT;
			}
		}

		// 下面这段代码，取格式转换串的精度域，并放入 precision 变量中。
		// 精度域开始的标志是'.'。 // 其处理过程与上面宽度域的类似。
		// 如果精度域中是数值则直接取其为精度值。
		// 如果精度域中是字符'*'，表示下一个参数指定精度。因此调用 va_arg 取精度值。
		// 若此时宽度值小于 0，则将字段精度值取为其绝对值。
		/* get the precision */
		precision = -1;
		if (*fmt == '.') {//如%5.2f
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

		// 下面这段代码分析长度修饰符，并将其存入 qualifer 变量。
		/* get the conversion qualifier */
		qualifier = -1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
			qualifier = *fmt;
			++fmt;
		}

		switch (*fmt) {// 下面分析转换指示符
		// 如果转换指示符是'c'，则表示对应参数应是字符。
		// 此时如果标志域表明不是左靠齐，则该字段前面放入宽度域值-1 个空格字符，
		// 然后再放入参数字符。
		// 如果宽度域还大于 0，则表示为左靠齐，则在参数字符后面添加宽度值-1 个空格字符。
		case 'c':
			if (!(flags & LEFT))//非左对齐，填左边
				while (--field_width > 0)
					*str++ = ' ';
			//若传进来的是'0'或'a'，则参数实际的值为48或97，不用再转换成字符
			*str++ = (unsigned char) va_arg(args, int);
			while (--field_width > 0)//左对齐，填右边
				*str++ = ' ';
			break;
		// 如果转换指示符是's'，则表示对应参数是字符串。首先取参数字符串的长度，
		// 若其超过了精度域值， 则扩展精度域=字符串长度。
		// 此时如果标志域表明不是左靠齐，则该字段前放入(宽度值-字符串长度)个空格字符。
		// 然后再放入参数字符串。如果宽度域还大于 0，则表示为左靠齐，
		// 则在参数字符串后面添加(宽度值-字符串长度)个空格字符。
		case 's':
			s = va_arg(args, char *);//传进来的是字符串的首地址
			len = strlen(s);
			//precision好像没什么用
			if (precision < 0)//没有'.'则precision为-1
				precision = len;
			else if (len > precision)//有precision则以其为准
				len = precision;

			if (!(flags & LEFT))//非左对齐，填左边
				while (len < field_width--)
					*str++ = ' ';
			for (i = 0; i < len; ++i)//真正内容
				*str++ = *s++;
			while (len < field_width--)//左对齐，填右边
				*str++ = ' ';
			break;
		//8进制
		case 'o':
			str = number(str, va_arg(args, unsigned long), 8,
				field_width, precision, flags);
			break;
		// 如果格式转换符是'p'，表示对应参数的一个指针类型。
		// 此时若该参数没有设置宽度域，则默认宽度为 8，并且需要添零。
		// 然后调用 number()函数进行处理。
		case 'p':
			if (field_width == -1) {
				field_width = 8;//地址为32位4字节，16进制占8个位置
				flags |= ZEROPAD;//前面要添0
			}
			str = number(str,
				(unsigned long) va_arg(args, void *), 16,
				field_width, precision, flags);
			break;
		// 若格式转换指示是'x'或'X'，则表示对应参数需要打印成十六进制数输出。
		// 'x'表示用小写字母表示。
		case 'x':
			flags |= SMALL;
		case 'X':
			str = number(str, va_arg(args, unsigned long), 16,
				field_width, precision, flags);
			break;

		// 如果格式转换字符是'd','i'或'u'，则表示对应参数是整数，'d', 'i'代表符号整数，
		// 因此需要加上带符号标志。'u'代表无符号整数。
		case 'd':
		case 'i':
			flags |= SIGN;
		case 'u'://无符号好像没做特别处理?
			str = number(str, va_arg(args, unsigned long), 10,
				field_width, precision, flags);
			break;
		// 若格式转换指示符是'n'，
		// 则表示要把到目前为止转换输出的字符数保存到对应参数指针指定的位置中。
		// 首先利用 va_arg()取得该参数指针，然后将已经转换好的字符数存入该指针所指的位置。
		case 'n':
			ip = va_arg(args, int *);
			*ip = (str - buf);//str是已经转换好的地方，buf是起始
			break;

		default:
			if (*fmt != '%')//不是%表示格式有错，还是输出%
				*str++ = '%';
			if (*fmt)//是其他字符，则直接输出
				*str++ = *fmt;
			else//到结尾了
				--fmt;
			break;
		}
	}
	*str = '\0';
	return str-buf;//返回字符串大小
}
