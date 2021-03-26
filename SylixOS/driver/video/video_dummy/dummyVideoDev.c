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
** 文   件   名: dummyVideoDev.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 12 月 7 日
**
** 描        述: 数字摄像头接口(DVP)驱动
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "SylixOS.h"
#include "config.h"
#include <string.h>
#include "driver/common.h"

#include "dummySubDev.h"
#include "driver/video/video-core/video_fw.h"
/*********************************************************************************************************
  定义
*********************************************************************************************************/
#define __DVP_CONTROLLER_CHANNEL_NR     (2)
#define __DVP_LCD_X_MAX                 (320)
#define __DVP_LCD_Y_MAX                 (240)
/*********************************************************************************************************
  DVP 类型定义
*********************************************************************************************************/
typedef struct {
    LW_VIDEO_DEVICE             DVP_videoEntry;                         /*  视频类字符设备实体          */
    LW_ASYNC_NOTIFIER           DVP_subDevNotify;
    LW_VIDEO_CARD_SUBDEV        DVP_vdieoSubDev;                        /*  摄像头ISP对象               */

    UINT32                      DVP_uiOtherInfo;                        /*  驱动程序的其它信息          */

} __DUMMY_DEV_INSTANCE,  *__PDUMMY_DEV_INSTANCE;
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
static __DUMMY_DEV_INSTANCE   _G_dummyDevInstance = {
        .DVP_uiOtherInfo    = 0xdeadbeef,
};
/*********************************************************************************************************
 LW_VIDEO_IOCTL_OPS
*********************************************************************************************************/
static struct lw_video_ioctl_ops __dummyDevIoctls = {
        .vidioc_s_chan_config   = LW_NULL,  //__dummyDevChannelConfig,
        .vidioc_prepare_vbuf    = LW_NULL,  //__dummyDevPrepareBuf,
        .vidioc_s_cap_ctl       = LW_NULL,  //__dummyDevVideoCapCtrl,
};
/*********************************************************************************************************
 LW_VIDEO_FILE_HANDLES
*********************************************************************************************************/
static VOID  __dummyDevOpen (PLW_VIDEO_DEVICE   pVideoDevice)
{
    /*
     * TODO:  __dummyDevOpen
     */
    bspDebugMsg("__dummyDevOpen CALLED.\r\n");
}

static VOID  __dummyDevClose (PLW_VIDEO_DEVICE   pVideoDevice)
{
    /*
     * TODO:  __dummyDevClose
     */
    bspDebugMsg("__dummyDevClose CALLED.\r\n");
}

static struct lw_video_file_handles __dummyFileOps = {
       .video_fh_open   =  __dummyDevOpen,
       .video_fh_close  =  __dummyDevClose,
};
/*********************************************************************************************************
 Video Device Driver
*********************************************************************************************************/
static LW_VIDEO_DEVICE_DRV _G_VideoDeviceDriver = {
        .DEVICEDRV_pvideoIoctls  = &__dummyDevIoctls,
        .DEVICEDRV_pvideoFileOps = &__dummyFileOps,
};
/*********************************************************************************************************
** 函数名称: k210DvpDrv
** 功能描述: 安装 DVP 驱动
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  dummyVideoDrv(VOID)
{
    INT                 iRet = ERROR_NONE;
    PLW_VIDEO_DEVICE    pVideoDevice;

    pVideoDevice = &_G_dummyDevInstance.DVP_videoEntry;
                                                                        /* 初始化Video结构并分配通道    */
    API_VideoDeviceInit(pVideoDevice, __DVP_CONTROLLER_CHANNEL_NR);
                                                                        /* 安装 video device 驱动       */
    iRet = API_VideoDeviceDrvInstall(pVideoDevice, &_G_VideoDeviceDriver);

    return  (iRet);
}
/*********************************************************************************************************
  异步匹配相关
*********************************************************************************************************/
static VOID  dummyAsyncMatchComplete (PLW_ASYNC_NOTIFIER pNotifier)
{
    PLW_VIDEO_DEVICE    pVideoDevice;

    pVideoDevice = pNotifier->NOTIFY_pVideoDevice;
    API_VideoDeviceCreate(pVideoDevice, "/dev/video_dummy");
}

static LW_MATCH_INFO  _G_matchInfo = {
        .uiMatchType  =  VIDEO_ASYNC_MATCH_DEVNAME,
        .pMatchName   =  "/dev/video_dummy",
};
/*********************************************************************************************************
** 函数名称: k210DvpDevAdd
** 功能描述: 创建 DVP 设备
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  dummyVideoDevAdd(VOID)
{
    INT                 iRet = ERROR_NONE;
    PLW_VIDEO_DEVICE    pVideoDevice;
    PLW_VIDEO_CHANNEL   pVideoChannel;
    PLW_ASYNC_NOTIFIER  pAsyncNotifier;

    pVideoDevice   = &_G_dummyDevInstance.DVP_videoEntry;
    pAsyncNotifier = &_G_dummyDevInstance.DVP_subDevNotify;

    /* To prepare our video device here. */
                                                                        /* 初始化Video设备图像特性参数  */
    API_VideoDevImageAttrSet(pVideoDevice, __DVP_LCD_X_MAX, __DVP_LCD_Y_MAX);

    API_VideoDevCapAttrSet(pVideoDevice,                                /* 初始化Video设备能力特性参数  */
                           VIDEO_CAP_FORMAT_RGB_888 | VIDEO_CAP_FORMAT_RGB_565,
                           VIDEO_CAP_EFFECT_BRIGHTNESS | VIDEO_CAP_EFFECT_CONTRAST);
                                                                        /* 由用户初始化通道相关属性参数 */
    pVideoChannel = &(pVideoDevice->VIDEO_channelArray[0]);
    strncpy(pVideoChannel->VCHAN_cDescription, "display_channel", strlen("display_channel"));
    pVideoChannel = &(pVideoDevice->VIDEO_channelArray[1]);
    strncpy(pVideoChannel->VCHAN_cDescription, "ai_caculate_channel", strlen("ai_caculate_channel"));
                                                                        /* 设置 video device 的私有数据 */
    pVideoDevice->VIDEO_pVideoDevPri =  &_G_dummyDevInstance;

    /* Video device preparing finished here. */


    /* To prepare our video notifier for matching sub device async here. */

    API_AsyncNotifierInit(pAsyncNotifier);                              /* 初始化 AsyncNotifier 结构体  */

    pAsyncNotifier->NOTIFY_uiSubDevNumbers  =  1;                       /* 设置所要匹配的sub device信息 */
    pAsyncNotifier->NOTIFY_pMatchInfoArray  =  &_G_matchInfo;

    API_AsyncCompleteFunSet(pAsyncNotifier, dummyAsyncMatchComplete);   /* 设置匹配成功时的回调函数     */

    API_VideoCardProberInstall(pVideoDevice, pAsyncNotifier);           /* 安装AsyncNotifier到异步匹配层*/

    /* Video notifier preparing finished here. */

    return  (iRet);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
