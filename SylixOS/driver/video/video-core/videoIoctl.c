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
** 文   件   名: videoIoctl.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 11 月 28 日
**
** 描        述: VIDEO DEVICE ioctl操作管理层
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include <string.h>
#include "video.h"
#include "driver/common.h"
#include "video_debug.h"

#include "videoIoctl.h"
#include "videoSubDev.h"
#include "videoChannel.h"
#include "videoDevice.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
extern PCHAR  *_G_video_format_desc_table;
extern PCHAR   _G_video_format_type_table;

static INT __deviceDescriptionGet (PLW_VIDEO_IOCTL_OPS  pIoctls,
                                   PLW_FD_ENTRY         pFdEntry,
                                   video_dev_desc      *pVideoDevDesc)
{
    PLW_VIDEO_CARD_SUBDEV   pSubDevice     =  LW_NULL;
    PLW_VIDEO_DEVICE        pVideoDevice   =  (PLW_VIDEO_DEVICE)pFdEntry->FDENTRY_pdevhdrHdr;
    PLW_VIDEO_DEVICE_DRV    pVideoDevDrv   =  pVideoDevice->VIDEO_pVideoDriver;

    if (!pVideoDevice || !pVideoDevDrv) {
        _PrintFormat("[video ioctl error] (%s:%d) %s", __FILE__, __LINE__);
        return  (ERROR_NONE);
    }

    /*
     * TODO: 获得设备描述信息
     */
    strncpy(pVideoDevDesc->driver, pVideoDevDrv->DEVICEDRV_descBuffer, sizeof(pVideoDevDesc->driver));
    strncpy(pVideoDevDesc->card, pVideoDevice->VIDEO_cDeviceName, sizeof(pVideoDevDesc->card));

    if (pVideoDevice->VIDEO_subDevList) {
        pSubDevice  =  container_of(pVideoDevice->VIDEO_subDevList, LW_VIDEO_CARD_SUBDEV, SUBDEV_lineNode);

        switch (pSubDevice->SUBDEV_clientBusType) {

        case SUBDEV_BUS_TYPE_SCCB:
            strncpy(pVideoDevDesc->bus, "subdev_bus_sccb", strlen("subdev_bus_sccb") + 1);
            break;

        case SUBDEV_BUS_TYPE_I2C:
            strncpy(pVideoDevDesc->bus, "subdev_bus_i2c", strlen("subdev_bus_i2c") + 1);
            break;

        case SUBDEV_BUS_TYPE_SPI:
            strncpy(pVideoDevDesc->bus, "subdev_bus_spi", strlen("subdev_bus_spi") + 1);
            break;

        case SUBDEV_BUS_TYPE_USB:
            strncpy(pVideoDevDesc->bus, "subdev_bus_usb", strlen("subdev_bus_usb") + 1);
            break;

        default:
            pVideoDevDesc->bus[0] = '\0';
            break;
        }
    }

    pVideoDevDesc->version  = VIDEO_DRV_VERSION;
    pVideoDevDesc->sources  = 1;                                        /*  一个video device 代表一个源 */
    pVideoDevDesc->channels = pVideoDevice->VIDEO_uiChannels;

    /*
     * TODO: pVideoDevDesc->capabilities 其它信息有驱动实现的 vidioc_g_dev_desc 接口进行补充或覆盖
     */
    pVideoDevDesc->capabilities = 0;
    if (pVideoDevDrv->DEVICEDRV_pvideoIoctls->vidioc_g_dev_desc) {
        pVideoDevDrv->DEVICEDRV_pvideoIoctls->vidioc_g_dev_desc(pVideoDevice, pVideoDevDesc);
    }

    return  (ERROR_NONE);
}

