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
** 文   件   名: videoSubDev.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 11 月 28 日
**
** 描        述: 视频卡子设备的抽象，如: ISP(Image Sensor Processor)
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include <string.h>
#include "video.h"
#include "video_debug.h"
#include "videoSubDev.h"
#include "videoDevice.h"
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define container_of(ptr, type, member) ({ \
        const typeof( ((type *)0)->member ) *__mptr = (ptr); \
        (type *)( (char *)__mptr - offsetof(type,member) );})
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
INT API_VideoCardSubDevInit (PLW_VIDEO_CARD_SUBDEV     pSubDevice,
                             PCHAR                     pBindCardName,
                             LW_CARD_CLIENT_BUS_TYPE   busType,
                             PCHAR                     pBusAdapterName,
                             INT32                     iClientAddr)
{
    INT     iRet = ERROR_NONE;

    if (!pSubDevice || !pBindCardName || !pBusAdapterName || (busType > SUBDEV_BUS_TYPE_MAX)) {
        printk("API_VideoCardSubDevInit(): invalid parameter.\n");
    }

    strncpy(pSubDevice->SUBDEV_bindCardName, pBindCardName, strlen(pBindCardName));

    pSubDevice->SUBDEV_clientBusType = busType;
    pSubDevice->SUBDEV_iClientAddr   = iClientAddr;
    strncpy(pSubDevice->SUBDEV_clientBusName, pBusAdapterName, strlen(pBusAdapterName));

    LW_SPIN_INIT(&pSubDevice->SUBDEV_lock);

    pSubDevice->SUBDEV_pCardDevice   = NULL;
    pSubDevice->SUBDEV_pBusClient    = NULL;
    pSubDevice->SUBDEV_pCommonOps    = NULL;
    pSubDevice->SUBDEV_pInternalOps  = NULL;
    pSubDevice->SUBDEV_pSubDevPri    = NULL;

    pSubDevice->SUBDEV_lineNode.LINE_plistNext  = LW_NULL;
    pSubDevice->SUBDEV_lineNode.LINE_plistPrev  = LW_NULL;

    /*
     * get the client device from specify transfer bus.
     */
    switch (busType) {

    case SUBDEV_BUS_TYPE_SCCB:
        //pSubDevice->SUBDEV_pBusClient = API_I2cDeviceCreate(....);
        printk("API_VideoCardSubDevInit(): get sccb device.\n");
        break;

    case SUBDEV_BUS_TYPE_I2C:
        //pSubDevice->SUBDEV_pBusClient = API_I2cDeviceCreate(pBusAdapterName, "video_subdev", iClientAddr, 0);
        printk("API_VideoCardSubDevInit(): get i2c device.\n");
        break;

    case SUBDEV_BUS_TYPE_SPI:
        //pSubDevice->SUBDEV_pBusClient = API_SpiDeviceCreate(....);
        printk("API_VideoCardSubDevInit(): get spi device.\n");
        break;

    case SUBDEV_BUS_TYPE_USB:
        //pSubDevice->SUBDEV_pBusClient = API_UsbDeviceCreate(....);
        printk("API_VideoCardSubDevInit(): get usb device.\n");
        break;

    default:
        printk("API_VideoCardSubDevInit(): unrecognized busType.\n");
        iRet = PX_ERROR;
        break;
    }

    if (!pSubDevice->SUBDEV_pBusClient) {
        printk("API_VideoCardSubDevInit(): create subdev client device failed.\n");
        iRet = PX_ERROR;
        return  (iRet);
    }

    return  (iRet);
}

INT   API_VideoCardSubDevOpsSet (PLW_VIDEO_CARD_SUBDEV     pSubDevice,
                                 PLW_SUBDEV_COMMON_OPS     pCommonOps,
                                 PLW_SUBDEV_INTERNAL_OPS   pInternalOps)
{
    if (!pSubDevice || !pCommonOps || !pInternalOps) {
        printk("API_VideoCardSubDevOpsSet(): invalid parameter.\n");
        return  (PX_ERROR);
    }

    API_SpinLock(&pSubDevice->SUBDEV_lock);
    pSubDevice->SUBDEV_pInternalOps = pInternalOps;
    pSubDevice->SUBDEV_pCommonOps   = pCommonOps;
    API_SpinUnlock(&pSubDevice->SUBDEV_lock);

    return  (ERROR_NONE);
}

VOID  API_VideoSubDevPriDataGet (PLW_VIDEO_CARD_SUBDEV pSubDevice, PVOID  *pSubDevPriData)
{
    if (!pSubDevice) {
        printk("API_VideoSubDevPriDataGet(): invalid pSubDevice.\n");
    }

    *pSubDevPriData = pSubDevice->SUBDEV_pSubDevPri;
}

VOID  API_VideoSubDevPriDataSet (PLW_VIDEO_CARD_SUBDEV pSubDevice, PVOID  pSubDevPriData)
{
    if (!pSubDevice || !pSubDevPriData) {
        printk("API_VideoSubDevPriDataSet(): invalid parameter.\n");
    }

    pSubDevice->SUBDEV_pSubDevPri = pSubDevPriData;
}

VOID API_VideoSubDevEntryGet (PLW_LIST_LINE  pSubDevNode, PLW_VIDEO_CARD_SUBDEV  *pSubDev)
{
    PLW_VIDEO_CARD_SUBDEV  pSubDevice;

    if (!pSubDevNode) {
        printk("API_VideoSubDevEntryGet(): invalid pSubDevNode.\n");
    }

    pSubDevice = container_of(pSubDevNode, LW_VIDEO_CARD_SUBDEV, SUBDEV_lineNode);

    *pSubDev = pSubDevice;
}

INT API_VideoSubDevEventAdd (PLW_VIDEO_DEVICE  pVideoDevice, PLW_VIDEO_EVENT  pVideoEvent)
{
    if (!pVideoDevice || !pVideoEvent) {
        printk("API_VideoCardEventAdd(): invalid parameter.\n");
        return  (PX_ERROR);
    }

    API_SpinLock(&pVideoDevice->VIDEO_slEventLock);
    _List_Line_Add_Ahead(&pVideoEvent->lineNode, &pVideoDevice->VIDEO_pEventListHdr);
    pVideoDevice->VIDEO_uiEventNr++;
    API_SpinUnlock(&pVideoDevice->VIDEO_slEventLock);

    return  (ERROR_NONE);
}

INT   API_VideoSubDevNotifyEvent (PLW_VIDEO_CARD_SUBDEV pSubDevice, PLW_VIDEO_EVENT pVideoEvent)
{
    if (!pSubDevice || !pVideoEvent) {
        printk("API_VideoSubDevNotifyEvent(): invalid parameter.\n");
        return  (PX_ERROR);
    }

    if (pSubDevice->SUBDEV_pCardDevice && pSubDevice->SUBDEV_pCardDevice->VIDEO_pfuncNotify) {
        pSubDevice->SUBDEV_pCardDevice->VIDEO_pfuncNotify(pSubDevice, pVideoEvent);
    } else {
        printk("API_VideoSubDevNotifyEvent(): invalid VIDEO_pfuncNotify.\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
