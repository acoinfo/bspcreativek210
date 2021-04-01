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
** 文   件   名: k210_gpiohs.h
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 10 月 26 日
**
** 描        述: GPIOHS 控制器驱动头文件
*********************************************************************************************************/
#define __SYLIXOS_KERNEL
#include "config.h"
#include "SylixOS.h"
#include "driver/gpiohs/k210_gpiohs.h"
#include "driver/clock/k210_clock.h"

#include "KendryteWare/include/common.h"
#include "KendryteWare/include/gpio_common.h"
#include "KendryteWare/include/gpiohs.h"
#include "KendryteWare/include/plic.h"
#include "KendryteWare/include/fpioa.h"
/*********************************************************************************************************
  定义
*********************************************************************************************************/
#define __GPIOHS_TOTAL_NR            KENDRYTE_GPIOHS_NR                 /*  K210 GPIOHS 引脚的总数      */
#define __GPIOHS_CHIP_PINS           (32)                               /*  每个控制器管控的引脚个数    */
#define __BANK_INDEX_SHIFT           (5)                                /*  取代计算 gpio_num / 32      */
#define __BANK_OFFSET_MASK           (0x1f)                             /*  取代计算 gpio_num % 32      */
#define __GPIOHS_INT_RISE_OFFSET     (0)
#define __GPIOHS_INT_FALL_OFFSET     (1)
#define __GPIOHS_INT_LOW_OFFSET      (2)
#define __GPIOHS_INT_HIGH_OFFSET     (3)
/*********************************************************************************************************
  GPIO 控制器类型定义
*********************************************************************************************************/
typedef struct {
    addr_t              GPIOHS_ulPhyAddrBase;                            /*  物理地址基地址             */
    addr_t              GPIOHS_stPhyAddrSize;                            /*  物理地址空间大小           */

    ULONG               CPIOHS_ulIrqBase;                                /*  基中断向量号               */
    UINT                GPIOHS_uiPinNumber;                              /*  该控制器芯片控制的引脚数   */

    UINT                GPIOHS_uiEdgeRaise;                              /*  边沿触发中断模式           */
    UINT                GPIOHS_uiEdgeFall;                               /*  边沿触发中断模式           */
    UINT                GPIOHS_uiHighTrigInt;                            /*  边沿触发中断模式           */
    UINT                GPIOHS_uiLowTrigInt;                             /*  边沿触发中断模式           */
    UINT                GPIOHS_uiIrqPending;                             /*  当前中断触发模式           */
    spinlock_t          GPIOHS_slMutex;
} __K210_GPIOHS_CONTROLER,  *__PK210_GPIOHS_CONTROLER;
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static __K210_GPIOHS_CONTROLER  _G_k210GpiohsControlers = {
        .GPIOHS_uiPinNumber   = 32,
        .GPIOHS_ulPhyAddrBase = GPIOHS_BASE_ADDR,
        .CPIOHS_ulIrqBase     = KENDRYTE_PLIC_VECTOR(IRQN_GPIOHS0_INTERRUPT),
};
/*********************************************************************************************************
** 函数名称: __k210GpiohsSetupIrq
** 功能描述: 设置指定 GPIO 为外部中断输入管脚
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
**           bIsLevel    是否为电平触发
**           uiType      不支持电平触发
**                       如果为边沿触发, 0: 表示不触发任何中断，1: 表示
**                       下降沿触发, 2: 上升沿触发, 3: 表示双边沿触发。
** 输  出  : IRQ 向量号 -1:错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ULONG  __k210GpiohsSetupIrq (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset, BOOL  bIsLevel, UINT  uiType)
{
    UINT32                      rise, fall, irq  = 0, low = 0, high = 0;
    __PK210_GPIOHS_CONTROLER    pGpiohsControler = &_G_k210GpiohsControlers;
    volatile gpiohs_t          *gpiohs;

    gpiohs = (volatile gpiohs_t *)pGpiohsControler->GPIOHS_ulPhyAddrBase;

    if (uiOffset >= __GPIOHS_CHIP_PINS) {
        printk(KERN_ERR "__k210GpiohsGet(): invalid uiOffset\n");
        return  (-1);
    }

    API_SpinLock(&pGpiohsControler->GPIOHS_slMutex);

    if (bIsLevel) {                                                     /*        电平触发模式          */
        switch (uiType) {
        case GPIO_PV_LOW:
            low = 1; high = 0;
            break;
        case GPIO_PV_HIGH:
            low = 0; high = 1;
            break;
        default:
            printk(KERN_ERR "Invalid gpio level-triger\n");
            break;
        }
        set_gpio_bit(gpiohs->high_ie.u32, uiOffset, high);
        set_gpio_bit(gpiohs->low_ie.u32, uiOffset, low);

        pGpiohsControler->GPIOHS_uiHighTrigInt = high;
        pGpiohsControler->GPIOHS_uiLowTrigInt  = low;

    } else {                                                            /*        边沿触发模式          */
        switch (uiType) {
        case GPIO_PE_NONE:
            rise = fall = irq = 0;
            break;
        case GPIO_PE_FALLING:
            rise = 0;
            fall = irq = 1;
            break;
        case GPIO_PE_RISING:
            fall = 0;
            rise = irq = 1;
            break;
        case GPIO_PE_BOTH:
            rise = fall = irq = 1;
            break;
        default:
            printk(KERN_ERR "Invalid gpio edge\n");
            break;
        }

        set_gpio_bit(gpiohs->rise_ie.u32, uiOffset, rise);
        set_gpio_bit(gpiohs->fall_ie.u32, uiOffset, fall);

        pGpiohsControler->GPIOHS_uiEdgeRaise = rise;
        pGpiohsControler->GPIOHS_uiEdgeFall  = fall;
    }

    API_SpinUnlock(&pGpiohsControler->GPIOHS_slMutex);

    return  (pGpiohsControler->CPIOHS_ulIrqBase + uiOffset);
}
/*********************************************************************************************************
** 函数名称: __k210GpiohsClearIrq
** 功能描述: 清除指定 GPIO 中断标志
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __k210GpiohsClearIrq (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset)
{
    __PK210_GPIOHS_CONTROLER    pGpiohsControler = &_G_k210GpiohsControlers;
    volatile gpiohs_t          *gpiohs;

    gpiohs = (volatile gpiohs_t *)pGpiohsControler->GPIOHS_ulPhyAddrBase;

    if (uiOffset >= __GPIOHS_CHIP_PINS) {
        printk(KERN_ERR "__k210GpiohsGet(): invalid uiOffset\n");
        return ;
    }

    if (pGpiohsControler->GPIOHS_uiEdgeRaise)
    {
        set_gpio_bit(gpiohs->rise_ie.u32, uiOffset, 0);
        set_gpio_bit(gpiohs->rise_ip.u32, uiOffset, 1);
        set_gpio_bit(gpiohs->rise_ie.u32, uiOffset, 1);
    }

    if (pGpiohsControler->GPIOHS_uiEdgeFall)
    {
        set_gpio_bit(gpiohs->fall_ie.u32, uiOffset, 0);
        set_gpio_bit(gpiohs->fall_ip.u32, uiOffset, 1);
        set_gpio_bit(gpiohs->fall_ie.u32, uiOffset, 1);
    }

    if (pGpiohsControler->GPIOHS_uiHighTrigInt)
    {
        set_gpio_bit(gpiohs->high_ie.u32, uiOffset, 0);
        set_gpio_bit(gpiohs->high_ip.u32, uiOffset, 1);
        set_gpio_bit(gpiohs->high_ie.u32, uiOffset, 1);
    }

    if (pGpiohsControler->GPIOHS_uiLowTrigInt)
    {
        set_gpio_bit(gpiohs->low_ie.u32, uiOffset,  0);
        set_gpio_bit(gpiohs->low_ip.u32, uiOffset,  1);
        set_gpio_bit(gpiohs->low_ie.u32, uiOffset,  1);
    }

    API_SpinLock(&pGpiohsControler->GPIOHS_slMutex);
    pGpiohsControler->GPIOHS_uiIrqPending = 0;
    API_SpinUnlock(&pGpiohsControler->GPIOHS_slMutex);
}
/*********************************************************************************************************
** 函数名称: __k210GpiohsSvrIrq
** 功能描述: 判断 GPIO 中断标志
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
** 输  出  : 中断返回值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static irqreturn_t  __k210GpiohsSvrIrq (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset)
{
    UINT32                      rise = 0, fall   = 0, irq = 0, low = 0, high = 0;
    __PK210_GPIOHS_CONTROLER    pGpiohsControler = &_G_k210GpiohsControlers;
    volatile gpiohs_t          *gpiohs;

    gpiohs = (volatile gpiohs_t *)pGpiohsControler->GPIOHS_ulPhyAddrBase;

    if (uiOffset >= __GPIOHS_CHIP_PINS) {
        printk(KERN_ERR "__k210GpiohsGet(): invalid uiOffset\n");
        return  (LW_IRQ_NONE);
    }

    rise = get_gpio_bit(gpiohs->rise_ip.u32, uiOffset);
    fall = get_gpio_bit(gpiohs->fall_ip.u32, uiOffset);
    low  = get_gpio_bit(gpiohs->low_ip.u32, uiOffset);
    high = get_gpio_bit(gpiohs->high_ip.u32, uiOffset);

    irq = (rise << __GPIOHS_INT_RISE_OFFSET) | (fall << __GPIOHS_INT_FALL_OFFSET) |
          (low << __GPIOHS_INT_LOW_OFFSET)   | (high << __GPIOHS_INT_HIGH_OFFSET);

    API_SpinLock(&pGpiohsControler->GPIOHS_slMutex);
    pGpiohsControler->GPIOHS_uiIrqPending = irq;
    API_SpinUnlock(&pGpiohsControler->GPIOHS_slMutex);

    if (irq) {
        return  (LW_IRQ_HANDLED);
    } else {
        return  (LW_IRQ_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: __k210GpiohsPinPullUpSet
** 功能描述: 设置指定 GPIO 上拉电阻
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
**           iPullUpBit  是否使能上拉电阻; 0: No Pull, 1: Pull Down, 2: Pull Up
** 输  出  : 0: 正确 -1:错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __k210GpiohsPinPullUpSet (PLW_GPIO_CHIP  pGpioChip, UINT uiOffset, UINT iPullUpBit)
{
    __PK210_GPIOHS_CONTROLER    pGpiohsControler;
    INT                         iIoNumber;

    if (uiOffset >= __GPIOHS_CHIP_PINS) {
        printk(KERN_ERR "__k210GpiohsGet(): invalid uiOffset\n");
       return  (-1);
    }

    if ((iPullUpBit >= 0 && iPullUpBit < FPIOA_PULL_MAX) != LW_TRUE) {
        printk(KERN_ERR "__k210GpioPinPullUpSet(): invalid io_number.\n");
        return  (PX_ERROR);
    }

    pGpiohsControler = &_G_k210GpiohsControlers;

    (VOID)pGpiohsControler;

    iIoNumber = fpioa_get_io_by_function(FUNC_GPIOHS0 + uiOffset);
    if (iIoNumber <= 0) {
        printk(KERN_ERR "__k210GpiohsPinPullUpSet(): invalid io_number.\n");
        return  (PX_ERROR);
    }

    fpioa_set_io_pull(iIoNumber, iPullUpBit);

    return  (0);
}
/*********************************************************************************************************
** 函数名称: __k210GpiohsGetDirection
** 功能描述: 获得指定 GPIO 方向
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
** 输  出  : 0: 输入 1:输出 -1: 错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210GpiohsGetDirection (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset)
{
    __PK210_GPIOHS_CONTROLER    pGpiohsControler =   &_G_k210GpiohsControlers;
    volatile gpiohs_t          *gpiohs;

    gpiohs = (volatile gpiohs_t *)pGpiohsControler->GPIOHS_ulPhyAddrBase;

    if (uiOffset >= __GPIOHS_CHIP_PINS) {
        printk(KERN_ERR "__k210GpiohsGet(): invalid uiOffset\n");
       return  (-1);
    }

    volatile UINT32 *reg = gpiohs->input_en.u32;

    return  (get_gpio_bit(reg, uiOffset) ? 0 : 1);
}
/*********************************************************************************************************
** 函数名称: __k210GpiohsSetDirInput
** 功能描述: 设置指定 GPIO 为输入模式
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
** 输  出  : 0: 正确 -1:错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210GpiohsSetDirInput (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset)
{
    INT                         iIoNumber;
    __PK210_GPIOHS_CONTROLER    pGpiohsControler = &_G_k210GpiohsControlers;
    volatile gpiohs_t          *gpiohs;

    gpiohs = (volatile gpiohs_t *)pGpiohsControler->GPIOHS_ulPhyAddrBase;

    if (uiOffset >= __GPIOHS_CHIP_PINS) {
        printk(KERN_ERR "__k210GpiohsGet(): invalid uiOffset\n");
       return  (-1);
    }

    iIoNumber = fpioa_get_io_by_function(FUNC_GPIOHS0 + uiOffset);
    if (iIoNumber <= 0) {
        printk(KERN_ERR "__k210GpiohsSetDirInput(): invalid io_number.\n");
        return  (PX_ERROR);
    }

    fpioa_set_io_pull(iIoNumber, FPIOA_PULL_NONE);                      /*  PULL_NONE when input mode   */

    volatile UINT32 *reg   = gpiohs->input_en.u32;
    volatile UINT32 *reg_d = gpiohs->output_en.u32;

    set_gpio_bit(reg_d, uiOffset, 0);
    set_gpio_bit(reg, uiOffset, 1);

    return  (0);
}
/*********************************************************************************************************
** 函数名称: __k210GpiohsSetDirOutput
** 功能描述: 设置指定 GPIO 为输出模式
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
**           iValue      输出电平
** 输  出  : 0: 正确 -1:错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210GpiohsSetDirOutput (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset, INT  iValue)
{
    INT                         iIoNumber;
    __PK210_GPIOHS_CONTROLER    pGpiohsControler = &_G_k210GpiohsControlers;
    volatile gpiohs_t          *gpiohs;

    gpiohs = (volatile gpiohs_t *)pGpiohsControler->GPIOHS_ulPhyAddrBase;

    if (uiOffset >= __GPIOHS_CHIP_PINS) {
        printk(KERN_ERR "__k210GpiohsGet(): invalid uiOffset\n");
       return  (-1);
    }

    iIoNumber = fpioa_get_io_by_function(FUNC_GPIOHS0 + uiOffset);
    if (iIoNumber <= 0) {
        printk(KERN_ERR "__k210GpiohsSetDirOutput(): invalid io_number.\n");
        return  (PX_ERROR);
    }

    fpioa_set_io_pull(iIoNumber, FPIOA_PULL_DOWN);                      /*  PULL_DOWN when output mode  */

    volatile UINT32 *reg   = gpiohs->output_en.u32;
    volatile UINT32 *reg_d = gpiohs->input_en.u32;

    set_gpio_bit(reg_d, uiOffset, 0);
    set_gpio_bit(reg, uiOffset, 1);

    set_bit_idx(gpiohs->output_val.u32, uiOffset, iValue);              /*  set default output value    */

    return  (0);
}
/*********************************************************************************************************
** 函数名称: __k210GpiohsGetPinValue
** 功能描述: 获得指定 GPIO 电平
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
** 输  出  : 0: 低电平 1:高电平 -1:错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210GpiohsGetPinValue (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset)
{
    __PK210_GPIOHS_CONTROLER    pGpiohsControler = &_G_k210GpiohsControlers;
    volatile gpiohs_t          *gpiohs;

    gpiohs = (volatile gpiohs_t *)pGpiohsControler->GPIOHS_ulPhyAddrBase;

    if (uiOffset >= __GPIOHS_CHIP_PINS) {
        printk(KERN_ERR "__k210GpiohsGet(): invalid uiOffset\n");
       return  (-1);
    }

    return get_bit_idx(gpiohs->input_val.u32, uiOffset);
}
/*********************************************************************************************************
** 函数名称: __k210GpiohsSetPinValue
** 功能描述: 设置指定 GPIO 电平
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
**           iValue      输出电平
** 输  出  : 0: 正确 -1:错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __k210GpiohsSetPinValue (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset, INT  iValue)
{
    __PK210_GPIOHS_CONTROLER    pGpiohsControler = &_G_k210GpiohsControlers;
    volatile gpiohs_t          *gpiohs;

    gpiohs = (volatile gpiohs_t *)pGpiohsControler->GPIOHS_ulPhyAddrBase;

    if (uiOffset >= __GPIOHS_CHIP_PINS) {
        printk(KERN_ERR "__k210GpiohsGet(): invalid uiOffset\n");
       return ;
    }

    set_bit_idx(gpiohs->output_val.u32, uiOffset, iValue);
}
/*********************************************************************************************************
** 函数名称: __k210GpiohsFree
** 功能描述: 释放一个正在被使用的 GPIO, 如果当前是中断模式则, 放弃中断输入功能.
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __k210GpiohsFree (PLW_GPIO_CHIP  pGpioChip, UINT uiOffset)
{
    /*
     * TODO:  __k210GpiohsFree
     */
}
/*********************************************************************************************************
** 函数名称: __k210GpiohsRequest
** 功能描述: 请求一个 GPIO 如果驱动没有特殊操作, 例如电源管理等等, 则可以为 LW_NULL
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
** 输  出  : 0: 成功，-1: 错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210GpiohsRequest (PLW_GPIO_CHIP  pGpioChip, UINT uiOffset)
{
    /*
     * TODO:  __k210GpiohsRequest
     */

    return  (0);
}
/*********************************************************************************************************
  GPIO 驱动程序
*********************************************************************************************************/
static LW_GPIO_CHIP  _G_k210GpiohsChip = {
        .GC_pcLabel              = "Kendryte K210 GPIOHS",
        .GC_ulVerMagic           = LW_GPIO_VER_MAGIC,

        .GC_pfuncRequest         = __k210GpiohsRequest,
        .GC_pfuncFree            = __k210GpiohsFree,

        .GC_pfuncGet             = __k210GpiohsGetPinValue,
        .GC_pfuncSet             = __k210GpiohsSetPinValue,

        .GC_pfuncDirectionInput  = __k210GpiohsSetDirInput,
        .GC_pfuncDirectionOutput = __k210GpiohsSetDirOutput,
        .GC_pfuncGetDirection    = __k210GpiohsGetDirection,

        .GC_pfuncSetPull         = __k210GpiohsPinPullUpSet,

        .GC_pfuncSetupIrq        = __k210GpiohsSetupIrq,
        .GC_pfuncClearIrq        = __k210GpiohsClearIrq,
        .GC_pfuncSvrIrq          = __k210GpiohsSvrIrq,
};
/*********************************************************************************************************
** 函数名称: k210GpiohsDrvInit
** 功能描述: 初始化各个 GPIOHS pin 的上下文
** 输  入  : pGpiohsControler     GPIOHS 控制器
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static void k210GpiohsDrvInit (__PK210_GPIOHS_CONTROLER  pGpiohsControler)
{
    volatile gpiohs_t  *gpiohs = (volatile gpiohs_t *)pGpiohsControler->GPIOHS_ulPhyAddrBase;

    gpiohs->rise_ie.u32[0] = 0;
    gpiohs->rise_ip.u32[0] = 0xFFFFFFFF;
    gpiohs->fall_ie.u32[0] = 0;
    gpiohs->fall_ip.u32[0] = 0xFFFFFFFF;
}
/*********************************************************************************************************
** 函数名称: k210GpiohsDrv
** 功能描述: 安装 Kendryte K210 GPIOHS 驱动
** 输  入  : NONE
** 输  出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  k210GpiohsDrv (VOID)
{
    __PK210_GPIOHS_CONTROLER  pGpiohsControler;

    _G_k210GpiohsChip.GC_uiBase   = KENDRYTE_GPIO_NR;                   /* GPIOHS base after GPIO base  */
    _G_k210GpiohsChip.GC_uiNGpios = KENDRYTE_GPIOHS_NR;

    pGpiohsControler = &_G_k210GpiohsControlers;

    LW_SPIN_INIT(&pGpiohsControler->GPIOHS_slMutex);
    k210GpiohsDrvInit(pGpiohsControler);

    return  (API_GpioChipAdd(&_G_k210GpiohsChip));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
