/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: k210_watchdog_test.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 11 �� 2 ��
**
** ��        ��: K210 ������ WATCHDOG �������Գ���
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "pthread.h"
#include "sys/select.h"
#include <stdio.h>
#include <unistd.h>
#include "KendryteWare/include/wdt.h"
/*********************************************************************************************************
** ��������: wdt0_irq
** ��������: WATCHDOG �жϴ�����
** �䡡��  : pvArg         ����
**           ulVector      �жϺ�
** �䡡��  : �̷߳���ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static irqreturn_t wdt0_irq (PVOID pvArg, ULONG ulVector)
{
    static int s_wdt_irq_cnt = 0;

    printf("%s\n", __func__);

    s_wdt_irq_cnt ++;
    if(s_wdt_irq_cnt < 2) {
        wdt_clear_interrupt(0);
    } else {
        while(1);
    }

    return 1;
}
/*********************************************************************************************************
** ��������: pthread_test
** ��������: WATCHDOG �߼����Դ������ pthread �߳�
** �䡡��  : pvArg         �̲߳���
** �䡡��  : �̷߳���ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  *pthread_test (VOID  *pvArg)
{
    printf("wdt start!\n");

    int timeout = 0;

    API_InterVectorSetPriority(IRQN_WDT0_INTERRUPT + 8, 1);
    API_InterVectorConnect(IRQN_WDT0_INTERRUPT + 8, (PINT_SVR_ROUTINE)wdt0_irq,
                           (PVOID)LW_NULL, "sdk_dog_isr");
    API_InterVectorEnable(IRQN_WDT0_INTERRUPT + 8);

    wdt_start(0, 1000000, LW_NULL);
    while(1)
    {
        timeout ++;
        if(timeout < 6)
            wdt_feed(0);

        sleep(1);
        bspDebugMsg("@ ");
    }

    return  (NULL);
}
/*********************************************************************************************************
** ��������: watchdogTestStart
** ��������: WATCHDOG ����
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  wdtTestStart (VOID)
{
    pthread_t   tid_key;
    INT         iError;

    iError  = pthread_create(&tid_key, NULL, pthread_test, NULL);

    return  (iError == 0 ? ERROR_NONE : PX_ERROR);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
