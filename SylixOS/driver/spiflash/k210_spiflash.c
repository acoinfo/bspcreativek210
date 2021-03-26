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
** ��   ��   ��: k210_spiflash.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 11 �� 8 ��
**
** ��        ��: SPI Flash (w25qxx) ����
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
  norflash ��ز�����
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
 SPI_NOR �ڲ��������ݽṹ
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
  ȫ�ֱ���
*********************************************************************************************************/
static  SPI_NOR_OBJ  _G_spiNorObj  = {
        .uiEraseSize = NOR_ERASE_SIZE,
        .uiTotalSize = NOR_TOTAL_SIZE,
};

LW_SPINLOCK_DEFINE(_G_k210SpiFlashSpinLock);

/*********************************************************************************************************
** ��������: __k210SpiNorErase
** ��������: ����һ����
** ��  ��  : nordev    nor�豸
**           block     ���ַ
** ��  ��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210SpiNorRead
** ��������: ���ݶ�ȡ
** ��  ��  : nordev    nor�豸
**           addr      �ֽڵ�ַ
**           data      �����ȡ������
**           len       ��Ҫ��ȡ���ݵĳ���
** ��  ��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210SpiNorWrite
** ��������: ����д��
** ��  ��  : nordev    nor�豸
**           addr      �ֽڵ�ַ
**           data      д�����ݻ����ַ
**           len       ��Ҫ��ȡ���ݵĳ���
** ��  ��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210SpiNorInitialise
** ��������: оƬ��ʼ��
** ��  ��  : nordev    nor�豸
** ��  ��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __k210SpiNorInitialise (struct yaffs_devnor  *nordev)
{
    return  (YAFFS_OK);
}
/*********************************************************************************************************
** ��������: __k210SpiNorDeinitialise
** ��������: оƬ����ʼ��
** ��  ��  : nordev    nor�豸
** ��  ��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __k210SpiNorDeinitialise (struct yaffs_devnor  *nordev)
{
    return  (YAFFS_OK);
}
/*********************************************************************************************************
** ��������: k210SpiNorFlashIdRead
** ��������: ��flash ID : flashRead_jedec_id
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __k210SpiIoMuxInit
** ��������: SpiFlash ����ӳ��
** ��  ��  : index        SPI Flash ��ʹ�õ� spi ͨ����
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: spiFlashDrvInstall
** ��������: SPI_NOR ������װ
** ��  ��  : uiOffsetByts   ʵ��ʹ��ʱ, ƫ��(����)���ֽ���
** ��  ��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: spiFlashDevCreate
** ��������: ������������SPI�ӿ��豸
** �䡡��  : pSpiBusName     SPI����������
**           uiSSPin         GPIO ���
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
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
     * ����Ķ�дֱ�ӵ����߼���Ľӿ�ʱ�����������³�ʼ��
     */
    w25qxx_init(SPI_CHANNEL_NR, 0);

    API_SpiDeviceBusRelease(pSpi_Nor->Spi_NorDev);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
