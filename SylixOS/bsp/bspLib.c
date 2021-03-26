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
** 文   件   名: bspLib.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2018 年 03 月 20 日
**
** 描        述: 处理器需要为 SylixOS 提供的功能支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"
#include "SylixOS.h"
#include "driver/clint/k210_clint.h"
#include "driver/irqchip/plic_irqchip.h"
#include "driver/clock/k210_clock.h"
#include "KendryteWare/include/uarths.h"
#include "KendryteWare/include/wdt.h"
/**********************************************************************************************************
  BSP 信息
*********************************************************************************************************/
static const CHAR   _G_pcCpuInfo[]     = "Kendryte K210 (RISC-V RV64IMAFDC 160MHz) 2core";
static const CHAR   _G_pcCacheInfo[]   = "L1-Cache (D-32K/I-32K) per core";
static const CHAR   _G_pcPacketInfo[]  = "Kendryte K210";
static const CHAR   _G_pcVersionInfo[] = "BSP version 1.0.0 for "__SYLIXOS_RELSTR;
/*********************************************************************************************************
  CLINT 中断相关
*********************************************************************************************************/
#if LW_CFG_RISCV_M_LEVEL > 0
#define xie     "mie"
#define xip     "mip"
#else
#define xie     "sie"
#define xip     "sip"
#endif
/*********************************************************************************************************
  CLINT 相关寄存器偏移
*********************************************************************************************************/
#define KENDRYTE_CLINT_SIP_OFFSET         (0x0)
#define KENDRYTE_CLINT_TIMECMP_OFFSET     (0x4000)
#define KENDRYTE_CLINT_TIME_OFFSET        (0xbff8)
/*********************************************************************************************************
  PLIC 中断相关
**********************************************************************************************************/
static PLIC_IRQCHIP  _G_plicIrqChip = {
        .PLIC_pvBase       = (PVOID)KENDRYTE_PLIC_BASE,
        .PLIC_ulVectorBase = KENDRYTE_PLIC_VECTOR_BASE,
        .PLIC_ulDevNr      = KENDRYTE_PLIC_DEV_NR,
};
/**********************************************************************************************************
** 函数名称: bspIntInit
** 功能描述: 中断系统初始化
** 输  入  : NONE
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  bspIntInit (VOID)
{
    write_csr(xie, 0);
    write_csr(xip, 0);

    plicIntInit(&_G_plicIrqChip);

    set_csr(xie, 1 << BSP_CFG_EXT_VECTOR);
}
/*********************************************************************************************************
** 函数名称: bspIntHandle
** 功能描述: 中断入口
** 输  入  : NONE
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  bspIntHandle (VOID)
{
    REGISTER ULONG  ulVector;
    REGISTER ULONG  ulXIP;

    ulXIP = read_csr(xip);
    for (ulVector = 0; ulVector < KENDRYTE_CLINT_VECTOR_NR; ulVector++) {
        if (ulXIP & (1 << ulVector)) {
            clear_csr(xip, 1 << ulVector);
            if (ulVector == BSP_CFG_EXT_VECTOR || ulVector == 9) {      /* 系统启动后会触发9号中断，而  */
                plicIntHandle(&_G_plicIrqChip, 0);                      /* 该中断不应产生，这里过滤掉   */
                continue;
            }
            archIntHandle(ulVector, 0);
        }
    }
}
/*********************************************************************************************************
** 函数名称: bspIntVectorEnable
** 功能描述: 使能指定的中断向量
** 输  入  : ulVector     中断向量
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  bspIntVectorEnable (ULONG  ulVector)
{
    if (ulVector < KENDRYTE_CLINT_VECTOR_NR) {
        set_csr(xie, 1 << ulVector);
    } else {
        plicIntVectorEnable(&_G_plicIrqChip, KENDRYTE_PLIC_IRQ(ulVector));
    }
}
/*********************************************************************************************************
** 函数名称: bspIntVectorDisable
** 功能描述: 禁能指定的中断向量
** 输  入  : ulVector     中断向量
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  bspIntVectorDisable (ULONG  ulVector)
{
    if (ulVector < KENDRYTE_CLINT_VECTOR_NR) {
        clear_csr(xie, 1 << ulVector);

    } else {
        plicIntVectorDisable(&_G_plicIrqChip, KENDRYTE_PLIC_IRQ(ulVector));
    }
}
/*********************************************************************************************************
** 函数名称: bspIntVectorIsEnable
** 功能描述: 检查指定的中断向量是否使能
** 输  入  : ulVector     中断向量
** 输  出  : LW_FALSE 或 LW_TRUE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL  bspIntVectorIsEnable (ULONG  ulVector)
{
    if (ulVector < KENDRYTE_CLINT_VECTOR_NR) {
        REGISTER ULONG  ulXIE;

        ulXIE = read_csr(xie);
        return  ((ulXIE & (1 << ulVector)) ? LW_TRUE : LW_FALSE);
    } else {
        return  (plicIntVectorIsEnable(&_G_plicIrqChip, KENDRYTE_PLIC_IRQ(ulVector)));
    }
}
/*********************************************************************************************************
** 函数名称: bspIntVectorSetPriority
** 功能描述: 设置指定的中断向量的优先级
** 输  入  : ulVector     中断向量号
**           uiPrio       优先级
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_INTER_PRIO > 0

ULONG   bspIntVectorSetPriority (ULONG  ulVector, UINT  uiPrio)
{
    if (ulVector < KENDRYTE_CLINT_VECTOR_NR) {
        return  (ERROR_NONE);

    } else {
        return  (plicIntVectorSetPriority(&_G_plicIrqChip, KENDRYTE_PLIC_IRQ(ulVector), uiPrio));
    }
}
/*********************************************************************************************************
** 函数名称: bspIntVectorGetPriority
** 功能描述: 获取指定的中断向量的优先级
** 输  入  : ulVector     中断向量号
**           puiPrio      优先级
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ULONG   bspIntVectorGetPriority (ULONG  ulVector, UINT  *puiPrio)
{
    if (ulVector < KENDRYTE_CLINT_VECTOR_NR) {
        *puiPrio = 0;
        return  (ERROR_NONE);

    } else {
        return  (plicIntVectorGetPriority(&_G_plicIrqChip, KENDRYTE_PLIC_IRQ(ulVector), puiPrio));
    }
}

#endif                                                                  /*  LW_CFG_INTER_PRIO > 0       */
/*********************************************************************************************************
** 函数名称: bspIntVectorSetTarget
** 功能描述: 设置指定的中断向量的目标 CPU
** 输　入  : ulVector      中断向量号
**           stSize        CPU 掩码集内存大小
**           pcpuset       CPU 掩码
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_INTER_TARGET > 0

ULONG   bspIntVectorSetTarget (ULONG  ulVector, size_t  stSize, const PLW_CLASS_CPUSET  pcpuset)
{
    if (ulVector < KENDRYTE_CLINT_VECTOR_NR) {
        return  (ERROR_NONE);

    } else {
        return  (plicIntVectorSetTarget(&_G_plicIrqChip, KENDRYTE_PLIC_IRQ(ulVector), stSize, pcpuset));
    }
}
/*********************************************************************************************************
** 函数名称: bspIntVectorGetTarget
** 功能描述: 获取指定的中断向量的目标 CPU
** 输　入  : ulVector      中断向量号
**           stSize        CPU 掩码集内存大小
**           pcpuset       CPU 掩码
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ULONG   bspIntVectorGetTarget (ULONG  ulVector, size_t  stSize, PLW_CLASS_CPUSET  pcpuset)
{
    if (ulVector < KENDRYTE_CLINT_VECTOR_NR) {
        LW_CPU_ZERO(pcpuset);
        LW_CPU_SET(LW_CPU_GET_CUR_ID(), pcpuset);
        return  (ERROR_NONE);

    } else {
        return  (plicIntVectorGetTarget(&_G_plicIrqChip, KENDRYTE_PLIC_IRQ(ulVector), stSize, pcpuset));
    }
}

#endif                                                                  /*  LW_CFG_INTER_TARGET > 0     */
/*********************************************************************************************************
  BSP 信息
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: bspInfoCpu
** 功能描述: BSP CPU 信息
** 输　入  : NONE
** 输　出  : CPU 信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
CPCHAR  bspInfoCpu (VOID)
{
    return  (_G_pcCpuInfo);
}
/*********************************************************************************************************
** 函数名称: bspInfoCache
** 功能描述: BSP CACHE 信息
** 输　入  : NONE
** 输　出  : CACHE 信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
CPCHAR  bspInfoCache (VOID)
{
    return  (_G_pcCacheInfo);
}
/*********************************************************************************************************
** 函数名称: bspInfoPacket
** 功能描述: BSP PACKET 信息
** 输　入  : NONE
** 输　出  : PACKET 信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
CPCHAR  bspInfoPacket (VOID)
{
    return  (_G_pcPacketInfo);
}
/*********************************************************************************************************
** 函数名称: bspInfoVersion
** 功能描述: BSP VERSION 信息
** 输　入  : NONE
** 输　出  : BSP VERSION 信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
CPCHAR  bspInfoVersion (VOID)
{
    return  (_G_pcVersionInfo);
}
/*********************************************************************************************************
** 函数名称: bspInfoHwcap
** 功能描述: BSP 硬件特性
** 输　入  : NONE
** 输　出  : 硬件特性 (如果支持硬浮点, 可以加入 HWCAP_VFP , HWCAP_VFPv3 , HWCAP_VFPv3D16 , HWCAP_NEON)
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ULONG  bspInfoHwcap (VOID)
{
    /*
     * TODO: 返回硬件特性 (如果支持硬浮点, 可以加入 HWCAP_VFP , HWCAP_VFPv3 , HWCAP_VFPv3D16 , HWCAP_NEON)
     */
    return  (0ul);
}
/*********************************************************************************************************
** 函数名称: bspInfoRomBase
** 功能描述: BSP ROM 基地址
** 输　入  : NONE
** 输　出  : ROM 基地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
addr_t bspInfoRomBase (VOID)
{
    return  (BSP_CFG_ROM_BASE);
}
/*********************************************************************************************************
** 函数名称: bspInfoRomSize
** 功能描述: BSP ROM 大小
** 输　入  : NONE
** 输　出  : ROM 大小
** 全局变量:
** 调用模块:
*********************************************************************************************************/
size_t bspInfoRomSize (VOID)
{
    return  (BSP_CFG_ROM_SIZE);
}
/*********************************************************************************************************
** 函数名称: bspInfoRamBase
** 功能描述: BSP RAM 基地址
** 输　入  : NONE
** 输　出  : RAM 基地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
addr_t bspInfoRamBase (VOID)
{
    return  (BSP_CFG_RAM_BASE);
}
/*********************************************************************************************************
** 函数名称: bspInfoRamSize
** 功能描述: BSP RAM 大小
** 输　入  : NONE
** 输　出  : RAM 大小
** 全局变量:
** 调用模块:
*********************************************************************************************************/
size_t bspInfoRamSize (VOID)
{
    return  (BSP_CFG_RAM_SIZE);
}
/*********************************************************************************************************
  BSP HOOK
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: bspTaskCreateHook
** 功能描述: 任务创建 hook
** 输  入  : ulId     任务 ID
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  bspTaskCreateHook (LW_OBJECT_ID  ulId)
{
    (VOID)ulId;

    /*
     * TODO: 加入你的处理代码
     */
}
/*********************************************************************************************************
** 函数名称: bspTaskDeleteHook
** 功能描述: 任务删除 hook
** 输  入  : ulId         任务 ID
**           pvReturnVal  返回值
**           ptcb         任务控制块
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  bspTaskDeleteHook (LW_OBJECT_ID  ulId, PVOID  pvReturnVal, PLW_CLASS_TCB  ptcb)
{
    (VOID)ulId;
    (VOID)pvReturnVal;
    (VOID)ptcb;

    /*
     * TODO: 加入你的处理代码
     */
}
/*********************************************************************************************************
** 函数名称: bspTaskSwapHook
** 功能描述: 任务切换 hook
** 输  入  : hOldThread       被换出的任务
**           hNewThread       要运行的任务
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  bspTaskSwapHook (LW_OBJECT_HANDLE   hOldThread, LW_OBJECT_HANDLE   hNewThread)
{
    (VOID)hOldThread;
    (VOID)hNewThread;

    /*
     * TODO: 加入你的处理代码
     */
}
/*********************************************************************************************************
** 函数名称: bspTaskIdleHook
** 功能描述: idle 任务调用此函数
** 输  入  : NONE
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  bspTaskIdleHook (VOID)
{
    /*
     * TODO: 加入你的处理代码
     */
}
/*********************************************************************************************************
** 函数名称: bspTaskIdleHook
** 功能描述: 每个操作系统时钟节拍，系统将调用这个函数
** 输  入  : i64Tick      系统当前时钟
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  bspTickHook (INT64   i64Tick)
{
    (VOID)i64Tick;

    /*
     * TODO: 加入你的处理代码
     */
}
/*********************************************************************************************************
** 函数名称: bspWdTimerHook
** 功能描述: 看门狗定时器到时间时都会调用这个函数
** 输  入  : ulId     任务 ID
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  bspWdTimerHook (LW_OBJECT_ID  ulId)
{
    (VOID)ulId;

    /*
     * TODO: 加入你的处理代码
     */
}
/*********************************************************************************************************
** 函数名称: bspTCBInitHook
** 功能描述: 初始化 TCB 时会调用这个函数
** 输  入  : ulId     任务 ID
**           ptcb     任务控制块
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  bspTCBInitHook (LW_OBJECT_ID  ulId, PLW_CLASS_TCB   ptcb)
{
    (VOID)ulId;
    (VOID)ptcb;

    /*
     * TODO: 加入你的处理代码
     */
}
/*********************************************************************************************************
** 函数名称: bspKernelInitHook
** 功能描述: 系统初始化完成时会调用这个函数
** 输  入  : NONE
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  bspKernelInitHook (VOID)
{
    /*
     * TODO: 加入你的处理代码
     */
}
/*********************************************************************************************************
** 函数名称: bspReboot
** 功能描述: 系统重新启动
** 输　入  : iRebootType       重启类型
**           ulStartAddress    启动开始地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  bspReboot (INT  iRebootType, addr_t  ulStartAddress)
{
    UINT8 uiTopValue = 0;

    (VOID)ulStartAddress;

    sysctl_reset(SYSCTL_RESET_WDT0);
    sysctl_clock_set_threshold(SYSCTL_THRESHOLD_WDT0, 0);
    sysctl_clock_enable(SYSCTL_CLOCK_WDT0);

    uiTopValue = k210_wdt_get_top(0, 10);

    k210_wdt_disable(0);
    k210_wdt_response_mode(0, WDT_CR_RMOD_RESET);
    k210_wdt_set_timeout(0, uiTopValue);
    k210_wdt_enable(0);

    while (1);
}
/*********************************************************************************************************
** 函数名称: bspDebugMsg
** 功能描述: 打印系统调试信息
** 输　入  : pcMsg     信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  bspDebugMsg (CPCHAR  pcMsg)
{
    uarths_puts(pcMsg);
}
/*********************************************************************************************************
  CACHE 相关接口
*********************************************************************************************************/
/*********************************************************************************************************
  MMU 相关接口
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

/*********************************************************************************************************
** 函数名称: bspMmuPgdMaxNum
** 功能描述: 获得 PGD 池的数量
** 输  入  : NONE
** 输  出  : PGD 池的数量 (1 个池可映射 4GB 空间, 推荐返回 1)
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ULONG  bspMmuPgdMaxNum (VOID)
{
    return  (1);
}
/*********************************************************************************************************
** 函数名称: bspMmuPmdMaxNum
** 功能描述: 获得 PMD 池的数量
** 输  入  : NONE
** 输  出  : PMD 池的数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_CPU_WORD_LENGHT == 64

ULONG   bspMmuPmdMaxNum(VOID)
{
    return  (256);
}
/*********************************************************************************************************
** 函数名称: bspMmuPtsMaxNum
** 功能描述: 获得 PTS 池的数量
** 输  入  : NONE
** 输  出  : PTS 池的数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ULONG   bspMmuPtsMaxNum(VOID)
{
    return  (32);
}
#endif
/*********************************************************************************************************
** 函数名称: bspMmuPgdMaxNum
** 功能描述: 获得 PTE 池的数量
** 输  入  : NONE
** 输  出  : PTE 池的数量 (映射 4GB 空间, 需要 4096 个 PTE 池)
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ULONG  bspMmuPteMaxNum (VOID)
{
#if LW_CFG_CPU_WORD_LENGHT == 32
    return  (1024);
#else
    return  (1024);
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 32*/
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  操作系统多核接口
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
/*********************************************************************************************************
  Secondary CPU 启动标志
*********************************************************************************************************/
#define __CPU_START_FLAG    0xaa55aa55bb44bb44

