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
** 文   件   名: sifive_sio.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2018 年 04 月 23 日
**
** 描        述: SIFIVE 串口 SIO 驱动.
**
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "SylixOS.h"
#include "config.h"
#include "driver/clock/k210_clock.h"

#include "KendryteWare/include/plic.h"
#include "KendryteWare/include/uarths.h"
/*********************************************************************************************************
  UARTHS 寄存器偏移定义
*********************************************************************************************************/
#define UART_REG_TXFIFO                 0x00                            /*  txdata transmit data reg    */
#define UART_REG_RXFIFO                 0x04                            /*  rxdata receive data reg     */
#define UART_REG_TXCTRL                 0x08                            /*  txctrl transmit control reg */
#define UART_REG_RXCTRL                 0x0C                            /*  rxctrl receive control reg  */
#define UART_REG_IE                     0x10                            /*  ie UART interrupt enable reg*/
#define UART_REG_IP                     0x14                            /*  ip UART interrupt pend reg  */
#define UART_REG_DIV                    0x18                            /*  div Baud rate divisor reg   */

#define UART_TX_EN                      (1 << 0)
#define UART_RX_EN                      (1 << 0)

#define UART_TXINT_EN                   (1 << 0)
#define UART_RXINT_EN                   (1 << 1)

#define UART_TXFIFO_FULL                (1 << 31)
#define UART_RXFIFO_EMPTY               (1 << 31)

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

#define __UARTHS_ODD_PARITY                 ((UART_LCR_PARITY_TYPE1_ODD << \
                                         UART_LCR_PARITY_TYPE1_SHIFT) | \
                                         UART_LCR_PARITY_EN)

#define __UARTHS_EVEN_PARITY                ((UART_LCR_PARITY_TYPE1_EVEN << \
                                         UART_LCR_PARITY_TYPE1_SHIFT) | \
                                         UART_LCR_PARITY_EN)

#define __UARTHS_PARITY_NONE                (UART_LCR_PARITY_EN_DISABLE << \
                                         UART_LCR_PARITY_EN_SHIFT)
/*********************************************************************************************************
  UART 硬件参数定义
*********************************************************************************************************/
#define __UARTHS_NR                  (1)                               /*  通道数                       */
#define __UARTHS_MODULE_INPUT_CLK    (500000000u)                      /*  UART 模块输入时钟            */

