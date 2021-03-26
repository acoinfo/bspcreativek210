/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ----------------------------------------------------------------------------------
**
** ��   ��   ��: sifive_sio.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 04 �� 23 ��
**
** ��        ��: SIFIVE ���� SIO ����.
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
  UARTHS �Ĵ���ƫ�ƶ���
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
  UART Ӳ����������
*********************************************************************************************************/
#define __UARTHS_NR                  (1)                               /*  ͨ����                       */
#define __UARTHS_MODULE_INPUT_CLK    (500000000u)                      /*  UART ģ������ʱ��            */

#define __UARTHS_DEFAULT_BAUD        (SIO_BAUD_115200)                 /*  ȱʡ������                   */
#define __UARTHS_DEFAULT_OPT         (CREAD | CS8)                     /*  ȱʡѡ��                     */
#define __UARTHS_BAUD_RATE_CONST     (16)
/*********************************************************************************************************
  UART SIO ͨ�����ƿ����Ͷ���
*********************************************************************************************************/
typedef struct {
    SIO_CHAN            SIOCHAN_sioChan;
    INT               (*SIOCHAN_pcbGetTxChar)();                        /*  �жϻص�����                */
    INT               (*SIOCHAN_pcbPutRcvChar)();
    PVOID               SIOCHAN_pvGetTxArg;                             /*  �ص���������                */
    PVOID               SIOCHAN_pvPutRcvArg;
    INT                 SIOCHAN_iBaud;                                  /*  ������                      */
    INT                 SIOCHAN_iHwOption;                              /*  Ӳ��ѡ��                    */
    addr_t              SIOCHAN_ulBase;
    ULONG               SIOCHAN_ulVector;
    UINT                SIOCHAN_uiChannel;
} __KENDRYTE_UART_SIOCHAN;
typedef __KENDRYTE_UART_SIOCHAN     *__PKENDRYTE_UART_SIOCHAN;          /*  ָ������                    */
/*********************************************************************************************************
  UART SIO ͨ�����ƿ����鶨��
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
** ��������: __kendryteUartHwOptionAnalyse
** ��������: ���� UART ͨ��Ӳ������
** �䡡��  : iHwOption                 Ӳ������
**           puiDataBits               ����λ��
**           puiStopBits               ����λ
**           puiParity                 У��λ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __kendryteUartHwOptionAnalyse (INT     iHwOption,
                                          UINT   *puiDataBits,
                                          UINT   *puiStopBits,
                                          UINT   *puiParity)
{

#if UART_CFG_DATABITS_EN > 0
    if ((iHwOption & CSIZE) == CS8) {                                   /*  ȷ������λ��                */
        *puiDataBits = UART_FRAME_WORD_LENGTH_8;
    } else {
        *puiDataBits = UART_FRAME_WORD_LENGTH_7;
    }
#endif

#if UART_CFG_STOPBITS_EN > 0
    if (iHwOption & STOPB) {                                            /*  ȷ������λ                  */
        *puiStopBits = UART_FRAME_NUM_STB_2;
    } else {
        *puiStopBits = UART_FRAME_NUM_STB_1;
    }
#endif

