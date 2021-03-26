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
** ��   ��   ��: k210_i2c.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 10 �� 19 ��
**
** ��        ��: I2C ����������
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "config.h"
#include "driver/common.h"
#include "driver/clock/k210_clock.h"
#include "driver/i2c/k210_i2c.h"

#include "KendryteWare/include/plic.h"
#include "KendryteWare/include/i2c.h"
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define __I2C_CHANNEL_NUM         (3)
#define __I2C_BUS_FREQ_MAX        (400000)
#define __I2C_BUS_FREQ_NORMAL     (200000)
#define __I2C_BUS_FREQ_MIN        (100000)
#define __I2C_ADDRESS_7BITS       (7)
#define __I2C_ADDRESS_10BITS      (10)
#define __I2C_CHANNEL_ISR_FMT     "i2c%d_isr"
/*********************************************************************************************************
  slave handler �� device ���� (ע��: ��ؽṹ��ο��� kendryte-standalone-sdk)
*********************************************************************************************************/
#define SLAVE_MAX_ADDR  14

struct slave_info_t {
    UINT8     acces_reg;
    UINT8     reg_data[SLAVE_MAX_ADDR];
};

typedef enum _i2c_event {
    I2C_EV_START,
    I2C_EV_RESTART,
    I2C_EV_STOP
} i2c_event_t;

