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
**--------------文件信息----------------------------------------------------------------------------------
**
** 文   件   名: k210_uart.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 11 月 12 日
**
** 描        述: Kendryte K210  串口 SIO 驱动.
**
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "SylixOS.h"
#include "config.h"
#include "driver/clock/k210_clock.h"

#include "KendryteWare/include/plic.h"
#include "KendryteWare/include/uart.h"
/*********************************************************************************************************
  UARTHS 寄存器偏移定义
*********************************************************************************************************/
#define UART_REG_RBR                    (0x00)
#define UART_REG_DLL                    (0x00)
#define UART_REG_THR                    (0x00)

#define UART_REG_DLH                    (0x04)
#define UART_REG_IER                    (0x04)

#define UART_REG_FCR                    (0x08)
#define UART_REG_IIR                    (0x08)

#define UART_REG_LCR                    (0x0C)
#define UART_REG_MCR                    (0x10)
#define UART_REG_LSR                    (0x14)
#define UART_REG_MSR                    (0x18)
#define UART_REG_SCR                    (0x1C)
#define UART_REG_LPDLL                  (0x20)
#define UART_REG_LPDLH                  (0x24)

#define UART_REG_FAR                    (0x70)
#define UART_REG_TFR                    (0x74)
#define UART_REG_RFW                    (0x78)
#define UART_REG_USR                    (0x7C)
#define UART_REG_TFL                    (0x80)
#define UART_REG_RFL                    (0x84)
#define UART_REG_SRR                    (0x88)
#define UART_REG_SRTS                   (0x8C)
#define UART_REG_SBCR                   (0x90)
#define UART_REG_SDMAM                  (0x94)
#define UART_REG_SFE                    (0x98)
#define UART_REG_SRT                    (0x9C)

#define UART_REG_STET                   (0xA0)
#define UART_REG_HTX                    (0xA4)
#define UART_REG_DMASA                  (0xA8)
#define UART_REG_TCR                    (0xAC)

#define UART_REG_DE_EN                  (0xB0)
#define UART_REG_RE_EN                  (0xB4)
#define UART_REG_DET                    (0xB8)
#define UART_REG_TAT                    (0xBC)

#define UART_REG_DLF                    (0xC0)
#define UART_REG_RAR                    (0xC4)
#define UART_REG_TAR                    (0xC8)
#define UART_REG_LCR_EXT                (0xCC)

#define UART_REG_CPR                    (0xf4)
#define UART_REG_UCV                    (0xf8)
#define UART_REG_CTR                    (0xfC)

#define UART_TXFIFO_EMPTY_INT_EN        (0x1 << 1)                      /*  发送寄存器空中断            */
#define UART_RXFIFO_RDY_INT_EN          (0x1 << 0)                      /*  接收数据可用中断            */

#define UART_IIR_RDA                    0x04                            /*  IP: received data available */
#define UART_IIR_THRE                   0x02                            /*  IP: trans holding reg empty */

#define UART_TXFIFO_EMPTY               (0x1 << 6)
#define UART_RXFIFO_DR                  (0x1 << 0)

#define UART_TXFIFO_DEEPTH              (8)

#define UART_CFG_DATABITS_EN            0                               /*  UART Hw configuration       */
#define UART_CFG_STOPBITS_EN            1
#define UART_CFG_PARITY_EN              0

#define UART_FRAME_WORD_LENGTH_8        8
#define UART_FRAME_WORD_LENGTH_7        7

#define UART_FRAME_NUM_STB_2            2
#define UART_FRAME_NUM_STB_1            1

#define UART_LCR_PARITY_EN              (0x00000008u)
#define UART_LCR_PARITY_EN_SHIFT        (0x00000003u)
#define UART_LCR_PARITY_EN_DISABLE      (0x0u)
#define UART_LCR_PARITY_EN_ENABLE       (0x1u)