static INT __channelDescriptionGet (PLW_VIDEO_IOCTL_OPS  pIoctls,
                                    PLW_FD_ENTRY         pFdEntry,
                                    video_channel_desc  *pVideoChanDesc)
{
    PLW_VIDEO_DEVICE        pVideoDevice   =  (PLW_VIDEO_DEVICE)pFdEntry->FDENTRY_pdevhdrHdr;
    PLW_VIDEO_DEVICE_DRV    pVideoDevDrv   =  pVideoDevice->VIDEO_pVideoDriver;
    PLW_VIDEO_CHANNEL       pChannel       =  &pVideoDevice->VIDEO_channelArray[pVideoChanDesc->channel];

    if (!pVideoDevice || !pVideoDevDrv) {
        _PrintFormat("[video ioctl error] (%s:%d) %s", __FILE__, __LINE__);
        return  (ERROR_NONE);
    }

    if (pVideoChanDesc->channel < 0 || pVideoChanDesc->channel > VIDEO_MAX_CHANNEL_NUM) {
        _PrintFormat("[video_channel_desc]: error channel number.\r\n");
        return  (ERROR_NONE);
    }

    /*
     * TODO: 获得指定通道的信息
     */
    pVideoChanDesc->queue_max = VIDEO_BUFFER_MAX_PLANE_NR;
    pVideoChanDesc->xsize_max = pChannel->VCHAN_width;
    pVideoChanDesc->ysize_max = pChannel->VCHAN_height;
    pVideoChanDesc->formats   = API_VideoDevFormatsNrGet(pVideoDevice);  /* 多少位被置1就有多少 formats */

    /*
     * TODO: formats, pVideoDevDesc->capabilities 由驱动完善或覆盖
     */
    pVideoChanDesc->capabilities = VIDEO_CHAN_ONESHOT;                  /*  这里 video.h 中的定义不完整 */
    if (pVideoDevDrv->DEVICEDRV_pvideoIoctls->vidioc_g_chan_desc) {
        pVideoDevDrv->DEVICEDRV_pvideoIoctls->vidioc_g_chan_desc(pVideoDevice, pVideoChanDesc);
    }

    return  (ERROR_NONE);
}

static INT __formatDescriptionGet (PLW_VIDEO_IOCTL_OPS  pIoctls,
                                   PLW_FD_ENTRY         pFdEntry,
                                   video_format_desc   *pVideoFormatDesc)
{
    UINT                    uiIndex;
    PLW_VIDEO_DEVICE        pVideoDevice   =  (PLW_VIDEO_DEVICE)pFdEntry->FDENTRY_pdevhdrHdr;
    PLW_VIDEO_DEVICE_DRV    pVideoDevDrv   =  pVideoDevice->VIDEO_pVideoDriver;

    if (!pVideoDevice || !pVideoDevDrv) {
        _PrintFormat("[video ioctl error] (%s:%d) %s", __FILE__, __LINE__);
        return  (ERROR_NONE);
    }

    if (pVideoFormatDesc->channel < 0 || pVideoFormatDesc->channel > VIDEO_MAX_CHANNEL_NUM) {
        _PrintFormat("[video_format_desc]: error channel number.\r\n");
        return  (ERROR_NONE);
    }

    /*
     * TODO: 获得指定通道支持的格式信息
     */
    if (pVideoFormatDesc->index >= LW_FORMATS_DESC_MAX || pVideoFormatDesc->index < 0) {
        _PrintFormat("[video_format_desc]: framework not support this format now.\r\n");
        return  (ERROR_NONE);
    }

//    video_debug("index: %d\r\n", pVideoFormatDesc->index);
//    uiIndex = pVideoDevice->VIDEO_pFormatsIdArray[pVideoFormatDesc->index];
//    video_debug("uiIndex: %d\r\n", uiIndex);
//    strncpy(pVideoFormatDesc->description,                              /* 注意: 当前未支持所有格式查询 */
//            _G_video_format_desc_table[uiIndex],
//            strlen(_G_video_format_desc_table[uiIndex]));
//    pVideoFormatDesc->format = (CHAR)_G_video_format_type_table[uiIndex];

    strncpy(pVideoFormatDesc->description,
            "VIDEO_PIXEL_FORMAT_RGB_565",
            strlen("VIDEO_PIXEL_FORMAT_RGB_565") + 1);
    pVideoFormatDesc->format = VIDEO_PIXEL_FORMAT_RGB_565;

    /*
     * TODO: format, order
     */
    pVideoFormatDesc->order = 0;
    if (pVideoDevDrv->DEVICEDRV_pvideoIoctls->vidioc_g_format_desc) {
        pVideoDevDrv->DEVICEDRV_pvideoIoctls->vidioc_g_format_desc(pVideoDevice, pVideoFormatDesc);
    }

    return  (ERROR_NONE);
}