typedef struct _i2c_slave_handler {
    VOID      (*on_receive)(VOID *device, UINT32 data);
    UINT32    (*on_transmit)(VOID *device);
    VOID      (*on_event)(VOID *device, i2c_event_t event);
} i2c_slave_handler_t;
/*********************************************************************************************************
  i2c ͨ�����Ͷ���
*********************************************************************************************************/
typedef struct {
    sysctl_clock_t         clock;
    sysctl_threshold_t     threshold;
    sysctl_dma_select_t    dma_req_base;

    addr_t                 I2CCHAN_ulPhyAddrBase;                       /*  �����ַ����ַ              */
    UINT                   I2CCHAN_uiBusFreq;                           /*  I2C SCL speed               */
    UINT                   I2CCHAN_SpeedMode;                           /*  i2c_bus_speed_mode          */
    ULONG                  I2CCHAN_ulIrqVector;                         /*  I2C �ж�����                */
    UINT                   I2CCHAN_uiChannel;                           /*  I2C ͨ�����                */
    BOOL                   I2CCHAN_bIsInit;                             /*  �Ƿ��Ѿ���ʼ��              */
    spinlock_t             I2CCHAN_lock;                                /*  ������                      */
    PUCHAR                 I2CCHAN_pucBuffer;                           /*  ���ݻ�����                  */
    UINT                   I2CCHAN_uiBufferLen;                         /*  ���ݻ���������              */

    BOOL                   I2CCHAN_bIsSlaveMode;                        /*  �Ƿ�Ϊ�ӻ�ģʽ              */
    LW_HANDLE              I2CCHAN_hSignal;                             /*  ͬ���ź���                  */
    i2c_slave_handler_t    I2CCHAN_i2cSlaveHandle;                      /*  �ӻ�ģʽʱ���¼�������    */

    struct slave_info_t    slave_device;

} __K210_I2C_CHANNEL, *__PK210_I2C_CHANNEL;
/*********************************************************************************************************
  ȫ�ֱ��� (������豸�����붯̬�����ڴ棬�������豸�����������õ���Ӳ�������ڸýṹ���У���Ϊpri_data)
*********************************************************************************************************/
static __K210_I2C_CHANNEL  _G_k210I2cChannels[__I2C_CHANNEL_NUM] = {
        {
                .clock                  =   SYSCTL_CLOCK_I2C0,
                .threshold              =   SYSCTL_THRESHOLD_I2C0,
                .dma_req_base           =   SYSCTL_DMA_SELECT_I2C0_RX_REQ,
                .I2CCHAN_uiChannel      =   0,
                .I2CCHAN_ulPhyAddrBase  =   I2C0_BASE_ADDR,
                .I2CCHAN_uiBusFreq      =   __I2C_BUS_FREQ_MIN,
                .I2CCHAN_SpeedMode      =   I2C_BS_STANDARD,
                .I2CCHAN_ulIrqVector    =   KENDRYTE_PLIC_VECTOR(IRQN_I2C0_INTERRUPT),
        },
        {
                .clock                  =   SYSCTL_CLOCK_I2C1,
                .threshold              =   SYSCTL_THRESHOLD_I2C1,
                .dma_req_base           =   SYSCTL_DMA_SELECT_I2C1_RX_REQ,
                .I2CCHAN_uiChannel      =   1,
                .I2CCHAN_ulPhyAddrBase  =   I2C1_BASE_ADDR,
                .I2CCHAN_uiBusFreq      =   __I2C_BUS_FREQ_MIN,
                .I2CCHAN_SpeedMode      =   I2C_BS_STANDARD,
                .I2CCHAN_ulIrqVector    =   KENDRYTE_PLIC_VECTOR(IRQN_I2C1_INTERRUPT),
        },
        {
                .clock                  =   SYSCTL_CLOCK_I2C2,
                .threshold              =   SYSCTL_THRESHOLD_I2C2,
                .dma_req_base           =   SYSCTL_DMA_SELECT_I2C2_RX_REQ,
                .I2CCHAN_uiChannel      =   2,
                .I2CCHAN_ulPhyAddrBase  =   I2C2_BASE_ADDR,
                .I2CCHAN_uiBusFreq      =   __I2C_BUS_FREQ_MIN,
                .I2CCHAN_SpeedMode      =   I2C_BS_STANDARD,
                .I2CCHAN_ulIrqVector    =   KENDRYTE_PLIC_VECTOR(IRQN_I2C2_INTERRUPT),
        }
};
/*********************************************************************************************************
** ��������: i2c_slave_receive
** ��������: Ĭ�ϵ� i2c slave �ӻ�����
** �䡡��  : device          slave_info_t ���豸��Ϣ
**           data            i2c �ӻ����յ�������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID i2c_slave_receive (VOID  *device, UINT32  data)
{
    struct slave_info_t *slave_device = device;

    if (slave_device->acces_reg == 0xFF)
    {
        if (data < SLAVE_MAX_ADDR)
            slave_device->acces_reg = data;
    }
    else if (slave_device->acces_reg < SLAVE_MAX_ADDR)
    {
        slave_device->reg_data[slave_device->acces_reg] = data;
        slave_device->acces_reg++;
    }
}
/*********************************************************************************************************
** ��������: i2c_slave_transmit
** ��������: Ĭ�ϵ� i2c slave �ӻ�����
** �䡡��  : device          slave_info_t ���豸��Ϣ
**           event           i2c �ӻ��¼�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT32 i2c_slave_transmit (VOID  *device)
{
    UINT32                ret          = 0;
    struct slave_info_t  *slave_device = device;

    if (slave_device->acces_reg >= SLAVE_MAX_ADDR) {
        slave_device->acces_reg = 0;
    }

    ret = slave_device->reg_data[slave_device->acces_reg];
    slave_device->acces_reg++;

    return ret;
}
/*********************************************************************************************************
** ��������: i2c_slave_event
** ��������: Ĭ�ϵ� i2c slave �¼�����ص�����
** �䡡��  : device          slave_info_t ���豸��Ϣ
**           event           i2c �ӻ��¼�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID i2c_slave_event (VOID  *device, i2c_event_t  event)
{
    struct slave_info_t  *slave_device = device;

    if(I2C_EV_START == event)
    {
        if (slave_device->acces_reg >= SLAVE_MAX_ADDR) {
            slave_device->acces_reg = 0xFF;
        }
    }
    else if(I2C_EV_STOP == event)
    {
        slave_device->acces_reg = 0xFF;
    }
}
/*********************************************************************************************************
** ��������: i2c_slave_set_clock_rate
** ��������: i2c��slaveģʽ�µĴ���ʱ�����ú���
** �䡡��  : pI2cChannel     i2c ͨ��
**           clock_rate      ʱ��Ƶ��
** �䡡��  : ��ɴ������Ϣ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static double i2c_slave_set_clock_rate (__PK210_I2C_CHANNEL  pI2cChannel, unsigned long  clock_rate)
{
    UINT32           hcnt;
    UINT32           lcnt;
    UINT32           uiI2cFreqValue;
    UINT16           uiClkPeriodCount;
    volatile i2c_t  *pI2cRegHandler = (i2c_t *)pI2cChannel->I2CCHAN_ulPhyAddrBase;

    if (pI2cRegHandler == LW_NULL) {
        printk(KERN_ERR "i2c_slave_set_clock_rate(): invalid pI2cRegHandler.\n");
        return  (0);
    }

    uiI2cFreqValue   = sysctl_clock_get_freq(pI2cChannel->clock);
    uiClkPeriodCount = uiI2cFreqValue / clock_rate / 2;

    if (uiClkPeriodCount == 0) {
        uiClkPeriodCount = 1;
    }

    hcnt = uiClkPeriodCount;
    lcnt = uiClkPeriodCount;

    clock_rate =  uiI2cFreqValue / uiClkPeriodCount * 2;

    pI2cRegHandler->ss_scl_hcnt = I2C_SS_SCL_HCNT_COUNT(hcnt);
    pI2cRegHandler->ss_scl_lcnt = I2C_SS_SCL_LCNT_COUNT(lcnt);

    return clock_rate;
}
/*********************************************************************************************************
** ��������: on_i2c_irq
** ��������: i2c��slaveģʽ�µ��жϴ�����
** �䡡��  : pI2cChannel     i2c ͨ��
**           slave_address   �ӻ���ַ
** �䡡��  : ��ɴ������Ϣ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static irqreturn_t on_i2c_irq (PVOID  dev, ULONG  irq)
{
    UINT32                status;
    UINT32                dummy;
    __PK210_I2C_CHANNEL     pI2cChannel;
    volatile i2c_t         *pI2cRegHandler;
    i2c_slave_handler_t    *pSlaveHandler;

    pI2cChannel    = (__PK210_I2C_CHANNEL)dev;
    pI2cRegHandler = (i2c_t *)pI2cChannel->I2CCHAN_ulPhyAddrBase;

    if (pI2cRegHandler == LW_NULL) {
        printk(KERN_ERR "on_i2c_irq(): invalid pI2cRegHandler.\n");
        return  (LW_IRQ_NONE);
    }

    status        = pI2cRegHandler->intr_stat;
    pSlaveHandler = &(pI2cChannel->I2CCHAN_i2cSlaveHandle);

    if (pSlaveHandler == LW_NULL) {
        printk(KERN_ERR "on_i2c_irq(): invalid pI2cRegHandler.\n");
        return  (LW_IRQ_NONE);
    }

    if (status & I2C_INTR_STAT_START_DET)
    {
        pSlaveHandler->on_event(&pI2cChannel->slave_device, I2C_EV_START);
        dummy = pI2cRegHandler->clr_start_det;
    }

    if (status & I2C_INTR_STAT_STOP_DET)
    {
        pSlaveHandler->on_event(&pI2cChannel->slave_device, I2C_EV_STOP);
        dummy = pI2cRegHandler->clr_stop_det;
    }

    if (status & I2C_INTR_STAT_RX_FULL)
    {
        pSlaveHandler->on_receive(&pI2cChannel->slave_device, pI2cRegHandler->data_cmd);
    }

    if (status & I2C_INTR_STAT_RD_REQ)
    {
        pI2cRegHandler->data_cmd = pSlaveHandler->on_transmit(&pI2cChannel->slave_device);
        dummy = pI2cRegHandler->clr_rd_req;
    }

    (void)dummy;

    return LW_IRQ_HANDLED;
}
/*********************************************************************************************************
** ��������: i2c_config_as_slave    (ע��: �ú���������i2c_config_as_slave֮�󱻵���)
** ��������: ���ÿ�����Ϊslaveģʽ
** �䡡��  : pI2cChannel     i2c ͨ��
**           slave_address   �ӻ���ַ
** �䡡��  : ��ɴ������Ϣ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID i2c_config_as_slave (__PK210_I2C_CHANNEL  pI2cChannel,
                                 UINT32               slave_address,
                                 UINT32               address_width)
{
    int                     speed_mode;
    i2c_bus_speed_mode_t    bus_speed_mode;
    volatile i2c_t         *pI2cRegHandler;
    CHAR                    cBuffer[64];

    pI2cChannel->I2CCHAN_bIsSlaveMode   =  LW_TRUE;

    pI2cChannel->I2CCHAN_i2cSlaveHandle.on_event    = i2c_slave_event;
    pI2cChannel->I2CCHAN_i2cSlaveHandle.on_receive  = i2c_slave_receive;
    pI2cChannel->I2CCHAN_i2cSlaveHandle.on_transmit = i2c_slave_transmit;

    lib_memset(&pI2cChannel->slave_device, 0, sizeof(pI2cChannel->slave_device));
    pI2cChannel->slave_device.acces_reg =  0xFF;

    pI2cRegHandler = (i2c_t *)pI2cChannel->I2CCHAN_ulPhyAddrBase;

    if (pI2cRegHandler == LW_NULL) {
        printk(KERN_ERR "i2c_config_as_slave(): invalid pI2cRegHandler.\n");
        return;
    }

    bus_speed_mode = pI2cChannel->I2CCHAN_SpeedMode;

    if ((address_width != 7) && (address_width != 10)) {
        printk(KERN_ERR "i2c_config_as_slave(): invalid address_width.\n");
        return;
    }

    switch (bus_speed_mode)
    {
    case I2C_BS_STANDARD:
        speed_mode = 1;
        break;
    default:
        printk(KERN_ERR "I2C bus speed is not supported.\n");
        break;
    }

    pI2cRegHandler->enable = 0;                                         /*      �ر� I2C ģ��           */

    pI2cRegHandler->con = (address_width == 10 ? I2C_CON_10BITADDR_SLAVE : 0) |
                          I2C_CON_SPEED(speed_mode) | I2C_CON_STOP_DET_IFADDRESSED;

    pI2cRegHandler->ss_scl_hcnt = I2C_SS_SCL_HCNT_COUNT(37);
    pI2cRegHandler->ss_scl_lcnt = I2C_SS_SCL_LCNT_COUNT(40);

    pI2cRegHandler->sar   = I2C_SAR_ADDRESS(slave_address);
    pI2cRegHandler->rx_tl = I2C_RX_TL_VALUE(0);
    pI2cRegHandler->tx_tl = I2C_TX_TL_VALUE(0);

    pI2cRegHandler->intr_mask = I2C_INTR_MASK_RX_FULL  | I2C_INTR_MASK_START_DET |
                                I2C_INTR_MASK_STOP_DET | I2C_INTR_MASK_RD_REQ;

    API_InterVectorSetPriority(pI2cChannel->I2CCHAN_ulIrqVector, 100);
    snprintf(cBuffer, sizeof(cBuffer), __I2C_CHANNEL_ISR_FMT, pI2cChannel->I2CCHAN_uiChannel);
    API_InterVectorConnect(pI2cChannel->I2CCHAN_ulIrqVector, on_i2c_irq, (void *)pI2cChannel, cBuffer);
    API_InterVectorEnable(pI2cChannel->I2CCHAN_ulIrqVector);

    pI2cRegHandler->enable = I2C_ENABLE_ENABLE;                         /*      ʹ�� I2C ģ��           */
}
/*********************************************************************************************************
   ���ϴ���ο��� kendryte-freertos-sdk ����ѭSylixOS����淶
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: __ls1xI2cTryTransfer
** ��������: i2c ���Դ��亯��
** �䡡��  : pI2cChannel     i2c ͨ��
**           pI2cAdapter     i2c ������
**           pI2cMsg         i2c ������Ϣ��
**           iNum            ��Ϣ����
** �䡡��  : ��ɴ������Ϣ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210I2cTryTransfer (__PK210_I2C_CHANNEL    pI2cChannel,
                                  PLW_I2C_ADAPTER        pI2cAdapter,
                                  PLW_I2C_MESSAGE        pI2cMsg,
                                  INT                    iNum)
{
    UINT32          uiI2cFreqValue;
    UINT16          uiClkPeriodCount;
    INT             iSpeedMode;
    INT             iIndex    = 0;
    INT             iRet      = 0;
    volatile i2c_t *pI2cRegHandler = (i2c_t *)pI2cChannel->I2CCHAN_ulPhyAddrBase;

    /*
     * Configure I2C bus frequency.
     */
    uiI2cFreqValue   =  sysctl_clock_get_freq(SYSCTL_CLOCK_I2C0 + pI2cChannel->I2CCHAN_uiChannel);
    uiClkPeriodCount =  uiI2cFreqValue / pI2cChannel->I2CCHAN_uiBusFreq / 2;

    if(uiClkPeriodCount == 0) {
        uiClkPeriodCount = 1;
    }

    switch (pI2cChannel->I2CCHAN_SpeedMode) {
    case I2C_BS_STANDARD:
        iSpeedMode = 1;
        break;
    default:
        printk(KERN_ERR "I2C bus speed is not supported.\n");
        break;
    }

    API_SpinLock(&pI2cChannel->I2CCHAN_lock);

    /*
     * Put I2C in reset/disabled state.
     */
    pI2cRegHandler->enable = 0;

    pI2cRegHandler->con = I2C_CON_MASTER_MODE | I2C_CON_SLAVE_DISABLE | I2C_CON_RESTART_EN |
                          (pI2cMsg->I2CMSG_usFlag & LW_I2C_M_TEN  ? I2C_CON_10BITADDR_SLAVE : 0) |
                          I2C_CON_SPEED(iSpeedMode);
    pI2cRegHandler->ss_scl_hcnt = I2C_SS_SCL_HCNT_COUNT(uiClkPeriodCount);
    pI2cRegHandler->ss_scl_lcnt = I2C_SS_SCL_LCNT_COUNT(uiClkPeriodCount);

    pI2cRegHandler->tar = I2C_TAR_ADDRESS(pI2cMsg->I2CMSG_usAddr);      /*  Set I2C slave address.      */

    pI2cRegHandler->intr_mask = 0;                                      /*  I2C interrupt setting.      */

    pI2cRegHandler->dma_cr    = 0x3;                                    /*  I2C dma setting.            */
    pI2cRegHandler->dma_rdlr  = 0;
    pI2cRegHandler->dma_tdlr  = 4;

    pI2cRegHandler->enable = I2C_ENABLE_ENABLE;                         /*  Bring I2C out of reset.     */

    API_SpinUnlock(&pI2cChannel->I2CCHAN_lock);

    for (iIndex = 0; iIndex < iNum; iIndex++, pI2cMsg++) {

        if (pI2cMsg->I2CMSG_usFlag & LW_I2C_M_RD) {
            API_SpinLock(&pI2cChannel->I2CCHAN_lock);
            iRet = i2c_recv_data(pI2cChannel->I2CCHAN_uiChannel, LW_NULL, 0,
                                 pI2cMsg->I2CMSG_pucBuffer, pI2cMsg->I2CMSG_usLen);
            API_SpinUnlock(&pI2cChannel->I2CCHAN_lock);
        } else {
            API_SpinLock(&pI2cChannel->I2CCHAN_lock);
            iRet = i2c_send_data(pI2cChannel->I2CCHAN_uiChannel,
                                 pI2cMsg->I2CMSG_pucBuffer, pI2cMsg->I2CMSG_usLen);
            API_SpinUnlock(&pI2cChannel->I2CCHAN_lock);
        }

        if (iRet) {
            printk("__k210I2cTryTransfer: failed occur! \r\n");
            break;
        }
    }

    return  (iIndex);
}
/*********************************************************************************************************
** ��������: __ls1xI2cTransfer
** ��������: i2c ���亯��
** �䡡��  : pI2cChannel     i2c ͨ��
**           pI2cAdapter     i2c ������
**           pI2cMsg         i2c ������Ϣ��
**           iNum            ��Ϣ����
** �䡡��  : ��ɴ������Ϣ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210I2cTransfer (__PK210_I2C_CHANNEL  pI2cChannel,
                               PLW_I2C_ADAPTER      pI2cAdapter,
                               PLW_I2C_MESSAGE      pI2cMsg,
                               INT                  iNum)
{
    REGISTER INT     i;

    /*
     * ����Ѿ�������Ϊ�ӻ�ģʽ����ʹ�øýӿ�
     */
    if (pI2cChannel->I2CCHAN_bIsSlaveMode) {
        printk("__k210I2cTransfer: I2cChannel = %d has been configured as slave\r\n",
               pI2cChannel->I2CCHAN_uiChannel);
        return  (PX_ERROR);
    }

    for (i = 0; i < pI2cAdapter->I2CADAPTER_iRetry; i++) {
         if (__k210I2cTryTransfer(pI2cChannel, pI2cAdapter, pI2cMsg, iNum) == iNum) {
             return  (iNum);
         } else {
             API_TimeSleep(LW_OPTION_WAIT_A_TICK);                      /*  �ȴ�һ��������������        */
         }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __k210I2c0Transfer
** ��������: i2c0 ���亯��
** �䡡��  : pI2cAdapter     i2c ������
**           pI2cMsg         i2c ������Ϣ��
**           iNum            ��Ϣ����
** �䡡��  : ��ɴ������Ϣ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210I2c0Transfer (PLW_I2C_ADAPTER  pI2cAdapter, PLW_I2C_MESSAGE  pI2cMsg, INT  iNum)
{
    return  (__k210I2cTransfer(&_G_k210I2cChannels[0], pI2cAdapter, pI2cMsg, iNum));
}
/*********************************************************************************************************
** ��������: __k210I2c1Transfer
** ��������: i2c1 ���亯��
** �䡡��  : pI2cAdapter     i2c ������
**           pI2cMsg         i2c ������Ϣ��
**           iNum            ��Ϣ����
** �䡡��  : ��ɴ������Ϣ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210I2c1Transfer (PLW_I2C_ADAPTER  pI2cAdapter, PLW_I2C_MESSAGE  pI2cMsg, INT  iNum)
{
    return  (__k210I2cTransfer(&_G_k210I2cChannels[1], pI2cAdapter, pI2cMsg, iNum));
}
/*********************************************************************************************************
** ��������: __k210I2c2Transfer
** ��������: i2c2 ���亯��
** �䡡��  : pI2cAdapter     i2c ������
**           pI2cMsg         i2c ������Ϣ��
**           iNum            ��Ϣ����
** �䡡��  : ��ɴ������Ϣ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210I2c2Transfer (PLW_I2C_ADAPTER  pI2cAdapter, PLW_I2C_MESSAGE  pI2cMsg, INT  iNum)
{
    return  (__k210I2cTransfer(&_G_k210I2cChannels[2], pI2cAdapter, pI2cMsg, iNum));
}
/*********************************************************************************************************
** ��������: __k210I2cMasterCtl
** ��������: i2c ���ƺ���
** �䡡��  : pI2cChannel     i2c ͨ��
**           pI2cAdapter     i2c ������
**           iCmd            ����
**           lArg            ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210I2cMasterCtl (__PK210_I2C_CHANNEL  pI2cChannel,
                                PLW_I2C_ADAPTER      pI2cAdapter,
                                INT                  iCmd,
                                LONG                 lArg)
{
    INT                  iRet = ERROR_NONE;
    i2c_slave_config_t  *pSlaveConfig;

    if (!pI2cChannel) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Invalid adapter address.\r\n");
        return  (PX_ERROR);
    }

    switch (iCmd) {

        case CONFIG_I2C_AS_SLAVE:
            pSlaveConfig = (i2c_slave_config_t  *)lArg;
            if (pSlaveConfig == LW_NULL) {
                printk(KERN_ERR "__k210I2cMasterCtl(): CONFIG_I2C_AS_SLAVE invalid pSlaveConfig.\n");
                return  (PX_ERROR);
            }
            API_SpinLock(&pI2cChannel->I2CCHAN_lock);
            i2c_config_as_slave(pI2cChannel, pSlaveConfig->slave_address, pSlaveConfig->address_width);
            API_SpinUnlock(&pI2cChannel->I2CCHAN_lock);
            break;

        case CONFIG_I2C_SLAVE_CLOCK_RATE:
            API_SpinLock(&pI2cChannel->I2CCHAN_lock);
            i2c_slave_set_clock_rate(pI2cChannel, lArg);
            API_SpinUnlock(&pI2cChannel->I2CCHAN_lock);
            break;

        case CONFIG_I2C_AS_MASTER:
            API_SpinLock(&pI2cChannel->I2CCHAN_lock);
            pI2cChannel->I2CCHAN_bIsSlaveMode = LW_FALSE;
            API_SpinUnlock(&pI2cChannel->I2CCHAN_lock);
            break;

        default:
            iRet = PX_ERROR;
            break;
    }

    return  (iRet);
}
/*********************************************************************************************************
** ��������: __k210I2c0MasterCtl
** ��������: i2c0 ���ƺ���
** �䡡��  : pI2cAdapter     i2c ������
**           iCmd            ����
**           lArg            ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210I2c0MasterCtl (PLW_I2C_ADAPTER  pI2cAdapter, INT  iCmd, LONG  lArg)
{
    return  (__k210I2cMasterCtl(&_G_k210I2cChannels[0], pI2cAdapter, iCmd, lArg));
}
/*********************************************************************************************************
** ��������: __k210I2c1MasterCtl
** ��������: i2c1 ���ƺ���
** �䡡��  : pI2cAdapter     i2c ������
**           iCmd            ����
**           lArg            ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210I2c1MasterCtl (PLW_I2C_ADAPTER  pI2cAdapter, INT  iCmd, LONG  lArg)
{
    return  (__k210I2cMasterCtl(&_G_k210I2cChannels[1], pI2cAdapter, iCmd, lArg));
}
/*********************************************************************************************************
** ��������: __k210I2c2MasterCtl
** ��������: i2c2 ���ƺ���
** �䡡��  : pI2cAdapter     i2c ������
**           iCmd            ����
**           lArg            ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210I2c2MasterCtl (PLW_I2C_ADAPTER  pI2cAdapter, INT  iCmd, LONG  lArg)
{
    return  (__k210I2cMasterCtl(&_G_k210I2cChannels[2], pI2cAdapter, iCmd, lArg));
}
/*********************************************************************************************************
** ��������: __k210I2cInit
** ��������: ��ʼ�� i2c ͨ��
** �䡡��  : pI2cChannel     i2c ͨ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210I2cInit (__PK210_I2C_CHANNEL  pI2cChannel)
{
    if (!pI2cChannel->I2CCHAN_bIsInit) {

        pI2cChannel->I2CCHAN_hSignal = API_SemaphoreBCreate("i2c_signal",
                                                            LW_FALSE,
                                                            LW_OPTION_OBJECT_GLOBAL, LW_NULL);
        if (pI2cChannel->I2CCHAN_hSignal == LW_OBJECT_HANDLE_INVALID) {
            printk(KERN_ERR "__ls1xI2cInit(): failed to create i2c_signal!\n");
            goto  __error_handle;
        }

        if (API_SpinInit(&pI2cChannel->I2CCHAN_lock) != ERROR_NONE) {
            printk(KERN_ERR "__sifiveSpiInit(): failed to create SPICHAN_lock!\n");
            goto  __error_handle;
        }

        /*
         * __k210I2cHwInit: GPIO clock under APB0 clock, so enable APB0 clock firstly
         */
        sysctl_clock_enable(pI2cChannel->clock);
        sysctl_clock_set_threshold(pI2cChannel->threshold, 3);

        pI2cChannel->I2CCHAN_bIsInit = LW_TRUE;
    }

    return  (ERROR_NONE);