#define UART_LCR_PARITY_TYPE1           (0x00000010u)
#define UART_LCR_PARITY_TYPE1_SHIFT     (0x00000004u)
#define UART_LCR_PARITY_TYPE1_EVEN      (0x1u)
#define UART_LCR_PARITY_TYPE1_ODD       (0x0u)

#define UART_LCR_PARITY_TYPE2           (0x00000020u)
#define UART_LCR_PARITY_TYPE2_SHIFT     (0x00000005u)
#define UART_LCR_PARITY_TYPE2_FORCE     (0x1u)
#define UART_LCR_PARITY_TYPE2_NORMAL    (0x0u)

#define __UART_ODD_PARITY               ((UART_LCR_PARITY_TYPE1_ODD <<   \
                                         UART_LCR_PARITY_TYPE1_SHIFT) |  \
                                         UART_LCR_PARITY_EN)

#define __UART_EVEN_PARITY              ((UART_LCR_PARITY_TYPE1_EVEN <<  \
                                         UART_LCR_PARITY_TYPE1_SHIFT) |  \
                                         UART_LCR_PARITY_EN)

#define __UART_PARITY_NONE              (UART_LCR_PARITY_EN_DISABLE <<   \
                                         UART_LCR_PARITY_EN_SHIFT)
/*********************************************************************************************************
  UART 硬件参数定义
*********************************************************************************************************/
#define __UART_NR                  (3)                                  /*  通道数                      */
#define __UART_MODULE_INPUT_CLK    (500000000u)                         /*  UART 模块输入时钟           */

#define __UART_DEFAULT_BAUD        (SIO_BAUD_115200)                    /*  缺省波特率                  */
#define __UART_DEFAULT_OPT         (CREAD | CS8)                        /*  缺省选项                    */
#define __UART_BAUD_RATE_CONST     (16)

#define UART_TRANS_GAP()           do {                                \
                                       for(int i = 0; i < 5000; i++);  \
                                   } while(0)                           /*  非正常处理，临时补丁        */
/*********************************************************************************************************
  UART SIO 通道控制块类型定义
*********************************************************************************************************/
typedef struct {
    SIO_CHAN            SIOCHAN_sioChan;
    INT               (*SIOCHAN_pcbGetTxChar)();                        /*  中断回调函数                */
    INT               (*SIOCHAN_pcbPutRcvChar)();
    PVOID               SIOCHAN_pvGetTxArg;                             /*  回调函数参数                */
    PVOID               SIOCHAN_pvPutRcvArg;
    INT                 SIOCHAN_iBaud;                                  /*  波特率                      */
    INT                 SIOCHAN_iHwOption;                              /*  硬件选项                    */
    addr_t              SIOCHAN_ulBase;
    ULONG               SIOCHAN_ulVector;
    UINT                SIOCHAN_uiChannel;
    LW_SPINLOCK_DEFINE  (slock);
} __KENDRYTE_UART_SIOCHAN;
typedef __KENDRYTE_UART_SIOCHAN     *__PKENDRYTE_UART_SIOCHAN;          /*  指针类型                    */
/*********************************************************************************************************
  UART SIO 通道控制块数组定义
*********************************************************************************************************/
static __KENDRYTE_UART_SIOCHAN  _G_kendryteUartChannels[__UART_NR] = {
        {
                .SIOCHAN_uiChannel = 0,
                .SIOCHAN_ulBase    = UART1_BASE_ADDR,                   /*  UART1                       */
                .SIOCHAN_ulVector  = KENDRYTE_PLIC_VECTOR(IRQN_UART1_INTERRUPT),
                .SIOCHAN_iBaud     = 115200,
        },
        {
                .SIOCHAN_uiChannel = 1,
                .SIOCHAN_ulBase    = UART2_BASE_ADDR,                   /*  UART2                       */
                .SIOCHAN_ulVector  = KENDRYTE_PLIC_VECTOR(IRQN_UART2_INTERRUPT),
                .SIOCHAN_iBaud     = 115200,
        },
        {
                .SIOCHAN_uiChannel = 2,
                .SIOCHAN_ulBase    = UART3_BASE_ADDR,                   /*  UART3                       */
                .SIOCHAN_ulVector  = KENDRYTE_PLIC_VECTOR(IRQN_UART3_INTERRUPT),
                .SIOCHAN_iBaud     = 115200,
        }
};
/*********************************************************************************************************
** 函数名称: __kendryteUartHwOptionAnalyse
** 功能描述: 分析 UART 通道硬件参数
** 输　入  : iHwOption                 硬件参数
**           puiDataBits               数据位数
**           puiStopBits               结束位
**           puiParity                 校验位
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __kendryteUartHwOptionAnalyse (INT     iHwOption,
                                            UINT   *puiDataBits,
                                            UINT   *puiStopBits,
                                            UINT   *puiParity)
{

#if UART_CFG_DATABITS_EN > 0
    if ((iHwOption & CSIZE) == CS8) {                                   /*  确定数据位数                */
        *puiDataBits = UART_FRAME_WORD_LENGTH_8;
    } else {
        *puiDataBits = UART_FRAME_WORD_LENGTH_7;
    }
