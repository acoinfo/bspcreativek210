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
** ��   ��   ��: k210_spi.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 10 �� 22 ��
**
** ��        ��: SPI ����������
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
  SPI �Ĵ���ƫ�ƶ���
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
  SPI ��غ궨��
*********************************************************************************************************/
#define __SPI_CHANNEL_NR            (4)                                 /*  SPI channel numbers.        */
#define __SPI_MAX_BAUDRATE          (25000000)                          /*  ! SPI3 max freq is 100MHz   */
#define __SPI_FIFO_DEEPTH           (32)                                /*  SPI FIFO max deepth         */
#define TMOD_MASK(data)             (3 << data->tmod_off)               /*  ��λ���ݵ�ָ��ƫ�ƴ�        */
#define TMOD_VALUE(data, value)     (value << data->tmod_off)           /*  ��λ���ݵ�ָ��ƫ�ƴ�        */
/*********************************************************************************************************
  ���������������
*********************************************************************************************************/
typedef spi_instruction_address_trans_mode_t    k210_spi_ins_addr_t;    /*  �������ض���                */
/*********************************************************************************************************
  SPI ��غ꺯������
*********************************************************************************************************/
#define WAIT_TRANS_OK   while((spi_handle->sr & 0x05) != 0x04);

#define COMMON_ENTRY(data)                                                          \
        volatile spi_t *const spi = (volatile spi_t *)(data->SPICHAN_phyAddrBase);  \
        (void)spi