static ULONG  _G_ulCpuBootFlag[LW_CFG_MAX_PROCESSORS];

/*********************************************************************************************************
** 函数名称: bspSecondaryCpuWait
** 功能描述: Secondary CPU 等待初始化完成
** 输  入  : NONE
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID   bspSecondaryCpuWait (VOID)
{
    while (_G_ulCpuBootFlag[LW_CPU_GET_CUR_ID()] != __CPU_START_FLAG) { /*  等待 BSP 启动信号           */
        archWaitForInterrupt();
    }
    _G_ulCpuBootFlag[LW_CPU_GET_CUR_ID()] = 0;
}
/*********************************************************************************************************
** 函数名称: bspMpInt
** 功能描述: 产生一个核间中断
** 输  入  : ulCPUId      目标 CPU
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID   bspMpInt (ULONG  ulCPUId)
{
#if LW_CFG_RISCV_M_LEVEL == 0
    ULONG  ulHartIdMask = 1 << RISCV_CPUID_TO_HARTID(ulCPUId);

    sbi_send_ipi(&ulHartIdMask);
#else
    clintIpiSend(ulCPUId);
#endif
}
/*********************************************************************************************************
** 函数名称: bspCpuUp
** 功能描述: 启动一个 CPU
** 输  入  : ulCPUId      目标 CPU
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID   bspCpuUp (ULONG  ulCPUId)
{
    _G_ulCpuBootFlag[ulCPUId] = __CPU_START_FLAG;
    KN_SMP_WMB();

    archMpInt(ulCPUId);
}
/*********************************************************************************************************
** 函数名称: bspCpuDown
** 功能描述: 停止一个 CPU
** 输  入  : ulCPUId      目标 CPU
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_SMP_CPU_DOWN_EN > 0

VOID   bspCpuDown (ULONG  ulCPUId)
{
    /*
     * TODO: 加入你的处理代码, 如果没有, 请保留下面的调试信息
     */
    bspDebugMsg("bspCpuDown() error: this cpu CAN NOT support this operate!\r\n");
}

