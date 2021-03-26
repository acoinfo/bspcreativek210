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
** 文   件   名: videoDevice.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 11 月 28 日
**
** 描        述: 视频设备抽象及操作接口定义
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include <string.h>
#include "video.h"
#include "driver/common.h"
#include "video_debug.h"

#include "videoDevice.h"
#include "videoSubDev.h"
#include "videoChannel.h"
#include "videoFileHandle.h"
#include "videoIoctl.h"
#include "videoBufCore.h"
#include "videoQueueLib.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/

PCHAR  _G_video_format_desc_table[3] = {
    "VIDEO_CAP_FORMAT_RGB_888" ,
    "VIDEO_CAP_FORMAT_RGB_565" ,
    "VIDEO_CAP_FORMAT_YUV_422_SP"
};
CHAR  _G_video_format_type_table[3] = {
    VIDEO_PIXEL_FORMAT_RGB_888,
    VIDEO_PIXEL_FORMAT_RGB_565,
    VIDEO_PIXEL_FORMAT_YCbCr_422_SP
};

PCHAR  _G_video_effect_desc_table[3] = {
    "VIDEO_CAP_EFFECT_BRIGHTNESS",
    "VIDEO_CAP_EFFECT_CONTRAST",
    "VIDEO_CAP_EFFECT_RED_BALANCE"
};
CHAR  _G_video_effect_type_table[3] = {
    VIDEO_EFFECT_BRIGHTNESS,
    VIDEO_EFFECT_CONTRAST,
    VIDEO_EFFECT_RED_BALANCE
};

