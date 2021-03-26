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
** 文   件   名: k210_i2c.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 10 月 19 日
**
** 描        述: I2C 控制器驱动
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
  宏定义
*********************************************************************************************************/
#define __I2C_CHANNEL_NUM         (3)
#define __I2C_BUS_FREQ_MAX        (400000)
#define __I2C_BUS_FREQ_NORMAL     (200000)
#define __I2C_BUS_FREQ_MIN        (100000)
#define __I2C_ADDRESS_7BITS       (7)
#define __I2C_ADDRESS_10BITS      (10)
#define __I2C_CHANNEL_ISR_FMT     "i2c%d_isr"
/*********************************************************************************************************
  slave handler 和 device 定义 (注意: 相关结构体参考自 kendryte-standalone-sdk)
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
  i2c 通道类型定义
*********************************************************************************************************/
typedef struct {
    sysctl_clock_t         clock;
    sysctl_threshold_t     threshold;
    sysctl_dma_select_t    dma_req_base;

    addr_t                 I2CCHAN_ulPhyAddrBase;                       /*  物理地址基地址              */
    UINT                   I2CCHAN_uiBusFreq;                           /*  I2C SCL speed               */
    UINT                   I2CCHAN_SpeedMode;                           /*  i2c_bus_speed_mode          */
    ULONG                  I2CCHAN_ulIrqVector;                         /*  I2C 中断向量                */
    UINT                   I2CCHAN_uiChannel;                           /*  I2C 通道编号                */
    BOOL                   I2CCHAN_bIsInit;                             /*  是否已经初始化              */
    spinlock_t             I2CCHAN_lock;                                /*  互斥锁                      */
    PUCHAR                 I2CCHAN_pucBuffer;                           /*  数据缓冲区                  */
    UINT                   I2CCHAN_uiBufferLen;                         /*  数据缓冲区长度              */

    BOOL                   I2CCHAN_bIsSlaveMode;                        /*  是否为从机模式              */
    LW_HANDLE              I2CCHAN_hSignal;                             /*  同步信号量                  */
    i2c_slave_handler_t    I2CCHAN_i2cSlaveHandle;                      /*  从机模式时的事件处理函数    */

    struct slave_info_t    slave_device;

} __K210_I2C_CHANNEL, *__PK210_I2C_CHANNEL;
/*********************************************************************************************************
  全局变量 (如果有设备树，请动态分配内存，并解析设备树，将解析得到的硬件保存在该结构体中，作为pri_data)
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
** 函数名称: i2c_slave_receive
** 功能描述: 默认的 i2c slave 从机接收
** 输　入  : device          slave_info_t 从设备信息
**           data            i2c 从机接收到的数据
** 输　出  : NONE
** 全局变量:
** 调用模块:
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
** 函数名称: i2c_slave_transmit
** 功能描述: 默认的 i2c slave 从机发送
** 输　入  : device          slave_info_t 从设备信息
**           event           i2c 从机事件
** 输　出  : NONE
** 全局变量:
** 调用模块:
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
** 函数名称: i2c_slave_event
** 功能描述: 默认的 i2c slave 事件处理回调函数
** 输　入  : device          slave_info_t 从设备信息
**           event           i2c 从机事件
** 输　出  : NONE
** 全局变量:
** 调用模块:
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
** 函数名称: i2c_slave_set_clock_rate
** 功能描述: i2c在slave模式下的传输时钟配置函数
** 输　入  : pI2cChannel     i2c 通道
**           clock_rate      时钟频率
** 输　出  : 完成传输的消息数量
** 全局变量:
** 调用模块:
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
** 函数名称: on_i2c_irq
** 功能描述: i2c在slave模式下的中断处理函数
** 输　入  : pI2cChannel     i2c 通道
**           slave_address   从机地址
** 输　出  : 完成传输的消息数量
** 全局变量:
** 调用模块:
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
** 函数名称: i2c_config_as_slave    (注意: 该函数必须在i2c_config_as_slave之后被调用)
** 功能描述: 配置控制器为slave模式
** 输　入  : pI2cChannel     i2c 通道
**           slave_address   从机地址
** 输　出  : 完成传输的消息数量
** 全局变量:
** 调用模块:
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

    pI2cRegHandler->enable = 0;                                         /*      关闭 I2C 模块           */

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

    pI2cRegHandler->enable = I2C_ENABLE_ENABLE;                         /*      使能 I2C 模块           */
}
/*********************************************************************************************************
   以上代码参考自 kendryte-freertos-sdk 不遵循SylixOS代码规范
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: __ls1xI2cTryTransfer
** 功能描述: i2c 尝试传输函数
** 输　入  : pI2cChannel     i2c 通道
**           pI2cAdapter     i2c 适配器
**           pI2cMsg         i2c 传输消息组
**           iNum            消息数量
** 输　出  : 完成传输的消息数量
** 全局变量:
** 调用模块:
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
** 函数名称: __ls1xI2cTransfer
** 功能描述: i2c 传输函数
** 输　入  : pI2cChannel     i2c 通道
**           pI2cAdapter     i2c 适配器
**           pI2cMsg         i2c 传输消息组
**           iNum            消息数量
** 输　出  : 完成传输的消息数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210I2cTransfer (__PK210_I2C_CHANNEL  pI2cChannel,
                               PLW_I2C_ADAPTER      pI2cAdapter,
                               PLW_I2C_MESSAGE      pI2cMsg,
                               INT                  iNum)
{
    REGISTER INT     i;

    /*
     * 如果已经被配置为从机模式则不能使用该接口
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
             API_TimeSleep(LW_OPTION_WAIT_A_TICK);                      /*  等待一个机器周期重试        */
         }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __k210I2c0Transfer
