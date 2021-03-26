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
** 文   件   名: k210_spiflash.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 11 月 8 日
**
** 描        述: SPI Flash (w25qxx) 驱动
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_YAFFS_DRV
#include "config.h"
#include "SylixOS.h"
#include "driver/spi/k210_spi.h"
#include "driver/spiflash/k210_spiflash.h"
#include "driver/spiflash/w25qxx.h"

#include "KendryteWare/include/fpioa.h"
#include "driver/clock/k210_clock.h"
/*********************************************************************************************************
  norflash 相关参数宏
*********************************************************************************************************/
#define    NOR_BASE_ADDR             (0)
#define    NOR_ERASE_SIZE            (4096)
#define    NOR_BLOCK_SIZE            NOR_ERASE_SIZE
#define    NOR_TOTAL_SIZE            (16777216UL)
#define    NOR_BLOCK_PER_CHIP        (NOR_TOTAL_SIZE / NOR_BLOCK_SIZE)

#define    W_BYTES                   (256)
#define    BUF_MAX                   (4096)

#define    SPI_CHANNEL_NR            (3)
#define    SPI_ADAPTER_NAME          "/bus/spi/3"
#define    SPI_NOR_DEVNAME           "/dev/spiflash"

#define    SPI_LOCK()                API_SpinLock(&_G_k210SpiFlashSpinLock)
#define    SPI_UNLOCK()              API_SpinUnlock(&_G_k210SpiFlashSpinLock)

#define    NOR_DEBUG(fmt, arg...)    printk("[SPI] %s() : " fmt, __func__, ##arg)
/*********************************************************************************************************
 SPI_NOR 内部对象数据结构
*********************************************************************************************************/
typedef struct {
    struct spi_flash    *pspiflash;
    struct yaffs_devnor  devnor;
    PLW_SPI_DEVICE       Spi_NorDev;
    UINT                 uiOffsetBytes;
    UINT                 uiEraseSize;
    UINT                 uiTotalSize;
    UINT                 uiPageSize;
    UINT                 uiSSPin;
    CPCHAR               pcName;
} SPI_NOR_OBJ, *PSPI_NOR_OBJ;
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static  SPI_NOR_OBJ  _G_spiNorObj  = {
        .uiEraseSize = NOR_ERASE_SIZE,
        .uiTotalSize = NOR_TOTAL_SIZE,
};

LW_SPINLOCK_DEFINE(_G_k210SpiFlashSpinLock);

