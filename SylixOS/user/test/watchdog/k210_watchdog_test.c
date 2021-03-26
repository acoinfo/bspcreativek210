/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: k210_watchdog_test.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 11 月 2 日
**
** 描        述: K210 处理器 WATCHDOG 驱动测试程序
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
** 函数名称: wdt0_irq
** 功能描述: WATCHDOG 中断处理函数
** 输　入  : pvArg         参数
**           ulVector      中断号
** 输　出  : 线程返回值
** 全局变量:
** 调用模块:
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
** 函数名称: pthread_test
** 功能描述: WATCHDOG 逻辑测试代码测试 pthread 线程
** 输　入  : pvArg         线程参数
** 输　出  : 线程返回值
** 全局变量:
** 调用模块:
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
** 函数名称: watchdogTestStart
** 功能描述: WATCHDOG 测试
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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
