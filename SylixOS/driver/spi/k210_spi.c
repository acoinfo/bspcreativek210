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
** 文   件   名: k210_spi.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 10 月 22 日
**
** 描        述: SPI 控制器驱动
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "SylixOS.h"
#include "config.h"
#include <linux/compat.h>
#include "k210_spi.h"
#include "driver/clock/k210_clock.h"
#include "driver/dmac/k210_dma.h"
#include "driver/common.h"
#include "driver/fix_arch_def.h"
#include "driver/spi_sdi/sdcard.h"

#include "KendryteWare/include/spi.h"
#include "KendryteWare/include/plic.h"
/*********************************************************************************************************
  SPI 寄存器偏移定义
*********************************************************************************************************/
#define  SPI_REG_CTRLR0             (0x00)
#define  SPI_REG_CTRLR1             (0x04)
#define  SPI_REG_SSIENR             (0x08)
#define  SPI_REG_MWCR               (0x0c)
#define  SPI_REG_SER                (0x10)
#define  SPI_REG_BAUDR              (0x14)
#define  SPI_REG_TXFTLR             (0x18)
#define  SPI_REG_RXFTLR             (0x1c)
#define  SPI_REG_TXFLR              (0x20)
#define  SPI_REG_RXFLR              (0x24)
#define  SPI_REG_SR                 (0x28)
#define  SPI_REG_IMR                (0x2c)
#define  SPI_REG_ISR                (0x30)
#define  SPI_REG_RISR               (0x34)
#define  SPI_REG_TXOICR             (0x38)
#define  SPI_REG_RXOICR             (0x3c)
#define  SPI_REG_RXUICR             (0x40)
#define  SPI_REG_MSTICR             (0x44)
#define  SPI_REG_ICR                (0x48)
#define  SPI_REG_DMACR              (0x4c)
#define  SPI_REG_DMATDLR            (0x50)
#define  SPI_REG_DMARDLR            (0x54)
#define  SPI_REG_IDR                (0x58)
#define  SPI_REG_SSIC_VERSION_ID    (0x5c)
#define  SPI_REG_DR                 (0x60)
#define  SPI_REG_RX_SAMPLE_DELAY    (0xf0)
#define  SPI_REG_SPI_CTRLR0         (0xf4)
#define  SPI_REG_XIP_MODE_BITS      (0xfc)
#define  SPI_REG_XIP_INCR_INST      (0x100)
#define  SPI_REG_XIP_WRAP_INST      (0x104)
#define  SPI_REG_XIP_CTRL           (0x108)
#define  SPI_REG_XIP_SER            (0x10c)
#define  SPI_REG_XRXOICR            (0x110)
#define  SPI_REG_XIP_CNT_TIME_OUT   (0x114)
/*********************************************************************************************************
  SPI 相关宏定义
*********************************************************************************************************/
#define __SPI_CHANNEL_NR            (4)                                 /*  SPI channel numbers.        */
#define __SPI_MAX_BAUDRATE          (25000000)                          /*  ! SPI3 max freq is 100MHz   */
#define __SPI_FIFO_DEEPTH           (32)                                /*  SPI FIFO max deepth         */
#define TMOD_MASK(data)             (3 << data->tmod_off)               /*  移位数据到指定偏移处        */
#define TMOD_VALUE(data, value)     (value << data->tmod_off)           /*  移位数据到指定偏移处        */
/*********************************************************************************************************
  裸机库类型重命名
*********************************************************************************************************/
typedef spi_instruction_address_trans_mode_t    k210_spi_ins_addr_t;    /*  类型名重定义                */
/*********************************************************************************************************
  SPI 相关宏函数定义
*********************************************************************************************************/
#define WAIT_TRANS_OK   while((spi_handle->sr & 0x05) != 0x04);

#define COMMON_ENTRY(data)                                                          \
        volatile spi_t *const spi = (volatile spi_t *)(data->SPICHAN_phyAddrBase);  \
        (void)spi