#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */
#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
  操作系统 CPU 速度控制接口
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: bspSuspend
** 功能描述: 系统进入休眠状态
** 输  入  : NONE
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID    bspSuspend (VOID)
{
    /*
     * TODO: 加入你的处理代码, 如果没有, 请保留下面的调试信息
     */
}
/*********************************************************************************************************
** 函数名称: bspCpuPowerSet
** 功能描述: CPU 设置运行速度
** 输  入  : uiPowerLevel     运行速度级别
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID    bspCpuPowerSet (UINT  uiPowerLevel)
{
    /*
     * TODO: 加入你的处理代码
     */
}
/*********************************************************************************************************
** 函数名称: bspCpuPowerGet
** 功能描述: CPU 获取运行速度
** 输  入  : puiPowerLevel    运行速度级别
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID    bspCpuPowerGet (UINT  *puiPowerLevel)
{
    /*
     * TODO: 加入你的处理代码, 如果没有, 请保留下面的代码
     */
    if (puiPowerLevel) {
        *puiPowerLevel = LW_CPU_POWERLEVEL_TOP;
    }
}
/*********************************************************************************************************
  操作系统时间相关函数
*********************************************************************************************************/
/*********************************************************************************************************
  TICK 服务相关配置
*********************************************************************************************************/
#define TICK_IN_THREAD  0
#if TICK_IN_THREAD > 0
static LW_HANDLE    htKernelTicks;                                      /*  操作系统时钟服务线程句柄    */
#endif                                                                  /*  TICK_IN_THREAD > 0          */
/*********************************************************************************************************
** 函数名称: __tickThread
** 功能描述: 初始化 tick 服务线程
** 输  入  : NONE
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if TICK_IN_THREAD > 0

static VOID  __tickThread (VOID)
{
    for (;;) {
        API_ThreadSuspend(htKernelTicks);
        API_KernelTicks();                                              /*  内核 TICKS 通知             */
        API_TimerHTicks();                                              /*  高速 TIMER TICKS 通知       */
    }
}