#if UART_CFG_PARITY_EN > 0
    if (iHwOption & PARENB) {                                           /*  ȷ��У��λ                  */
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
** ��������: __kendryteUartHwOptionSet
** ��������: ��ʼ�� UART Ӳ��
** �䡡��  : pUartSioChan              UART SIO ͨ�����ƿ�ָ��
**           uiBaudRate                ������
**           uiParity                  У��λ
**           uiStopBits                ����λ
**           uiDataBits                ����λ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
     * uarths ֻ֧������ֹͣλ��λ��
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
** ��������: __kendryteSioCbInstall
** ��������: SIO ͨ����װ�ص�����
** �䡡��  : pSioChan                  SIO ͨ��
**           iCallbackType             �ص�����
**           callbackRoute             �ص�����
**           pvCallbackArg             �ص�����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __kendryteSioCbInstall (SIO_CHAN        *pSioChan,
                                    INT              iCallbackType,
                                    VX_SIO_CALLBACK  callbackRoute,
                                    PVOID            pvCallbackArg)
{
    __PKENDRYTE_UART_SIOCHAN  pUartSioChan = (__PKENDRYTE_UART_SIOCHAN)pSioChan;
    
    switch (iCallbackType) {
    
    case SIO_CALLBACK_GET_TX_CHAR:                                      /*  ���ͻص�����                */
        pUartSioChan->SIOCHAN_pcbGetTxChar = callbackRoute;
        pUartSioChan->SIOCHAN_pvGetTxArg   = pvCallbackArg;
        return  (ERROR_NONE);
        
    case SIO_CALLBACK_PUT_RCV_CHAR:                                     /*  ���ջص�����                */
        pUartSioChan->SIOCHAN_pcbPutRcvChar = callbackRoute;
        pUartSioChan->SIOCHAN_pvPutRcvArg   = pvCallbackArg;
        return  (ERROR_NONE);
        
    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __kendryteSioPollRxChar
** ��������: SIO ͨ����ѯ����
** �䡡��  : pSioChan                  SIO ͨ��
**           pcInChar                  ���յ��ֽ�
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __kendryteSioPollRxChar (SIO_CHAN  *pSioChan, PCHAR  pcInChar)
{
    __PKENDRYTE_UART_SIOCHAN  pUartSioChan = (__PKENDRYTE_UART_SIOCHAN)pSioChan;

    (VOID)pUartSioChan;

    *pcInChar  =  uarths_getc();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __kendryteSioPollTxChar
** ��������: SIO ͨ����ѯ����
** �䡡��  : pSioChan                  SIO ͨ��
**           cOutChar                  ���͵��ֽ�
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __kendryteSioPollTxChar (SIO_CHAN  *pSioChan, CHAR  cOutChar)
{
    __PKENDRYTE_UART_SIOCHAN  pUartSioChan = (__PKENDRYTE_UART_SIOCHAN)pSioChan;

    (VOID)pUartSioChan;

    uarths_putchar(cOutChar);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __kendryteSioStartup
** ��������: SIO ͨ������(û��ʹ���ж�)
** �䡡��  : pSioChan                  SIO ͨ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**
** ע    ��: ��������п����ڷǷ����жϵ��ж������!
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
** ��������: __kendryteSioIsr
** ��������: SIO ͨ������(û��ʹ���ж�)
** �䡡��  : pSioChan                  SIO ͨ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**
** ע    ��: ��������п����ڷǷ����жϵ��ж������!
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
** ��������: __kendryteSioInit
** ��������: ��ʼ��һ�� SIO ͨ��
** �䡡��  : pUartSioChan              SIO ͨ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __kendryteSioInit (__PKENDRYTE_UART_SIOCHAN  pUartSioChan)
{
    addr_t  ulBase    = pUartSioChan->SIOCHAN_ulBase;

    write32(UART_TX_EN | (1 << 16), ulBase + UART_REG_TXCTRL);
    write32(UART_RX_EN | (0 << 16), ulBase + UART_REG_RXCTRL);
    write32(UART_RXINT_EN | UART_TXINT_EN, ulBase + UART_REG_IE);
}
/*********************************************************************************************************
** ��������: __kendryteSioHup
** ��������: Hup һ�� SIO ͨ��
** �䡡��  : pUartSioChan              SIO ͨ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __kendryteSioHup (__PKENDRYTE_UART_SIOCHAN  pUartSioChan)
{
    addr_t  ulBase    = pUartSioChan->SIOCHAN_ulBase;

    write32(0, ulBase + UART_REG_IE);
}
/*********************************************************************************************************
** ��������: __kendryteSioIoctl
** ��������: SIO ͨ������
** �䡡��  : pSioChan                  SIO ͨ��
**           iCmd                      ����
**           lArg                      ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __kendryteSioIoctl (SIO_CHAN  *pSioChan, INT  iCmd, LONG  lArg)
{
    __PKENDRYTE_UART_SIOCHAN  pUartSioChan = (__PKENDRYTE_UART_SIOCHAN)pSioChan;
    INT                       error        = ERROR_NONE;
    UINT                      uiDataBits;
    UINT                      uiStopBits;
    UINT                      uiParity;

    switch (iCmd) {
    
    case SIO_OPEN:                                                      /*  �򿪴���                    */
        API_InterVectorDisable(pUartSioChan->SIOCHAN_ulVector);
        pUartSioChan->SIOCHAN_iBaud     = __UARTHS_DEFAULT_BAUD;        /*  ��ʼ��������                */
        pUartSioChan->SIOCHAN_iHwOption = __UARTHS_DEFAULT_OPT;         /*  ��ʼ��Ӳ��״̬              */
        __kendryteSioInit(pUartSioChan);                                /*  ��ʼ������                  */
        API_InterVectorEnable(pUartSioChan->SIOCHAN_ulVector);          /*  ʹ�ܴ����ж�                */
        break;

    case SIO_HUP:                                                       /*  �رմ���                    */
        API_InterVectorDisable(pUartSioChan->SIOCHAN_ulVector);
        __kendryteSioHup(pUartSioChan);
        break;

    case SIO_BAUD_SET:                                                  /*  ���ò�����                  */
        API_InterVectorDisable(pUartSioChan->SIOCHAN_ulVector);         /*  �رմ����ж�                */

        volatile uarths_t *const uarths = (volatile uarths_t *)pUartSioChan->SIOCHAN_ulBase;

        UINT32 freq = sysctl_clock_get_freq(SYSCTL_CLOCK_CPU);
        UINT16 div  = freq / pUartSioChan->SIOCHAN_iBaud - 1;

        uarths->div.div = div;
        pUartSioChan->SIOCHAN_iBaud = lArg;                             /*  ��¼������                  */

        API_InterVectorEnable(pUartSioChan->SIOCHAN_ulVector);          /*  ʹ�ܴ����ж�                */
        break;

    case SIO_BAUD_GET:                                                  /*  ��ò�����                  */
        *((LONG *)lArg) = pUartSioChan->SIOCHAN_iBaud;
        break;

    case SIO_HW_OPTS_SET:                                               /*  ����Ӳ������                */
        API_InterVectorDisable(pUartSioChan->SIOCHAN_ulVector);         /*  �رմ����ж�                */

        __kendryteUartHwOptionAnalyse(pUartSioChan->SIOCHAN_iHwOption,
                                      &uiDataBits,
                                      &uiStopBits,
                                      &uiParity);

        __kendryteUartHwOptionSet(pUartSioChan,
                                  pUartSioChan->SIOCHAN_iBaud,
                                  uiParity,
                                  uiStopBits,
                                  uiDataBits);                          /*  ��ʼ������                  */

        pUartSioChan->SIOCHAN_iHwOption = (INT)lArg;                    /*  ��¼Ӳ������                */

        API_InterVectorEnable(pUartSioChan->SIOCHAN_ulVector);          /*  ʹ�ܴ����ж�                */
        break;

    case SIO_HW_OPTS_GET:                                               /*  ��ȡӲ������                */
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
  UART SIO ��������
*********************************************************************************************************/
static SIO_DRV_FUNCS    _G_kendryteSioDrvFuncs = {
    (INT (*)(SIO_CHAN *,INT, PVOID))__kendryteSioIoctl,
    __kendryteSioStartup,
    __kendryteSioCbInstall,
    __kendryteSioPollRxChar,
    __kendryteSioPollTxChar
};
/*********************************************************************************************************
** ��������: sioChanCreate
** ��������: ����һ�� SIO ͨ��
** �䡡��  : uiChannel    UART ͨ����
** �䡡��  : SIO ͨ��
** ȫ�ֱ���:
** ����ģ��:
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