INT  API_VideoDeviceDrvInstall (PLW_VIDEO_DEVICE  pVideoDevice, PLW_VIDEO_DEVICE_DRV  pVideoDeviceDrv)
{
    struct file_operations  redirectFileOper;

    if (!pVideoDevice || !pVideoDeviceDrv) {
        printk("API_VideoDeviceDrvInstall(): invalid parameter.\n");
        return  (PX_ERROR);
    }

    if (pVideoDeviceDrv->DEVICEDRV_iVideoDrvNum > 0) {
        printk("API_VideoDeviceDrvInstall(): error this drv had been installed.\n");
        return  (ERROR_NONE);
    }

    if (!pVideoDeviceDrv->DEVICEDRV_pvideoFileOps || !pVideoDeviceDrv->DEVICEDRV_pvideoIoctls) {
        printk("API_VideoDeviceDrvInstall(): video driver not been init property.\n");
        return  (PX_ERROR);
    }

    pVideoDevice->VIDEO_pVideoDriver = pVideoDeviceDrv;

    /*
     * 注册字符设备驱动程序
     */
    lib_memset(&redirectFileOper, 0, sizeof(struct file_operations));

    redirectFileOper.owner     = THIS_MODULE;
    redirectFileOper.fo_create = API_VideoFileOpen;
    redirectFileOper.fo_open   = API_VideoFileOpen;
    redirectFileOper.fo_close  = API_VideoFileClose;
    redirectFileOper.fo_lstat  = API_VideoFileLstat;
    redirectFileOper.fo_ioctl  = API_VideoIoctlManager;
    //redirectFileOper.fo_read   = API_VideoFileRead;                   /*       Not Support Now!       */
    //redirectFileOper.fo_select = __v4l2Select;                        /*       Not Support Now!       */
#if LW_CFG_VMM_EN > 0
    redirectFileOper.fo_mmap   = __v4l2Mmap;                            /*       Not Support Now!       */
    redirectFileOper.fo_unmap  = __v4l2Unmap;                           /*       Not Support Now!       */
#endif

    pVideoDeviceDrv->DEVICEDRV_iVideoDrvNum = iosDrvInstallEx2(&redirectFileOper, LW_DRV_TYPE_NEW_1);

    DRIVER_LICENSE(pVideoDeviceDrv->DEVICEDRV_iVideoDrvNum,     "Dual BSD/GPL->Ver 1.0");
    DRIVER_AUTHOR(pVideoDeviceDrv->DEVICEDRV_iVideoDrvNum,      "Yu.KangZhi");
    DRIVER_DESCRIPTION(pVideoDeviceDrv->DEVICEDRV_iVideoDrvNum, "k210 video core driver.");

    return  ((pVideoDeviceDrv->DEVICEDRV_iVideoDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}

INT  API_VideoDeviceCreate (PLW_VIDEO_DEVICE  pVideoDevice, CPCHAR  pcDeviceNameName)
{
    PLW_VIDEO_DEVICE_DRV    pVideoDeviceDrv;
    UINT                    uiStrLen;

    if (!pVideoDevice || !pcDeviceNameName) {
        printk("API_VideoDeviceCreate(): invalid parameters.\n");
        return  (PX_ERROR);
    }

    if (pVideoDevice->VIDEO_pVideoDriver == LW_NULL) {
        printk("API_VideoDeviceCreate(): pVideoDevice is invalid without pVideoDriver.\n");
        return  (PX_ERROR);
    }

    uiStrLen = strlen(pcDeviceNameName);
    if (uiStrLen > sizeof(pVideoDevice->VIDEO_cDeviceName)) {
        printk("API_VideoDeviceCreate(): pcDeviceNameName is too long.\n");
        return  (PX_ERROR);
    }
    strncpy(pVideoDevice->VIDEO_cDeviceName, pcDeviceNameName, uiStrLen);

    pVideoDeviceDrv = pVideoDevice->VIDEO_pVideoDriver;

    /*
     * 注册字符设备
     */
    pVideoDevice->VIDEO_hlMsgQueue = API_MsgQueueCreate("xkbd_q", (ULONG)VIDEO_MAX_INPUT_VQUEUE,
                                              sizeof(struct video_frame_msg),
                                              LW_OPTION_OBJECT_GLOBAL, NULL);
    if (pVideoDevice->VIDEO_hlMsgQueue == LW_OBJECT_HANDLE_INVALID) {
        printk("API_VideoDeviceCreate(): create hlMsgQueue failed.\n");
        return  (PX_ERROR);
    }

    SEL_WAKE_UP_LIST_INIT(&pVideoDevice->VIDEO_waitList);

    pVideoDevice->VIDEO_time = time(LW_NULL);

    if (API_IosDevAddEx(&pVideoDevice->VIDEO_devHdr,
                        pVideoDevice->VIDEO_cDeviceName,
                        pVideoDeviceDrv->DEVICEDRV_iVideoDrvNum,
                        DT_CHR) != ERROR_NONE) {
        printk(KERN_ERR "API_VideoDeviceCreate(): can not add device : %s.\n", strerror(errno));
        return  (PX_ERROR);
    }

    pVideoDevice->VIDEO_flags |= VIDEO_DEV_REGISTED;                  /*    设置设备已注册标志位        */

    return  (ERROR_NONE);
}

INT API_VideoDeviceInit (PLW_VIDEO_DEVICE  pVideoDevice, UINT  uiChannels)
{
    PLW_VIDEO_CHANNEL  pVideoChanArray;

    if (!pVideoDevice || (uiChannels > VIDEO_CARD_MAX_CHANNEL_NUM)) {
        printk("API_VideoCardDeviceInit(): invalid parameter.\n");
        return  (PX_ERROR);
    }

    pVideoDevice->VIDEO_pVideoDriver = LW_NULL;

    pVideoDevice->VIDEO_pVideoDevPri = LW_NULL;
    pVideoDevice->VIDEO_pfuncNotify  = LW_NULL;

    pVideoDevice->VIDEO_uiEventNr     = 0;
    pVideoDevice->VIDEO_pEventListHdr = LW_NULL;

    pVideoDevice->VIDEO_uiChannels   = uiChannels;
    pVideoDevice->VIDEO_channelArray = (PLW_VIDEO_CHANNEL)__SHEAP_ALLOC((sizeof(LW_VIDEO_CHANNEL) * uiChannels));
    if (!pVideoDevice->VIDEO_channelArray) {
        printk("API_VideoCardDeviceInit(): __SHEAP_ALLOC %d ChannelArray failed.\n", uiChannels);
        return  (PX_ERROR);
    }
    lib_memset(pVideoDevice->VIDEO_channelArray, 0, (sizeof(LW_VIDEO_CHANNEL) * uiChannels));

    pVideoChanArray = pVideoDevice->VIDEO_channelArray;
    for (INT index = 0; index < uiChannels; ++index) {
        API_VideoChannelInit(&pVideoChanArray[index], pVideoDevice, index);
    }

    if (API_SpinInit(&pVideoDevice->VIDEO_lock) != ERROR_NONE) {
        printk("API_VideoCardDeviceInit(): failed to create lock!\n");
        return  (PX_ERROR);
    }

    if (API_SpinInit(&pVideoDevice->VIDEO_slSubDevLock) != ERROR_NONE) {
        printk("API_VideoCardDeviceInit(): failed to create SubDevLock!\n");
        return  (PX_ERROR);
    }

    if (API_SpinInit(&pVideoDevice->VIDEO_slEventLock) != ERROR_NONE) {
        printk("API_VideoCardDeviceInit(): failed to create EventLock!\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}

INT  API_VideoDevImageAttrSet (PLW_VIDEO_DEVICE  pVideoDevice, UINT  uiMaxWidth,  UINT  uiMaxHeight)
{
    if (!pVideoDevice) {
        printk("API_VideoDevImageAttrSet(): invalid pVideoDevice.\n");
        return  (PX_ERROR);
    }

    API_SpinLock(&pVideoDevice->VIDEO_lock);
    pVideoDevice->VIDEO_uiMaxImageHeight = uiMaxWidth;
    pVideoDevice->VIDEO_uiMaxImageWidth  = uiMaxHeight;
    API_SpinUnlock(&pVideoDevice->VIDEO_lock);

    return  (ERROR_NONE);
}

INT  API_VideoDevCapAttrSet (PLW_VIDEO_DEVICE  pVideoDevice, UINT32  uiFormatCap, UINT32  uiEffectCap)
{
    if (!pVideoDevice) {
        printk("API_VideoCardEventAdd(): invalid pVideoDevice.\n");
        return  (PX_ERROR);
    }

    API_SpinLock(&pVideoDevice->VIDEO_lock);
    pVideoDevice->VIDEO_formatCapability = uiFormatCap;
    pVideoDevice->VIDEO_effectCapability = uiEffectCap;
    API_SpinUnlock(&pVideoDevice->VIDEO_lock);

    return  (ERROR_NONE);
}

INT API_VideoDeviceNotifierInstall (PLW_VIDEO_DEVICE  pVideoDevice, PLW_VIDEO_NOTIFIER  pfuncNotifier)
{
    if (!pVideoDevice || !pfuncNotifier) {
        printk("API_VideoCardNotifierInstall(): invalid parameter.\n");
        return  (PX_ERROR);
    }

    API_SpinLock(&pVideoDevice->VIDEO_lock);
    pVideoDevice->VIDEO_pfuncNotify = pfuncNotifier;
    API_SpinUnlock(&pVideoDevice->VIDEO_lock);

    return  (ERROR_NONE);
}

INT API_VideoDeviceSubDevInstall (PLW_VIDEO_DEVICE  pVideoDevice, PLW_VIDEO_CARD_SUBDEV  pSubDevice)
{
    INT     iRet = ERROR_NONE;

    if (!pVideoDevice || !pSubDevice || pSubDevice->SUBDEV_pCardDevice) {
        printk("API_VideoCardSubdevInstall(): valid input.\n");
        return  (PX_ERROR);
    }

    pSubDevice->SUBDEV_pCardDevice = pVideoDevice;

    if (pSubDevice->SUBDEV_pInternalOps && pSubDevice->SUBDEV_pInternalOps->bind) {
        iRet = pSubDevice->SUBDEV_pInternalOps->bind(pSubDevice);

        if (iRet != ERROR_NONE) {
            pSubDevice->SUBDEV_pCardDevice = NULL;
            printk("API_VideoCardSubDevInstall(): pSubDevice registered failed.\n");
            return  (PX_ERROR);
        }
    }

    API_SpinLock(&pVideoDevice->VIDEO_lock);
    _List_Line_Add_Ahead(&pSubDevice->SUBDEV_lineNode, &pVideoDevice->VIDEO_subDevList);
    API_SpinUnlock(&pVideoDevice->VIDEO_lock);

    return  (ERROR_NONE);
}

INT API_VideoDeviceSubDevUninstall (PLW_VIDEO_CARD_SUBDEV  pSubDevice)
{
    PLW_VIDEO_DEVICE   pVideoDevice;

    if (pSubDevice == NULL || pSubDevice->SUBDEV_pCardDevice == NULL) {
        printk("API_VideoDeviceSubDevUninstall(): valid input.\n");
        return  (PX_ERROR);
    }

    pVideoDevice = pSubDevice->SUBDEV_pCardDevice;

    API_SpinLock(&pVideoDevice->VIDEO_lock);
    _List_Line_Del(&pSubDevice->SUBDEV_lineNode, &pVideoDevice->VIDEO_subDevList);
    API_SpinUnlock(&pVideoDevice->VIDEO_lock);

    if (pSubDevice->SUBDEV_pInternalOps && pSubDevice->SUBDEV_pInternalOps->unbind) {
        pSubDevice->SUBDEV_pInternalOps->unbind(pSubDevice);
    }
    pSubDevice->SUBDEV_pCardDevice = NULL;

    /*  如果 subdev 实际上是一个 i2c_client, 那么注销 i2c_client */
#if 0 /* IS_ENABLED(CONFIG_I2C) */
    if (pSubDevice->SUBDEV_clientBusType & SUBDEV_BUS_TYPE_I2C) {
        PLW_I2C_DEVICE pI2cDevice = pSubDevice->SUBDEV_pBusClient;
        if (pI2cDevice) {
            API_I2cDeviceDelete(pI2cDevice);
        }
    }
#endif

    return  (ERROR_NONE);
}

INT  API_VideoDevFormatsNrGet (PLW_VIDEO_DEVICE pVideoDevice)
{
    PCHAR   pDescID;
    UINT    uiCount = 0;
    UINT32  uiVlue  = pVideoDevice->VIDEO_formatCapability;

    pDescID = (PCHAR)__SHEAP_ALLOC(32);
    pVideoDevice->VIDEO_pFormatsIdArray = pDescID;

    for (INT index = 0; index < 32; ++index) {
        if (uiVlue & (0x1 << index) ) {
            pDescID[uiCount] = (CHAR)index;
            uiCount++;
        }
    }

    return  (uiCount);
}

INT  API_VideoDevEffectsNrGet (PLW_VIDEO_DEVICE pVideoDevice)
{
    PCHAR   pDescID;
    UINT    uiCount = 0;
    UINT32  uiVlue  = pVideoDevice->VIDEO_effectCapability;

    pDescID = (PCHAR)__SHEAP_ALLOC(32);
    pVideoDevice->VIDEO_pEffectsIdArray = pDescID;

    for (INT index = 0; index < 32; ++index) {
        if (uiVlue & (0x1 << index) ) {
            pDescID[uiCount] = (CHAR)index;
            uiCount++;
        }
    }

    return  (uiCount);
}

INT  API_VideoDeviceClear (PLW_VIDEO_DEVICE  pVideoDevice)
{
    printk("API_VideoDeviceClear(): NOT COMPLETE NOW!\r\n");
    return  (ERROR_NONE);
}

/*********************************************************************************************************
  END
*********************************************************************************************************/