#endif                                                                  /*  TICK_IN_THREAD > 0          */
/*********************************************************************************************************
** 函数名称: __tickTimerIsr
** 功能描述: tick 定时器中断服务程序
** 输  入  : NONE
** 输  出  : 中断服务返回
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static irqreturn_t  __tickTimerIsr (VOID)
{
    volatile UINT64  *pulTime = (volatile UINT64 *)(KENDRYTE_CLINT_BASE + KENDRYTE_CLINT_TIME_OFFSET);
    volatile UINT64  *pulCmp  = (volatile UINT64 *)(KENDRYTE_CLINT_BASE + KENDRYTE_CLINT_TIMECMP_OFFSET);
	
    *pulCmp = (*pulTime + KENDRYTE_CLINT_TIMEBASE_FREQ / LW_TICK_HZ);
	
#if LW_CFG_RISCV_M_LEVEL > 0
    clear_csr(xip, MIP_MTIP);
	set_csr(xie, MIP_MTIP);
#else
	clear_csr(xip, MIP_STIP);
	set_csr(xie, MIP_STIP);
#endif

    API_KernelTicksContext();                                           /*  保存被时钟中断的线程控制块  */

#if TICK_IN_THREAD > 0
    API_ThreadResume(htKernelTicks);
#else
    API_KernelTicks();                                                  /*  内核 TICKS 通知             */
    API_TimerHTicks();                                                  /*  高速 TIMER TICKS 通知       */
