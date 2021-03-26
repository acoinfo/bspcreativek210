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
** 文   件   名: k210_clint.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 10 月 18 日
**
** 描        述: CLIC 中断控制器驱动
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "config.h"
#include "k210_clint.h"
#include "driver/fix_arch_def.h"
/*********************************************************************************************************
 寄存器地址和偏移定义
*********************************************************************************************************/
#define CLINT_MSIP           (0x0000)                                   /* Reg msip address offset      */
#define CLINT_MSIP_SIZE      (0x4)
#define CLINT_MTIMECMP       (0x4000)                                   /* Reg mtimecmp address offset  */
#define CLINT_MTIMECMP_SIZE  (0x8)
#define CLINT_MTIME          (0xBFF8)                                   /* Reg mtime address offset     */
#define CLINT_MTIME_SIZE     (0x8)
#define CLINT_MAX_HARTS      (4095)                                     /* Max number of cores          */
#define CLINT_NUM_HARTS      (2)                                        /* Real number of cores         */
#define CLINT_CLOCK_DIV      (50)                                       /* Clock freq division factor   */
/*********************************************************************************************************
 * CLINT Object 定义
*********************************************************************************************************/
struct clint_msip_t {                                                   /* MSIP Registers               */
    uint32_t            msip : 1;                                       /* Bit 0 is msip                */
    uint32_t            zero : 31;                                      /* Bits [32:1] is 0             */
} __attribute__((packed, aligned(4)));

typedef uint64_t        clint_mtimecmp_t;                               /* MTIMECMP Registers           */

typedef uint64_t        clint_mtime_t;                                  /* Timer Registers              */

struct clint_t {                                                        /* Coreplex-Local INTerrupts    */
    struct clint_msip_t  msip[CLINT_MAX_HARTS];                         /* 0x0000 to 0x3FF8, Reg msip   */
    uint32_t             resv0;                                         /* Resverd space, do not use    */
    clint_mtimecmp_t     mtimecmp[CLINT_MAX_HARTS];                     /* 0x4000 to 0xBFF0, mtimecmp   */
    clint_mtime_t        mtime;                                         /* 0xBFF8, Time Register        */
} __attribute__((packed, aligned(4)));
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
volatile struct clint_t* const clint = (volatile struct clint_t*)CLINT_BASE_ADDR;
/*********************************************************************************************************
** 函数名称: clintMTimeCmpSet
** 功能描述: 设置 Machine Mode 下指定核的比较器mtimecmp中的值
** 输  入  : hartid       指定核的hartid
**           value        要设置的值
** 输  出  : ERROR_CODE   0: 成功, -1: hartid错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT clintMTimeCmpSet (UINT uiHartId, UINT uiValue)
{
    if (uiHartId >= CLINT_NUM_HARTS) {
        return -1;
    }

    KN_SMP_WMB();
    clint->mtimecmp[uiHartId] = uiValue;

    return 0;
}
/*********************************************************************************************************
** 函数名称: clintMTimeCmpGet
** 功能描述: 获得 Machine Mode 下指定核的比较器mtimecmp中的值
** 输  入  : hartid       指定核的hartid
** 输  出  : 指定核比较器mtimecmp中的值, 或错误码-1
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT clintMTimeCmpGet (UINT uiHartId)
{
    if (uiHartId >= CLINT_NUM_HARTS) {
        return -1;
    }

    return (clint->mtimecmp[uiHartId]);
}
/*********************************************************************************************************
** 函数名称: clintMTimeGet
** 功能描述: 获得 Machine Mode 下的计数器mtime的值
** 输  入  : NONE
** 输  出  : 计数器mtime的值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT clintMTimeGet (VOID)
{
    return (clint->mtime);
}
/*********************************************************************************************************
** 函数名称: clintIpiInit
** 功能描述: 初始化 Machine Mode 下的软件中断使能位
** 输  入  : NONE
** 输  出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT clintIpiInit (VOID)
{
    /* Clear the Machine-Software bit in MIE */
    clear_csr(mie, MIP_MSIP);

    return 0;
}
/*********************************************************************************************************
** 函数名称: clintIpiEnable
** 功能描述: 使能 Machine Mode 下的软件中断
** 输  入  : NONE
** 输  出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT clintIpiEnable (VOID)
{
    /* Enable interrupts in general */
    set_csr(mstatus, MSTATUS_MIE);

    /* Set the Machine-Software bit in MIE */
    set_csr(mie, MIP_MSIP);

    return 0;
}
/*********************************************************************************************************
** 函数名称: clintIpiDisable
** 功能描述: 禁止 Machine Mode 下的软件中断
** 输  入  : NONE
** 输  出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT clintIpiDisable (VOID)
{
    /* Clear the Machine-Software bit in MIE */
    clear_csr(mie, MIP_MSIP);

    return 0;
}
/*********************************************************************************************************
** 函数名称: clintIpiSend
** 功能描述: Machine Mode 下向指定核发送核间中断
** 输  入  : core_id  核的hartid
** 输  出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT clintIpiSend (UINT uiCoreId)
{
    if (uiCoreId >= CLINT_NUM_HARTS) {
        return -1;
    }

    KN_SMP_WMB();
    clint->msip[uiCoreId].msip = 1;

    return 0;
}
/*********************************************************************************************************
** 函数名称: clintIpiClear
** 功能描述: 清除 Machine Mode 下指定核的软件中断
** 输  入  : core_id        指定核的hartid
** 输  出  : ERROR_CODE     1: 成功, 0: 没有中断, -1: core_id 错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT clintIpiClear(UINT uiCoreId)
{
    if (uiCoreId >= CLINT_NUM_HARTS) {
        return -1;
    }

    if (clint->msip[uiCoreId].msip) {
        KN_SMP_WMB();
        clint->msip[uiCoreId].msip = 0;
        return 1;
    }

    return 0;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