#endif

#if UART_CFG_STOPBITS_EN > 0
    if (iHwOption & STOPB) {                                            /*  确定结束位                  */
        *puiStopBits = UART_FRAME_NUM_STB_2;
    } else {
        *puiStopBits = UART_FRAME_NUM_STB_1;
    }
#endif

#if UART_CFG_PARITY_EN > 0
    if (iHwOption & PARENB) {                                           /*  确定校验位                  */
        if (iHwOption & PARODD) {
            *puiParity = UART_ODD_PARITY;
        } else {
            *puiParity = UART_EVEN_PARITY;
        }
    } else {
        *puiParity = UART_PARITY_NONE;
    }
#endif
}
/*********************************************************************************************************
** 函数名称: __kendryteUartCbInstall
** 功能描述: SIO 通道安装回调函数
** 输　入  : pSioChan                  SIO 通道
**           iCallbackType             回调类型
**           callbackRoute             回调函数
**           pvCallbackArg             回调参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __kendryteUartCbInstall (SIO_CHAN        *pSioChan,
                                     INT              iCallbackType,
                                     VX_SIO_CALLBACK  callbackRoute,
                                     PVOID            pvCallbackArg)
{
    __PKENDRYTE_UART_SIOCHAN  pUartSioChan = (__PKENDRYTE_UART_SIOCHAN)pSioChan;

    switch (iCallbackType) {

    case SIO_CALLBACK_GET_TX_CHAR:                                      /*  发送回调函数                */
        pUartSioChan->SIOCHAN_pcbGetTxChar = callbackRoute;
        pUartSioChan->SIOCHAN_pvGetTxArg   = pvCallbackArg;
        return  (ERROR_NONE);

    case SIO_CALLBACK_PUT_RCV_CHAR:                                     /*  接收回调函数                */
        pUartSioChan->SIOCHAN_pcbPutRcvChar = callbackRoute;
        pUartSioChan->SIOCHAN_pvPutRcvArg   = pvCallbackArg;
        return  (ERROR_NONE);

    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __kendryteUartPollRxChar
