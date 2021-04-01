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
** 文   件   名: netif.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2019 年 1 月 17 日
**
** 描        述: K210 处理器 DM9051 SPI 网卡驱动
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "config.h"
#include <linux/compat.h>
#include "driver/fix_arch_def.h"
#include "driver/clock/k210_clock.h"
#include "netdev.h"
#include "netif.h"
#include "driver/gpiohs/k210_gpiohs.h"
#include "driver/spi/k210_spi.h"

#include "KendryteWare/include/common.h"
#include "KendryteWare/include/gpiohs.h"
#include "KendryteWare/include/fpioa.h"
/*********************************************************************************************************
   DM9051 网卡驱动参数宏定义
*********************************************************************************************************/
#define READ_VID_TEST_EN       (1)

#define INT_GPIONUM            (2)
#define NETIF_CS_PIN           (3)
#define NET_POWER_PIN          (4)
#define NET_POWER_PIN2         (5)
#define NET_GND_PIN            (6)
#define NET_GND_PIN2           (3)
//#define NETIF_SPI_CHANNEL    (0)

#define SPI_ADAPTER_NAME       "/bus/spi/0"
#define SPI_NETIF_DEVNAME      "/dev/spi_netif"

#define CONFIG_MDIO_TIMEOUT    (10000)

#define CONFIG_ETH_RXSIZE      (2044)                                   /* Note must fit in ETH_BUFSIZE */
#define CONFIG_ETH_BUFSIZE     (2048)                                   /* Note must be dma aligned     */
#define CONFIG_TX_DESCR_NUM    (64)
#define CONFIG_RX_DESCR_NUM    (64)

#define SPI_DM9051_CS_LOW()     API_GpioSetValue(KENDRYTE_GET_GPIOHS_ID(NETIF_CS_PIN), 0)
#define SPI_DM9051_CS_HIGH()    API_GpioSetValue(KENDRYTE_GET_GPIOHS_ID(NETIF_CS_PIN), 1)
/*********************************************************************************************************
  DEBUG开关
*********************************************************************************************************/
#define __NETIF_DEBUG   0                                               /* NETIF debug switcher         */

#if     __NETIF_DEBUG > 0
#define netif_debug(fmt, arg...)  _DebugFormat(__PRINTMESSAGE_LEVEL, fmt, ##arg)
#else
#define netif_debug(fmt, arg...)
#endif
/*********************************************************************************************************
  dm9051 register
*********************************************************************************************************/
#define DM9051_PHY        (0x40)                                        /* PHY address 0x01             */
#define DM9051_ID         (0x90510A46)                                  /* DM9051A ID                   */
#define DM9051_PKT_MAX    (1536)                                        /* Received packet max size     */
#define DM9051_PKT_RDY    (0x01)                                        /* Packet ready to receive      */
#define DM9051_PKT_ERR    (0x02)                                        /* Packet error occur           */
/*********************************************************************************************************
   DM9051 网卡寄存器偏移
*********************************************************************************************************/
#define DM9051_NCR        (0x00)
#define DM9051_NSR        (0x01)
#define DM9051_TCR        (0x02)
#define DM9051_TSR1       (0x03)
#define DM9051_TSR2       (0x04)
#define DM9051_RCR        (0x05)
#define DM9051_RSR        (0x06)
#define DM9051_ROCR       (0x07)
#define DM9051_BPTR       (0x08)
#define DM9051_FCTR       (0x09)
#define DM9051_FCR        (0x0A)
#define DM9051_EPCR       (0x0B)
#define DM9051_EPAR       (0x0C)
#define DM9051_EPDRL      (0x0D)
#define DM9051_EPDRH      (0x0E)
#define DM9051_WCR        (0x0F)

#define DM9051_PAR        (0x10)
#define DM9051_MAR        (0x16)

#define DM9051_GPCR       (0x1e)
#define DM9051_GPR        (0x1f)
#define DM9051_TRPAL      (0x22)
#define DM9051_TRPAH      (0x23)
#define DM9051_RWPAL      (0x24)
#define DM9051_RWPAH      (0x25)

#define DM9051_VIDL       (0x28)
#define DM9051_VIDH       (0x29)
#define DM9051_PIDL       (0x2A)
#define DM9051_PIDH       (0x2B)

#define DM9051_CHIPR      (0x2C)
#define DM9051_TCR2       (0x2D)
#define DM9051_OTCR       (0x2E)
#define DM9051_SMCR       (0x2F)

#define DM9051_ETCR       (0x30)                              /* early transmit control/status register */
#define DM9051_CSCR       (0x31)                              /* check sum control register             */
#define DM9051_RCSSR      (0x32)                              /* receive check sum status register      */

#define DM9051_PBCR       (0x38)
#define DM9051_INTR       (0x39)
#define DM9051_MPCR       (0x55)
#define DM9051_MRCMDX     (0x70)
#define DM9051_MRCMDX1    (0x71)
#define DM9051_MRCMD      (0x72)
#define DM9051_MRRL       (0x74)
#define DM9051_MRRH       (0x75)
#define DM9051_MWCMDX     (0x76)
#define DM9051_MWCMD      (0x78)
#define DM9051_MWRL       (0x7A)
#define DM9051_MWRH       (0x7B)
#define DM9051_TXPLL      (0x7C)
#define DM9051_TXPLH      (0x7D)
#define DM9051_ISR        (0x7E)
#define DM9051_IMR        (0x7F)

#define CHIPR_DM9051A     (0x19)
#define CHIPR_DM9051B     (0x1B)

#define DM9051_REG_RESET  (0x01)
#define DM9051_IMR_OFF    (0x80)
#define DM9051_TCR2_SET   (0x90)                                        /*      set one packet          */
#define DM9051_RCR_SET    (0x31)
#define DM9051_BPTR_SET   (0x37)
#define DM9051_FCTR_SET   (0x38)
#define DM9051_FCR_SET    (0x28)
#define DM9051_TCR_SET    (0x01)

#define NCR_EXT_PHY       (1 << 7)
#define NCR_WAKEEN        (1 << 6)
#define NCR_FCOL          (1 << 4)
#define NCR_FDX           (1 << 3)
#define NCR_LBK           (3 << 1)
#define NCR_LBK_INT_MAC   (1 << 1)
#define NCR_LBK_INT_PHY   (2 << 1)
#define NCR_RST           (1 << 0)
#define NCR_DEFAULT       (0x0)                                         /*      Disable Wakeup          */

#define NSR_SPEED         (1 << 7)
#define NSR_LINKST        (1 << 6)
#define NSR_WAKEST        (1 << 5)
#define NSR_TX2END        (1 << 3)
#define NSR_TX1END        (1 << 2)
#define NSR_RXOV          (1 << 1)
#define NSR_CLR_STATUS    (NSR_WAKEST | NSR_TX2END | NSR_TX1END)

#define TCR_TJDIS         (1 << 6)
#define TCR_EXCECM        (1 << 5)
#define TCR_PAD_DIS2      (1 << 4)
#define TCR_CRC_DIS2      (1 << 3)
#define TCR_PAD_DIS1      (1 << 2)
#define TCR_CRC_DIS1      (1 << 1)
#define TCR_TXREQ         (1 << 0)                                      /*          Start TX            */
#define TCR_DEFAULT       (0x0)