** 功能描述: i2c0 传输函数
** 输　入  : pI2cAdapter     i2c 适配器
**           pI2cMsg         i2c 传输消息组
**           iNum            消息数量
** 输　出  : 完成传输的消息数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210I2c0Transfer (PLW_I2C_ADAPTER  pI2cAdapter, PLW_I2C_MESSAGE  pI2cMsg, INT  iNum)
{
    return  (__k210I2cTransfer(&_G_k210I2cChannels[0], pI2cAdapter, pI2cMsg, iNum));
}
/*********************************************************************************************************
** 函数名称: __k210I2c1Transfer
** 功能描述: i2c1 传输函数
** 输　入  : pI2cAdapter     i2c 适配器
**           pI2cMsg         i2c 传输消息组
**           iNum            消息数量
** 输　出  : 完成传输的消息数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210I2c1Transfer (PLW_I2C_ADAPTER  pI2cAdapter, PLW_I2C_MESSAGE  pI2cMsg, INT  iNum)
{
    return  (__k210I2cTransfer(&_G_k210I2cChannels[1], pI2cAdapter, pI2cMsg, iNum));
}
/*********************************************************************************************************
** 函数名称: __k210I2c2Transfer
** 功能描述: i2c2 传输函数
** 输　入  : pI2cAdapter     i2c 适配器
**           pI2cMsg         i2c 传输消息组
**           iNum            消息数量
** 输　出  : 完成传输的消息数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210I2c2Transfer (PLW_I2C_ADAPTER  pI2cAdapter, PLW_I2C_MESSAGE  pI2cMsg, INT  iNum)
{
    return  (__k210I2cTransfer(&_G_k210I2cChannels[2], pI2cAdapter, pI2cMsg, iNum));
}
/*********************************************************************************************************
** 函数名称: __k210I2cMasterCtl
** 功能描述: i2c 控制函数
** 输　入  : pI2cChannel     i2c 通道
**           pI2cAdapter     i2c 适配器
**           iCmd            命令
**           lArg            参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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
** 函数名称: __k210I2c0MasterCtl
** 功能描述: i2c0 控制函数
** 输　入  : pI2cAdapter     i2c 适配器
**           iCmd            命令
**           lArg            参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210I2c0MasterCtl (PLW_I2C_ADAPTER  pI2cAdapter, INT  iCmd, LONG  lArg)
{
    return  (__k210I2cMasterCtl(&_G_k210I2cChannels[0], pI2cAdapter, iCmd, lArg));
}
/*********************************************************************************************************
** 函数名称: __k210I2c1MasterCtl
** 功能描述: i2c1 控制函数
** 输　入  : pI2cAdapter     i2c 适配器
**           iCmd            命令
**           lArg            参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210I2c1MasterCtl (PLW_I2C_ADAPTER  pI2cAdapter, INT  iCmd, LONG  lArg)
{
    return  (__k210I2cMasterCtl(&_G_k210I2cChannels[1], pI2cAdapter, iCmd, lArg));
}
/*********************************************************************************************************
** 函数名称: __k210I2c2MasterCtl
** 功能描述: i2c2 控制函数
** 输　入  : pI2cAdapter     i2c 适配器
**           iCmd            命令
**           lArg            参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210I2c2MasterCtl (PLW_I2C_ADAPTER  pI2cAdapter, INT  iCmd, LONG  lArg)
{
    return  (__k210I2cMasterCtl(&_G_k210I2cChannels[2], pI2cAdapter, iCmd, lArg));
}
/*********************************************************************************************************
** 函数名称: __k210I2cInit
** 功能描述: 初始化 i2c 通道
** 输　入  : pI2cChannel     i2c 通道
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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
  K210 处理器 I2C 驱动程序
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
** 函数名称: i2cBusFuns
** 功能描述: 初始化 i2c 总线并获取驱动程序
** 输　入  : iChan         通道号
**
** 输　出  : i2c 总线驱动程序
** 全局变量:
** 调用模块:
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
