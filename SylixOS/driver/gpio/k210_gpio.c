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
** 文   件   名: k210_gpio.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 10 月 22 日
**
** 描        述: GPIO 控制器驱动头文件
*********************************************************************************************************/
#define __SYLIXOS_KERNEL
#include "config.h"
#include "SylixOS.h"
#include "driver/clock/clock.h"
#include "driver/gpio/k210_gpio.h"

#include "KendryteWare/include/common.h"
#include "KendryteWare/include/fpioa.h"
#include "KendryteWare/include/gpio.h"
#include "KendryteWare/include/plic.h"
/*********************************************************************************************************
  定义
*********************************************************************************************************/
#define __GPIO_TOTAL_NR             KENDRYTE_GPIO_NR                    /*  K210 GPIO 引脚的总数        */
#define __GPIO_CONTROLER_NR         (1)                                 /*  GPIO 控制器芯片的个数       */
#define __GPIO_EACH_CHIP_PINS       (8)                                 /*  每个控制器管控的引脚个数    */
#define __GPIO_BANK_INDEX_SHIFT     (3)                                 /*  取代计算 gpio_num / 8       */
#define __GPIO_OFFSET_MASK          (0x7)                               /*  取代计算 gpio_num % 8       */
/*********************************************************************************************************
  GPIO 控制器类型定义
*********************************************************************************************************/
typedef struct {
    addr_t     GPIOC_ulPhyAddrBase;                                     /*  物理地址基地址              */
    addr_t     GPIOC_stPhyAddrSize;                                     /*  物理地址空间大小            */
    ULONG      CPIOC_ulIntVector;                                       /*  中断向量号                  */
    UINT       GPIOC_uiPinNumber;                                       /*  该控制器芯片控制的引脚数    */
} __K210_GPIO_CONTROLER,  *__PK210_GPIO_CONTROLER;
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static __K210_GPIO_CONTROLER  _G_k210GpioControlers[__GPIO_CONTROLER_NR] = {
    {
        .GPIOC_uiPinNumber   = 8,
        .GPIOC_ulPhyAddrBase = GPIO_BASE_ADDR,
        .CPIOC_ulIntVector   = KENDRYTE_PLIC_VECTOR(IRQN_APB_GPIO_INTERRUPT),
    },
};
/*********************************************************************************************************
** 函数名称: __k210GpioGetDirection
** 功能描述: 获得指定 GPIO 方向
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
** 输  出  : 0: 输入 1:输出 -1: 错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210GpioGetDirection (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset)
{
    __PK210_GPIO_CONTROLER    pGpioControler;
    UINT                      uiPinNumber;
    UINT                      uiPinGrpIndex;

    pGpioControler  =   &_G_k210GpioControlers[uiOffset >> __GPIO_BANK_INDEX_SHIFT];
    uiPinNumber     =   pGpioControler->GPIOC_uiPinNumber;
    uiPinGrpIndex   =   (uiOffset & __GPIO_OFFSET_MASK);

    if (uiPinGrpIndex >= uiPinNumber) {
        printk(KERN_ERR "__k210GpioGetDirection(): invalid uiOffset\n");
        return  (-1);
    }

    return  (get_gpio_bit(gpio->direction.u32, uiPinGrpIndex));
}
/*********************************************************************************************************
** 函数名称: __k210GpioDirectionInput
** 功能描述: 设置指定 GPIO 为输入模式
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
** 输  出  : 0: 正确 -1:错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210GpioDirectionInput (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset)
{
    __PK210_GPIO_CONTROLER    pGpioControler;
    UINT                      uiPinNumber;
    UINT                      uiPinGrpIndex;

    pGpioControler = &_G_k210GpioControlers[uiOffset >> __GPIO_BANK_INDEX_SHIFT];
    uiPinNumber    = pGpioControler->GPIOC_uiPinNumber;
    uiPinGrpIndex  = (uiOffset & __GPIO_OFFSET_MASK);

    if (uiPinGrpIndex >= uiPinNumber) {
        printk(KERN_ERR "__k210GpioDirectionInput(): invalid uiOffset\n");
        return  (-1);
    }

    set_gpio_bit(gpio->direction.u32, uiPinGrpIndex, 0);

    return  (0);
}
/*********************************************************************************************************
** 函数名称: __k210GpioDirectionOutput
** 功能描述: 设置指定 GPIO 为输出模式
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
**           iValue      输出电平
** 输  出  : 0: 正确 -1:错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210GpioDirectionOutput (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset, INT  iValue)
{
    __PK210_GPIO_CONTROLER    pGpioControler;
    UINT                      uiPinNumber;
    UINT                      uiPinGrpIndex;

    pGpioControler = &_G_k210GpioControlers[uiOffset >> __GPIO_BANK_INDEX_SHIFT];
    uiPinNumber    = pGpioControler->GPIOC_uiPinNumber;
    uiPinGrpIndex  = (uiOffset & __GPIO_OFFSET_MASK);

    if (uiPinGrpIndex >= uiPinNumber) {
        printk(KERN_ERR "__k210GpioDirectionOutput(): invalid uiOffset\n");
        return  (-1);
    }

    set_gpio_bit(gpio->direction.u32, uiPinGrpIndex, 1);
    gpio_set_pin(uiPinGrpIndex, iValue);

    return  (0);
}
/*********************************************************************************************************
** 函数名称: __k210GpioGet
** 功能描述: 获得指定 GPIO 电平
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
** 输  出  : 0: 低电平 1:高电平 -1:错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210GpioGet (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset)
{
    __PK210_GPIO_CONTROLER    pGpioControler;
    UINT                      uiPinNumber;
    UINT                      uiPinGrpIndex;

    pGpioControler = &_G_k210GpioControlers[uiOffset >> __GPIO_BANK_INDEX_SHIFT];
    uiPinNumber    = pGpioControler->GPIOC_uiPinNumber;
    uiPinGrpIndex  = (uiOffset & __GPIO_OFFSET_MASK);

    if (uiPinGrpIndex >= uiPinNumber) {
        printk(KERN_ERR "__k210GpioGet(): invalid uiOffset\n");
        return  (-1);
    }

    return  (gpio_get_pin(uiPinGrpIndex));
}
/*********************************************************************************************************
** 函数名称: __k210GpioSet
** 功能描述: 设置指定 GPIO 电平
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
**           iValue      输出电平
** 输  出  : 0: 正确 -1:错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __k210GpioSet (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset, INT  iValue)
{
    __PK210_GPIO_CONTROLER    pGpioControler;
    UINT                      uiPinNumber;
    UINT                      uiPinGrpIndex;

    pGpioControler = &_G_k210GpioControlers[uiOffset >> __GPIO_BANK_INDEX_SHIFT];
    uiPinNumber    = pGpioControler->GPIOC_uiPinNumber;
    uiPinGrpIndex  = (uiOffset & __GPIO_OFFSET_MASK);

    if (uiPinGrpIndex >= uiPinNumber) {
        printk(KERN_ERR "__k210GpioSet(): invalid uiOffset\n");
        return ;
    }

    gpio_set_pin(uiPinGrpIndex, iValue);
}
/*********************************************************************************************************
** 函数名称: __k210GpioPinPullUpSet
** 功能描述: 设置指定 GPIO 上拉电阻
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
**           iPullUpBit  是否使能上拉电阻; 0: No Pull, 1: Pull Down, 2: Pull Up
** 输  出  : 0: 正确 -1:错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __k210GpioPinPullUpSet (PLW_GPIO_CHIP  pGpioChip, UINT uiOffset, UINT iPullUpBit)
{
    __PK210_GPIO_CONTROLER    pGpioControler;
    UINT                      uiPinNumber;
    UINT                      uiPinGrpIndex;
    INT                       iIoNumber;

    pGpioControler = &_G_k210GpioControlers[uiOffset >> __GPIO_BANK_INDEX_SHIFT];
    uiPinNumber    = pGpioControler->GPIOC_uiPinNumber;
    uiPinGrpIndex  = (uiOffset & __GPIO_OFFSET_MASK);

    if (uiPinGrpIndex >= uiPinNumber) {
        printk(KERN_ERR "__k210GpioSet(): invalid uiOffset\n");
        return  (PX_ERROR);
    }

    if ((iPullUpBit >= 0 && iPullUpBit < FPIOA_PULL_MAX) != LW_TRUE) {
        printk(KERN_ERR "__k210GpioPinPullUpSet(): invalid io_number.\n");
        return  (PX_ERROR);
    }

    iIoNumber = fpioa_get_io_by_function(FUNC_GPIO0 + uiPinGrpIndex);
    if (iIoNumber <= 0) {
        printk(KERN_ERR "__k210GpioPinPullUpSet(): invalid io_number.\n");
        return  (PX_ERROR);
    }

    fpioa_set_io_pull(iIoNumber, iPullUpBit);

    return  (0);
}
/*********************************************************************************************************
** 函数名称: __k210GpioFree
** 功能描述: 释放一个正在被使用的 GPIO, 如果当前是中断模式则, 放弃中断输入功能.
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __k210GpioFree (PLW_GPIO_CHIP  pGpioChip, UINT uiOffset)
{
    /*
     * TODO:  __k210GpioFree
     */
}
/*********************************************************************************************************
** 函数名称: __k210GpioRequest
** 功能描述: 请求一个 GPIO 如果驱动没有特殊操作, 例如电源管理等等, 则可以为 LW_NULL
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
** 输  出  : 0: 成功，-1: 错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210GpioRequest (PLW_GPIO_CHIP  pGpioChip, UINT uiOffset)
{
    /*
     * TODO:  __k210GpioRequest
     */

    return  (0);
}
/*********************************************************************************************************
  GPIO 驱动程序
*********************************************************************************************************/
static LW_GPIO_CHIP  _G_k210GpioChip = {
        .GC_pcLabel              = "Kendryte K210 GPIO",
        .GC_ulVerMagic           = LW_GPIO_VER_MAGIC,

        .GC_pfuncRequest         = __k210GpioRequest,
        .GC_pfuncFree            = __k210GpioFree,

        .GC_pfuncGetDirection    = __k210GpioGetDirection,
        .GC_pfuncDirectionInput  = __k210GpioDirectionInput,
        .GC_pfuncDirectionOutput = __k210GpioDirectionOutput,

        .GC_pfuncGet             = __k210GpioGet,
        .GC_pfuncSet             = __k210GpioSet,

        .GC_pfuncSetPull         = __k210GpioPinPullUpSet,
};
/*********************************************************************************************************
** 函数名称: k210GpioDrv
** 功能描述: 安装 Kendryte K210 GPIO 驱动
** 输  入  : NONE
** 输  出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  k210GpioDrv (VOID)
{
    __PK210_GPIO_CONTROLER  pGpioControler;
    INT                     i;

    _G_k210GpioChip.GC_uiBase   = 0;                                    /* GPIO base id start form zero */
    _G_k210GpioChip.GC_uiNGpios = 0;

    for (i = 0; i < __GPIO_CONTROLER_NR; i++) {
        pGpioControler = &_G_k210GpioControlers[i];
        _G_k210GpioChip.GC_uiNGpios += pGpioControler->GPIOC_uiPinNumber;
    }

    /*
     * GPIO clock under APB0 clock, so enable APB0 clock firstly
     */
    sysctlClockEnable(SYSCTL_CLOCK_APB0);                               /* Refer to sysctl_clock_enable */
    sysctlClockEnable(SYSCTL_CLOCK_APB1);
    sysctlClockEnable(SYSCTL_CLOCK_GPIO);

    return  (API_GpioChipAdd(&_G_k210GpioChip));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