#define TSR_TJTO          (1 << 7)
#define TSR_LC            (1 << 6)
#define TSR_NC            (1 << 5)
#define TSR_LCOL          (1 << 4)
#define TSR_COL           (1 << 3)
#define TSR_EC            (1 << 2)

#define RCR_WTDIS         (1 << 6)
#define RCR_DIS_LONG      (1 << 5)
#define RCR_DIS_CRC       (1 << 4)
#define RCR_ALL           (1 << 3)
#define RCR_RUNT          (1 << 2)
#define RCR_PRMSC         (1 << 1)
#define RCR_RXEN          (1 << 0)
#define RCR_DEFAULT       (RCR_DIS_LONG | RCR_DIS_CRC)

#define RSR_RF            (1 << 7)
#define RSR_MF            (1 << 6)
#define RSR_LCS           (1 << 5)
#define RSR_RWTO          (1 << 4)
#define RSR_PLE           (1 << 3)
#define RSR_AE            (1 << 2)
#define RSR_CE            (1 << 1)
#define RSR_FOE           (1 << 0)

#define BPTR_DEFAULT      (0x3f)
#define FCTR_DEAFULT      (0x38)
#define FCR_DEFAULT       (0xFF)
#define SMCR_DEFAULT      (0x0)
#define PBCR_MAXDRIVE     (0x44)

#define IMR_PAR           (1 << 7)
#define IMR_LNKCHGI       (1 << 5)
#define IMR_UDRUN         (1 << 4)
#define IMR_ROOM          (1 << 3)
#define IMR_ROM           (1 << 2)
#define IMR_PTM           (1 << 1)
#define IMR_PRM           (1 << 0)
#define IMR_FULL          (IMR_PAR | IMR_LNKCHGI | IMR_UDRUN | IMR_ROOM | IMR_ROM | IMR_PTM | IMR_PRM)
#define IMR_OFF           (IMR_PAR)
#define IMR_DEFAULT       (IMR_PAR | IMR_PRM | IMR_PTM)

#define ISR_ROOS          (1 << 3)
#define ISR_ROS           (1 << 2)
#define ISR_PTS           (1 << 1)
#define ISR_PRS           (1 << 0)
#define ISR_CLR_STATUS    (0x80 | 0x3F)

#define EPCR_REEP         (1 << 5)
#define EPCR_WEP          (1 << 4)
#define EPCR_EPOS         (1 << 3)
#define EPCR_ERPRR        (1 << 2)
#define EPCR_ERPRW        (1 << 1)
#define EPCR_ERRE         (1 << 0)

#define GPCR_GEP_CNTL     (1 << 0)

#define SPI_WR_BURST      (0xF8)
#define SPI_RD_BURST      (0x72)

#define SPI_READ          (0x03)
#define SPI_WRITE         (0x04)
#define SPI_WRITE_BUFFER  (0x05)                /* Send a series of bytes from the Master to the Slave  */
#define SPI_READ_BUFFER   (0x06)                /* Send a series of bytes from the Slave  to the Master */
/*********************************************************************************************************
   DM9051 网卡驱动工作模式及内部接口声明
*********************************************************************************************************/
enum DM9051_TYPE
{
    TYPE_DM9051E,
    TYPE_DM9051A,
    TYPE_DM9051B,
    TYPE_DM9051
};
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
#define MAX_ADAPTER_NUM     8                                           /* ADAPTER NAME TABLE MAX SIZE */