/*********************************************************************************************************
  SPI �������ṹ�嶨��
*********************************************************************************************************/
typedef struct k210_spi {
    sysctl_clock_t      clock;                                          /*  ����  Kendryte-SDK Դ��     */
    sysctl_dma_select_t dma_req_base;                                   /*  ����  Kendryte-SDK Դ��     */

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
                                                                        /*  ���¶�Ӧ�Ĵ��� SPI_CTRLR0   */
    size_t              SPICHAN_stInstLen;                              /*  ָ���(bit)               */
    size_t              SPICHAN_stAddrLen;                              /*  ��ַ����(bit)               */
    size_t              SPICHAN_stInstWidth;                            /*  ָ���(byte)              */
    size_t              SPICHAN_stAddrWidth;                            /*  ��ַ����(byte)              */
    k210_spi_ins_addr_t SPICHAN_transMode;                              /*  ��ַ��ָ����ʽ          */
    size_t              SPICHAN_stWaitCycles;                           /*  ����֡���ͺ����ݽ���֮���  */
                                                                        /*  CTRLR0 λƫ��(�ο���SDK)    */
    UINT8               tmod_off;                                       /*  CTRLR0[9:8]: TMOD ����ģʽ  */
    UINT8               mod_off;                                        /*  CTRLR0[7:6]: SCPOL & SCPH   */
    UINT8               frf_off;                                        /*  CTRLR0[5:4]: FRF ֡��ʽ     */
    UINT8               dfs_off;                                        /*  CTRLR0[3:0]: DFS ���ݿ��С */

    LW_OBJECT_HANDLE    SPICHAN_hSignal;                                /* Wake up from interrupt       */
} __K210_SPI_CHANNEL, *__PK210_SPI_CHANNEL;
/*********************************************************************************************************
  ȫ�ֱ�������
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
** ��������: __k210Bit2ByteWidthGet
** ��������: spi ������Ϣ
** �䡡��  : pSpiChannel     spi ͨ��
**           pSpiMsg         spi ������Ϣ
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210SpiTmodSet
** ��������: SPI ����ģʽ����
** �䡡��  : pSpiChannel     spi ͨ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210SpiExternConfig
** ��������: spi �Ǳ�׼ģʽ���ýӿ�
** �䡡��  : pSpiChannel     spi ͨ��
**           pSpiMsg         spi ������Ϣ
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: k210SpiChipSelect
** ��������: spi Ƭѡ�źſ��ƺ���
** �䡡��  : iMasterID  BUS Number
**         : iDeviceID  Slave Device CS
**         : bEnable    Enable or Disable Device
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
     * ����� enable ��Ӧ low_level; disable ��Ӧ high_level��
     * ����ÿһ�� SPI ͨ��ֻ��Ӧһ���豸���ԣ�����û���õ� iDeviceID��
     */
    spi_handle = (volatile spi_t *)pSpiChannel->SPICHAN_phyAddrBase;
    if (bEnable) {
        spi_handle->ser = 1U << pSpiChannel->SPICHAN_chipSelectLine;
    } else {
        spi_handle->ser &= ~(1U << pSpiChannel->SPICHAN_chipSelectLine);
    }
}
/*********************************************************************************************************
** ��������: k210SdiChipSelect
** ��������: spi-SDcard Ƭѡ�ź����ú���
** �䡡��  : iMasterID  BUS Number
**         : iDeviceID  Slave Device CS
**         : bEnable    Enable or Disable Device
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210SpiTransferPrep
** ��������: spi ������Ϣ
** �䡡��  : pSpiChannel     spi ͨ��
**           pSpiMsg         spi ������Ϣ
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210SpiDevTransfer
** ��������: spi ������Ϣ
** �䡡��  : pSpiChannel     spi ͨ��
**           pSpiMsg         spi ������Ϣ
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210SpiTransferMsg
** ��������: spi ������Ϣ
** �䡡��  : pSpiChannel     spi ͨ��
**           pSpiMsg         spi ������Ϣ
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210SpiTransfer
** ��������: spi ���亯��
** �䡡��  : pSpiChannel     spi ͨ��
**           pSpiAdapter     spi ������
**           pSpiMsg         spi ������Ϣ��
**           iNum            ��Ϣ����
** �䡡��  : ��ɴ������Ϣ����
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210Spi0Transfer
** ��������: spi0 ���亯��
** �䡡��  : pSpiAdapter     spi ������
**           pSpiMsg         spi ������Ϣ��
**           iNum            ��Ϣ����
** �䡡��  : ��ɴ������Ϣ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210Spi0Transfer (PLW_SPI_ADAPTER     pSpiAdapter,
                                PLW_SPI_MESSAGE     pSpiMsg,
                                INT                 iNum)
{
    return  (__k210SpiTransfer(&_G_k210SpiChannels[0], pSpiAdapter, pSpiMsg, iNum));
}
/*********************************************************************************************************
** ��������: __k210Spi1Transfer
** ��������: spi1 ���亯��
** �䡡��  : pSpiAdapter     spi ������
**           pSpiMsg         spi ������Ϣ��
**           iNum            ��Ϣ����
** �䡡��  : ��ɴ������Ϣ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210Spi1Transfer (PLW_SPI_ADAPTER     pSpiAdapter,
                                PLW_SPI_MESSAGE     pSpiMsg,
                                INT                 iNum)
{
    return  (__k210SpiTransfer(&_G_k210SpiChannels[1], pSpiAdapter, pSpiMsg, iNum));
}
/*********************************************************************************************************
** ��������: __k210Spi3Transfer
** ��������: spi1 ���亯��
** �䡡��  : pSpiAdapter     spi ������
**           pSpiMsg         spi ������Ϣ��
**           iNum            ��Ϣ����
** �䡡��  : ��ɴ������Ϣ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210Spi3Transfer (PLW_SPI_ADAPTER     pSpiAdapter,
                                PLW_SPI_MESSAGE     pSpiMsg,
                                INT                 iNum)
{
    return  (__k210SpiTransfer(&_G_k210SpiChannels[3], pSpiAdapter, pSpiMsg, iNum));
}
/*********************************************************************************************************
** ��������: __k210SpiMasterCtl
** ��������: spi ���ƺ���
** �䡡��  : pSpiChannel     spi ͨ��
**           pSpiAdapter     spi ������
**           iCmd            ����
**           lArg            ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210Spi0MasterCtl
** ��������: spi0 ���ƺ���
** �䡡��  : pSpiAdapter     spi ������
**           iCmd            ����
**           lArg            ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210Spi0MasterCtl (PLW_SPI_ADAPTER   pSpiAdapter,
                                 INT               iCmd,
                                 LONG              lArg)
{
    return  (__k210SpiMasterCtl(&_G_k210SpiChannels[0], pSpiAdapter, iCmd, lArg));
}
/*********************************************************************************************************
** ��������: __k210Spi1MasterCtl
** ��������: spi1 ���ƺ���
** �䡡��  : pSpiAdapter     spi ������
**           iCmd            ����
**           lArg            ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210Spi1MasterCtl (PLW_SPI_ADAPTER   pSpiAdapter,
                                 INT               iCmd,
                                 LONG              lArg)
{
    return  (__k210SpiMasterCtl(&_G_k210SpiChannels[1], pSpiAdapter, iCmd, lArg));
}
/*********************************************************************************************************
** ��������: __k210Spi3MasterCtl
** ��������: spi1 ���ƺ���
** �䡡��  : pSpiAdapter     spi ������
**           iCmd            ����
**           lArg            ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210Spi3MasterCtl (PLW_SPI_ADAPTER   pSpiAdapter,
                                 INT               iCmd,
                                 LONG              lArg)
{
    return  (__k210SpiMasterCtl(&_G_k210SpiChannels[3], pSpiAdapter, iCmd, lArg));
}
/*********************************************************************************************************
** ��������: __k210SpiInit
** ��������: ��ʼ�� spi ͨ��
** �䡡��  : pSpiChannel     spi ͨ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
         * ��ʼ�� SPI Ϊ��׼����ģʽ:
         *
         * MODE0  ģ    ʽ:   ����ʱ�ӵ���Ч״̬Ϊ��(SCPOL), ����ʱ���ڵ�һ������λ���м��л�(SCPH)
         * SPI_FF_STANDARD:   ���ߴ���ģʽ(MISO/SIMO)
         * ENDIAN ��    ��:   MSB/LSB (Kendryte�ֲ���û���ҵ����˵��, �������Ĭ������)
         * CS Ƭ  ѡ    ��:   Ĭ��Ƭѡ CS-pin0
         *
         * ע��: ����SylixOS��SPI-MSG��������ƣ���ǰSPI�������̶��� SPI_TMOD_TRANS_RECV ģʽ�¹���
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
  AM335X ������ spi ��������
  ע��:  ��������ӿ���4�� SPI �ӿڣ�����SPI0��SPI1��SPI3 ֻ�ܹ�����MASTER ģʽ��SPI2 ֻ�ܹ�����SLAVE ģʽ
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
** ��������: spiBusDrv
** ��������: ��ʼ�� spi ���߲���ȡ��������
** �䡡��  : uiChannel         ͨ����
** �䡡��  : spi ������������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PLW_SPI_FUNCS  spiBusDrv (UINT  uiChannel)
{
    if (uiChannel >= __SPI_CHANNEL_NR) {
        printk(KERN_ERR "spiBusDrv(): spi channel invalid!\n");
        return  (LW_NULL);
    }

    /*
     * �ݲ�֧�� SPI2 (slaver mode)
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