#define __UARTHS_DEFAULT_BAUD        (SIO_BAUD_115200)                 /*  缺省波特率                   */
#define __UARTHS_DEFAULT_OPT         (CREAD | CS8)                     /*  缺省选项                     */
#define __UARTHS_BAUD_RATE_CONST     (16)
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
} __KENDRYTE_UART_SIOCHAN;
typedef __KENDRYTE_UART_SIOCHAN     *__PKENDRYTE_UART_SIOCHAN;          /*  指针类型                    */
/*********************************************************************************************************
  UART SIO 通道控制块数组定义
*********************************************************************************************************/
static __KENDRYTE_UART_SIOCHAN  _G_kendryteSioChannels[__UARTHS_NR] = {
        {
                .SIOCHAN_uiChannel = 0,
                .SIOCHAN_ulBase    = KENDRYTE_UART0_BASE,               /*  UARTHS                      */
                .SIOCHAN_ulVector  = KENDRYTE_UART0_VECTOR,
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
** 函数名称: __kendryteUartHwOptionSet
** 功能描述: 初始化 UART 硬件
** 输　入  : pUartSioChan              UART SIO 通道控制块指针
**           uiBaudRate                波特率
**           uiParity                  校验位
**           uiStopBits                结束位
**           uiDataBits                数据位数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __kendryteUartHwOptionSet (__PKENDRYTE_UART_SIOCHAN  pUartSioChan,
                                       UINT                    uiBaud,
                                       UINT                    uiParity,
                                       UINT                    uiStopBits,
                                       UINT                    uiDataBits)
{
    UINT        uiCurrentCfg;

    /*
     * Configuring the system clocks for UART instance here.
     * But this board's UART instance clock equals to tlclock which is coreclock/2.
     */

    /*
     * Performing the Pin Multiplexing for UART instance here.
     * But this board's UART instance pins needn't configuring.
     */

    /*
     * Performing a module reset here.
     * No need.
     */

    /*
     * uarths 只支持配置停止位的位数
     */
    uiCurrentCfg = read32(pUartSioChan->SIOCHAN_ulBase + UART_REG_TXCTRL);
    if (uiStopBits == UART_FRAME_NUM_STB_2) {
        write32(uiCurrentCfg | (0x1<<1), pUartSioChan->SIOCHAN_ulBase + UART_REG_TXCTRL);
    } else {
        write32(uiCurrentCfg & ~(0x1<<1), pUartSioChan->SIOCHAN_ulBase + UART_REG_TXCTRL);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __kendryteSioCbInstall
** 功能描述: SIO 通道安装回调函数
** 输　入  : pSioChan                  SIO 通道
**           iCallbackType             回调类型
**           callbackRoute             回调函数
**           pvCallbackArg             回调参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __kendryteSioCbInstall (SIO_CHAN        *pSioChan,
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
** 函数名称: __kendryteSioPollRxChar
** 功能描述: SIO 通道轮询接收
** 输　入  : pSioChan                  SIO 通道
**           pcInChar                  接收的字节
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __kendryteSioPollRxChar (SIO_CHAN  *pSioChan, PCHAR  pcInChar)
{
    __PKENDRYTE_UART_SIOCHAN  pUartSioChan = (__PKENDRYTE_UART_SIOCHAN)pSioChan;

    (VOID)pUartSioChan;

    *pcInChar  =  uarths_getc();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __kendryteSioPollTxChar
** 功能描述: SIO 通道轮询发送
** 输　入  : pSioChan                  SIO 通道
**           cOutChar                  发送的字节
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __kendryteSioPollTxChar (SIO_CHAN  *pSioChan, CHAR  cOutChar)
{
    __PKENDRYTE_UART_SIOCHAN  pUartSioChan = (__PKENDRYTE_UART_SIOCHAN)pSioChan;

    (VOID)pUartSioChan;

    uarths_putchar(cOutChar);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __kendryteSioStartup
** 功能描述: SIO 通道发送(没有使用中断)
** 输　入  : pSioChan                  SIO 通道
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**
** 注    意: 这个函数有可能在非发送中断的中断里调用!
*********************************************************************************************************/
static INT  __kendryteSioStartup (SIO_CHAN  *pSioChan)
{
    __PKENDRYTE_UART_SIOCHAN  pUartSioChan = (__PKENDRYTE_UART_SIOCHAN)pSioChan;
    addr_t                    ulBase       = pUartSioChan->SIOCHAN_ulBase;
    CHAR                      cChar;
    INTREG                    intreg;

    while (!(read32(ulBase + UART_REG_TXFIFO) & UART_TXFIFO_FULL)) {
        if (pUartSioChan->SIOCHAN_pcbGetTxChar(
                pUartSioChan->SIOCHAN_pvGetTxArg, &cChar) == ERROR_NONE) {
            write8(cChar, ulBase + UART_REG_TXFIFO);
        } else {
            break;
        }
    }

    intreg = KN_INT_DISABLE();
    write32(read32(ulBase + UART_REG_IE) | UART_TXINT_EN, ulBase + UART_REG_IE);
    KN_INT_ENABLE(intreg);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __kendryteSioIsr
** 功能描述: SIO 通道发送(没有使用中断)
** 输　入  : pSioChan                  SIO 通道
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**
** 注    意: 这个函数有可能在非发送中断的中断里调用!
*********************************************************************************************************/
static INT  __kendryteSioIsr (__PKENDRYTE_UART_SIOCHAN  pUartSioChan, ULONG ulVector)
{
    ULONG   ulPend    = 0;
    addr_t  ulBase    = pUartSioChan->SIOCHAN_ulBase;

    ulPend = read32(ulBase + UART_REG_IP);

    if (ulPend & UART_TXINT_EN) {
        CHAR    cChar;
        while (!(read32(ulBase + UART_REG_TXFIFO) & UART_TXFIFO_FULL)) {
            if (pUartSioChan->SIOCHAN_pcbGetTxChar(pUartSioChan->SIOCHAN_pvGetTxArg, &cChar)
                == ERROR_NONE) {
                write8(cChar, ulBase + UART_REG_TXFIFO);
            } else {
                write32(read32(ulBase + UART_REG_IE) & ~UART_TXINT_EN, ulBase + UART_REG_IE);
                break;
            }
        }
    }

    if (ulPend & UART_RXINT_EN) {
        INT     iChar;

        while ((iChar = read32(ulBase + UART_REG_RXFIFO)) >= 0) {
            pUartSioChan->SIOCHAN_pcbPutRcvChar(pUartSioChan->SIOCHAN_pvPutRcvArg, iChar);
        }
    }

    return  (LW_IRQ_HANDLED);
}
/*********************************************************************************************************
** 函数名称: __kendryteSioInit
** 功能描述: 初始化一个 SIO 通道
** 输　入  : pUartSioChan              SIO 通道
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __kendryteSioInit (__PKENDRYTE_UART_SIOCHAN  pUartSioChan)
{
    addr_t  ulBase    = pUartSioChan->SIOCHAN_ulBase;

    write32(UART_TX_EN | (1 << 16), ulBase + UART_REG_TXCTRL);
    write32(UART_RX_EN | (0 << 16), ulBase + UART_REG_RXCTRL);
    write32(UART_RXINT_EN | UART_TXINT_EN, ulBase + UART_REG_IE);
}
/*********************************************************************************************************
** 函数名称: __kendryteSioHup
** 功能描述: Hup 一个 SIO 通道
** 输　入  : pUartSioChan              SIO 通道
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __kendryteSioHup (__PKENDRYTE_UART_SIOCHAN  pUartSioChan)
{
    addr_t  ulBase    = pUartSioChan->SIOCHAN_ulBase;

    write32(0, ulBase + UART_REG_IE);
}
/*********************************************************************************************************
** 函数名称: __kendryteSioIoctl
** 功能描述: SIO 通道控制
** 输　入  : pSioChan                  SIO 通道
**           iCmd                      命令
**           lArg                      参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __kendryteSioIoctl (SIO_CHAN  *pSioChan, INT  iCmd, LONG  lArg)
{
    __PKENDRYTE_UART_SIOCHAN  pUartSioChan = (__PKENDRYTE_UART_SIOCHAN)pSioChan;
    INT                       error        = ERROR_NONE;
    UINT                      uiDataBits;
    UINT                      uiStopBits;
    UINT                      uiParity;

    switch (iCmd) {
    
    case SIO_OPEN:                                                      /*  打开串口                    */
        API_InterVectorDisable(pUartSioChan->SIOCHAN_ulVector);
        pUartSioChan->SIOCHAN_iBaud     = __UARTHS_DEFAULT_BAUD;        /*  初始化波特率                */
        pUartSioChan->SIOCHAN_iHwOption = __UARTHS_DEFAULT_OPT;         /*  初始化硬件状态              */
        __kendryteSioInit(pUartSioChan);                                /*  初始化串口                  */
        API_InterVectorEnable(pUartSioChan->SIOCHAN_ulVector);          /*  使能串口中断                */
        break;

    case SIO_HUP:                                                       /*  关闭串口                    */
        API_InterVectorDisable(pUartSioChan->SIOCHAN_ulVector);
        __kendryteSioHup(pUartSioChan);
        break;

    case SIO_BAUD_SET:                                                  /*  设置波特率                  */
        API_InterVectorDisable(pUartSioChan->SIOCHAN_ulVector);         /*  关闭串口中断                */

        volatile uarths_t *const uarths = (volatile uarths_t *)pUartSioChan->SIOCHAN_ulBase;

        UINT32 freq = sysctl_clock_get_freq(SYSCTL_CLOCK_CPU);
        UINT16 div  = freq / pUartSioChan->SIOCHAN_iBaud - 1;

        uarths->div.div = div;
        pUartSioChan->SIOCHAN_iBaud = lArg;                             /*  记录波特率                  */

        API_InterVectorEnable(pUartSioChan->SIOCHAN_ulVector);          /*  使能串口中断                */
        break;

    case SIO_BAUD_GET:                                                  /*  获得波特率                  */
        *((LONG *)lArg) = pUartSioChan->SIOCHAN_iBaud;
        break;

    case SIO_HW_OPTS_SET:                                               /*  设置硬件参数                */
        API_InterVectorDisable(pUartSioChan->SIOCHAN_ulVector);         /*  关闭串口中断                */

        __kendryteUartHwOptionAnalyse(pUartSioChan->SIOCHAN_iHwOption,
                                      &uiDataBits,
                                      &uiStopBits,
                                      &uiParity);

        __kendryteUartHwOptionSet(pUartSioChan,
                                  pUartSioChan->SIOCHAN_iBaud,
                                  uiParity,
                                  uiStopBits,
                                  uiDataBits);                          /*  初始化串口                  */

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
static SIO_DRV_FUNCS    _G_kendryteSioDrvFuncs = {
    (INT (*)(SIO_CHAN *,INT, PVOID))__kendryteSioIoctl,
    __kendryteSioStartup,
    __kendryteSioCbInstall,
    __kendryteSioPollRxChar,
    __kendryteSioPollTxChar
};
/*********************************************************************************************************
** 函数名称: sioChanCreate
** 功能描述: 创建一个 SIO 通道
** 输　入  : uiChannel    UART 通道号
** 输　出  : SIO 通道
** 全局变量:
** 调用模块:
*********************************************************************************************************/
SIO_CHAN  *sioChanCreate (UINT  uiChannel)
{
    __PKENDRYTE_UART_SIOCHAN  pUartSioChan;

    if (uiChannel >= __UARTHS_NR) {
        printk("sioChanCreate: invalid uiChannel value!\n");
        return  (LW_NULL);
    }

    pUartSioChan = &_G_kendryteSioChannels[uiChannel];
    pUartSioChan->SIOCHAN_sioChan.pDrvFuncs = &_G_kendryteSioDrvFuncs;

    API_InterVectorDisable(pUartSioChan->SIOCHAN_ulVector);
    API_InterVectorConnect(pUartSioChan->SIOCHAN_ulVector,
                           (PINT_SVR_ROUTINE)__kendryteSioIsr,
                           pUartSioChan,
                           "uarths_isr");

    return  (&pUartSioChan->SIOCHAN_sioChan);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