static  CHAR  *_G_pcSpiNetifAdapterName[MAX_ADAPTER_NUM] = {            /* SPI NETIF ADAPTER NAME TABLE */
        "/bus/spi/0", "/bus/spi/1", "/bus/spi/2", "/bus/spi/3",
        "/bus/spi/4", "/bus/spi/5", "/bus/spi/6", "/bus/spi/7",
};
/*********************************************************************************************************
** 函数名称: __dm9051SendByte
** 功能描述: DM9051 设备读操作
** 输　入  : NONE
** 输　出  : 错误码
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static inline uint8_t __dm9051SendByte (ENET *pNetInfo, uint8_t byte)
{
    uint8_t          uiSpiDummy = 0;
    LW_SPI_MESSAGE   spiMessage;

    lib_memset(&spiMessage, 0, sizeof(LW_SPI_MESSAGE));

    spiMessage.SPIMSG_uiLen = 1;
    spiMessage.SPIMSG_pucWrBuffer = &byte;
    spiMessage.SPIMSG_pucRdBuffer = &uiSpiDummy;

    API_SpiDeviceTransfer(pNetInfo->ENET_pSpiNetDev, &spiMessage, 1);

    return  (uiSpiDummy);
}
/*********************************************************************************************************
** 函数名称: __dm9051Read
** 功能描述: DM9051 设备读操作
** 输　入  : NONE
** 输　出  : 错误码
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static uint8_t __dm9051Read (ENET *pNetInfo, uint8_t regAddr)
{
    uint8_t          uiReadData = 0;
    uint8_t          uiSpiDummy = 0;
    LW_SPI_MESSAGE   spiMessage[2];

    lib_memset(spiMessage, 0, sizeof(LW_SPI_MESSAGE) * 2);

    /*
     * SPI read command: bit7 = 0, bit[6:0] = Reg offset addr.
     */
    SPI_DM9051_CS_LOW();

    //Read command + Register offset address
    spiMessage[0].SPIMSG_uiLen = 1;
    spiMessage[0].SPIMSG_pucWrBuffer = &regAddr;
    spiMessage[0].SPIMSG_pucRdBuffer = &uiSpiDummy;

    //Dummy for read register value.
    spiMessage[1].SPIMSG_uiLen = 1;
    spiMessage[1].SPIMSG_pucWrBuffer = &uiSpiDummy;
    spiMessage[1].SPIMSG_pucRdBuffer = &uiReadData;

    API_SpiDeviceTransfer(pNetInfo->ENET_pSpiNetDev, spiMessage, 2);

    SPI_DM9051_CS_HIGH();

    return  (uiReadData);
}
/*********************************************************************************************************
** 函数名称: __dm9051Write
** 功能描述: DM9051 设备读操作
** 输　入  : NONE
** 输　出  : 错误码
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static void __dm9051Write (ENET *pNetInfo, uint8_t regAddr, uint8_t regData)
{
    uint8_t          cmdaddr;
    uint8_t          uiSpiDummy = 0;
    LW_SPI_MESSAGE   spiMessage[2];

    lib_memset(spiMessage, 0, sizeof(LW_SPI_MESSAGE) * 2);

    /*
     * SPI write command: bit7 = 1,  bit[6:0] = Reg offset addr.
     */
    cmdaddr = (regAddr | 0x80);

    SPI_DM9051_CS_LOW();

    //Read command + Register offset address
    spiMessage[0].SPIMSG_uiLen = 1;
    spiMessage[0].SPIMSG_pucWrBuffer = &cmdaddr;
    spiMessage[0].SPIMSG_pucRdBuffer = &uiSpiDummy;

    //Read command + Register offset address
    spiMessage[1].SPIMSG_uiLen = 1;
    spiMessage[1].SPIMSG_pucWrBuffer = &regData;
    spiMessage[1].SPIMSG_pucRdBuffer = &uiSpiDummy;

    API_SpiDeviceTransfer(pNetInfo->ENET_pSpiNetDev, spiMessage, 2);

    SPI_DM9051_CS_HIGH();

    return;
}
/*********************************************************************************************************
* __dm9051ShellRead 和 __dm9051ShellRead 用于 ISR，避免API_SpiDeviceTransfer中的API_SemaphoreMPend报错
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: __dm9051ShellRead
** 功能描述: DM9051 设备读操作
** 输　入  : NONE
** 输　出  : 错误码
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static uint8_t __dm9051ShellRead (ENET *pNetInfo, uint8_t regAddr)
{
    uint8_t          uiReadData = 0;
    uint8_t          uiSpiDummy = 0;
    LW_SPI_MESSAGE   spiMessage[2];

    lib_memset(spiMessage, 0, sizeof(LW_SPI_MESSAGE) * 2);

    /*
     * SPI read command: bit7 = 0, bit[6:0] = Reg offset addr.
     */

    SPI_DM9051_CS_LOW();

    //Read command + Register offset address
    spiMessage[0].SPIMSG_uiLen = 1;
    spiMessage[0].SPIMSG_pucWrBuffer = &regAddr;
    spiMessage[0].SPIMSG_pucRdBuffer = &uiSpiDummy;
    spi_netif_transfer_shell(pNetInfo->ENET_uiSpiChannel, &spiMessage[0]);

    //Dummy for read register value.
    spiMessage[1].SPIMSG_uiLen = 1;
    spiMessage[1].SPIMSG_pucWrBuffer = &uiSpiDummy;
    spiMessage[1].SPIMSG_pucRdBuffer = &uiReadData;
    spi_netif_transfer_shell(pNetInfo->ENET_uiSpiChannel, &spiMessage[1]);

    SPI_DM9051_CS_HIGH();

    return  (uiReadData);
}
/*********************************************************************************************************
** 函数名称: __dm9051ShellWrite
** 功能描述: DM9051 设备读操作
** 输　入  : NONE
** 输　出  : 错误码
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static void __dm9051ShellWrite (ENET *pNetInfo, uint8_t regAddr, uint8_t regData)
{
    uint8_t          cmdaddr;
    uint8_t          uiSpiDummy = 0;
    LW_SPI_MESSAGE   spiMessage[2];

    lib_memset(spiMessage, 0, sizeof(LW_SPI_MESSAGE) * 2);

    /*
     * SPI write command: bit7 = 1,  bit[6:0] = Reg offset addr.
     */
    cmdaddr = (regAddr | 0x80);

    SPI_DM9051_CS_LOW();

    //Read command + Register offset address
    spiMessage[0].SPIMSG_uiLen = 1;
    spiMessage[0].SPIMSG_pucWrBuffer = &cmdaddr;
    spiMessage[0].SPIMSG_pucRdBuffer = &uiSpiDummy;
    spi_netif_transfer_shell(pNetInfo->ENET_uiSpiChannel, &spiMessage[0]);

    //Read command + Register offset address
    spiMessage[1].SPIMSG_uiLen = 1;
    spiMessage[1].SPIMSG_pucWrBuffer = &regData;
    spiMessage[1].SPIMSG_pucRdBuffer = &uiSpiDummy;
    spi_netif_transfer_shell(pNetInfo->ENET_uiSpiChannel, &spiMessage[1]);

    SPI_DM9051_CS_HIGH();

    return;
}
/*********************************************************************************************************
** 函数名称: __dm9051MdioRead
** 功能描述: GMAC mdio 读操作
** 输　入  : PHY_DEV  *pPhyDev, UCHAR  ucReg, UINT16  *pusVal
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __dm9051MdioRead (PHY_DEV  *pPhyDev, UCHAR  ucReg, UINT16  *pusVal)
{
    struct netdev  *pNetDev  = pPhyDev->PHY_pvMacDrv;
           ENET    *pNetInfo = pNetDev->priv;

    if (!pPhyDev || !pusVal) {
        return  (PX_ERROR);
    }

    if (!pNetInfo) {
        return  (PX_ERROR);
    }

    /* Fill the phyxcer register into REG_0C */
    __dm9051Write(pNetInfo, DM9051_EPAR, DM9051_PHY | ucReg);

    /* Issue phyxcer read command */
    __dm9051Write(pNetInfo, DM9051_EPCR, 0xc);

    /* Wait read complete; _dm9000_Delay_ms(100); */
    while (__dm9051Read(pNetInfo, DM9051_EPCR) & 0x1) {
        usleep(10);
    };

    /* Clear phyxcer read command */
    __dm9051Write(pNetInfo, DM9051_EPCR, 0x0);

    *pusVal = (__dm9051Read(pNetInfo, DM9051_EPDRH) << 8) | __dm9051Read(pNetInfo, DM9051_EPDRL);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __dm9051MdioWrite