#endif                                                                  /*  TICK_IN_THREAD > 0          */
    
    return  (LW_IRQ_HANDLED);
}
/*********************************************************************************************************
** 函数名称: bspTickInit
** 功能描述: 初始化 tick 时钟
** 输  入  : NONE
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  bspTickInit (VOID)
{
#if TICK_IN_THREAD > 0
    LW_CLASS_THREADATTR  threadattr;
#endif                                                                  /*  TICK_IN_THREAD > 0          */
    ULONG                ulVector = BSP_CFG_TICK_VECTOR;

#if TICK_IN_THREAD > 0
    API_ThreadAttrBuild(&threadattr, (8 * LW_CFG_KB_SIZE),
                        LW_PRIO_T_TICK,
                        LW_OPTION_THREAD_STK_CHK |
                        LW_OPTION_THREAD_UNSELECT |
                        LW_OPTION_OBJECT_GLOBAL |
                        LW_OPTION_THREAD_SAFE, LW_NULL);

    htKernelTicks = API_ThreadCreate("t_tick",
                                     (PTHREAD_START_ROUTINE)__tickThread,
                                     &threadattr,
                                     NULL);
#endif                                                                  /*  TICK_IN_THREAD > 0          */

    volatile UINT64  *pulTime = (volatile UINT64 *)(KENDRYTE_CLINT_BASE + KENDRYTE_CLINT_TIME_OFFSET);
    volatile UINT64  *pulCmp  = (volatile UINT64 *)(KENDRYTE_CLINT_BASE + KENDRYTE_CLINT_TIMECMP_OFFSET);
	
    *pulCmp = (*pulTime + KENDRYTE_CLINT_TIMEBASE_FREQ / LW_TICK_HZ);
	