__error_handle:
    if (pI2cChannel->I2CCHAN_hSignal) {
        API_SemaphoreBDelete(&pI2cChannel->I2CCHAN_hSignal);
        pI2cChannel->I2CCHAN_hSignal = 0;
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
  K210 ������ I2C ��������
*********************************************************************************************************/
static LW_I2C_FUNCS  _G_k210I2cFuncs[__I2C_CHANNEL_NUM] = {
        {
                __k210I2c0Transfer,
                __k210I2c0MasterCtl,
        },
        {
                __k210I2c1Transfer,
                __k210I2c1MasterCtl,
        },
        {
                __k210I2c2Transfer,
                __k210I2c2MasterCtl,
        }
};
/*********************************************************************************************************
** ��������: i2cBusFuns
** ��������: ��ʼ�� i2c ���߲���ȡ��������
** �䡡��  : iChan         ͨ����
**
** �䡡��  : i2c ������������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PLW_I2C_FUNCS  i2cBusFuncs (UINT uiChannel)
{
    if (uiChannel >= __I2C_CHANNEL_NUM) {
        printk(KERN_ERR "i2cBusFuncs(): I2C channel invalid!\r\n");
        return  (LW_NULL);
    }

    if (__k210I2cInit(&_G_k210I2cChannels[uiChannel]) != ERROR_NONE) {
        return  (LW_NULL);
    }

    return  (&_G_k210I2cFuncs[uiChannel]);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
