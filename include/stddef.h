/*
stddef.h ͷ�ļ�������Ҳ���� C ��׼����֯�� X3J11 �������ģ������Ǳ�׼�� std ������ (def) ��
��Ҫ���ڴ��һЩ����׼���塱������һ���������׻�����ͷ�ļ��� stdlib.h ��Ҳ���ɱ�׼����֯�����ġ� 
stdlib.h��Ҫ��������һЩ��������ͷ�ļ�������صĸ��ֺ�����
��������ͷ�ļ��е����ݳ������˸㲻����Щ�������ĸ�ͷ�ļ��С�
*/


#ifndef _STDDEF_H
#define _STDDEF_H

#ifndef _PTRDIFF_T
#define _PTRDIFF_T
typedef long ptrdiff_t;			// ����ָ�������������͡�
#endif

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned long size_t;	// sizeof ���ص����͡�
#endif

#undef NULL
#define NULL ((void *)0)		// ��ָ�롣

// ��Ա�������е�ƫ��λ��
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)	//��0���е㶫��

#endif
