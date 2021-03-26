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
** ��   ��   ��: clock.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 10 �� 25 ��
**
** ��        ��: Kendryte K210 ������ʱ�Ӳ��������� (�����k210_clock.h��˵�İ�ȫ�汾)
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "config.h"
#include "driver/fix_arch_def.h"
#include "driver/clock/k210_clock.h"                                    /* �����ʱ�����ýӿ�           */
/*********************************************************************************************************
 * ȫ�ֱ�������
*********************************************************************************************************/
static  LW_SPINLOCK_DEFINE(_G_kendryteClkSpl);

#define __KENDRYTE_CLK_OP_LOCK()      LW_SPIN_LOCK_QUICK(&_G_kendryteClkSpl, &intReg)

#define __KENDRYTE_CLK_OP_UNLOCK()    LW_SPIN_UNLOCK_QUICK(&_G_kendryteClkSpl, intReg)
/*********************************************************************************************************
** ��������: sysctlClockEnable
** ��������: ʹ��ָ�������ʹ��Դ
** ��  ��  : clock          Ҫ���õ�ʱ��Դ
** ��  ��  : ERROR_CODE     0 : Success, Other : Fail
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: sysctlClockDisable
** ��������: �ر�ָ�������ʹ��Դ
** ��  ��  : clock          Ҫ���õ�ʱ��Դ
** ��  ��  : ERROR_CODE     0 : Success, Other : Fail
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: sysctlClockSetThreshold
** ��������: ����ʱ�ӷ�Ƶ
** ��  ��  : which          Ҫ���õ�ʱ��Դ
**           threshold      ��Ƶϵ��ֵ
** ��  ��  : ERROR_CODE     0 : Success, Other : Fail
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: sysctlClockGetThreshold
** ��������: ����ʱ�ӷ�Ƶ
** ��  ��  : which          Ҫ���õ�ʱ��Դ
** ��  ��  : ����ֵ         ���ط�Ƶϵ��, �������-1��ʾʧ��
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: sysctlClockSetClockSelect
** ��������: ʱ��Դѡ��
** ��  ��  : which          ʱ��ԴID
**           select         �Ƿ�ѡ��
** ��  ��  : ERROR_CODE     0 : Success, Other : Fail
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: sysctlClockGetClockSelect
** ��������: ��ȡʱ��Դѡ��
** ��  ��  : which          ʱ��ԴID
** ��  ��  : ����ֵ         Value of clock select, -1 : Fail
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: sysctlClockSourceGetFreq
** ��������: ��ȡʱ��ԴƵ��
** ��  ��  : input          ����ʱ��ԴID
** ��  ��  : ����ֵ         ����ʱ��Դ��Ƶ��
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: sysctlPllGetFreq
** ��������: ��ȡPLLʱ��Ƶ��
** ��  ��  : pll            PLL ID
** ��  ��  : ����ֵ         ����PLLʱ�ӵ�Ƶ��
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: sysctlPllSetFreq
** ��������: ���û�ȡPLLʱ��Ƶ�ʺ�����ʱ��
** ��  ��  : pll            PLL ID
**           sel            ��ѡ���PPL����ʱ��
**           freq           ����ʱ��Ƶ��
** ��  ��  : ����ֵ         ����PLLʱ�ӵ�Ƶ��
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: sysctlClockGetFreq
** ��������: ͨ��ʱ��id��ȡ����ʱ��Ƶ��
** ��  ��  : clock          clock ID
** ��  ��  : ����ֵ         ����ʱ��Ƶ��
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: sysctlReset
** ��������: ͨ�����ÿ������������豸
** ��  ��  : clock          clock ID
**           reset          �����ź�
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  sysctlReset (enum sysctl_reset_e reset)
{
    INTREG   intReg;

    __KENDRYTE_CLK_OP_LOCK();

    sysctl_reset(reset);

    __KENDRYTE_CLK_OP_UNLOCK();
}
/*********************************************************************************************************
** ��������: sysctlGetGitId
** ��������: ���Git�汾��Ϣ Git short commit id
** ��  ��  : NONE
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: sysctlGetFreq
** ��������: ��ȡ����ʱ��Ƶ��, Ĭ����26MHz
** ��  ��  : NONE
** ��  ��  : ����ֵ      ����ʱ��Ƶ��
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: sysctlPllIsLock
** ��������: ��ȡ PLL ʱ�ӵ�״̬
** ��  ��  : pll         The pll id
** ��  ��  : ����ֵ      1 : Pll is lock, 0 : Pll have lost lock
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: sysctlPllClearSlip
** ��������: ��� PLL ״̬λ
** ��  ��  : pll         The pll id
** ��  ��  : ����ֵ      0 : Success, Other : Fail
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: sysctlPllEnable
** ��������: PLL ʹ��
** ��  ��  : pll         The pll id
** ��  ��  : ����ֵ      0 : Success, Other : Fail
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: sysctlPllDisable
** ��������: ��ֹ PLL ���ϵ�
** ��  ��  : pll         The pll id
** ��  ��  : ����ֵ      0 : Success, Other : Fail
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: sysctlDmaSelect
** ��������: ѡ�� DMA ͨ��������������ź�
** ��  ��  : channel     The DMA channel
**           select      The peripheral select
** ��  ��  : ����ֵ      0 : Success, Other : Fail
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: sysctlPllFastEnablePll
** ��������: ���ٳ�ʼ������PLL��CPUʱ��
** ��  ��  : NONE
** ��  ��  : ����ֵ      0 : Success, Other : Fail
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: sysctlSpi0DvpDataSet
** ��������: Set SPI0_D0-D7 DVP_D0-D7 as spi and dvp data pin
** ��  ��  : en          Enable or not
** ��  ��  : ����ֵ      0 : Success, Other : Fail
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: sysctlPowerModeSel
** ��������: ����ָ��IO�ĵ�ѹģʽ
** ��  ��  : power_bank          IO power bank
**           io_power_mode       Set power mode 3.3v or 1.8v
** ��  ��  : ����ֵ      0 : Success, Other : Fail
** ȫ�ֱ���:
** ����ģ��:
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
