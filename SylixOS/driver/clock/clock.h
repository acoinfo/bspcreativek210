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
** ��   ��   ��: clock.h
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 10 �� 25 ��
**
** ��        ��: Kendryte K210 ������ʱ�Ӳ��������� (�����k210_clock.h��˵�İ�ȫ�汾)
*********************************************************************************************************/
#ifndef __CLOCK_H
#define __CLOCK_H
/*********************************************************************************************************
 ͷ�ļ�����
*********************************************************************************************************/
#include "driver/clock/k210_clock.h"
/*********************************************************************************************************
 ϵͳʱ�����ýӿ�
*********************************************************************************************************/

INT sysctlClockEnable(enum sysctl_clock_e clock);
INT sysctlClockDisable(enum sysctl_clock_e clock);
INT sysctlClockSetThreshold(enum sysctl_threshold_e which, INT iThreshold);
INT sysctlClockGetThreshold(enum sysctl_threshold_e which);
INT sysctlClockSetClockSelect(enum sysctl_clock_select_e which, INT iSelect);
INT sysctlClockGetClockSelect(enum sysctl_clock_select_e which);

UINT32 sysctlClockSourceGetFreq(enum sysctl_clock_source_e input);
UINT32 sysctlPllGetFreq(enum sysctl_pll_e pll);
UINT32 sysctlPllSetFreq(enum sysctl_pll_e pll, enum sysctl_clock_source_e source, UINT32 uiFreq);
UINT32 sysctlClockGetFreq(enum sysctl_clock_e clock);

VOID   sysctlReset(enum sysctl_reset_e reset);
UINT32 sysctlGetGitId(VOID);
UINT32 sysctlGetFreq(VOID);

INT sysctlPllIsLock(enum sysctl_pll_e pll);
INT sysctlPllClearSlip(enum sysctl_pll_e pll);
INT sysctlPllEnable(enum sysctl_pll_e pll);
INT sysctlPllDisable(enum sysctl_pll_e pll);
INT sysctlDmaSelect(enum sysctl_dma_channel_e channel, enum sysctl_dma_select_e select);

UINT32 sysctlPllFastEnablePll(VOID);
UINT32 sysctlSpi0DvpDataSet(UINT8 uiEnable);
UINT32 sysctlPowerModeSel(UINT8 uiPowerBank, io_power_mode_t io_power_mode);

#endif                                                                  /*          __CLOCK_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
