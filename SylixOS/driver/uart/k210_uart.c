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
** ��   ��   ��: k210_uart.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 11 �� 12 ��
**
** ��        ��: Kendryte K210  ���� SIO ����.
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
  UARTHS �Ĵ���ƫ�ƶ���
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

#define UART_TXFIFO_EMPTY_INT_EN        (0x1 << 1)                      /*  ���ͼĴ������ж�            */
#define UART_RXFIFO_RDY_INT_EN          (0x1 << 0)                      /*  �������ݿ����ж�            */

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
  UART Ӳ����������
*********************************************************************************************************/
#define __UART_NR                  (3)                                  /*  ͨ����                      */
#define __UART_MODULE_INPUT_CLK    (500000000u)                         /*  UART ģ������ʱ��           */

#define __UART_DEFAULT_BAUD        (SIO_BAUD_115200)                    /*  ȱʡ������                  */
#define __UART_DEFAULT_OPT         (CREAD | CS8)                        /*  ȱʡѡ��                    */
#define __UART_BAUD_RATE_CONST     (16)

#define UART_TRANS_GAP()           do {                                \
                                       for(int i = 0; i < 5000; i++);  \
                                   } while(0)                           /*  ������������ʱ����        */
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
    LW_SPINLOCK_DEFINE  (slock);
} __KENDRYTE_UART_SIOCHAN;
typedef __KENDRYTE_UART_SIOCHAN     *__PKENDRYTE_UART_SIOCHAN;          /*  ָ������                    */
/*********************************************************************************************************
  UART SIO ͨ�����ƿ����鶨��
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
** ��������: __kendryteUartCbInstall
** ��������: SIO ͨ����װ�ص�����
** �䡡��  : pSioChan                  SIO ͨ��
**           iCallbackType             �ص�����
**           callbackRoute             �ص�����
**           pvCallbackArg             �ص�����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __kendryteUartCbInstall (SIO_CHAN        *pSioChan,
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
** ��������: __kendryteUartPollRxChar
** ��������: SIO ͨ����ѯ����
** �䡡��  : pSioChan                  SIO ͨ��
**           pcInChar                  ���յ��ֽ�
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __kendryteUartPollRxChar (SIO_CHAN  *pSioChan, PCHAR  pcInChar)
{
    __PKENDRYTE_UART_SIOCHAN  pUartSioChan = (__PKENDRYTE_UART_SIOCHAN)pSioChan;
    UINT                      uiChannel    = pUartSioChan - _G_kendryteUartChannels;

    *pcInChar  =  uartapb_getc(uiChannel);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __kendryteUartPollTxChar
** ��������: SIO ͨ����ѯ����
** �䡡��  : pSioChan                  SIO ͨ��
**           cOutChar                  ���͵��ֽ�
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __kendryteUartPollTxChar (SIO_CHAN  *pSioChan, CHAR  cOutChar)
{
    __PKENDRYTE_UART_SIOCHAN  pUartSioChan = (__PKENDRYTE_UART_SIOCHAN)pSioChan;
    UINT                      uiChannel    = pUartSioChan - _G_kendryteUartChannels;

    uartapb_putc(uiChannel, cOutChar);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __kendryteUartStartup
** ��������: SIO ͨ������(û��ʹ���ж�)
** �䡡��  : pSioChan                  SIO ͨ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**
** ע    ��: ��������п����ڷǷ����жϵ��ж������!
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

    intreg = KN_INT_DISABLE();                                          /*     ���������ж�             */
    write32(read32(ulBase + UART_REG_IER) | UART_TXFIFO_EMPTY_INT_EN, ulBase + UART_REG_IER);
    KN_INT_ENABLE(intreg);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __kendryteUartIsr
