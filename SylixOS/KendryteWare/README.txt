
/*********************************************************************************************************
 * KendryteWare�µ�Դ�ļ��Ǵӹٷ��� standalone-sdk-v0.2 �� driver Ŀ¼��ֲ�����ģ�Դ�����ص�ַ:
 * https://github.com/kendryte/kendryte-standalone-sdk. SylxiOS�е�������ʹ�ò��ֹ���, ���Ҷ�
 * Դ������������޸ģ����ﻹ�Ǳ�����ȫ���Ĵ����ļ�, ����鿴��
 ********************************************************************************************************/

MODIFY_1: 
# �� bspKendryteK210 ��������� �߼�����Դ���
# ���� bspKendryteK210�����ͷ�ļ�·�� /KendryteWare/include
# �� sysctl.c Ϊ freertos-sdk ��

MODIFY_2: 
# ����߼��⣬��������include���ļ��м��뵽���̵�ͷ�ļ�·������������Դ���ļ������ 
# #define  __SYLIXOS_KERNEL
# #define  __SYLIXOS_STDIO
# #include "SylixOS.h"
# #include "config.h"
# ɾ���л��߱�ǵ�ͷ�ļ�(�ҵ�������ͷ�ļ�): ��Ҫ��#include "encoding.h" #include "syscalls.h" #include "Platform.h" #include "atomic.h"
# ɾ��������е� sysctl.c �� sysctl.h �ļ� (����ʹ�ô�freertos-sdk����ֲ������ sysctl.c �� sysctl.h)
# ���������ʹ�õ� sysctl.h ͷ�ļ��滻�� #include "driver/clock/k210_clock.h"
# ���������ʹ�õ� utils �� clint 
# ��� common.h 
# ɾ�� uart ʹ�������Լ��� uart ����
# ɾ�� i2c ʹ�������Լ��� i2c ���� 