** 功能描述: SIO 通道轮询接收
** 输　入  : pSioChan                  SIO 通道
**           pcInChar                  接收的字节
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __kendryteUartPollRxChar (SIO_CHAN  *pSioChan, PCHAR  pcInChar)
{
    __PKENDRYTE_UART_SIOCHAN  pUartSioChan = (__PKENDRYTE_UART_SIOCHAN)pSioChan;
    UINT                      uiChannel    = pUartSioChan - _G_kendryteUartChannels;

    *pcInChar  =  uartapb_getc(uiChannel);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __kendryteUartPollTxChar
** 功能描述: SIO 通道轮询发送
** 输　入  : pSioChan                  SIO 通道
**           cOutChar                  发送的字节
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __kendryteUartPollTxChar (SIO_CHAN  *pSioChan, CHAR  cOutChar)
{
    __PKENDRYTE_UART_SIOCHAN  pUartSioChan = (__PKENDRYTE_UART_SIOCHAN)pSioChan;
    UINT                      uiChannel    = pUartSioChan - _G_kendryteUartChannels;

    uartapb_putc(uiChannel, cOutChar);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __kendryteUartStartup
** 功能描述: SIO 通道发送(没有使用中断)
** 输　入  : pSioChan                  SIO 通道
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**
** 注    意: 这个函数有可能在非发送中断的中断里调用!
*********************************************************************************************************/
static INT  __kendryteUartStartup (SIO_CHAN  *pSioChan)
{
    CHAR                      cChar;
    INTREG                    intreg;
    __PKENDRYTE_UART_SIOCHAN  pUartSioChan = (__PKENDRYTE_UART_SIOCHAN)pSioChan;
    addr_t                    ulBase       = pUartSioChan->SIOCHAN_ulBase;

    if ((read32(ulBase + UART_REG_LSR) & UART_TXFIFO_EMPTY)) {
        for (int i = 0; i < UART_TXFIFO_DEEPTH; i++) {
            if (pUartSioChan->SIOCHAN_pcbGetTxChar(
                    pUartSioChan->SIOCHAN_pvGetTxArg, &cChar) == ERROR_NONE) {
                write32(cChar, ulBase + UART_REG_THR);
            } else {
                break;
            }
        }
    }

    intreg = KN_INT_DISABLE();                                          /*     开启发送中断             */
    write32(read32(ulBase + UART_REG_IER) | UART_TXFIFO_EMPTY_INT_EN, ulBase + UART_REG_IER);
    KN_INT_ENABLE(intreg);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __kendryteUartIsr
** 功能描述: SIO 通道发送(没有使用中断)
** 输　入  : pSioChan                  SIO 通道
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**
** 注    意: 这个函数有可能在非发送中断的中断里调用!
*********************************************************************************************************/
static INT  __kendryteUartIsr (__PKENDRYTE_UART_SIOCHAN  pUartSioChan, ULONG ulVector)
{
    ULONG   ulPend    = 0;
    addr_t  ulBase    = pUartSioChan->SIOCHAN_ulBase;

    API_InterVectorDisable(pUartSioChan->SIOCHAN_ulVector);             /*          禁止中断            */

    ulPend = read32(ulBase + UART_REG_IIR);                             /*          读取中断标志        */

    if (ulPend & UART_IIR_THRE) {                                       /*          发送中断            */
        CHAR   tx_data;

        if (read32(ulBase + UART_REG_LSR) & UART_TXFIFO_EMPTY) {        /*          发送缓冲为空        */
            for (int i = 0; i < UART_TXFIFO_DEEPTH; i++) {
                if (pUartSioChan->SIOCHAN_pcbGetTxChar(
                        pUartSioChan->SIOCHAN_pvGetTxArg, &tx_data) == ERROR_NONE) {
                    write32(tx_data, ulBase + UART_REG_THR);
                } else {                                                /*    若无数据，关闭发送中断    */
                    write32(read32(ulBase + UART_REG_IER) & ~UART_TXFIFO_EMPTY_INT_EN, ulBase + UART_REG_IER);
                    break;
                }
            }
        }
    }

    if (ulPend & UART_IIR_RDA) {                                        /*          接收中断            */
        CHAR   rx_data;

        while (read32(ulBase + UART_REG_LSR) & UART_RXFIFO_DR) {        /*          接收缓冲非空        */
            rx_data = (CHAR)(read32(ulBase + UART_REG_RBR) & 0xff);
            pUartSioChan->SIOCHAN_pcbPutRcvChar(pUartSioChan->SIOCHAN_pvPutRcvArg, rx_data);
        }
    }

    UART_TRANS_GAP();

//    bspDebugMsg("======>\r\n");
//    for (int i = 0; i < 5000; i++);
//    KN_IO_MB();

    API_InterVectorEnable(pUartSioChan->SIOCHAN_ulVector);              /*          中断使能            */

    return  (LW_IRQ_HANDLED);
}
/*********************************************************************************************************
** 函数名称: __kendryteUartInit
** 功能描述: 初始化一个 SIO 通道
** 输　入  : pUartSioChan              SIO 通道
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __kendryteUartInit (__PKENDRYTE_UART_SIOCHAN  pUartSioChan)
{
    UINT    uiChannel = pUartSioChan - _G_kendryteUartChannels;

    sysctl_clock_enable(SYSCTL_CLOCK_UART1 + uiChannel);

    uart_config(uiChannel, pUartSioChan->SIOCHAN_iBaud,
                UART_FRAME_WORD_LENGTH_8, UART_STOP_1, 0);              /*    这里使用修正后的stop bit  */
}
/*********************************************************************************************************
** 函数名称: __kendryteUartHup
** 功能描述: Hup 一个 SIO 通道
** 输　入  : pUartSioChan              SIO 通道
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __kendryteUartHup (__PKENDRYTE_UART_SIOCHAN  pUartSioChan)
{
    addr_t  ulBase    = pUartSioChan->SIOCHAN_ulBase;
    write8(0x0, ulBase + UART_REG_IER);
}
/*********************************************************************************************************
** 函数名称: __kendryteUartIoctl
** 功能描述: SIO 通道控制
** 输　入  : pSioChan                  SIO 通道
**           iCmd                      命令
**           lArg                      参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __kendryteUartIoctl (SIO_CHAN  *pSioChan, INT  iCmd, LONG  lArg)
{
    __PKENDRYTE_UART_SIOCHAN  pUartSioChan = (__PKENDRYTE_UART_SIOCHAN)pSioChan;
    UINT                      uiChannel    = pUartSioChan - _G_kendryteUartChannels;
    INT                       error        = ERROR_NONE;
    UINT                      uiDataBits;
    UINT                      uiStopBits;
    UINT                      uiParity;

    switch (iCmd) {

    case SIO_OPEN:                                                      /*  打开串口                    */
        API_InterVectorDisable(pUartSioChan->SIOCHAN_ulVector);
        pUartSioChan->SIOCHAN_iBaud     = __UART_DEFAULT_BAUD;          /*  初始化波特率                */
        pUartSioChan->SIOCHAN_iHwOption = __UART_DEFAULT_OPT;           /*  初始化硬件状态              */
        __kendryteUartInit(pUartSioChan);                               /*  初始化串口                  */
        API_InterVectorEnable(pUartSioChan->SIOCHAN_ulVector);          /*  使能串口中断                */
        break;

    case SIO_HUP:                                                       /*  关闭串口                    */
        API_InterVectorDisable(pUartSioChan->SIOCHAN_ulVector);
        __kendryteUartHup(pUartSioChan);
        break;

    case SIO_BAUD_SET:                                                  /*  设置波特率                  */
        API_InterVectorDisable(pUartSioChan->SIOCHAN_ulVector);         /*  关闭串口中断                */

        addr_t ulBase = pUartSioChan->SIOCHAN_ulBase;
        UINT32 freq   = sysctl_clock_get_freq(SYSCTL_CLOCK_APB0);

        UINT32 u16Divider = ((freq + __UART_BAUD_RATE_CONST * pUartSioChan->SIOCHAN_iBaud / 2)
                              / (__UART_BAUD_RATE_CONST * pUartSioChan->SIOCHAN_iBaud));

        write8((u16Divider & 0xFF), ulBase + UART_REG_DLL);
        write8((u16Divider >> 8), ulBase + UART_REG_DLH);

        pUartSioChan->SIOCHAN_iBaud = lArg;                             /*  记录波特率                  */

        API_InterVectorEnable(pUartSioChan->SIOCHAN_ulVector);          /*  使能串口中断                */
        break;

    case SIO_BAUD_GET:                                                  /*  获得波特率                  */
        *((LONG *)lArg) = pUartSioChan->SIOCHAN_iBaud;
        break;

    case SIO_HW_OPTS_SET:                                               /*  设置硬件参数                */
        API_InterVectorDisable(pUartSioChan->SIOCHAN_ulVector);         /*  关闭串口中断                */

        __kendryteUartHwOptionAnalyse(pUartSioChan->SIOCHAN_iHwOption,  /*  获得具体参数                */
                                    &uiDataBits,
                                    &uiStopBits,
                                    &uiParity);

        if (uiParity == __UART_PARITY_NONE) {                           /*  修正 uiParity 的值          */
            uiParity = 0;
        } else if (uiParity == __UART_ODD_PARITY) {
            uiParity = 1;
        } else if (uiParity == __UART_EVEN_PARITY) {
            uiParity = 2;
        } else {
            _ErrorHandle(EINVAL);
            error = PX_ERROR;
            return  (error);
        }

        if (uiStopBits == UART_FRAME_NUM_STB_1) {                       /*  修正  STOPBITS 的值         */
            uiStopBits = UART_STOP_1;
        } else if (uiStopBits == UART_FRAME_NUM_STB_2) {
            uiStopBits = UART_STOP_2;
        } else {
            if (uiDataBits == 5) {
                uiStopBits = UART_STOP_1_5;
            } else {
                printk("Unrecognized stop bit!\r\n");
            }
        }

        uart_config(uiChannel, pUartSioChan->SIOCHAN_iBaud, uiDataBits, uiStopBits, uiParity);

        pUartSioChan->SIOCHAN_iHwOption = (INT)lArg;                    /*  记录硬件参数                */
        API_InterVectorEnable(pUartSioChan->SIOCHAN_ulVector);          /*  使能串口中断                */
        break;

    case SIO_HW_OPTS_GET:                                               /*  获取硬件参数                */
        *((INT *)lArg) = pUartSioChan->SIOCHAN_iHwOption;
        break;

    default:
        _ErrorHandle(ENOSYS);
        error = PX_ERROR;
        break;
    }

    return  (error);
}
/*********************************************************************************************************
  UART SIO 驱动程序
*********************************************************************************************************/
static SIO_DRV_FUNCS    _G_kendryteUartDrvFuncs = {
    (INT (*)(SIO_CHAN *,INT, PVOID))__kendryteUartIoctl,
    __kendryteUartStartup,
    __kendryteUartCbInstall,
    __kendryteUartPollRxChar,
    __kendryteUartPollTxChar
};
/*********************************************************************************************************
** 函数名称: sioChanCreate
** 功能描述: 创建一个 SIO 通道
** 输　入  : uiChannel    UART 通道号
** 输　出  : SIO 通道
** 全局变量:
** 调用模块:
*********************************************************************************************************/
SIO_CHAN  *uartChanCreate (UINT  uiChannel)
{
    __PKENDRYTE_UART_SIOCHAN  pUartSioChan;
    CHAR                      cIsrName[64];

    if (uiChannel >= __UART_NR) {
        printk("uartChanCreate: invalid uiChannel value!\n");
        return  (LW_NULL);
    }

    pUartSioChan = &_G_kendryteUartChannels[uiChannel];
    pUartSioChan->SIOCHAN_sioChan.pDrvFuncs = &_G_kendryteUartDrvFuncs;

    API_InterVectorDisable(pUartSioChan->SIOCHAN_ulVector);
    snprintf(cIsrName, sizeof(cIsrName), "uart_isr%d", uiChannel + 1);
    API_InterVectorConnect(pUartSioChan->SIOCHAN_ulVector,
                           (PINT_SVR_ROUTINE)__kendryteUartIsr,
                           pUartSioChan,
                           cIsrName);

    return  (&pUartSioChan->SIOCHAN_sioChan);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
