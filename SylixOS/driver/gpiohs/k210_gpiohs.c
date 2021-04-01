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
** ��   ��   ��: k210_gpiohs.h
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 10 �� 26 ��
**
** ��        ��: GPIOHS ����������ͷ�ļ�
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
  ����
*********************************************************************************************************/
#define __GPIOHS_TOTAL_NR            KENDRYTE_GPIOHS_NR                 /*  K210 GPIOHS ���ŵ�����      */
#define __GPIOHS_CHIP_PINS           (32)                               /*  ÿ���������ܿص����Ÿ���    */
#define __BANK_INDEX_SHIFT           (5)                                /*  ȡ������ gpio_num / 32      */
#define __BANK_OFFSET_MASK           (0x1f)                             /*  ȡ������ gpio_num % 32      */
#define __GPIOHS_INT_RISE_OFFSET     (0)
#define __GPIOHS_INT_FALL_OFFSET     (1)
#define __GPIOHS_INT_LOW_OFFSET      (2)
#define __GPIOHS_INT_HIGH_OFFSET     (3)
/*********************************************************************************************************
  GPIO ���������Ͷ���
*********************************************************************************************************/
typedef struct {
    addr_t              GPIOHS_ulPhyAddrBase;                            /*  �����ַ����ַ             */
    addr_t              GPIOHS_stPhyAddrSize;                            /*  �����ַ�ռ��С           */

    ULONG               CPIOHS_ulIrqBase;                                /*  ���ж�������               */
    UINT                GPIOHS_uiPinNumber;                              /*  �ÿ�����оƬ���Ƶ�������   */

    UINT                GPIOHS_uiEdgeRaise;                              /*  ���ش����ж�ģʽ           */
    UINT                GPIOHS_uiEdgeFall;                               /*  ���ش����ж�ģʽ           */
    UINT                GPIOHS_uiHighTrigInt;                            /*  ���ش����ж�ģʽ           */
    UINT                GPIOHS_uiLowTrigInt;                             /*  ���ش����ж�ģʽ           */
    UINT                GPIOHS_uiIrqPending;                             /*  ��ǰ�жϴ���ģʽ           */
    spinlock_t          GPIOHS_slMutex;
} __K210_GPIOHS_CONTROLER,  *__PK210_GPIOHS_CONTROLER;
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static __K210_GPIOHS_CONTROLER  _G_k210GpiohsControlers = {
        .GPIOHS_uiPinNumber   = 32,
        .GPIOHS_ulPhyAddrBase = GPIOHS_BASE_ADDR,
        .CPIOHS_ulIrqBase     = KENDRYTE_PLIC_VECTOR(IRQN_GPIOHS0_INTERRUPT),
};
/*********************************************************************************************************
** ��������: __k210GpiohsSetupIrq
** ��������: ����ָ�� GPIO Ϊ�ⲿ�ж�����ܽ�
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
**           bIsLevel    �Ƿ�Ϊ��ƽ����
**           uiType      ��֧�ֵ�ƽ����
**                       ���Ϊ���ش���, 0: ��ʾ�������κ��жϣ�1: ��ʾ
**                       �½��ش���, 2: �����ش���, 3: ��ʾ˫���ش�����
** ��  ��  : IRQ ������ -1:����
** ȫ�ֱ���:
** ����ģ��:
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

    if (bIsLevel) {                                                     /*        ��ƽ����ģʽ          */
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

    } else {                                                            /*        ���ش���ģʽ          */
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
** ��������: __k210GpiohsClearIrq
** ��������: ���ָ�� GPIO �жϱ�־
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210GpiohsSvrIrq
** ��������: �ж� GPIO �жϱ�־
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
** ��  ��  : �жϷ���ֵ
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210GpiohsPinPullUpSet
** ��������: ����ָ�� GPIO ��������
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
**           iPullUpBit  �Ƿ�ʹ����������; 0: No Pull, 1: Pull Down, 2: Pull Up
** ��  ��  : 0: ��ȷ -1:����
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210GpiohsGetDirection
** ��������: ���ָ�� GPIO ����
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
** ��  ��  : 0: ���� 1:��� -1: ����
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210GpiohsSetDirInput
** ��������: ����ָ�� GPIO Ϊ����ģʽ
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
** ��  ��  : 0: ��ȷ -1:����
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210GpiohsSetDirOutput
** ��������: ����ָ�� GPIO Ϊ���ģʽ
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
**           iValue      �����ƽ
** ��  ��  : 0: ��ȷ -1:����
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210GpiohsGetPinValue
** ��������: ���ָ�� GPIO ��ƽ
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
** ��  ��  : 0: �͵�ƽ 1:�ߵ�ƽ -1:����
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210GpiohsSetPinValue
** ��������: ����ָ�� GPIO ��ƽ
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
**           iValue      �����ƽ
** ��  ��  : 0: ��ȷ -1:����
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210GpiohsFree
** ��������: �ͷ�һ�����ڱ�ʹ�õ� GPIO, �����ǰ���ж�ģʽ��, �����ж����빦��.
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __k210GpiohsFree (PLW_GPIO_CHIP  pGpioChip, UINT uiOffset)
{
    /*
     * TODO:  __k210GpiohsFree
     */
}
/*********************************************************************************************************
** ��������: __k210GpiohsRequest
** ��������: ����һ�� GPIO �������û���������, �����Դ����ȵ�, �����Ϊ LW_NULL
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
** ��  ��  : 0: �ɹ���-1: ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210GpiohsRequest (PLW_GPIO_CHIP  pGpioChip, UINT uiOffset)
{
    /*
     * TODO:  __k210GpiohsRequest
     */

    return  (0);
}
/*********************************************************************************************************
  GPIO ��������
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
** ��������: k210GpiohsDrvInit
** ��������: ��ʼ������ GPIOHS pin ��������
** ��  ��  : pGpiohsControler     GPIOHS ������
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: k210GpiohsDrv
** ��������: ��װ Kendryte K210 GPIOHS ����
** ��  ��  : NONE
** ��  ��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