#if LW_CFG_RISCV_M_LEVEL > 0
    clear_csr(xip, MIP_MTIP);
	set_csr(xie, MIP_MTIP);
#else
	clear_csr(xip, MIP_STIP);
	set_csr(xie, MIP_STIP);
#endif
	
    API_InterVectorConnect(ulVector,
                           (PINT_SVR_ROUTINE)__tickTimerIsr,
                           LW_NULL,
                           "tick_timer");

    API_InterVectorEnable(ulVector);
}
/*********************************************************************************************************
** 函数名称: bspTickHighResolution
** 功能描述: 修正从最近一次 tick 到当前的精确时间.
** 输　入  : ptv       需要修正的时间
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  bspTickHighResolution (struct timespec *ptv)
{
    /*
     * TODO: 修改为你的处理代码
     */
}
/*********************************************************************************************************
** 函数名称: bspDelayUs
** 功能描述: 延迟微秒
** 输  入  : ulUs     微秒数
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID bspDelayUs (ULONG ulUs)
{
    /*
     * TODO: 根据你的处理器性能, 修改为你的处理代码
     */
    volatile UINT  i;

    while (ulUs) {
        ulUs--;
        for (i = 0; i < 80; i++) {
        }
    }
}
/*********************************************************************************************************
** 函数名称: bspDelayNs
** 功能描述: 延迟纳秒
** 输  入  : ulNs     纳秒数
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  bspDelayNs (ULONG ulNs)
{
    /*
     * TODO: 根据你的处理器性能, 修改为你的处理代码
     */
    volatile UINT  i;

    while (ulNs) {
        ulNs = (ulNs < 100) ? (0) : (ulNs - 100);
        for (i = 0; i < 40; i++) {
        }
    }
}
/*********************************************************************************************************
  semihosting
*********************************************************************************************************/
#define import(__use_no_semihosting_swi)

void  _ttywrch (int  ch)
{
    bspDebugMsg("__use_no_semihosting_swi _ttywrch()!\n");
    while (1);
}

void  _sys_exit (int  return_code)
{
    bspDebugMsg("__use_no_semihosting_swi _sys_exit()!\n");
    while (1);
}
/*********************************************************************************************************
  GCC 需求
*********************************************************************************************************/
#ifdef __GNUC__
int  __aeabi_read_tp (void)
{
    return  (0);
}
#endif                                                                  /*  __GNUC__                    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