** 功能描述: GMAC mdio 写操作
** 输　入  : PHY_DEV  *pPhyDev, UCHAR  ucReg, UINT16  usVal
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __dm9051MdioWrite (PHY_DEV  *pPhyDev, UCHAR  ucReg, UINT16  usVal)
{
    struct netdev  *pNetDev  = pPhyDev->PHY_pvMacDrv;
           ENET    *pNetInfo = pNetDev->priv;

    if (!pPhyDev || !pNetInfo) {
        return  (PX_ERROR);
    }

    /* Fill the phyxcer register into REG_0C */
    __dm9051Write(pNetInfo, DM9051_EPAR, DM9051_PHY | ucReg);

    /* Fill the written data into REG_0D & REG_0E */
    __dm9051Write(pNetInfo, DM9051_EPDRL, (usVal & 0xff));
    __dm9051Write(pNetInfo, DM9051_EPDRH, ((usVal >> 8) & 0xff));

    /* Issue phyxcer write command */
    __dm9051Write(pNetInfo, DM9051_EPCR, 0xa);

    /* Wait write complete; _dm9000_Delay_ms(500); */
    while (__dm9051Read(pNetInfo, DM9051_EPCR) & 0x1) {
        usleep(10);
    };

    /* Clear phyxcer write command */
    __dm9051Write(pNetInfo, DM9051_EPCR, 0x0);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __dm9051PhyModeSet
** 功能描述: 设置 PHY 的工作模式
** 输　入  : pvArg  :  网络结构，ulVector 中断号
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __dm9051PhyModeSet (PHY_DEV  *pPhyDev, uint32_t media_mode)
{
    uint16_t phy_reg4 = 0x01e1, phy_reg0 = 0x1000;

    if (!(media_mode & DM9051_AUTO))
    {
        switch (media_mode)
        {
        case DM9051_10MHD:
            phy_reg4 = 0x21;
            phy_reg0 = 0x0000;
            break;
        case DM9051_10MFD:
            phy_reg4 = 0x41;
            phy_reg0 = 0x1100;
            break;
        case DM9051_100MHD:
            phy_reg4 = 0x81;
            phy_reg0 = 0x2000;
            break;
        case DM9051_100MFD:
            phy_reg4 = 0x101;
            phy_reg0 = 0x3100;
            break;
        case DM9051_10M:
            phy_reg4 = 0x61;
            phy_reg0 = 0x1200;
            break;
        }

        /* Set PHY media mode */
        __dm9051MdioWrite(pPhyDev, 4, phy_reg4);

        /* Write rphy_reg0 to Tmp */
        __dm9051MdioWrite(pPhyDev, 0, phy_reg0);

        /* Wait for PHY work mode stable */
        usleep(10000);
    }
}
/*********************************************************************************************************
** 函数名称: __dm9051MacAddrSet
** 功能描述: 设置 MAC 的物理地址
** 输　入  : pvArg  :  网络结构，ulVector 中断号
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __dm9051MacAddrSet (ENET *pNetInfo, const uint8_t *mac_addr)
{
    __dm9051Write(pNetInfo, DM9051_PAR + 0, mac_addr[0]);
    __dm9051Write(pNetInfo, DM9051_PAR + 1, mac_addr[1]);
    __dm9051Write(pNetInfo, DM9051_PAR + 2, mac_addr[2]);
    __dm9051Write(pNetInfo, DM9051_PAR + 3, mac_addr[3]);
    __dm9051Write(pNetInfo, DM9051_PAR + 4, mac_addr[4]);
    __dm9051Write(pNetInfo, DM9051_PAR + 5, mac_addr[5]);

    if (__dm9051Read(pNetInfo, DM9051_PAR) != mac_addr[0]) {
        printk("[__dm9051MacAddrSet]: mac address setting failed!\n");
    } else {
        printk("[__dm9051MacAddrSet]: mac address setting success!\n");
    }
}
/*********************************************************************************************************
** 函数名称: __dm9051WriteMem
** 功能描述: DM9051_Read_Mem, DM9051 burst read command: SPI_RD_BURST = 0x72
** 输　入  : VOID  *pvMacDrv
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static inline void __dm9051ReadMem(ENET *pNetInfo, uint8_t *pucData, uint16_t uiDateLen)
{
    uint32_t    i;
    uint8_t     burstcmd = SPI_RD_BURST;

    /*
     * Read SPI_Data_Array back from the slave
     */
    SPI_DM9051_CS_LOW();

    __dm9051SendByte(pNetInfo, burstcmd);

    for ( i = 0; i < uiDateLen; i++) {
        pucData[i] = __dm9051SendByte(pNetInfo, 0x0);
    }

    SPI_DM9051_CS_HIGH();
}
/*********************************************************************************************************
** 函数名称: __dm9051WriteMem
** 功能描述: DM9051_Write_Mem, DM9051 burst write command: SPI_WR_BURST = 0xF8
** 输　入  : VOID  *pvMacDrv
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static inline void __dm9051WriteMem (ENET *pNetInfo, uint8_t *pucData, uint16_t uiDateLen)
{
    uint32_t  i;
    uint8_t   burstcmd = SPI_WR_BURST;

    /*
     * SPI_WR_BURST: Send the array to the slave
     */
    SPI_DM9051_CS_LOW();

    __dm9051SendByte(pNetInfo, burstcmd);

    for (i = 0; i < uiDateLen; i++) {
        __dm9051SendByte(pNetInfo, pucData[i]);
    }

    SPI_DM9051_CS_HIGH();
}
/*********************************************************************************************************
** 函数名称: __dm9051BuffInit
** 功能描述: 初始化  ENET 缓冲描述符
** 输　入  : pEnet : ENET 结构指针
** 输　出  : 错误码
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __dm9051BuffInit (ENET  *pEnet)
{
    INT     i;

    if (!pEnet) {
        return  (PX_ERROR);
    }
    /*
     * 分配物理地址连续的内存供DMA使用，
     */
    pEnet->ENET_pRxDesc   = (VOID *)__SHEAP_ALLOC_ALIGN(CONFIG_RX_DESCR_NUM * sizeof(DMA_BUF_DESC), ARCH_DMA_MINALIGN);
    pEnet->ENET_pTxDesc   = (VOID *)__SHEAP_ALLOC_ALIGN(CONFIG_TX_DESCR_NUM * sizeof(DMA_BUF_DESC), ARCH_DMA_MINALIGN);
    pEnet->ENET_pRxBuffer = (VOID *)__SHEAP_ALLOC_ALIGN(CONFIG_RX_DESCR_NUM * CONFIG_ETH_BUFSIZE, 4);
    pEnet->ENET_pTxBuffer = (VOID *)__SHEAP_ALLOC_ALIGN(CONFIG_TX_DESCR_NUM * CONFIG_ETH_BUFSIZE, 4);

    if (pEnet->ENET_pRxDesc   == NULL ||
        pEnet->ENET_pTxDesc   == NULL ||
        pEnet->ENET_pRxBuffer == NULL ||
        pEnet->ENET_pTxBuffer == NULL) {

        if (pEnet->ENET_pRxDesc) {
            __SHEAP_FREE(pEnet->ENET_pRxDesc);
        }
        if (pEnet->ENET_pTxDesc) {
            __SHEAP_FREE(pEnet->ENET_pTxDesc);
        }
        if (pEnet->ENET_pRxBuffer) {
            __SHEAP_FREE(pEnet->ENET_pRxBuffer);
        }
        if (pEnet->ENET_pTxBuffer) {
            __SHEAP_FREE(pEnet->ENET_pTxBuffer);
        }
        return  (PX_ERROR);
    }

    for (i = 0; i < CONFIG_RX_DESCR_NUM; i++) {
        pEnet->ENET_pRxDesc[i].uiAddr   = (UINT32)(pEnet->ENET_pRxBuffer + i * CONFIG_ETH_BUFSIZE);
        pEnet->ENET_pRxDesc[i].uiSize   = CONFIG_ETH_RXSIZE;
        pEnet->ENET_pRxDesc[i].uiNext   = (UINT32)&pEnet->ENET_pRxDesc[i+1];
        pEnet->ENET_pRxDesc[i].uiStatus = BIT(31);
    }
    pEnet->ENET_pRxDesc[CONFIG_RX_DESCR_NUM - 1].uiNext = (UINT32)&pEnet->ENET_pRxDesc[0];
    pEnet->ENET_uiRxDescNum = 0;

    for (i = 0; i < CONFIG_TX_DESCR_NUM; i++) {
        pEnet->ENET_pTxDesc[i].uiAddr   = (UINT32)(pEnet->ENET_pTxBuffer + i * CONFIG_ETH_BUFSIZE);
        pEnet->ENET_pTxDesc[i].uiSize   = 0;
        pEnet->ENET_pTxDesc[i].uiNext   = (UINT32)&pEnet->ENET_pTxDesc[i+1];
        pEnet->ENET_pTxDesc[i].uiStatus = BIT(31);
    }
    pEnet->ENET_pTxDesc[CONFIG_TX_DESCR_NUM - 1].uiNext = (UINT32)&pEnet->ENET_pTxDesc[0];
    pEnet->ENET_uiTxDescNum = 0;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __dm9051EnetIsr
