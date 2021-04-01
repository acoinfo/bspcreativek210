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
** ��   ��   ��: k210_gpio.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 10 �� 22 ��
**
** ��        ��: GPIO ����������ͷ�ļ�
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
  ����
*********************************************************************************************************/
#define __GPIO_TOTAL_NR             KENDRYTE_GPIO_NR                    /*  K210 GPIO ���ŵ�����        */
#define __GPIO_CONTROLER_NR         (1)                                 /*  GPIO ������оƬ�ĸ���       */
#define __GPIO_EACH_CHIP_PINS       (8)                                 /*  ÿ���������ܿص����Ÿ���    */
#define __GPIO_BANK_INDEX_SHIFT     (3)                                 /*  ȡ������ gpio_num / 8       */
#define __GPIO_OFFSET_MASK          (0x7)                               /*  ȡ������ gpio_num % 8       */
/*********************************************************************************************************
  GPIO ���������Ͷ���
*********************************************************************************************************/
typedef struct {
    addr_t     GPIOC_ulPhyAddrBase;                                     /*  �����ַ����ַ              */
    addr_t     GPIOC_stPhyAddrSize;                                     /*  �����ַ�ռ��С            */
    ULONG      CPIOC_ulIntVector;                                       /*  �ж�������                  */
    UINT       GPIOC_uiPinNumber;                                       /*  �ÿ�����оƬ���Ƶ�������    */
} __K210_GPIO_CONTROLER,  *__PK210_GPIO_CONTROLER;
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static __K210_GPIO_CONTROLER  _G_k210GpioControlers[__GPIO_CONTROLER_NR] = {
    {
        .GPIOC_uiPinNumber   = 8,
        .GPIOC_ulPhyAddrBase = GPIO_BASE_ADDR,
        .CPIOC_ulIntVector   = KENDRYTE_PLIC_VECTOR(IRQN_APB_GPIO_INTERRUPT),
    },
};
/*********************************************************************************************************
** ��������: __k210GpioGetDirection
** ��������: ���ָ�� GPIO ����
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
** ��  ��  : 0: ���� 1:��� -1: ����
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210GpioDirectionInput
** ��������: ����ָ�� GPIO Ϊ����ģʽ
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
** ��  ��  : 0: ��ȷ -1:����
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210GpioDirectionOutput
** ��������: ����ָ�� GPIO Ϊ���ģʽ
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
**           iValue      �����ƽ
** ��  ��  : 0: ��ȷ -1:����
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210GpioGet
** ��������: ���ָ�� GPIO ��ƽ
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
** ��  ��  : 0: �͵�ƽ 1:�ߵ�ƽ -1:����
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210GpioSet
** ��������: ����ָ�� GPIO ��ƽ
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
**           iValue      �����ƽ
** ��  ��  : 0: ��ȷ -1:����
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210GpioPinPullUpSet
** ��������: ����ָ�� GPIO ��������
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
**           iPullUpBit  �Ƿ�ʹ����������; 0: No Pull, 1: Pull Down, 2: Pull Up
** ��  ��  : 0: ��ȷ -1:����
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210GpioFree
** ��������: �ͷ�һ�����ڱ�ʹ�õ� GPIO, �����ǰ���ж�ģʽ��, �����ж����빦��.
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __k210GpioFree (PLW_GPIO_CHIP  pGpioChip, UINT uiOffset)
{
    /*
     * TODO:  __k210GpioFree
     */
}
/*********************************************************************************************************
** ��������: __k210GpioRequest
** ��������: ����һ�� GPIO �������û���������, �����Դ����ȵ�, �����Ϊ LW_NULL
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
** ��  ��  : 0: �ɹ���-1: ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210GpioRequest (PLW_GPIO_CHIP  pGpioChip, UINT uiOffset)
{
    /*
     * TODO:  __k210GpioRequest
     */

    return  (0);
}
/*********************************************************************************************************
  GPIO ��������
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
** ��������: k210GpioDrv
** ��������: ��װ Kendryte K210 GPIO ����
** ��  ��  : NONE
** ��  ��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
