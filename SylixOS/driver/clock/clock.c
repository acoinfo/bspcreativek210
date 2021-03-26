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
** 文   件   名: clock.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 10 月 25 日
**
** 描        述: Kendryte K210 处理器时钟操作函数库 (相对于k210_clock.h来说的安全版本)
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "config.h"
#include "driver/fix_arch_def.h"
#include "driver/clock/k210_clock.h"                                    /* 裸机库时钟设置接口           */
/*********************************************************************************************************
 * 全局变量定义
*********************************************************************************************************/
static  LW_SPINLOCK_DEFINE(_G_kendryteClkSpl);

#define __KENDRYTE_CLK_OP_LOCK()      LW_SPIN_LOCK_QUICK(&_G_kendryteClkSpl, &intReg)

#define __KENDRYTE_CLK_OP_UNLOCK()    LW_SPIN_UNLOCK_QUICK(&_G_kendryteClkSpl, intReg)
/*********************************************************************************************************
** 函数名称: sysctlClockEnable
** 功能描述: 使能指定外设的使用源
** 输  入  : clock          要设置的时钟源
** 输  出  : ERROR_CODE     0 : Success, Other : Fail
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  sysctlClockEnable (enum sysctl_clock_e  clock)
{
    INT      res;
    INTREG   intReg;

    __KENDRYTE_CLK_OP_LOCK();

    res = sysctl_clock_enable(clock);

    __KENDRYTE_CLK_OP_UNLOCK();

    return  (res);
}
/*********************************************************************************************************
** 函数名称: sysctlClockDisable
** 功能描述: 关闭指定外设的使用源
** 输  入  : clock          要设置的时钟源
** 输  出  : ERROR_CODE     0 : Success, Other : Fail
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  sysctlClockDisable (enum sysctl_clock_e  clock)
{
    INT      res;
    INTREG   intReg;

    __KENDRYTE_CLK_OP_LOCK();

    res = sysctl_clock_disable(clock);

    __KENDRYTE_CLK_OP_UNLOCK();

    return  (res);
}
/*********************************************************************************************************
** 函数名称: sysctlClockSetThreshold
** 功能描述: 设置时钟分频
** 输  入  : which          要设置的时钟源
**           threshold      分频系数值
** 输  出  : ERROR_CODE     0 : Success, Other : Fail
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  sysctlClockSetThreshold (enum sysctl_threshold_e  which, INT  iThreshold)
{
    INT      res;
    INTREG   intReg;

    __KENDRYTE_CLK_OP_LOCK();

    res = sysctl_clock_set_threshold(which, iThreshold);

    __KENDRYTE_CLK_OP_UNLOCK();

    return  (res);
}
/*********************************************************************************************************
** 函数名称: sysctlClockGetThreshold
** 功能描述: 设置时钟分频
** 输  入  : which          要设置的时钟源
** 输  出  : 返回值         返回分频系数, 如果返回-1表示失败
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  sysctlClockGetThreshold (enum sysctl_threshold_e  which)
{
    INT      res;
    INTREG   intReg;

    __KENDRYTE_CLK_OP_LOCK();

    res = sysctl_clock_get_threshold(which);

    __KENDRYTE_CLK_OP_UNLOCK();

    return  (res);
}
/*********************************************************************************************************
** 函数名称: sysctlClockSetClockSelect
** 功能描述: 时钟源选择
** 输  入  : which          时钟源ID
**           select         是否选择
** 输  出  : ERROR_CODE     0 : Success, Other : Fail
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  sysctlClockSetClockSelect (enum sysctl_clock_select_e  which, INT  iSelect)
{
    INT      res;
    INTREG   intReg;

    __KENDRYTE_CLK_OP_LOCK();

    res = sysctl_clock_set_clock_select(which, iSelect);

    __KENDRYTE_CLK_OP_UNLOCK();

    return  (res);
}
/*********************************************************************************************************
** 函数名称: sysctlClockGetClockSelect
** 功能描述: 获取时钟源选择
** 输  入  : which          时钟源ID
** 输  出  : 返回值         Value of clock select, -1 : Fail
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  sysctlClockGetClockSelect (enum sysctl_clock_select_e  which)
{
    INT      res;
    INTREG   intReg;

    __KENDRYTE_CLK_OP_LOCK();

    res = sysctl_clock_get_clock_select(which);

    __KENDRYTE_CLK_OP_UNLOCK();

    return  (res);
}
/*********************************************************************************************************
** 函数名称: sysctlClockSourceGetFreq
** 功能描述: 获取时钟源频率
** 输  入  : input          输入时钟源ID
** 输  出  : 返回值         返回时钟源的频率
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT32  sysctlClockSourceGetFreq (enum sysctl_clock_source_e  input)
{
    UINT32      res;
    INTREG   intReg;

    __KENDRYTE_CLK_OP_LOCK();

    res = sysctl_clock_source_get_freq(input);

    __KENDRYTE_CLK_OP_UNLOCK();

    return  (res);
}
/*********************************************************************************************************
** 函数名称: sysctlPllGetFreq
** 功能描述: 获取PLL时钟频率
** 输  入  : pll            PLL ID
** 输  出  : 返回值         返回PLL时钟的频率
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT32  sysctlPllGetFreq (enum sysctl_pll_e  pll)
{
    UINT32      res;
    INTREG   intReg;

    __KENDRYTE_CLK_OP_LOCK();

    res = sysctl_pll_get_freq(pll);

    __KENDRYTE_CLK_OP_UNLOCK();

    return  (res);
}
/*********************************************************************************************************
** 函数名称: sysctlPllSetFreq
** 功能描述: 设置获取PLL时钟频率和输入时钟
** 输  入  : pll            PLL ID
**           sel            被选择的PPL输入时钟
**           freq           输入时钟频率
** 输  出  : 返回值         返回PLL时钟的频率
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT32  sysctlPllSetFreq (enum sysctl_pll_e  pll, enum sysctl_clock_source_e  source, UINT32  uiFreq)
{
    UINT32      res;
    INTREG   intReg;

    __KENDRYTE_CLK_OP_LOCK();

    res = sysctl_pll_set_freq(pll, source, uiFreq);

    __KENDRYTE_CLK_OP_UNLOCK();

    return  (res);
}
/*********************************************************************************************************
** 函数名称: sysctlClockGetFreq
** 功能描述: 通过时钟id获取基础时钟频率
** 输  入  : clock          clock ID
** 输  出  : 返回值         返回时钟频率
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT32  sysctlClockGetFreq (enum sysctl_clock_e  clock)
{
    UINT32      res;
    INTREG   intReg;

    __KENDRYTE_CLK_OP_LOCK();

    res = sysctl_clock_get_freq(clock);

    __KENDRYTE_CLK_OP_UNLOCK();

    return  (res);
}
/*********************************************************************************************************
** 函数名称: sysctlReset
** 功能描述: 通过重置控制器来重置设备
** 输  入  : clock          clock ID
**           reset          重置信号
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  sysctlReset (enum sysctl_reset_e reset)
{
    INTREG   intReg;

    __KENDRYTE_CLK_OP_LOCK();

    sysctl_reset(reset);

    __KENDRYTE_CLK_OP_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: sysctlGetGitId
** 功能描述: 获得Git版本信息 Git short commit id
** 输  入  : NONE
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT32  sysctlGetGitId (VOID)
{
    UINT32      res;
    INTREG   intReg;

    __KENDRYTE_CLK_OP_LOCK();

    res = sysctl_get_git_id();

    __KENDRYTE_CLK_OP_UNLOCK();

    return  (res);
}
/*********************************************************************************************************
** 函数名称: sysctlGetFreq
** 功能描述: 获取基础时钟频率, 默认是26MHz
** 输  入  : NONE
** 输  出  : 返回值      基础时钟频率
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT32  sysctlGetFreq (VOID)
{
    UINT32      res;
    INTREG   intReg;

    __KENDRYTE_CLK_OP_LOCK();

    res = sysctl_get_freq();

    __KENDRYTE_CLK_OP_UNLOCK();

    return  (res);
}
/*********************************************************************************************************
** 函数名称: sysctlPllIsLock
** 功能描述: 获取 PLL 时钟的状态
** 输  入  : pll         The pll id
** 输  出  : 返回值      1 : Pll is lock, 0 : Pll have lost lock
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  sysctlPllIsLock (enum sysctl_pll_e  pll)
{
    INT      res;
    INTREG   intReg;

    __KENDRYTE_CLK_OP_LOCK();

    res = sysctl_pll_is_lock(pll);

    __KENDRYTE_CLK_OP_UNLOCK();

    return  (res);
}
/*********************************************************************************************************
** 函数名称: sysctlPllClearSlip
** 功能描述: 清除 PLL 状态位
** 输  入  : pll         The pll id
** 输  出  : 返回值      0 : Success, Other : Fail
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  sysctlPllClearSlip (enum sysctl_pll_e  pll)
{
    INT      res;
    INTREG   intReg;

    __KENDRYTE_CLK_OP_LOCK();

    res = sysctl_pll_clear_slip(pll);

    __KENDRYTE_CLK_OP_UNLOCK();

    return  (res);
}
/*********************************************************************************************************
** 函数名称: sysctlPllEnable
** 功能描述: PLL 使能
** 输  入  : pll         The pll id
** 输  出  : 返回值      0 : Success, Other : Fail
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  sysctlPllEnable (enum sysctl_pll_e  pll)
{
    INT      res;
    INTREG   intReg;

    __KENDRYTE_CLK_OP_LOCK();

    res = sysctl_pll_enable(pll);

    __KENDRYTE_CLK_OP_UNLOCK();

    return  (res);
}
/*********************************************************************************************************
** 函数名称: sysctlPllDisable
** 功能描述: 禁止 PLL 并断电
** 输  入  : pll         The pll id
** 输  出  : 返回值      0 : Success, Other : Fail
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  sysctlPllDisable (enum sysctl_pll_e  pll)
{
    INT      res;
    INTREG   intReg;

    __KENDRYTE_CLK_OP_LOCK();

    res = sysctl_pll_disable(pll);

    __KENDRYTE_CLK_OP_UNLOCK();

    return  (res);
}
/*********************************************************************************************************
** 函数名称: sysctlDmaSelect
** 功能描述: 选择 DMA 通道和外设的握手信号
** 输  入  : channel     The DMA channel
**           select      The peripheral select
** 输  出  : 返回值      0 : Success, Other : Fail
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  sysctlDmaSelect (enum sysctl_dma_channel_e  channel, enum sysctl_dma_select_e  select)
{
    INT      res;
    INTREG   intReg;

    __KENDRYTE_CLK_OP_LOCK();

    res = sysctl_dma_select(channel, select);

    __KENDRYTE_CLK_OP_UNLOCK();

    return  (res);
}
/*********************************************************************************************************
** 函数名称: sysctlPllFastEnablePll
** 功能描述: 快速初始化所有PLL和CPU时钟
** 输  入  : NONE
** 输  出  : 返回值      0 : Success, Other : Fail
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT32  sysctlPllFastEnablePll (VOID)
{
    INT      res;
    INTREG   intReg;

    __KENDRYTE_CLK_OP_LOCK();

    res = sysctl_pll_fast_enable_pll();

    __KENDRYTE_CLK_OP_UNLOCK();

    return  (res);
}
/*********************************************************************************************************
** 函数名称: sysctlSpi0DvpDataSet
** 功能描述: Set SPI0_D0-D7 DVP_D0-D7 as spi and dvp data pin
** 输  入  : en          Enable or not
** 输  出  : 返回值      0 : Success, Other : Fail
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT32  sysctlSpi0DvpDataSet (UINT8  uiEnable)
{
    INT      res;
    INTREG   intReg;

    __KENDRYTE_CLK_OP_LOCK();

    res = sysctl_spi0_dvp_data_set(uiEnable);

    __KENDRYTE_CLK_OP_UNLOCK();

    return  (res);
}
/*********************************************************************************************************
** 函数名称: sysctlPowerModeSel
** 功能描述: 设置指定IO的电压模式
** 输  入  : power_bank          IO power bank
**           io_power_mode       Set power mode 3.3v or 1.8v
** 输  出  : 返回值      0 : Success, Other : Fail
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT32  sysctlPowerModeSel (UINT8  uiPowerBank, io_power_mode_t  io_power_mode)
{
    UINT32      res;
    INTREG   intReg;

    __KENDRYTE_CLK_OP_LOCK();

    res = sysctl_power_mode_sel(uiPowerBank, io_power_mode);

    __KENDRYTE_CLK_OP_UNLOCK();

    return  (res);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