static INT __channelConfigGet (PLW_VIDEO_IOCTL_OPS  pIoctls,
                               PLW_FD_ENTRY         pFdEntry,
                               video_channel_ctl   *pVideoChanCtl)
{
    PLW_VIDEO_DEVICE        pVideoDevice   =  (PLW_VIDEO_DEVICE)pFdEntry->FDENTRY_pdevhdrHdr;
    PLW_VIDEO_DEVICE_DRV    pVideoDevDrv   =  pVideoDevice->VIDEO_pVideoDriver;
    PLW_VIDEO_CHANNEL       pChannel       =  &pVideoDevice->VIDEO_channelArray[pVideoChanCtl->channel];

    if (!pVideoDevice || !pVideoDevDrv) {
        _PrintFormat("[video ioctl error] (%s:%d) %s", __FILE__, __LINE__);
        return  (ERROR_NONE);
    }

    if (pVideoChanCtl->channel < 0 || pVideoChanCtl->channel > VIDEO_MAX_CHANNEL_NUM) {
        _PrintFormat("[video_channel_ctl]: error channel number.\r\n");
        return  (ERROR_NONE);
    }

    /*
     * TODO: 获取当前通道控制字
     */
    pVideoChanCtl->xsize  = pChannel->VCHAN_width;
    pVideoChanCtl->ysize  = pChannel->VCHAN_height;
    pVideoChanCtl->x_off  = pChannel->VCHAN_x_off;
    pVideoChanCtl->y_off  = pChannel->VCHAN_y_off;
    pVideoChanCtl->x_cut  = pChannel->VCHAN_x_cut;
    pVideoChanCtl->y_cut  = pChannel->VCHAN_y_cut;
    pVideoChanCtl->format = pChannel->VCHAN_format;

    /*
     * TODO: format, order
     */
    if (pVideoDevDrv->DEVICEDRV_pvideoIoctls->vidioc_g_chan_config) {
        pVideoDevDrv->DEVICEDRV_pvideoIoctls->vidioc_g_chan_config(pVideoDevice, pVideoChanCtl);
    }

    return  (ERROR_NONE);
}