** ��������: SIO ͨ������(û��ʹ���ж�)
** �䡡��  : pSioChan                  SIO ͨ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**
** ע    ��: ��������п����ڷǷ����жϵ��ж������!
*********************************************************************************************************/
static INT  __kendryteUartIsr (__PKENDRYTE_UART_SIOCHAN  pUartSioChan, ULONG ulVector)
{
    ULONG   ulPend    = 0;
    addr_t  ulBase    = pUartSioChan->SIOCHAN_ulBase;

    API_InterVectorDisable(pUartSioChan->SIOCHAN_ulVector);             /*          ��ֹ�ж�            */

    ulPend = read32(ulBase + UART_REG_IIR);                             /*          ��ȡ�жϱ�־        */

    if (ulPend & UART_IIR_THRE) {                                       /*          �����ж�            */
        CHAR   tx_data;

        if (read32(ulBase + UART_REG_LSR) & UART_TXFIFO_EMPTY) {        /*          ���ͻ���Ϊ��        */
            for (int i = 0; i < UART_TXFIFO_DEEPTH; i++) {
                if (pUartSioChan->SIOCHAN_pcbGetTxChar(
                        pUartSioChan->SIOCHAN_pvGetTxArg, &tx_data) == ERROR_NONE) {
                    write32(tx_data, ulBase + UART_REG_THR);
                } else {                                                /*    �������ݣ��رշ����ж�    */
                    write32(read32(ulBase + UART_REG_IER) & ~UART_TXFIFO_EMPTY_INT_EN, ulBase + UART_REG_IER);
                    break;
                }
            }
        }
    }

    if (ulPend & UART_IIR_RDA) {                                        /*          �����ж�            */
        CHAR   rx_data;

        while (read32(ulBase + UART_REG_LSR) & UART_RXFIFO_DR) {        /*          ���ջ���ǿ�        */
            rx_data = (CHAR)(read32(ulBase + UART_REG_RBR) & 0xff);
            pUartSioChan->SIOCHAN_pcbPutRcvChar(pUartSioChan->SIOCHAN_pvPutRcvArg, rx_data);
        }
    }

    UART_TRANS_GAP();

//    bspDebugMsg("======>\r\n");
//    for (int i = 0; i < 5000; i++);
//    KN_IO_MB();

    API_InterVectorEnable(pUartSioChan->SIOCHAN_ulVector);              /*          �ж�ʹ��            */

    return  (LW_IRQ_HANDLED);
}
/*********************************************************************************************************
** ��������: __kendryteUartInit
** ��������: ��ʼ��һ�� SIO ͨ��
** �䡡��  : pUartSioChan              SIO ͨ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __kendryteUartInit (__PKENDRYTE_UART_SIOCHAN  pUartSioChan)
{
    UINT    uiChannel = pUartSioChan - _G_kendryteUartChannels;

    sysctl_clock_enable(SYSCTL_CLOCK_UART1 + uiChannel);

    uart_config(uiChannel, pUartSioChan->SIOCHAN_iBaud,
                UART_FRAME_WORD_LENGTH_8, UART_STOP_1, 0);              /*    ����ʹ���������stop bit  */
}
/*********************************************************************************************************
** ��������: __kendryteUartHup
** ��������: Hup һ�� SIO ͨ��
** �䡡��  : pUartSioChan              SIO ͨ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __kendryteUartHup (__PKENDRYTE_UART_SIOCHAN  pUartSioChan)
{
    addr_t  ulBase    = pUartSioChan->SIOCHAN_ulBase;
    write8(0x0, ulBase + UART_REG_IER);
}
/*********************************************************************************************************
** ��������: __kendryteUartIoctl
** ��������: SIO ͨ������
** �䡡��  : pSioChan                  SIO ͨ��
**           iCmd                      ����
**           lArg                      ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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

    case SIO_OPEN:                                                      /*  �򿪴���                    */
        API_InterVectorDisable(pUartSioChan->SIOCHAN_ulVector);
        pUartSioChan->SIOCHAN_iBaud     = __UART_DEFAULT_BAUD;          /*  ��ʼ��������                */
        pUartSioChan->SIOCHAN_iHwOption = __UART_DEFAULT_OPT;           /*  ��ʼ��Ӳ��״̬              */
        __kendryteUartInit(pUartSioChan);                               /*  ��ʼ������                  */
        API_InterVectorEnable(pUartSioChan->SIOCHAN_ulVector);          /*  ʹ�ܴ����ж�                */
        break;

    case SIO_HUP:                                                       /*  �رմ���                    */
        API_InterVectorDisable(pUartSioChan->SIOCHAN_ulVector);
        __kendryteUartHup(pUartSioChan);
        break;

    case SIO_BAUD_SET:                                                  /*  ���ò�����                  */
        API_InterVectorDisable(pUartSioChan->SIOCHAN_ulVector);         /*  �رմ����ж�                */

        addr_t ulBase = pUartSioChan->SIOCHAN_ulBase;
        UINT32 freq   = sysctl_clock_get_freq(SYSCTL_CLOCK_APB0);

        UINT32 u16Divider = ((freq + __UART_BAUD_RATE_CONST * pUartSioChan->SIOCHAN_iBaud / 2)
                              / (__UART_BAUD_RATE_CONST * pUartSioChan->SIOCHAN_iBaud));

        write8((u16Divider & 0xFF), ulBase + UART_REG_DLL);
        write8((u16Divider >> 8), ulBase + UART_REG_DLH);

        pUartSioChan->SIOCHAN_iBaud = lArg;                             /*  ��¼������                  */

        API_InterVectorEnable(pUartSioChan->SIOCHAN_ulVector);          /*  ʹ�ܴ����ж�                */
        break;

    case SIO_BAUD_GET:                                                  /*  ��ò�����                  */
        *((LONG *)lArg) = pUartSioChan->SIOCHAN_iBaud;
        break;

    case SIO_HW_OPTS_SET:                                               /*  ����Ӳ������                */
        API_InterVectorDisable(pUartSioChan->SIOCHAN_ulVector);         /*  �رմ����ж�                */

        __kendryteUartHwOptionAnalyse(pUartSioChan->SIOCHAN_iHwOption,  /*  ��þ������                */
                                    &uiDataBits,
                                    &uiStopBits,
                                    &uiParity);

        if (uiParity == __UART_PARITY_NONE) {                           /*  ���� uiParity ��ֵ          */
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

        if (uiStopBits == UART_FRAME_NUM_STB_1) {                       /*  ����  STOPBITS ��ֵ         */
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
static SIO_DRV_FUNCS    _G_kendryteUartDrvFuncs = {
    (INT (*)(SIO_CHAN *,INT, PVOID))__kendryteUartIoctl,
    __kendryteUartStartup,
    __kendryteUartCbInstall,
    __kendryteUartPollRxChar,
    __kendryteUartPollTxChar
};
/*********************************************************************************************************
** ��������: sioChanCreate
** ��������: ����һ�� SIO ͨ��
** �䡡��  : uiChannel    UART ͨ����
** �䡡��  : SIO ͨ��
** ȫ�ֱ���:
** ����ģ��:
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
