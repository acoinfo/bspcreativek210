/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: k210_rtc.h
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 10 �� 30 ��
**
** ��        ��: K210 ������ RTC ����
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
  ����
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
  RTC ���������Ͷ���
*********************************************************************************************************/
typedef struct {
    sysctl_clock_t      RTCC_clock;
    addr_t              RTCC_ulPhyAddrBase;                             /*  �����ַ����ַ              */
    BOOL                RTCC_bIsInit;                                   /*  �Ƿ��Ѿ���ʼ��              */
} __K210_RTC_CONTROLER, *__PK210_RTC_CONTROLER;
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static __K210_RTC_CONTROLER  _G_k210RtcControlers[__RTC_CONTROLER_NR] = {
        {
                .RTCC_ulPhyAddrBase = RTC_BASE_ADDR,
                .RTCC_clock         = SYSCTL_CLOCK_RTC,
        }
};
/*********************************************************************************************************
** ��������: __k210BinToBcd
** ��������: BIN ��ת BCD ��
** �䡡��  : ucBin         BIN ��
** �䡡��  : BCD ��
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210BcdToBin
** ��������: BCD ��ת BIN ��
** �䡡��  : ucBcd         BCD ��
** �䡡��  : BIN ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT8  __k210BcdToBin (UINT8  ucBcd)
{
    UINT8  ucBin;

    ucBin = ((ucBcd & 0xF0) >> 4) * 10 + (ucBcd & 0x0F);

    return  (ucBin);
}
/*********************************************************************************************************
** ��������: __k210RtcSetTime
** ��������: ���� RTC ʱ��
** �䡡��  : pRtcFuncs         RTC ��������
**           pTimeNow          ��ǰʱ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210RtcSetTime (PLW_RTC_FUNCS  pRtcFuncs, time_t  *pTimeNow)
{
    INTREG                 iregInterLevel;
    struct tm              tmNow;
    UINT                   uiRet;

    gmtime_r(pTimeNow, &tmNow);                                         /*  ת���� tm ʱ���ʽ          */

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
** ��������: __k210RtcGetTime
** ��������: ��ȡ RTC ʱ��
** �䡡��  : pRtcFuncs         RTC ��������
**           pTimeNow          ��ǰʱ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
  AM335X ������ RTC ��������
*********************************************************************************************************/
static LW_RTC_FUNCS     _G_k210RtcFuncs = {
        LW_NULL,
        __k210RtcSetTime,
        __k210RtcGetTime,
        LW_NULL
};
/*********************************************************************************************************
** ��������: __k210RtcInit
** ��������: ��ʼ�� RTC �豸
** �䡡��  : pRtcControler     RTC ������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210RtcInit (__PK210_RTC_CONTROLER  pRtcControler)
{
    rtc_init();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: rtcGetFuncs
** ��������: ��ȡ RTC ��������
** �䡡��  : NONE
** �䡡��  : RTC ��������
** ȫ�ֱ���:
** ����ģ��:
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