static INT __channelConfigSet (PLW_VIDEO_IOCTL_OPS  pIoctls,
                               PLW_FD_ENTRY         pFdEntry,
                               video_channel_ctl   *pVideoChanCtl)
{
    PLW_VIDEO_DEVICE        pVideoDevice   =  (PLW_VIDEO_DEVICE)pFdEntry->FDENTRY_pdevhdrHdr;
    PLW_VIDEO_DEVICE_DRV    pVideoDevDrv   =  pVideoDevice->VIDEO_pVideoDriver;
    PLW_VIDEO_CHANNEL       pChannel       =  &pVideoDevice->VIDEO_channelArray[pVideoChanCtl->channel];

    if (!pVideoDevice || !pVideoDevDrv) {
        _PrintFormat("[video ioctl error] (%s:%d) %s", __FILE__, __LINE__);
        return  (ERROR_NONE);
    }

    if (pVideoChanCtl->channel < 0 || pVideoChanCtl->channel > VIDEO_MAX_CHANNEL_NUM) {
        _PrintFormat("[video_channel_ctl]: error channel number.\r\n");
        return  (ERROR_NONE);
    }

    /*
     * TODO: 设置通道控制字
     */
    pChannel->VCHAN_width  =  pVideoChanCtl->xsize;
    pChannel->VCHAN_height =  pVideoChanCtl->ysize;
    pChannel->VCHAN_x_off  =  pVideoChanCtl->x_off;
    pChannel->VCHAN_y_off  =  pVideoChanCtl->y_off;
    pChannel->VCHAN_x_cut  =  pVideoChanCtl->x_cut;
    pChannel->VCHAN_y_cut  =  pVideoChanCtl->y_cut;
    pChannel->VCHAN_format =  pVideoChanCtl->format;
                                                                        /* 当前仅支持RGB格式大小的获取  */
    pChannel->VCHAN_formatByteSize = API_VideoChanFormatByte(pChannel);

    /*
     * TODO: format, order (注意: 该接口驱动程序必须实现)
     */
    if (pVideoDevDrv->DEVICEDRV_pvideoIoctls->vidioc_s_chan_config) {
        pVideoDevDrv->DEVICEDRV_pvideoIoctls->vidioc_s_chan_config(pVideoDevice, pVideoChanCtl);
    } else {
        printk("__channelConfigSet(): vidioc_s_chan_config cannot be null.\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}

static INT __videoBufInfoGet (PLW_VIDEO_IOCTL_OPS  pIoctls,
                              PLW_FD_ENTRY         pFdEntry,
                              video_buf_cal       *pVideoBufInfo)
{
    PLW_VIDEO_DEVICE        pVideoDevice   =  (PLW_VIDEO_DEVICE)pFdEntry->FDENTRY_pdevhdrHdr;
    PLW_VIDEO_DEVICE_DRV    pVideoDevDrv   =  pVideoDevice->VIDEO_pVideoDriver;
    PLW_VIDEO_CHANNEL       pChannel       =  &pVideoDevice->VIDEO_channelArray[pVideoBufInfo->channel];

    if (!pVideoDevice || !pVideoDevDrv || !pChannel) {
        _PrintFormat("[video ioctl error] (%s:%d) %s", __FILE__, __LINE__);
        return  (ERROR_NONE);
    }

    if (pVideoBufInfo->channel < 0 || pVideoBufInfo->channel > VIDEO_MAX_CHANNEL_NUM) {
        _PrintFormat("[video_buf_cal]: error channel number.\r\n");
        return  (ERROR_NONE);
    }

    /*
     * TODO: 读取内存需求情况
     */
    pVideoBufInfo->align  =  VIDOE_BUFFER_ALING_SIZE;

    /*
     * TODO: 根据 pVideoChannel->VCHAN_format 得到 VIDEO_BYTES_PER_PIXEL
     */
    video_debug("formatByteSize: %d\r\n", pChannel->VCHAN_formatByteSize);
    pVideoBufInfo->size           =  pChannel->VCHAN_videoBuffer.VBUF_uiMemSize;
    pVideoBufInfo->size_per_line  =  pChannel->VCHAN_formatByteSize * pChannel->VCHAN_width;
    pVideoBufInfo->size_per_fq    =  pVideoBufInfo->size_per_line * pChannel->VCHAN_height;

    /*
     * TODO: format, order （注意: 该接口不需要驱动完成，在后面会删除该接口 vidioc_g_vbuf_info）
     */
    if (pVideoDevDrv->DEVICEDRV_pvideoIoctls->vidioc_g_vbuf_info) {
        pVideoDevDrv->DEVICEDRV_pvideoIoctls->vidioc_g_vbuf_info(pVideoDevice, pVideoBufInfo);
    }

    return  (ERROR_NONE);
}

static INT __prepareVideoBuf (PLW_VIDEO_IOCTL_OPS  pIoctls,
                              PLW_FD_ENTRY         pFdEntry,
                              video_buf_ctl       *pVideoBufConfig)
{
    PLW_VIDEO_DEVICE        pVideoDevice   =  (PLW_VIDEO_DEVICE)pFdEntry->FDENTRY_pdevhdrHdr;
    PLW_VIDEO_DEVICE_DRV    pVideoDevDrv   =  pVideoDevice->VIDEO_pVideoDriver;

    if (!pVideoDevice || !pVideoDevDrv) {
        _PrintFormat("[video ioctl error] (%s:%d) %s", __FILE__, __LINE__);
        return  (ERROR_NONE);
    }

    if (pVideoBufConfig->channel < 0 || pVideoBufConfig->channel > VIDEO_MAX_CHANNEL_NUM) {
        _PrintFormat("[video_buf_ctl]: error channel number.\r\n");
        return  (ERROR_NONE);
    }

    /*
     * TODO: format, order （注意: 驱动程序必须实现该接口）
     */
    if (pVideoDevDrv->DEVICEDRV_pvideoIoctls->vidioc_prepare_vbuf) {
        pVideoDevDrv->DEVICEDRV_pvideoIoctls->vidioc_prepare_vbuf(pVideoDevice, pVideoBufConfig);
    } else {
        printk("__prepareVideoBuf(): vidioc_s_chan_config cannot be null.\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}

static INT __queryCaptureStat (PLW_VIDEO_IOCTL_OPS  pIoctls,
                               PLW_FD_ENTRY         pFdEntry,
                               video_cap_stat      *pVideoCapStat)
{
    addr_t                  memAddr;
    PLW_VIDEO_DEVICE        pVideoDevice   =  (PLW_VIDEO_DEVICE)pFdEntry->FDENTRY_pdevhdrHdr;
    PLW_VIDEO_DEVICE_DRV    pVideoDevDrv   =  pVideoDevice->VIDEO_pVideoDriver;
    PLW_VIDEO_CHANNEL       pChannel       =  &pVideoDevice->VIDEO_channelArray[pVideoCapStat->channel];

    if (!pVideoDevice || !pVideoDevDrv) {
        _PrintFormat("[video ioctl error] (%s:%d) %s", __FILE__, __LINE__);
        return  (ERROR_NONE);
    }

    if (pVideoCapStat->channel < 0 || pVideoCapStat->channel > VIDEO_MAX_CHANNEL_NUM) {
        _PrintFormat("[video_cap_stat]: error channel number.\r\n");
        return  (ERROR_NONE);
    }

    /*
     * TODO: 查询当前捕获信息
     */
    pVideoCapStat->on         = pChannel->VCHAN_flags & VCHAN_STREAM_ON;
    pVideoCapStat->qindex_cur = pChannel->VCHAN_videoBuffer.VBUF_uiCapPlaneId;
    API_VideoBufImageFrameGet(&pChannel->VCHAN_videoBuffer, &memAddr, &pVideoCapStat->qindex_vaild);

    /*
     * TODO: （注意: 该接口不需要驱动完成，在后面会删除该接口 vidioc_querycap_stat）
     */
    if (pVideoDevDrv->DEVICEDRV_pvideoIoctls->vidioc_querycap_stat) {
        pVideoDevDrv->DEVICEDRV_pvideoIoctls->vidioc_querycap_stat(pVideoDevice, pVideoCapStat);
    }

    return  (ERROR_NONE);
}

static INT __captureCtlWordGet (PLW_VIDEO_IOCTL_OPS  pIoctls,
                                PLW_FD_ENTRY         pFdEntry,
                                video_cap_ctl      *pVideoCaptureCtl)
{
    PLW_VIDEO_DEVICE        pVideoDevice   =  (PLW_VIDEO_DEVICE)pFdEntry->FDENTRY_pdevhdrHdr;
    PLW_VIDEO_DEVICE_DRV    pVideoDevDrv   =  pVideoDevice->VIDEO_pVideoDriver;
    PLW_VIDEO_CHANNEL       pChannel       =  &pVideoDevice->VIDEO_channelArray[pVideoCaptureCtl->channel];

    if (!pVideoDevice || !pVideoDevDrv) {
        _PrintFormat("[video ioctl error] (%s:%d) %s", __FILE__, __LINE__);
        return  (ERROR_NONE);
    }

    if (pVideoCaptureCtl->channel < 0 || pVideoCaptureCtl->channel > VIDEO_MAX_CHANNEL_NUM) {
        _PrintFormat("[video_cap_stat]: error channel number.\r\n");
        return  (ERROR_NONE);
    }

    /*
     * TODO: 获得当前视频捕获状态
     */
    pVideoCaptureCtl->on     =  pChannel->VCHAN_flags & VCHAN_STREAM_ON;
    pVideoCaptureCtl->flags  =  pChannel->VCHAN_flags;

    /*
     * TODO: （注意: 该接口不需要驱动完成，在后面会删除该接口 vidioc_g_cap_ctl）
     */
    if (pVideoDevDrv->DEVICEDRV_pvideoIoctls->vidioc_g_cap_ctl) {
        pVideoDevDrv->DEVICEDRV_pvideoIoctls->vidioc_g_cap_ctl(pVideoDevice, pVideoCaptureCtl);
    }

    return  (ERROR_NONE);
}

static INT __captureCtlWordSet (PLW_VIDEO_IOCTL_OPS  pIoctls,
                                PLW_FD_ENTRY         pFdEntry,
                                video_cap_ctl       *pVideoCaptureCtl)
{
    PLW_VIDEO_DEVICE        pVideoDevice   =  (PLW_VIDEO_DEVICE)pFdEntry->FDENTRY_pdevhdrHdr;
    PLW_VIDEO_DEVICE_DRV    pVideoDevDrv   =  pVideoDevice->VIDEO_pVideoDriver;
    PLW_VIDEO_CHANNEL       pChannel       =  &pVideoDevice->VIDEO_channelArray[pVideoCaptureCtl->channel];

    if (!pVideoDevice || !pVideoDevDrv) {
        _PrintFormat("[video ioctl error] (%s:%d) %s", __FILE__, __LINE__);
        return  (ERROR_NONE);
    }

    if (pVideoCaptureCtl->channel < 0 || pVideoCaptureCtl->channel > VIDEO_MAX_CHANNEL_NUM) {
        _PrintFormat("[video_cap_stat]: error channel number.\r\n");
        return  (ERROR_NONE);
    }

    /*
     * TODO: 设置视频捕获状态 1:ON 0:OFF
     */
    pChannel->VCHAN_flags  = pVideoCaptureCtl->flags;

    /*
     * TODO: （注意: 该部分接口可能不需要，有待完善）
     */
    if (pVideoDevDrv->DEVICEDRV_pvideoIoctls->vidioc_s_cap_ctl) {
        pVideoDevDrv->DEVICEDRV_pvideoIoctls->vidioc_s_cap_ctl(pVideoDevice, pVideoCaptureCtl);
    }

    return  (ERROR_NONE);
}

static INT __imageEffectGet (PLW_VIDEO_IOCTL_OPS    pIoctls,
                             PLW_FD_ENTRY           pFdEntry,
                             video_effect_info     *pVideoImageEffect)
{
    /*
     * TODO: 通过 pIoctls 和 subdev.ops (互补) 获得的特效信息 （注意: 有待完善）
     */

    return  (ERROR_NONE);
}

static INT __imageEffectSet (PLW_VIDEO_IOCTL_OPS    pIoctls,
                             PLW_FD_ENTRY           pFdEntry,
                             video_effect_ctl      *pVideoEffectConfig)
{
    /*
     * TODO: 通过 pIoctls 和 subdev.ops (互补) 设置的特效信息 （注意: 有待完善）
     */

    return  (ERROR_NONE);
}

/* ==================================================================================================== */

INT  API_VideoIoctlManager (PLW_FD_ENTRY      pFdEntry,
                            INT               iCmd,
                            LONG              lArg)
{
    struct stat            *pStat;
    PLW_SEL_WAKEUPNODE      pselnode;
    ULONG                   count        =  0;
    INT                     iRet         =  ERROR_NONE;
    PLW_VIDEO_DEVICE        pVideoDevice =  (PLW_VIDEO_DEVICE)pFdEntry->FDENTRY_pdevhdrHdr;
    PLW_VIDEO_IOCTL_OPS     pIoctls      =  pVideoDevice->VIDEO_pVideoDriver->DEVICEDRV_pvideoIoctls;

    if (pIoctls == NULL) {
        printk("API_VideoIoctlManager(): invalid pIoctls.\n");
        return  (PX_ERROR);
    }

    switch (iCmd) {                                                     /*  进入时,已被pVideoDevice加锁 */

    case  FIOFSTATGET:
        pStat = (struct stat *)lArg;
        if (pStat) {
            pStat->st_dev     = (dev_t)pVideoDevice;
            pStat->st_ino     = (ino_t)0;
            pStat->st_mode    = (S_IRUSR | S_IRGRP | S_IROTH);
            pStat->st_nlink   = 1;
            pStat->st_uid     = 0;
            pStat->st_gid     = 0;
            pStat->st_rdev    = 0;
            pStat->st_size    = 0;
            pStat->st_blksize = 0;
            pStat->st_blocks  = 0;
            pStat->st_atime   = pVideoDevice->VIDEO_time;
            pStat->st_mtime   = pVideoDevice->VIDEO_time;
            pStat->st_ctime   = pVideoDevice->VIDEO_time;
            return  (ERROR_NONE);
        }
        return  (PX_ERROR);

    case  FIOSETFL:
        if ((int)lArg & O_NONBLOCK) {
            pFdEntry->FDENTRY_iFlag |= O_NONBLOCK;
        } else {
            pFdEntry->FDENTRY_iFlag &= ~O_NONBLOCK;
        }
        return  (ERROR_NONE);

    case FIOSELECT:                                                     /*      select 支持             */
        pselnode = (PLW_SEL_WAKEUPNODE)lArg;
        SEL_WAKE_NODE_ADD(&pVideoDevice->VIDEO_waitList, pselnode);
        switch (pselnode->SELWUN_seltypType) {

        case SELREAD:
            if (API_MsgQueueStatus(pVideoDevice->VIDEO_hlMsgQueue, NULL, &count, NULL, NULL, NULL)) {
                SEL_WAKE_UP(pselnode);
            } else if (count) {
                SEL_WAKE_UP(pselnode);
            }
            break;

        case SELWRITE:
        case SELEXCEPT:
            break;
        }
        return  (ERROR_NONE);

    case FIOUNSELECT:
        SEL_WAKE_NODE_DELETE(&pVideoDevice->VIDEO_waitList, (PLW_SEL_WAKEUPNODE)lArg);
        return  (ERROR_NONE);

                                                                        /*      调用框架的处理函数      */
    case VIDIOC_DEVDESC:
        iRet  =  __deviceDescriptionGet(pIoctls, pFdEntry, (video_dev_desc *)lArg);
        break;

    case VIDIOC_CHANDESC:
        iRet  =  __channelDescriptionGet(pIoctls, pFdEntry, (video_channel_desc *)lArg);
        break;

    case VIDIOC_FORMATDESC:
        iRet  =  __formatDescriptionGet(pIoctls, pFdEntry, (video_format_desc *)lArg);
        break;

    case VIDIOC_GCHANCTL:
        iRet  =  __channelConfigGet(pIoctls, pFdEntry, (video_channel_ctl *)lArg);
        break;

    case VIDIOC_SCHANCTL:
        iRet  =  __channelConfigSet(pIoctls, pFdEntry, (video_channel_ctl *)lArg);
        break;

    case VIDIOC_MAPCAL:
        iRet  =  __videoBufInfoGet(pIoctls, pFdEntry, (video_buf_cal *)lArg);
        break;

    case VIDIOC_MAPPREPAIR:
        iRet  =  __prepareVideoBuf(pIoctls, pFdEntry, (video_buf_ctl *)lArg);
        break;

    case VIDIOC_CAPSTAT:
        iRet  =  __queryCaptureStat(pIoctls, pFdEntry, (video_cap_stat *)lArg);
        break;

    case VIDIOC_GCAPCTL:
        iRet  =  __captureCtlWordGet(pIoctls, pFdEntry, (video_cap_ctl *)lArg);
        break;

    case VIDIOC_SCAPCTL:
        iRet  =  __captureCtlWordSet(pIoctls, pFdEntry, (video_cap_ctl *)lArg);
        break;

    case VIDIOC_GEFFCTL:
        iRet  =  __imageEffectGet(pIoctls, pFdEntry, (video_effect_info *)lArg);
        break;

    case VIDIOC_SEFFCTL:
        iRet  =  __imageEffectSet(pIoctls, pFdEntry, (video_effect_ctl *)lArg);
        break;

    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }

    return  (iRet);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