/*********************************************************************************************************
  SPI 控制器结构体定义
*********************************************************************************************************/
typedef struct k210_spi {
    sysctl_clock_t      clock;                                          /*  参照  Kendryte-SDK 源码     */
    sysctl_dma_select_t dma_req_base;                                   /*  参照  Kendryte-SDK 源码     */

    addr_t              SPICHAN_phyAddrBase;                            /*  SPI physical base address   */
    UINT32              SPICHAN_uiVector;                               /*  SPI interrupt irq vector    */
    UINT32              SPICHAN_baudRate;                               /*  SPI transfer bus clock      */
    UINT                SPICHAN_uiChannelId;                            /*  SPI channel id              */
    UINT                SPICHAN_uiCsPinNum;                             /*  SPI chip select pins number */
    UINT                SPICHAN_uiFifoDepth;                            /*  SPI chip fifo buf depth     */
    BOOL                SPICHAN_bIsInit;                                /*  SPI channel whether init    */
    BOOL                SPICHAN_bIsSlaveMode;                           /*  SPI whether work in slave   */

    spinlock_t          SPICHAN_lock;                                   /*  SPI spinlock for SMP safety */
    spi_work_mode_t     SPICHAN_workMode;                               /*  SPI working mode POL & PHA  */
    spi_frame_format_t  SPICHAN_frameFormat;                            /*  SPI transfer frame format   */
    size_t              SPICHAN_dataBitLength;                          /*  SPI frame bit-size          */
    size_t              SPICHAN_chipSelectLine;                         /*  SPI default CS pin          */
    BOOL                SPICHAN_endian;                                 /*  data endian                 */
    BOOL                SPICHAN_bIsCfgChanged;                          /*  SPI working mode changed    */
                                                                        /*  以下对应寄存器 SPI_CTRLR0   */
    size_t              SPICHAN_stInstLen;                              /*  指令长度(bit)               */
    size_t              SPICHAN_stAddrLen;                              /*  地址长度(bit)               */
    size_t              SPICHAN_stInstWidth;                            /*  指令长度(byte)              */
    size_t              SPICHAN_stAddrWidth;                            /*  地址长度(byte)              */
    k210_spi_ins_addr_t SPICHAN_transMode;                              /*  地址和指令传输格式          */
    size_t              SPICHAN_stWaitCycles;                           /*  控制帧发送和数据接收之间的  */
                                                                        /*  CTRLR0 位偏移(参考自SDK)    */
    UINT8               tmod_off;                                       /*  CTRLR0[9:8]: TMOD 传输模式  */
    UINT8               mod_off;                                        /*  CTRLR0[7:6]: SCPOL & SCPH   */
    UINT8               frf_off;                                        /*  CTRLR0[5:4]: FRF 帧格式     */
    UINT8               dfs_off;                                        /*  CTRLR0[3:0]: DFS 数据框大小 */

    LW_OBJECT_HANDLE    SPICHAN_hSignal;                                /* Wake up from interrupt       */
} __K210_SPI_CHANNEL, *__PK210_SPI_CHANNEL;
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
static __K210_SPI_CHANNEL  _G_k210SpiChannels[__SPI_CHANNEL_NR] = {
        {
                .SPICHAN_uiChannelId    =  0,
                .SPICHAN_phyAddrBase    =  SPI0_BASE_ADDR,
                .SPICHAN_chipSelectLine =  SPI_CHIP_SELECT_3,           /* default for sd card          */
                .SPICHAN_uiVector       =  KENDRYTE_PLIC_VECTOR(IRQN_SPI0_INTERRUPT),
                .clock                  =  SYSCTL_CLOCK_SPI0,
                .dma_req_base           =  SYSCTL_DMA_SELECT_SSI0_RX_REQ,
                .SPICHAN_baudRate       =  400000,
                .SPICHAN_uiFifoDepth    =  __SPI_FIFO_DEEPTH,
                .mod_off                =  6,
                .dfs_off                =  16,
                .tmod_off               =  8,
                .frf_off                =  21,
        },
        {
                .SPICHAN_uiChannelId    =  1,
                .SPICHAN_phyAddrBase    =  SPI1_BASE_ADDR,
                .SPICHAN_chipSelectLine =  0,
                .SPICHAN_uiVector       =  KENDRYTE_PLIC_VECTOR(IRQN_SPI1_INTERRUPT),
                .clock                  =  SYSCTL_CLOCK_SPI1,
                .dma_req_base           =  SYSCTL_DMA_SELECT_SSI1_RX_REQ,
                .SPICHAN_baudRate       =  400000,
                .SPICHAN_uiFifoDepth    =  __SPI_FIFO_DEEPTH,
                .mod_off                =  6,
                .dfs_off                =  16,
                .tmod_off               =  8,
                .frf_off                =  21,
        },
        {
                .SPICHAN_uiChannelId    =  2,                           /*  only spi slave mode allowed */
                .SPICHAN_phyAddrBase    =  SPI_SLAVE_BASE_ADDR,
                .SPICHAN_chipSelectLine =  0,
                .SPICHAN_uiVector       =  KENDRYTE_PLIC_VECTOR(IRQN_SPI_SLAVE_INTERRUPT),
                .clock                  =  SYSCTL_CLOCK_SPI2,
                .dma_req_base           =  SYSCTL_DMA_SELECT_SSI2_RX_REQ,
                .SPICHAN_baudRate       =  400000,
                .SPICHAN_uiFifoDepth    =  __SPI_FIFO_DEEPTH,
                .dfs_off                =  0,                           /*  data sheet book unknown     */
                .tmod_off               =  0,
                .frf_off                =  0,
        },
        {
                .SPICHAN_uiChannelId    =  3,
                .SPICHAN_phyAddrBase    =  SPI3_BASE_ADDR,
                .SPICHAN_chipSelectLine =  0,
                .SPICHAN_uiVector       =  KENDRYTE_PLIC_VECTOR(IRQN_SPI3_INTERRUPT),
                .clock                  =  SYSCTL_CLOCK_SPI3,
                .dma_req_base           =  SYSCTL_DMA_SELECT_SSI3_RX_REQ,
                .SPICHAN_baudRate       =  400000,
                .SPICHAN_uiFifoDepth    =  __SPI_FIFO_DEEPTH,
                .mod_off                =  8,
                .dfs_off                =  0,
                .tmod_off               =  10,
                .frf_off                =  22,
        },
};
/*********************************************************************************************************
** 函数名称: __k210Bit2ByteWidthGet
** 功能描述: spi 传输消息
** 输　入  : pSpiChannel     spi 通道
**           pSpiMsg         spi 传输消息
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static int __k210Bit2ByteWidthGet(size_t length)
{
    if (length == 0) {
        return 0;
    } else if (length <= 8) {
        return 1;
    } else if (length <= 16) {
        return 2;
    } else if (length <= 24) {
        return 3;
    }

    return 4;
}
/*********************************************************************************************************
** 函数名称: __k210SpiTmodSet
** 功能描述: SPI 传输模式设置
** 输　入  : pSpiChannel     spi 通道
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __k210SpiTmodSet (__PK210_SPI_CHANNEL  pSpiChannel, UINT32  tmod)
{
    UINT8           tmod_offset = 0;
    volatile spi_t *spi_handle  = (volatile spi_t *)pSpiChannel->SPICHAN_phyAddrBase;

    if (spi_handle == LW_NULL) {
        printk(KERN_ERR "__k210SpiTmodSet(): invalid spi_handle.\n");
        return;
    }

    switch(pSpiChannel->SPICHAN_uiChannelId) {
    case 0:
    case 1:
        tmod_offset = 8;
        break;
    case 2:
        printk(KERN_ERR "Spi Bus 2 Not Support!\n");
        break;
    case 3:
    default:
        tmod_offset = 10;
        break;
    }

    set_bit(&spi_handle->ctrlr0, 3 << tmod_offset, tmod << tmod_offset);
}
/*********************************************************************************************************
** 函数名称: __k210SpiExternConfig
** 功能描述: spi 非标准模式配置接口
** 输　入  : pSpiChannel     spi 通道
**           pSpiMsg         spi 传输消息
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static void __k210SpiExternConfig (__PK210_SPI_CHANNEL  pSpiChannel)
{
    size_t    instruction_length;
    size_t    address_length;
    size_t    wait_cycles;
    UINT32    uiInstLen = 0;
    UINT32    uiTrans   = 0;
    UINT32    uiAddrLen = 0;

    spi_instruction_address_trans_mode_t trans_mode;

    if (pSpiChannel == LW_NULL) {
        printk(KERN_ERR "__k210SpiExternConfig(): invalid param pSpiChannel.\n");
        return;
    }

    COMMON_ENTRY(pSpiChannel);

    instruction_length = pSpiChannel->SPICHAN_stInstLen;
    address_length     = pSpiChannel->SPICHAN_stAddrLen;
    wait_cycles        = pSpiChannel->SPICHAN_stWaitCycles;
    trans_mode         = pSpiChannel->SPICHAN_transMode;

    if (pSpiChannel->SPICHAN_frameFormat == SPI_FF_STANDARD) {
        printk(KERN_ERR "__k210SpiExternConfig(): invalid param frameFormat.\n");
        return;
    }

    if (wait_cycles >= (1 << 5)) {
        printk(KERN_ERR "__k210SpiExternConfig(): invalid param wait_cycles.\n");
        return;
    }

    switch (instruction_length) {
    case 0:
        uiInstLen = 0;
        break;

    case 4:
        uiInstLen = 1;
        break;

    case 8:
        uiInstLen = 2;
        break;

    case 16:
        uiInstLen = 3;
        break;

    default:
        printk(KERN_ERR "Invalid instruction length.\n");
        break;
    }

    switch (trans_mode) {
    case SPI_AITM_STANDARD:
        uiTrans = 0;
        break;

    case SPI_AITM_ADDR_STANDARD:
        uiTrans = 1;
        break;

    case SPI_AITM_AS_FRAME_FORMAT:
        uiTrans = 2;
        break;

    default:
        printk(KERN_ERR "Invalid trans mode.\n");
        break;
    }

    if ((address_length % 4 == 0 && address_length <= 60) != LW_TRUE) {
        printk(KERN_ERR "invalid address type.\n");
        return;
    }

    uiAddrLen = address_length / 4;

    spi->spi_ctrlr0 = (wait_cycles << 11) | (uiInstLen << 8) | (uiAddrLen << 2) | uiTrans;
    pSpiChannel->SPICHAN_stInstWidth = __k210Bit2ByteWidthGet(instruction_length);
    pSpiChannel->SPICHAN_stAddrWidth = __k210Bit2ByteWidthGet(address_length);
}
/*********************************************************************************************************
** 函数名称: k210SpiChipSelect
** 功能描述: spi 片选信号控制函数
** 输　入  : iMasterID  BUS Number
**         : iDeviceID  Slave Device CS
**         : bEnable    Enable or Disable Device
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  k210SpiChipSelect (UINT  iMasterID, UINT iDeviceID, BOOL bEnable)
{
    __PK210_SPI_CHANNEL  pSpiChannel;
    volatile spi_t      *spi_handle;

    if (iMasterID >= __SPI_CHANNEL_NR) {
        printk("k210SpiChipSelect: spi channel invalid!\n");
        return;
    }

    pSpiChannel = &_G_k210SpiChannels[iMasterID];
    if (!pSpiChannel) {
        printk("k210SpiChipSelect: pSpiChannel invalid!\n");
        return;
    }

    if (pSpiChannel->SPICHAN_bIsInit == LW_FALSE) {
        printk("k210SpiChipSelect: pSpiChannel has not been initialized\r\n");
        return;
    }

    /*
     * 这里的 enable 对应 low_level; disable 对应 high_level。
     * 由于每一个 SPI 通道只对应一个设备所以，这里没有用到 iDeviceID。
     */
    spi_handle = (volatile spi_t *)pSpiChannel->SPICHAN_phyAddrBase;
    if (bEnable) {
        spi_handle->ser = 1U << pSpiChannel->SPICHAN_chipSelectLine;
    } else {
        spi_handle->ser &= ~(1U << pSpiChannel->SPICHAN_chipSelectLine);
    }
}
/*********************************************************************************************************
** 函数名称: k210SdiChipSelect
** 功能描述: spi-SDcard 片选信号设置函数
** 输　入  : iMasterID  BUS Number
**         : iDeviceID  Slave Device CS
**         : bEnable    Enable or Disable Device
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  k210SdiChipSelect (INT  iMasterID, INT iDeviceID, BOOL bEnable)
{
    if (bEnable) {
        SD_CS_LOW();
    } else {
        SD_CS_HIGH();
    }
}
/*********************************************************************************************************
** 函数名称: __k210SpiTransferPrep
** 功能描述: spi 传输消息
** 输　入  : pSpiChannel     spi 通道
**           pSpiMsg         spi 传输消息
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __k210SpiTransferPrep (__PK210_SPI_CHANNEL  pSpiChannel,
                                  PLW_SPI_MESSAGE      pSpiMsg)
{
    volatile spi_t *spi_handle = (volatile spi_t *)pSpiChannel->SPICHAN_phyAddrBase;

    if (spi_handle == LW_NULL) {
        printk(KERN_ERR "__k210SpiTransferPrep(): invalid param spi_handle.\n");
        return  (PX_ERROR);
    }

    if (pSpiChannel->SPICHAN_bIsCfgChanged == LW_FALSE) {
        /* Do nothing here! */
    } else {

        /* Update the transfer clock */
        spi_set_clk_rate(pSpiChannel->SPICHAN_uiChannelId, pSpiChannel->SPICHAN_baudRate);

        /* Update the data endian (MSB) */
        spi_handle->endian = pSpiChannel->SPICHAN_endian;

        /* CONFIG: POL PHA  FrameSize  */
        spi_handle->ctrlr0 = (pSpiChannel->SPICHAN_workMode << pSpiChannel->mod_off) |
                             (pSpiChannel->SPICHAN_frameFormat << pSpiChannel->frf_off) |
                             ((pSpiChannel->SPICHAN_dataBitLength - 1) << pSpiChannel->dfs_off);

        /* Not Normal transfer mode setting register */
        if (pSpiChannel->SPICHAN_frameFormat != SPI_FF_STANDARD) {
                __k210SpiExternConfig(pSpiChannel);
        } else {
            spi_handle->spi_ctrlr0 = 0;
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __k210SpiDevTransfer
** 功能描述: spi 传输消息
** 输　入  : pSpiChannel     spi 通道
**           pSpiMsg         spi 传输消息
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __k210SpiDevTransfer (__PK210_SPI_CHANNEL  pSpiChannel,
                                 PLW_SPI_MESSAGE      pSpiMsg)
{
    INT         iRet = ERROR_NONE;
    UINT        total_len, index, fifo_len = 0;
    UCHAR      *pWriteBuf;
    UCHAR      *pReadBuf;
    addr_t      ulBaseAddr;

    ulBaseAddr = pSpiChannel->SPICHAN_phyAddrBase;
    pWriteBuf  = pSpiMsg->SPIMSG_pucWrBuffer;
    pReadBuf   = pSpiMsg->SPIMSG_pucRdBuffer;

    //FOR_DEBUG
    API_SpinLock(&pSpiChannel->SPICHAN_lock);

    __k210SpiTmodSet(pSpiChannel, SPI_TMOD_TRANS_RECV);

    write32((UINT32)(pSpiMsg->SPIMSG_uiLen - 1), ulBaseAddr + SPI_REG_CTRLR1);
    write32((1U << pSpiChannel->SPICHAN_chipSelectLine), ulBaseAddr + SPI_REG_SER);

    total_len = pSpiMsg->SPIMSG_uiLen;

    while (total_len) {
        int n_words, tx_words, rx_words;
        n_words = min(total_len, pSpiChannel->SPICHAN_uiFifoDepth);

        if (pSpiMsg->SPIMSG_usFlag & LW_SPI_M_WRBUF_FIX) {
            for (tx_words = 0; tx_words < n_words; ++tx_words) {
//                if (pSpiChannel->SPICHAN_uiChannelId == 0) {
//                    _PrintFormat("write-f: 0x%x\r\n", *pWriteBuf);
//                }
                write32(*pWriteBuf, ulBaseAddr + SPI_REG_DR);
            }
        } else {
            for (tx_words = 0; tx_words < n_words; ++tx_words) {
//                if (pSpiChannel->SPICHAN_uiChannelId == 0) {
//                    _PrintFormat("write: 0x%x\r\n", *pWriteBuf);
//                }
                write32(*pWriteBuf++, ulBaseAddr + SPI_REG_DR);
            }
        }

        while((read32(ulBaseAddr + SPI_REG_SR) & 0x05) != 0x04);        /*          WAIT_TRANS_OK       */

        if (pSpiMsg->SPIMSG_usFlag & LW_SPI_M_RDBUF_FIX) {
            for (rx_words = 0; rx_words < n_words; ) {
                fifo_len = read32(ulBaseAddr + SPI_REG_RXFLR);
                fifo_len = fifo_len < n_words ? fifo_len : n_words;
                for (index = 0; index < fifo_len; index++) {
                    *pReadBuf = (UINT8)read32(ulBaseAddr + SPI_REG_DR);
//                    if (pSpiChannel->SPICHAN_uiChannelId == 0) {
//                        _PrintFormat("read-f: 0x%x\r\n", *pReadBuf);
//                    }
                }
                rx_words += fifo_len;
            }
        } else {
            for (rx_words = 0; rx_words < n_words; ) {
                fifo_len = read32(ulBaseAddr + SPI_REG_RXFLR);
                fifo_len = fifo_len < n_words ? fifo_len : n_words;
                for (index = 0; index < fifo_len; index++) {
                    *pReadBuf++ = (UINT8)read32(ulBaseAddr + SPI_REG_DR);
//                    if (pSpiChannel->SPICHAN_uiChannelId == 0) {
//                        _PrintFormat("read: 0x%x\r\n", *(pReadBuf - 1));
//                    }
                }
                rx_words += fifo_len;
            }
        }

        total_len -= n_words;
    }

    write32((read32(ulBaseAddr + SPI_REG_SER) & \
            (~(1U << pSpiChannel->SPICHAN_chipSelectLine))), ulBaseAddr + SPI_REG_SER);

//    if (pSpiChannel->SPICHAN_uiChannelId == 0) {
//        _PrintFormat("\r\n");
//    }

    //FOR_DEBUG
    API_SpinUnlock(&pSpiChannel->SPICHAN_lock);

    return  (iRet);
}

INT spi_netif_transfer_shell (INT uiChannel, PLW_SPI_MESSAGE pSpiMsg)
{
    if (uiChannel >= __SPI_CHANNEL_NR || !pSpiMsg) {
        printk(KERN_ERR "spiBusDrv(): spi channel invalid!\n");
        return  (-1);
    }

    return __k210SpiDevTransfer(&_G_k210SpiChannels[uiChannel], pSpiMsg);
}
/*********************************************************************************************************
** 函数名称: __k210SpiTransferMsg
** 功能描述: spi 传输消息
** 输　入  : pSpiChannel     spi 通道
**           pSpiMsg         spi 传输消息
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210SpiTransferMsg (__PK210_SPI_CHANNEL  pSpiChannel,
                                  PLW_SPI_MESSAGE      pSpiMsg)
{
    INT    iError = ERROR_NONE;

    if ((pSpiMsg->SPIMSG_pucWrBuffer == LW_NULL &&
         pSpiMsg->SPIMSG_pucRdBuffer == LW_NULL) ||
         pSpiMsg->SPIMSG_uiLen == 0) {
        return  (PX_ERROR);
    }

    iError = __k210SpiTransferPrep(pSpiChannel, pSpiMsg);
    iError = __k210SpiDevTransfer(pSpiChannel, pSpiMsg);

    if (pSpiMsg->SPIMSG_pfuncComplete) {
        pSpiMsg->SPIMSG_pfuncComplete(pSpiMsg->SPIMSG_pvContext);
    }

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __k210SpiTransfer
** 功能描述: spi 传输函数
** 输　入  : pSpiChannel     spi 通道
**           pSpiAdapter     spi 适配器
**           pSpiMsg         spi 传输消息组
**           iNum            消息数量
** 输　出  : 完成传输的消息数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210SpiTransfer (__PK210_SPI_CHANNEL  pSpiChannel,
                               PLW_SPI_ADAPTER      pSpiAdapter,
                               PLW_SPI_MESSAGE      pSpiMsg,
                               INT                  iNum)
{
    REGISTER INT    i;
    INT             iError;

    for (i = 0; i < iNum; i++, pSpiMsg++) {
        iError = __k210SpiTransferMsg(pSpiChannel, pSpiMsg);
        if (iError != ERROR_NONE) {
            return  (i);
        }
    }

    return  (iNum);
}
/*********************************************************************************************************
** 函数名称: __k210Spi0Transfer
** 功能描述: spi0 传输函数
** 输　入  : pSpiAdapter     spi 适配器
**           pSpiMsg         spi 传输消息组
**           iNum            消息数量
** 输　出  : 完成传输的消息数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210Spi0Transfer (PLW_SPI_ADAPTER     pSpiAdapter,
                                PLW_SPI_MESSAGE     pSpiMsg,
                                INT                 iNum)
{
    return  (__k210SpiTransfer(&_G_k210SpiChannels[0], pSpiAdapter, pSpiMsg, iNum));
}
/*********************************************************************************************************
** 函数名称: __k210Spi1Transfer
** 功能描述: spi1 传输函数
** 输　入  : pSpiAdapter     spi 适配器
**           pSpiMsg         spi 传输消息组
**           iNum            消息数量
** 输　出  : 完成传输的消息数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210Spi1Transfer (PLW_SPI_ADAPTER     pSpiAdapter,
                                PLW_SPI_MESSAGE     pSpiMsg,
                                INT                 iNum)
{
    return  (__k210SpiTransfer(&_G_k210SpiChannels[1], pSpiAdapter, pSpiMsg, iNum));
}
/*********************************************************************************************************
** 函数名称: __k210Spi3Transfer
** 功能描述: spi1 传输函数
** 输　入  : pSpiAdapter     spi 适配器
**           pSpiMsg         spi 传输消息组
**           iNum            消息数量
** 输　出  : 完成传输的消息数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210Spi3Transfer (PLW_SPI_ADAPTER     pSpiAdapter,
                                PLW_SPI_MESSAGE     pSpiMsg,
                                INT                 iNum)
{
    return  (__k210SpiTransfer(&_G_k210SpiChannels[3], pSpiAdapter, pSpiMsg, iNum));
}
/*********************************************************************************************************
** 函数名称: __k210SpiMasterCtl
** 功能描述: spi 控制函数
** 输　入  : pSpiChannel     spi 通道
**           pSpiAdapter     spi 适配器
**           iCmd            命令
**           lArg            参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210SpiMasterCtl (__PK210_SPI_CHANNEL  pSpiChannel,
                                PLW_SPI_ADAPTER      pSpiAdapter,
                                INT                  iCmd,
                                LONG                 lArg)
{
    INT                  iRet = ERROR_NONE;
    spi_quick_config_t  *pConfig;

    if (!pSpiChannel) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Invalid SpiChannel address.\r\n");
        return  (PX_ERROR);
    }

    switch (iCmd) {

    case LW_SPI_CTL_BAUDRATE:
        API_SpinLock(&pSpiChannel->SPICHAN_lock);
        pSpiChannel->SPICHAN_baudRate = spi_set_clk_rate(pSpiChannel->SPICHAN_uiChannelId, (UINT32)lArg);
        API_SpinUnlock(&pSpiChannel->SPICHAN_lock);
        break;

    case SPI_QUICK_CONFIG:
        pConfig = (spi_quick_config_t  *)lArg;
        API_SpinLock(&pSpiChannel->SPICHAN_lock);
        spi_init_non_standard(pSpiChannel->SPICHAN_uiChannelId, pConfig->uiInstLen,
                              pConfig->uiAddrLen, pConfig->uiWaitCycles,
                              pConfig->instAddrMode);
        pSpiChannel->SPICHAN_stInstLen    = pConfig->uiInstLen;
        pSpiChannel->SPICHAN_stAddrLen    = pConfig->uiAddrLen;
        pSpiChannel->SPICHAN_stWaitCycles = pConfig->uiWaitCycles;
        pSpiChannel->SPICHAN_transMode    = pConfig->instAddrMode;
        API_SpinUnlock(&pSpiChannel->SPICHAN_lock);
        break;

    default:
        iRet = PX_ERROR;
        break;
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __k210Spi0MasterCtl
** 功能描述: spi0 控制函数
** 输　入  : pSpiAdapter     spi 适配器
**           iCmd            命令
**           lArg            参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210Spi0MasterCtl (PLW_SPI_ADAPTER   pSpiAdapter,
                                 INT               iCmd,
                                 LONG              lArg)
{
    return  (__k210SpiMasterCtl(&_G_k210SpiChannels[0], pSpiAdapter, iCmd, lArg));
}
/*********************************************************************************************************
** 函数名称: __k210Spi1MasterCtl
** 功能描述: spi1 控制函数
** 输　入  : pSpiAdapter     spi 适配器
**           iCmd            命令
**           lArg            参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210Spi1MasterCtl (PLW_SPI_ADAPTER   pSpiAdapter,
                                 INT               iCmd,
                                 LONG              lArg)
{
    return  (__k210SpiMasterCtl(&_G_k210SpiChannels[1], pSpiAdapter, iCmd, lArg));
}
/*********************************************************************************************************
** 函数名称: __k210Spi3MasterCtl
** 功能描述: spi1 控制函数
** 输　入  : pSpiAdapter     spi 适配器
**           iCmd            命令
**           lArg            参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210Spi3MasterCtl (PLW_SPI_ADAPTER   pSpiAdapter,
                                 INT               iCmd,
                                 LONG              lArg)
{
    return  (__k210SpiMasterCtl(&_G_k210SpiChannels[3], pSpiAdapter, iCmd, lArg));
}
/*********************************************************************************************************
** 函数名称: __k210SpiInit
** 功能描述: 初始化 spi 通道
** 输　入  : pSpiChannel     spi 通道
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210SpiInit(__PK210_SPI_CHANNEL  pSpiChannel)
{
    volatile spi_t     *spi_handle;

    if (!pSpiChannel->SPICHAN_bIsInit) {

        pSpiChannel->SPICHAN_hSignal = API_SemaphoreCCreate("spi_signal", 0, 3,
                                                            LW_OPTION_OBJECT_GLOBAL, LW_NULL);
        if (pSpiChannel->SPICHAN_hSignal == LW_OBJECT_HANDLE_INVALID) {
            printk(KERN_ERR "__k210SpiInit(): failed to create spi_signal!\n");
            goto  __error_handle;
        }

        if (API_SpinInit(&pSpiChannel->SPICHAN_lock)) {
            printk("__k210SpiInit(): failed to create SPICHAN_lock!\n");
            goto  __error_handle;
        }

        /*
         * 初始化 SPI 为标准工作模式:
         *
         * MODE0  模    式:   串行时钟的无效状态为低(SCPOL), 串行时钟在第一个数据位的中间切换(SCPH)
         * SPI_FF_STANDARD:   单线传输模式(MISO/SIMO)
         * ENDIAN 设    置:   MSB/LSB (Kendryte手册上没有找到相关说明, 这里采用默认设置)
         * CS 片  选    线:   默认片选 CS-pin0
         *
         * 注意: 由于SylixOS中SPI-MSG定义的限制，当前SPI驱动被固定在 SPI_TMOD_TRANS_RECV 模式下工作
         */

        pSpiChannel->SPICHAN_workMode      = SPI_WORK_MODE_0;
        pSpiChannel->SPICHAN_frameFormat   = SPI_FF_STANDARD;
        pSpiChannel->SPICHAN_dataBitLength = 8;
        pSpiChannel->SPICHAN_endian        = 0;

        spi_init(pSpiChannel->SPICHAN_uiChannelId, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
        spi_handle = (volatile spi_t *)pSpiChannel->SPICHAN_phyAddrBase;
        spi_handle->ssienr = 0x01;

        pSpiChannel->SPICHAN_bIsInit = LW_TRUE;
    }

    return  (ERROR_NONE);

__error_handle:

    if (pSpiChannel->SPICHAN_hSignal) {
        API_SemaphoreCDelete(&pSpiChannel->SPICHAN_hSignal);
        pSpiChannel->SPICHAN_hSignal = 0;
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
  AM335X 处理器 spi 驱动程序
  注意:  串行外设接口有4组 SPI 接口，其中SPI0、SPI1、SPI3 只能工作在MASTER 模式，SPI2 只能工作在SLAVE 模式
*********************************************************************************************************/
static LW_SPI_FUNCS  _G_k210SpiFuncs[__SPI_CHANNEL_NR] = {
        {
                __k210Spi0Transfer,
                __k210Spi0MasterCtl,
        },
        {
                __k210Spi1Transfer,
                __k210Spi1MasterCtl,
        },
        {
                LW_NULL,                                                /*   SPI Slaver                 */
                LW_NULL,
        },
        {
                __k210Spi3Transfer,
                __k210Spi3MasterCtl,
        },
};
/*********************************************************************************************************
** 函数名称: spiBusDrv
** 功能描述: 初始化 spi 总线并获取驱动程序
** 输　入  : uiChannel         通道号
** 输　出  : spi 总线驱动程序
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PLW_SPI_FUNCS  spiBusDrv (UINT  uiChannel)
{
    if (uiChannel >= __SPI_CHANNEL_NR) {
        printk(KERN_ERR "spiBusDrv(): spi channel invalid!\n");
        return  (LW_NULL);
    }

    /*
     * 暂不支持 SPI2 (slaver mode)
     */
    if (uiChannel == 2) {
        printk(KERN_ERR "spiBusDrv(): spi-2 channel not support!\n");
        return  (LW_NULL);
    }

    if (__k210SpiInit(&_G_k210SpiChannels[uiChannel]) != ERROR_NONE) {
        return  (LW_NULL);
    }

    return  (&_G_k210SpiFuncs[uiChannel]);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