/*********************************************************************************************************
** 函数名称: __k210SpiNorErase
** 功能描述: 擦除一个块
** 输  入  : nordev    nor设备
**           block     块地址
** 输  出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __k210SpiNorErase (struct yaffs_devnor  *nordev, INT  iBlock)
{
    addr_t  addr = norBlock2Addr(nordev, iBlock);
    INT     iRet = -1;

    SPI_LOCK();
    iRet = w25qxx_sector_erase(addr);
    SPI_UNLOCK();

    return  (iRet == 0 ? YAFFS_OK : YAFFS_FAIL);
}
/*********************************************************************************************************
** 函数名称: __k210SpiNorRead
** 功能描述: 数据读取
** 输  入  : nordev    nor设备
**           addr      字节地址
**           data      保存读取的数据
**           len       需要读取数据的长度
** 输  出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __k210SpiNorRead (struct yaffs_devnor *nordev, addr_t addr, VOID *pvData, size_t stLen)
{
    INT     iRet = -1;

    SPI_LOCK();
    iRet = w25qxx_read_data(addr, pvData, stLen, W25QXX_STANDARD);
    SPI_UNLOCK();

    return  (iRet == 0 ? YAFFS_OK : YAFFS_FAIL);
}
/*********************************************************************************************************
** 函数名称: __k210SpiNorWrite
** 功能描述: 数据写入
** 输  入  : nordev    nor设备
**           addr      字节地址
**           data      写入数据缓冲地址
**           len       需要读取数据的长度
** 输  出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __k210SpiNorWrite (struct yaffs_devnor *nordev, addr_t addr, const VOID *pvData, size_t stLen)
{
    INT     iRet = -1;

    SPI_LOCK();
    iRet = w25qxx_write_data(addr, (VOID *)pvData, stLen);
    SPI_UNLOCK();

    return  (iRet == 0 ? YAFFS_OK : YAFFS_FAIL);
}
/*********************************************************************************************************
** 函数名称: __k210SpiNorInitialise
** 功能描述: 芯片初始化
** 输  入  : nordev    nor设备
** 输  出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __k210SpiNorInitialise (struct yaffs_devnor  *nordev)
{
    return  (YAFFS_OK);
}
/*********************************************************************************************************
** 函数名称: __k210SpiNorDeinitialise
** 功能描述: 芯片反初始化
** 输  入  : nordev    nor设备
** 输  出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __k210SpiNorDeinitialise (struct yaffs_devnor  *nordev)
{
    return  (YAFFS_OK);
}
/*********************************************************************************************************
** 函数名称: k210SpiNorFlashIdRead
** 功能描述: 读flash ID : flashRead_jedec_id
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT32 k210SpiNorFlashIdRead (VOID)
{
    LW_SPI_MESSAGE  spiMsg;
    uint8_t         cmd[6]  = {0x90, 0x00, 0x00, 0x00}; //0x90 = READ_ID
    uint8_t         data[6] = {0};

    lib_bzero(&spiMsg, sizeof(spiMsg));
    spiMsg.SPIMSG_pucWrBuffer = cmd;
    spiMsg.SPIMSG_pucRdBuffer = data;
    spiMsg.SPIMSG_uiLen       = 6;

    API_SpiDeviceTransfer(_G_spiNorObj.Spi_NorDev, &spiMsg, 1);
    printf("manuf_id:0x%02x, device_id:0x%02x\n", data[4], data[5]);

    return  (data[4]);
}
/*********************************************************************************************************
** 函数名称: __k210SpiIoMuxInit
** 功能描述: SpiFlash 引脚映射
** 输  入  : index        SPI Flash 所使用的 spi 通道号
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static void __k210SpiIoMuxInit (UINT8  iIndex)
{

    if (iIndex == 0)
    {
        fpioa_set_function(30, FUNC_SPI0_SS0);
        fpioa_set_function(32, FUNC_SPI0_SCLK);
        fpioa_set_function(34, FUNC_SPI0_D0);
        fpioa_set_function(13, FUNC_SPI0_D1);
        fpioa_set_function(15, FUNC_SPI0_D2);
        fpioa_set_function(17, FUNC_SPI0_D3);
    }
    else
    {
        fpioa_set_function(30, FUNC_SPI1_SS0);
        fpioa_set_function(32, FUNC_SPI1_SCLK);
        fpioa_set_function(34, FUNC_SPI1_D0);
        fpioa_set_function(13, FUNC_SPI1_D1);
        fpioa_set_function(15, FUNC_SPI1_D2);
        fpioa_set_function(17, FUNC_SPI1_D3);
    }
}
/*********************************************************************************************************
** 函数名称: spiFlashDrvInstall
** 功能描述: SPI_NOR 驱动安装
** 输  入  : uiOffsetByts   实际使用时, 偏移(保留)的字节数
** 输  出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT spiFlashDrvInstall (CPCHAR  pcName, UINT  ulOffsetBytes)
{
    struct yaffs_devnor  *nordev;
    struct yaffs_nor     *nor;
    struct yaffs_dev     *dev;
    struct yaffs_param   *param;
    struct yaffs_driver  *drv;

    if (ulOffsetBytes >= _G_spiNorObj.uiTotalSize) {
        NOR_DEBUG("invalid bytes offset.\n");
        return  (PX_ERROR);
    }

    if (!pcName) {
        pcName = "/flash";
    }

    _G_spiNorObj.uiOffsetBytes = ulOffsetBytes;
    _G_spiNorObj.pcName        = pcName;

    nordev = &_G_spiNorObj.devnor;

    nor    = &nordev->nor;
    dev    = &nordev->dev;

    param  = &dev->param;
    drv    = &dev->drv;

    nor->nor_base            = NOR_BASE_ADDR;
    nor->block_size          = NOR_BLOCK_SIZE;
    nor->chunk_size          = 512 + 16;
    nor->bytes_per_chunk     = 512;
    nor->spare_per_chunk     = 16;
    nor->chunk_per_block     = nor->block_size / nor->chunk_size;
    nor->block_per_chip      = NOR_BLOCK_PER_CHIP;

    nor->nor_erase_fn        = __k210SpiNorErase;
    nor->nor_read_fn         = __k210SpiNorRead;
    nor->nor_write_fn        = __k210SpiNorWrite;
    nor->nor_initialise_fn   = __k210SpiNorInitialise;
    nor->nor_deinitialise_fn = __k210SpiNorDeinitialise;

    param->name                  = pcName;
    param->total_bytes_per_chunk = nor->bytes_per_chunk;
    param->chunks_per_block      = nor->chunk_per_block;
    param->n_reserved_blocks     = 4;
    param->start_block           = (ulOffsetBytes / NOR_BLOCK_SIZE) + 1;
    param->end_block             = NOR_BLOCK_PER_CHIP - 1;
    param->use_nand_ecc          = 0;
    param->disable_soft_del      = 1;
    param->n_caches              = 10;

    drv->drv_write_chunk_fn  = ynorWriteChunk;
    drv->drv_read_chunk_fn   = ynorReadChunk;
    drv->drv_erase_fn        = ynorEraseBlock;
    drv->drv_initialise_fn   = ynorInitialise;
    drv->drv_deinitialise_fn = ynorDeinitialise;

    dev->driver_context = &nordev;

    yaffs_add_device(dev);
    yaffs_mount(pcName);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: spiFlashDevCreate
** 功能描述: 向适配器挂载SPI接口设备
** 输　入  : pSpiBusName     SPI适配器名称
**           uiSSPin         GPIO 序号
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  spiFlashDevCreate (PCHAR  pSpiBusName, UINT  uiSSPin)
{
    INT                 iError;
    PSPI_NOR_OBJ        pSpi_Nor = &_G_spiNorObj;

    if (pSpi_Nor->Spi_NorDev == NULL) {
        pSpi_Nor->Spi_NorDev = API_SpiDeviceCreate(SPI_ADAPTER_NAME, SPI_NOR_DEVNAME);
    }

    iError = API_SpiDeviceBusRequest(pSpi_Nor->Spi_NorDev);
    if (iError == PX_ERROR) {
        NOR_DEBUG("Spi Request Bus error\n");
        return  (PX_ERROR);
    }

    __k210SpiIoMuxInit(SPI_CHANNEL_NR);                                 /*      SPI3 Fpioa Init         */

    /*
     * 当块的读写直接调用逻辑库的接口时，必须做如下初始化
     */
    w25qxx_init(SPI_CHANNEL_NR, 0);

    API_SpiDeviceBusRelease(pSpi_Nor->Spi_NorDev);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
