/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: k210_rtc.h
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 10 月 30 日
**
** 描        述: K210 处理器 RTC 驱动
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "SylixOS.h"
#include "config.h"
#include <string.h>
#include <timer.h>
#include "k210_rtc.h"
#include "driver/clock/k210_clock.h"
#include "driver/fix_arch_def.h"
#include <linux/compat.h>

#include "KendryteWare/include/common.h"
#include "KendryteWare/include/rtc.h"
/*********************************************************************************************************
  定义
*********************************************************************************************************/
#define __MASK_HOUR           (0xFF000000)
#define __MASK_MINUTE         (0x00FF0000)
#define __MASK_SECOND         (0x0000FF00)
#define __MASK_MERIDIEM       (0x000000FF)

#define __HOUR_SHIFT          (24)
#define __MINUTE_SHIFT        (16)
#define __SECOND_SHIFT        (8)

#define __MASK_DAY            (0xFF000000)
#define __MASK_MONTH          (0x00FF0000)
#define __MASK_YEAR           (0x0000FF00)
#define __MASK_DOTW           (0x000000FF)

#define __DAY_SHIFT           (24)
#define __MONTH_SHIFT         (16)
#define __YEAR_SHIFT          (8)

#define __RTC_CONTROLER_NR    (1)
/*********************************************************************************************************
  RTC 控制器类型定义
*********************************************************************************************************/
typedef struct {
    sysctl_clock_t      RTCC_clock;
    addr_t              RTCC_ulPhyAddrBase;                             /*  物理地址基地址              */
    BOOL                RTCC_bIsInit;                                   /*  是否已经初始化              */
} __K210_RTC_CONTROLER, *__PK210_RTC_CONTROLER;
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static __K210_RTC_CONTROLER  _G_k210RtcControlers[__RTC_CONTROLER_NR] = {
        {
                .RTCC_ulPhyAddrBase = RTC_BASE_ADDR,
                .RTCC_clock         = SYSCTL_CLOCK_RTC,
        }
};
/*********************************************************************************************************
** 函数名称: __k210BinToBcd
** 功能描述: BIN 码转 BCD 码
** 输　入  : ucBin         BIN 码
** 输　出  : BCD 码
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT8  __k210BinToBcd (UINT8  ucBin)
{
    UINT8  ucBcd;

    if (ucBin >= 100) {
        return  (0);
    }

    ucBcd  = ucBin % 10;
    ucBcd |= (ucBin / 10) << 4;

    return  (ucBcd);
}
/*********************************************************************************************************
** 函数名称: __k210BcdToBin
** 功能描述: BCD 码转 BIN 码
** 输　入  : ucBcd         BCD 码
** 输　出  : BIN 码
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT8  __k210BcdToBin (UINT8  ucBcd)
{
    UINT8  ucBin;

    ucBin = ((ucBcd & 0xF0) >> 4) * 10 + (ucBcd & 0x0F);

    return  (ucBin);
}
/*********************************************************************************************************
** 函数名称: __k210RtcSetTime
** 功能描述: 设置 RTC 时间
** 输　入  : pRtcFuncs         RTC 驱动程序
**           pTimeNow          当前时间
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210RtcSetTime (PLW_RTC_FUNCS  pRtcFuncs, time_t  *pTimeNow)
{
    INTREG                 iregInterLevel;
    struct tm              tmNow;
    UINT                   uiRet;

    gmtime_r(pTimeNow, &tmNow);                                         /*  转换成 tm 时间格式          */

    API_InterLock(&iregInterLevel);

    rtc_timer_set_mode(RTC_TIMER_PAUSE);

    uiRet = rtc_timer_set_tm(&tmNow);

    rtc_timer_set_mode(RTC_TIMER_RUNNING);

    API_InterUnlock(iregInterLevel);

    if (uiRet < 0) {
        printk("__k210RtcSetTime: failed to set timer.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __k210RtcGetTime
** 功能描述: 获取 RTC 时间
** 输　入  : pRtcFuncs         RTC 驱动程序
**           pTimeNow          当前时间
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210RtcGetTime (PLW_RTC_FUNCS  pRtcFuncs, time_t  *pTimeNow)
{
    struct tm              tmNow;

    struct tm *tm = rtc_timer_get_tm();

    if (tm == LW_NULL) {
        return  (PX_ERROR);
    }

    tmNow = *tm;

    if (pTimeNow) {
        *pTimeNow = timegm(&tmNow);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  AM335X 处理器 RTC 驱动程序
*********************************************************************************************************/
static LW_RTC_FUNCS     _G_k210RtcFuncs = {
        LW_NULL,
        __k210RtcSetTime,
        __k210RtcGetTime,
        LW_NULL
};
/*********************************************************************************************************
** 函数名称: __k210RtcInit
** 功能描述: 初始化 RTC 设备
** 输　入  : pRtcControler     RTC 控制器
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210RtcInit (__PK210_RTC_CONTROLER  pRtcControler)
{
    rtc_init();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: rtcGetFuncs
** 功能描述: 获取 RTC 驱动程序
** 输　入  : NONE
** 输　出  : RTC 驱动程序
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PLW_RTC_FUNCS  k210RtcGetFuncs (VOID)
{
    __PK210_RTC_CONTROLER  pRtcControler = &_G_k210RtcControlers[0];

    if (!pRtcControler->RTCC_bIsInit) {
        if (__k210RtcInit(pRtcControler) != ERROR_NONE) {
            printk(KERN_ERR "rtcGetFuncs(): failed to init!\n");
            return  (LW_NULL);
        }
    }

    return  (&_G_k210RtcFuncs);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