** 功能描述: Lwip 中断响应函数
** 输　入  : pvArg  :  网络结构，ulVector 中断号
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static irqreturn_t __dm9051EnetIsr (PVOID pvArg, ULONG ulVector)
{
            UINT8    reg;
            UINT8    int_status;
            ENET     *pNetInfo;
    struct netdev    *pNetDev = (struct netdev *)pvArg;

    pNetInfo = (ENET *)(pNetDev->priv);

    if (API_GpioSvrIrq(KENDRYTE_GET_GPIOHS_ID(INT_GPIONUM)) == LW_IRQ_NONE) {
        return  (LW_IRQ_NONE);
    }

    API_GpioClearIrq(KENDRYTE_GET_GPIOHS_ID(INT_GPIONUM));              /* 判断是否是该GPIO的中断并处理 */
    API_InterVectorDisable(pNetInfo->ENET_ulIrqLine);

    reg = __dm9051ShellRead(pNetInfo, DM9051_IMR);                      /*  Disable all interrupts      */
    __dm9051ShellWrite(pNetInfo, DM9051_IMR, IMR_PAR);

    int_status = __dm9051ShellRead(pNetInfo, DM9051_ISR);               /*  Get ISR status              */
    __dm9051ShellWrite(pNetInfo, DM9051_ISR, int_status);               /*  Clear ISR status            */

    if (int_status & ISR_ROS) {
        netif_debug("dm9051_isr: rx overflow\n");
        netdev_linkinfo_err_inc(pNetInfo->ENET_pNetDev);
    }

    if (int_status & ISR_ROOS) {
        netif_debug("dm9051_isr: rx overflow counter overflow\n");
        netdev_linkinfo_err_inc(pNetInfo->ENET_pNetDev);
    }

    if (int_status & ISR_PRS) {                                         /*  Received the coming packet  */
        netdev_notify(pNetInfo->ENET_pNetDev, LINK_INPUT, 1);
        reg &= ~(IMR_PRM);                                              /*  disable recv interrupt      */
    }

    __dm9051ShellWrite(pNetInfo, DM9051_IMR, reg);                      /*  Re-enable interrupt mask    */

    API_InterVectorEnable(pNetInfo->ENET_ulIrqLine);

    return  (LW_IRQ_HANDLED);
}
/*********************************************************************************************************
** 函数名称: __dm9051NetDevSysInit
** 功能描述: GMAC硬件初始化
** 输　入  : pvArg  :  网络结构，ulVector 中断号
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __dm9051NetDevSysInit (struct netdev  *pNetDev)
{
    ENET     *pNetInfo;
    PHY_DEV  *pPhyDev;

    pNetInfo = pNetDev->priv;
    pPhyDev  = pNetInfo->ENET_phy_dev;

    __dm9051Write(pNetInfo, DM9051_NCR, DM9051_REG_RESET);              /*  RESET device                */
    usleep(10);
    netif_debug("[DEBUG-(0x%x)]: must be zero 0x%x\r\n", DM9051_NCR, __dm9051Read(pNetInfo, DM9051_NCR));
    __dm9051Write(pNetInfo, DM9051_NCR, 0);

    __dm9051Write(pNetInfo, DM9051_NCR, DM9051_REG_RESET);              /*  RESET device                */
    usleep(10);
    __dm9051Write(pNetInfo, DM9051_NCR, 0);

    __dm9051Write(pNetInfo, DM9051_GPCR, GPCR_GEP_CNTL);
    __dm9051Write(pNetInfo, DM9051_GPR, 0x00);                          /*  Power on PHY                */

    usleep(1000 * 1000);
    netif_debug("[POWER ON PHY]: 0x%x\r\n", __dm9051Read(pNetInfo, DM9051_GPR));

    __dm9051PhyModeSet(pPhyDev, DM9051_AUTO);                           /*  set PHY link mode           */

    __dm9051MacAddrSet(pNetInfo, pNetDev->hwaddr);                      /*  set MAC address             */

    for (size_t i = 0; i < 8; i++) {                                    /*  set multicast address       */
        __dm9051Write(pNetInfo, DM9051_MAR + i, (7 == i) ? 0x80 : 0x00);
    }

    /* Activate DM9051 */
    /* Setup DM9051 Registers */
    __dm9051Write(pNetInfo, DM9051_NCR, NCR_DEFAULT);
    __dm9051Write(pNetInfo, DM9051_TCR, TCR_DEFAULT);
    //DM9051_Write_Reg(DM9000_TCR, 0x20); //Disable underrun retry.
    __dm9051Write(pNetInfo, DM9051_RCR, RCR_DEFAULT);
    __dm9051Write(pNetInfo, DM9051_BPTR, BPTR_DEFAULT);
    //DM9051_Write_Reg(DM9000_FCTR, FCTR_DEAFULT);
    __dm9051Write(pNetInfo, DM9051_FCTR, 0x3A);
    __dm9051Write(pNetInfo, DM9051_FCR,  FCR_DEFAULT);
    //DM9051_Write_Reg(DM9000_FCR,  0x0); //Disable Flow_Control
    __dm9051Write(pNetInfo, DM9051_SMCR, SMCR_DEFAULT);
    __dm9051Write(pNetInfo, DM9051_TCR2, DM9051_TCR2_SET);
    //DM9051_Write_Reg(DM9000_TCR2, 0x80);
    __dm9051Write(pNetInfo, DM9051_INTR, 0x1);

    /* Clear status */
    __dm9051Write(pNetInfo, DM9051_NSR, NSR_CLR_STATUS);
    __dm9051Write(pNetInfo, DM9051_ISR, ISR_CLR_STATUS);

    __dm9051Write(pNetInfo, DM9051_IMR, IMR_PAR | IMR_PRM);
    __dm9051Write(pNetInfo, DM9051_RCR, (RCR_DEFAULT | RCR_RXEN)); /* Enable RX */

    return (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: enetCoreInit
** 功能描述: Lwip 网络初始化函数
** 输　入  : pNetDev  :  网络结构
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __dm9051CoreInit (struct netdev  *pNetDev)
{
    ENET        *pNetInfo;
    PHY_DEV     *pPhyDev;

    if (!pNetDev) {
        printk("[enetCoreInit]: invalid pNetDev.\n");
        return  (PX_ERROR);
    }

    pNetInfo = (ENET *)(pNetDev->priv);
    pPhyDev  = pNetInfo->ENET_phy_dev;

    if (!pNetInfo || !pPhyDev) {
        printk("[enetCoreInit]: invalid pNetInfo or pPhyDev.\n");
        return  (PX_ERROR);
    }

    if (__dm9051NetDevSysInit(pNetDev) != ERROR_NONE) {
        return  (PX_ERROR);
    }

    API_InterVectorConnect(pNetInfo->ENET_ulIrqLine, (PINT_SVR_ROUTINE)__dm9051EnetIsr, (PVOID)pNetDev, "enet_isr");
    API_InterVectorEnable(pNetInfo->ENET_ulIrqLine);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __dm9051Receive
** 功能描述: 网络接收函数
** 输　入  : pNetDev  :  网络结构, input 提交数据到协议栈的回调函数指针
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __dm9051Receive (struct netdev *netdev, int (*input)(struct netdev *, struct pbuf *))
{
    struct pbuf  *pBuf;
    ENET         *pNetInfo = netdev->priv;
    DMA_BUF_DESC *pDesc    = &pNetInfo->ENET_pRxDesc[pNetInfo->ENET_uiRxDescNum];

    UINT8    reg;
    UINT8    rx_byte;
    UINT16   rx_len;
    UINT16   rx_status;
    int      good_pkt;
    uint8_t  header[4];

    /*
     * Step1: lock DM9051
     */
    API_SemaphoreMPend(pNetInfo->ENET_lock, LW_OPTION_WAIT_INFINITE);
    API_InterVectorDisable(pNetInfo->ENET_ulIrqLine);                   /* disable dm9000a interrupt    */

    /*
     * Step2: Check packet ready or not
     */
    rx_byte = __dm9051Read(pNetInfo, DM9051_MRCMDX);
    rx_byte = __dm9051Read(pNetInfo, DM9051_MRCMDX);
    rx_byte = rx_byte & 0x03;

    if (rx_byte & DM9051_PKT_ERR) {
        netdev_linkinfo_err_inc(netdev);
        goto    __recv_over;
    }

    if (!(rx_byte & DM9051_PKT_RDY)) {
        goto    __recv_over;
    }

    good_pkt = TRUE;

    /*
     * Step3: Get packet status and length
     */
    __dm9051Read(pNetInfo, DM9051_MRCMDX);
    __dm9051ReadMem(pNetInfo, header, 4);

    rx_status = header[0] | (header[1] << 8);
    rx_len    = header[2] | (header[3] << 8);

    if (rx_len < 0x40) {
        netdev_linkinfo_lenerr_inc(netdev);
        good_pkt = FALSE;
    }

    if (rx_len > DM9051_PKT_MAX) {
        netdev_linkinfo_lenerr_inc(netdev);
        good_pkt = FALSE;
    }

    rx_status >>= 8;
    if (rx_status & (RSR_FOE | RSR_CE | RSR_AE |
                     RSR_PLE | RSR_RWTO |
                     RSR_LCS | RSR_RF)) {
        if (rx_status & RSR_FOE) {
            netdev_linkinfo_err_inc(netdev);
        }

        if (rx_status & RSR_CE) {
            netdev_linkinfo_chkerr_inc(netdev);
        }

        if (rx_status & RSR_RF) {
            netdev_linkinfo_lenerr_inc(netdev);
        }
        good_pkt = FALSE;
    }

    /*
     * Step4: Process packet data and packet statistics
     */
    if (good_pkt) {

        __dm9051ReadMem(pNetInfo, (uint8_t *)pDesc->uiAddr, rx_len);    /*  read_memory                 */

        pBuf = netdev_pbuf_alloc(rx_len);

        if (!pBuf) {
            netdev_linkinfo_memerr_inc(netdev);
            netdev_linkinfo_drop_inc(netdev);
            netdev_statinfo_discards_inc(netdev, LINK_INPUT);
            netif_debug("[__dm9051CoreRecv]: netdev_pbuf_alloc failed!\n");
        } else {
            pbuf_take(pBuf, (void *)pDesc->uiAddr, (UINT16)rx_len);

            if (input(netdev, pBuf)) {                                  /*  提交数据到协议栈            */
                netdev_pbuf_free(pBuf);
                netdev_linkinfo_memerr_inc(netdev);
                netdev_statinfo_discards_inc(netdev, LINK_INPUT);
            } else {
                netdev_linkinfo_recv_inc(netdev);
                netdev_statinfo_total_add(netdev, LINK_INPUT, rx_len);  /*   统计发送数据长度           */
                if (((UINT8 *)pBuf->payload)[0] & 1) {
                    netdev_statinfo_mcasts_inc(netdev, LINK_INPUT);     /*   统计发送广播数据包数       */
                } else {
                    netdev_statinfo_ucasts_inc(netdev, LINK_INPUT);     /*   统计发送单播数据包数       */
                }
            }

            /* Move to next desc and wrap-around condition. */
            if (pNetInfo->ENET_uiRxDescNum == CONFIG_RX_DESCR_NUM -1) {
                pNetInfo->ENET_uiRxDescNum = 0;
            } else {
                pNetInfo->ENET_uiRxDescNum ++;
            }
			
        }

    }

__recv_over:
                                                                        /* Reset RX FIFO pointer        */
    __dm9051Write(pNetInfo, DM9051_RCR, RCR_DEFAULT);                   /* RX disable                   */
    __dm9051Write(pNetInfo, DM9051_MPCR, 0x01);                         /* Reset RX FIFO pointer        */
    usleep(2);
    __dm9051Write(pNetInfo, DM9051_RCR, (RCR_DEFAULT | RCR_RXEN));      /* RX Enable                    */

    reg  = __dm9051Read(pNetInfo, DM9051_IMR);                          /* restore receive interrupt    */
    reg |= IMR_PRM;
    __dm9051Write(pNetInfo, DM9051_IMR, reg);

    API_InterVectorEnable(pNetInfo->ENET_ulIrqLine);
    API_SemaphoreMPost(pNetInfo->ENET_lock);
}
/*********************************************************************************************************
** 函数名称: __dm9051Transmit
** 功能描述: 网络发送函数
** 输　入  : pNetDev  :  网络结构
**           pbuf     :  lwip 核心 buff
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static int  __dm9051Transmit (struct  netdev  *pNetDev, struct  pbuf  *pbuf)
{
    ENET          *pNetInfo  = pNetDev->priv;
    DMA_BUF_DESC  *pDesc     = &pNetInfo->ENET_pTxDesc[pNetInfo->ENET_uiTxDescNum];
    SINT32         iLinkUp;
    size_t         length    = 0;
    UINT32         uiTimeout = 10000;

    /*
     * 如果网络是断开状态，返回错误，flags 在用户程序中为只读，其设置在
     * link_down link_up 函数中设置
     */
    netdev_get_linkup(pNetDev, &iLinkUp);
    if (!iLinkUp) {
        netdev_linkinfo_err_inc(pNetDev);
        netdev_linkinfo_drop_inc(pNetDev);
        return  (PX_ERROR);
    }

    /* lock DM9051 */
    API_SemaphoreMPend(pNetInfo->ENET_lock, LW_OPTION_WAIT_INFINITE);

    /* disable dm9000a interrupt */
    API_InterVectorDisable(pNetInfo->ENET_ulIrqLine);
    //__dm9051Write(pNetInfo, DM9051_IMR, IMR_PAR);

    /*
     * 发送队列已满延时一段时间，再尝试发送
     */
    while ((__dm9051Read(pNetInfo, DM9051_TCR) & DM9051_TCR_SET) && uiTimeout--) {
         bspDelayUs(5);
    }

    if (uiTimeout == 0) {
        printk("emac send queue full!!\n");
        return  (PX_ERROR);
    }

    /*
     * copy sending data from pbuf to net_tx_buf
     */
    length = pbuf->tot_len;
    pbuf_copy_partial(pbuf, (void *)pDesc->uiAddr, pbuf->tot_len, 0);

    /*
     * Issue TX length command
     */
    __dm9051Write(pNetInfo, DM9051_TXPLL, length & 0xff);
    __dm9051Write(pNetInfo, DM9051_TXPLH, (length >> 8) & 0xff);

    /*
     * Write data to FIFO
     */
    __dm9051WriteMem(pNetInfo, (uint8_t *)pDesc->uiAddr, length);

    /*
     * Issue TX polling end command
     */
    __dm9051Write(pNetInfo, DM9051_TCR, TCR_TXREQ);

    /*
     * Move to next Descriptor and wrap around
     */
    if (pNetInfo->ENET_uiTxDescNum == CONFIG_TX_DESCR_NUM -1) {
        pNetInfo->ENET_uiTxDescNum = 0;
    } else {
        pNetInfo->ENET_uiTxDescNum ++;
    }
    KN_SMP_WMB();

    netdev_linkinfo_xmit_inc(pNetDev);
    netdev_statinfo_total_add(pNetDev, LINK_OUTPUT, pbuf->tot_len);

    if (((UINT8 *)pbuf->payload)[0] & 1) {
        netdev_statinfo_mcasts_inc(pNetDev, LINK_OUTPUT);               /*   统计发送广播数据包数       */
    } else {
        netdev_statinfo_ucasts_inc(pNetDev, LINK_OUTPUT);               /*   统计发送单播数据包数       */
    }

    /* enable dm9000a interrupt    */
    //__dm9051Write(pNetInfo, DM9051_IMR, dm9000_device.imr_all);
    API_InterVectorEnable(pNetInfo->ENET_ulIrqLine);

    /* unlock DM9051 */
    API_SemaphoreMPost(pNetInfo->ENET_lock);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: sifive_net_link_check
** 功能描述: 以太网状态监测线程函数
** 输　入  : pvArg   参数(网络设备对象)
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static void  __dm9051Watchdog (struct netdev  *netdev)
{
    ENET   *pNetInfo = netdev->priv;
    UINT8   nsr, ncr;
    int     linkup;
    UINT16  uiValue;

    API_InterVectorDisable(pNetInfo->ENET_ulIrqLine);

    __dm9051MdioRead(pNetInfo->ENET_phy_dev, 1, &uiValue);

    if (uiValue & 0x20) {
        netdev_get_linkup(netdev, &linkup);
        if (!linkup || (netdev->speed == 0)) {
            nsr = __dm9051Read(pNetInfo, DM9051_NSR);
            ncr = __dm9051Read(pNetInfo, DM9051_NCR);
            netdev_set_linkup(netdev, 1, (nsr & NSR_SPEED) ? 10000000 : 100000000);
            _PrintFormat("dm9051_watchdog: operating at %dM %s duplex mode\n",
                   (nsr & NSR_SPEED) ? 10 : 100, (ncr & NCR_FDX) ? "full" : "half");
        }

    } else {
        netdev_get_linkup(netdev, &linkup);
        if (linkup) {
            netdev_set_linkup(netdev, 0, 0);
            _PrintFormat("dm9051_watchdog: can not establish link\n");
        }
    }

    API_InterVectorEnable(pNetInfo->ENET_ulIrqLine);
}
/*********************************************************************************************************
** 函数名称: __dm9051Config
** 功能描述: ENET 初始化
** 输　入  : NONE
** 输　出  : 错误码
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __dm9051Config (ENET *pNetInfo, fpioa_cfg_t *pFpioaConfig)
{
    INT     iResult = ERROR_NONE;
    ULONG   irqVector;

    /*
     * FPIOA INIT
     */
    if (pFpioaConfig != LW_NULL) {                                      /* 如果不为空，则使用用户的配置 */
        API_FpioaSetup(pFpioaConfig);
    } else {                                                            /* 如果为空，则使用默认的配置   */
        fpioa_set_function(41, FUNC_SPI0_SCLK);
        fpioa_set_function(42, FUNC_SPI0_D1);
        fpioa_set_function(43, FUNC_SPI0_D0);
        fpioa_set_function(44, FUNC_GPIOHS0 + NETIF_CS_PIN);
        fpioa_set_function(33, FUNC_GPIOHS0 + INT_GPIONUM);

        fpioa_set_function(46, FUNC_GPIOHS0 + NET_POWER_PIN);
        fpioa_set_function(45, FUNC_GPIOHS0 + NET_POWER_PIN2);          /* NET_POWER_PIN2               */
        fpioa_set_function(31, FUNC_GPIOHS0 + NET_GND_PIN);
        fpioa_set_function(24, FUNC_GPIO3);                             /* NET_GND_PIN2                 */
    }

    /*
     * configure netif interrupt line
     */
    API_GpioRequestOne(KENDRYTE_GET_GPIOHS_ID(INT_GPIONUM), LW_GPIOF_OUT_INIT_HIGH, "Kendryte K210 GPIOHS");
    API_GpioDirectionInput(KENDRYTE_GET_GPIOHS_ID(INT_GPIONUM));
    irqVector = API_GpioSetupIrq(KENDRYTE_GET_GPIOHS_ID(INT_GPIONUM), 0, GPIO_PE_FALLING);
    if (irqVector == LW_VECTOR_INVALID) {
        printk("[enetInit]: error gpio irq vector\n");
        iResult = PX_ERROR;
        goto  err_handle;
    }
    API_InterVectorDisable(irqVector);
    pNetInfo->ENET_ulIrqLine = irqVector;

    /*
     * configure chip select pin
     */
    API_GpioRequestOne(KENDRYTE_GET_GPIOHS_ID(NETIF_CS_PIN), LW_GPIOF_OUT_INIT_HIGH, "Kendryte K210 GPIOHS");
    API_GpioDirectionOutput(KENDRYTE_GET_GPIOHS_ID(NETIF_CS_PIN), 0);

    /*
     * configure power pin & grand pin
     */
    API_GpioRequestOne(KENDRYTE_GET_GPIOHS_ID(NET_POWER_PIN), LW_GPIOF_OUT_INIT_HIGH, "Kendryte K210 GPIOHS");
    API_GpioDirectionOutput(KENDRYTE_GET_GPIOHS_ID(NET_POWER_PIN), 1);
    API_GpioRequestOne(KENDRYTE_GET_GPIOHS_ID(NET_POWER_PIN2), LW_GPIOF_OUT_INIT_HIGH, "Kendryte K210 GPIOHS");
    API_GpioDirectionOutput(KENDRYTE_GET_GPIOHS_ID(NET_POWER_PIN2), 1);
    API_GpioRequestOne(KENDRYTE_GET_GPIOHS_ID(NET_GND_PIN), LW_GPIOF_OUT_INIT_LOW, "Kendryte K210 GPIOHS");
    API_GpioDirectionOutput(KENDRYTE_GET_GPIOHS_ID(NET_GND_PIN), 0);
    API_GpioRequestOne(NET_GND_PIN2, LW_GPIOF_OUT_INIT_LOW, "Kendryte K210 GPIO");
    API_GpioDirectionOutput(NET_GND_PIN2, 0);
    API_GpioSetPull(NET_GND_PIN2, 0);

    /*
     * create spi device
     */
    //pNetInfo->ENET_pSpiNetDev = API_SpiDeviceCreate(SPI_ADAPTER_NAME, SPI_NETIF_DEVNAME);
    if (pNetInfo->ENET_uiSpiChannel >= MAX_ADAPTER_NUM) {
        printk("[enetInit]: error spi channel\n");
        iResult = PX_ERROR;
        goto  err_handle;
    }
    pNetInfo->ENET_pSpiNetDev = API_SpiDeviceCreate(_G_pcSpiNetifAdapterName[pNetInfo->ENET_uiSpiChannel], SPI_NETIF_DEVNAME);
    API_SpiDeviceCtl(pNetInfo->ENET_pSpiNetDev, LW_SPI_CTL_BAUDRATE, 200000); //200KHz
    usleep(100);

err_handle:
    return  (iResult);
}
/*********************************************************************************************************
** 函数名称: enetInit
** 功能描述: ENET 初始化
** 输　入  : NONE
** 输　出  : 错误码
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  enetInit (LINK_SPEED  linkspeed, fpioa_cfg_t  *pFpioaConfig, UINT  uiSpiChannel)
{
    UINT32            uiValue = 0;
    LW_OBJECT_HANDLE  handle;

    static  PHY_DRV_FUNC  phyDrvFunc = {                                /*  PHY 驱动结构体              */
        .PHYF_pfuncWrite       = (FUNCPTR)__dm9051MdioWrite,
        .PHYF_pfuncRead        = (FUNCPTR)__dm9051MdioRead,
        .PHYF_pfuncLinkDown    = (FUNCPTR)NULL,
        .PHYF_pfuncLinkSetHook = (FUNCPTR)NULL,
    };

    static  PHY_DEV  phyDev = {                                         /*  PHY 设备结构体              */
        .PHY_pPhyDrvFunc    = &phyDrvFunc,
        .PHY_ucPhyAddr      = DM9051_PHY,
        .PHY_uiPhyID        = 0x181b8a0,                                /*  和之前驱动的ID高低位反的    */
        .PHY_uiPhyIDMask    = 0,                                        /*  同系列芯片不同信号识别      */
        .PHY_uiTryMax       = 1000,
        .PHY_uiLinkDelay    = 3,                                        /*  延时100毫秒自动协商过程     */
        .PHY_uiPhyFlags     = MII_PHY_AUTO |                            /*  自动协商标志                */
                              MII_PHY_FD   |                            /*  全双工模式                  */
                              MII_PHY_100  |                            /*  100Mbit                     */
                              MII_PHY_10   |                            /*  10Mbit                      */
                              MII_PHY_FD   |                            /*  全双工模式                  */
                              MII_PHY_HD   |                            /*  全双工模式                  */
                              MII_PHY_1000T_FD   |                      /*  1000Mbit全双工              */
                              MII_PHY_1000T_HD   |                      /*  1000Mbit半双工              */
                              MII_PHY_GMII_TYPE  |                      /*  千兆                        */
                              MII_PHY_MONITOR,                          /*  启用自动监视功能            */
    };

    static  ENET  enetInfo  = {                                         /* 自定义网卡实例结构体         */
        .ENET_phy_dev = &phyDev,
    };

    static  struct  netdev_funcs  net_drv = {                           /*  网卡 驱动结构体             */
        .init      = __dm9051CoreInit,
        .transmit  = __dm9051Transmit,
        .receive   = __dm9051Receive,
    };

    static  struct  netdev  netDev = {                                  /*  网卡 设备结构体             */
        .priv         = &enetInfo,
        .magic_no     = NETDEV_MAGIC,
        .dev_name     = "spi_netif",
        .if_name      = "en",
        .if_hostname  = "SylixOS",
        .init_flags   = NETDEV_INIT_LOAD_PARAM
                      | NETDEV_INIT_LOAD_DNS
                      | NETDEV_INIT_IPV6_AUTOCFG
                      | NETDEV_INIT_AS_DEFAULT,

        .chksum_flags = NETDEV_CHKSUM_ENABLE_ALL,
        .net_type     = NETDEV_TYPE_ETHERNET,
        .speed        = 0,
        .mtu          = 1500,
        .drv          = &net_drv,
        .hwaddr_len   = 6,
        .hwaddr       = {0x04,0x02,0x35,0x00,0x00,0x01},
    };

    phyDev.PHY_pvMacDrv = &netDev;
    enetInfo.ENET_pNetDev = &netDev;
    enetInfo.ENET_uiSpiChannel = uiSpiChannel;

    handle = API_SemaphoreMCreate("dm9051_lock", 0,
                                  LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                  &enetInfo.ENET_lock);
    if (handle == LW_HANDLE_INVALID) {
        return  (-1);
    }

    handle = API_SemaphoreBCreate("dm9051_txsync", 0,
                                  LW_OPTION_WAIT_FIFO | LW_OPTION_OBJECT_GLOBAL,
                                  &enetInfo.ENET_tx_sync);
    if (handle == LW_HANDLE_INVALID) {
        API_SemaphoreMDelete(&enetInfo.ENET_lock);
        return  (-1);
    }

    if (ERROR_NONE != __dm9051Config(&enetInfo, pFpioaConfig)) {        /* Configure spi netif pins     */
        printk("[__dm9051Config]: dm9051 spi interface config failed!\n");
        goto  error_handle;
    }

    __dm9051Write(&enetInfo, DM9051_PBCR, PBCR_MAXDRIVE);               /* BUS Driving capability       */

