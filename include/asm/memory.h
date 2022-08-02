/*
该文件含有一个内存复制嵌入式汇编宏 memcpy() 。
与 string.h 中定义的 memcpy() 相同，只是后者采用的是嵌入式汇编 C 函数形式定义的。
即一个是宏替换，一个是真正的函数。
*/

/*
 *  NOTE!!! memcpy(dest,src,n) assumes ds=es=normal data segment. This
 *  goes for all kernel functions (ds=es=kernel space, fs=local data,
 *  gs=null), as well as for all well-behaving user programs (ds=es=
 *  user data space). This is NOT a bug, as any user program that changes
 *  es deserves to die if it isn't careful.
 */
/*
 * 注意!!!memcpy(dest,src,n)假设段寄存器 ds=es=通常数据段。在内核中使用的
 * 所有函数都基于该假设（ds=es=内核空间，fs=局部数据空间，gs=null）,具有良好
 * 行为的应用程序也是这样（ds=es=用户数据空间）。如果任何用户程序随意改动了
 * es 寄存器而出错，则并不是由于系统程序错误造成的。
 */
// 内存块复制。从源地址 src 处开始复制 n 个字节到目的地址 dest 处。
// 参数：dest - 复制的目的地址，src - 复制的源地址，n - 复制字节数。
// %0 - edi(目的地址 dest)，%1 - esi(源地址 src)，%2 - ecx(字节数 n)，
#define memcpy(dest,src,n) ({ \
void * _res = dest; \
__asm__ ("cld;rep;movsb" \
	::"D" ((long)(_res)),"S" ((long)(src)),"c" ((long) (n)) \
	:"di","si","cx"); \
_res; \
})
