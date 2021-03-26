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
** 文   件   名: clock.h
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 10 月 25 日
**
** 描        述: Kendryte K210 处理器时钟操作函数库 (相对于k210_clock.h来说的安全版本)
*********************************************************************************************************/
#ifndef __CLOCK_H
#define __CLOCK_H
/*********************************************************************************************************
 头文件包含
*********************************************************************************************************/
#include "driver/clock/k210_clock.h"
/*********************************************************************************************************
 系统时钟设置接口
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