#if READ_VID_TEST_EN
    uiValue |= (uint32_t)__dm9051Read(&enetInfo, DM9051_VIDL);          /* Read DM9051 PID / VID        */
    uiValue |= (uint32_t)__dm9051Read(&enetInfo, DM9051_VIDH) << 8;
    uiValue |= (uint32_t)__dm9051Read(&enetInfo, DM9051_PIDL) << 16;
    uiValue |= (uint32_t)__dm9051Read(&enetInfo, DM9051_PIDH) << 24;
    if (uiValue != 0x90510a46) {
        printk("[enetInit]: ERROR DM9051 PID/VID value=0x%x.\n", uiValue);
        goto  error_handle;
    } else {
        printk("[enetInit]: SUCCESS DM9051 PID/VID value=0x%x.\n", uiValue);
        enetInfo.ENET_uiNetifType = TYPE_DM9051;
    }
#endif

    if (ERROR_NONE != __dm9051BuffInit(&enetInfo)) {                    /* prepare net device buffer    */
        printk("[__dm9051BuffDescInit]: dm9051 net buffer alloc failed!\n");
        goto  error_handle;
    }

    if (netdev_add(&netDev, "10.4.0.216", "255.255.0.0", "10.4.0.1",    /* register net device          */
                  IFF_UP | IFF_RUNNING  | IFF_BROADCAST | IFF_MULTICAST) == 0){
        printk("[netdev_add]: SUCCESS!\n");
        netdev_set_tcpwnd(&netDev, 8192);
    } else {
        printk("[netdev_add]: FAILED!\n");
        goto  error_handle;
    }

    enetInfo.ENET_uiPhyMode = DM9051_AUTO;                              /* init phy work mode           */
    enetInfo.ENET_uiLinkStatus = 0;                                     /* init net-if link status      */

    if (linkspeed == DM9051_AUTO) {
        hotplugPollAdd(__dm9051Watchdog, (void *)&netDev);              /* add net-if hot plug poller   */
    } else {
        __dm9051PhyModeSet(&phyDev, linkspeed);                         /* configure phy work mode      */
    }

    return  (ERROR_NONE);

error_handle:
    API_GpioFree(KENDRYTE_GET_GPIOHS_ID(INT_GPIONUM));

    return  (PX_ERROR);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
