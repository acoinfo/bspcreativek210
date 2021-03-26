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
** 文   件   名: videoChannel.h
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 11 月 28 日
**
** 描        述: 视频通道抽象及操作接口定义头文件
*********************************************************************************************************/

#ifndef __VIDEOCHANNEL_H
#define __VIDEOCHANNEL_H

/*********************************************************************************************************
   头文件包含及超前声明
*********************************************************************************************************/
struct lw_video_device;
struct lw_video_buffer;
#include "videoBufCore.h"
#include "video.h"
/*********************************************************************************************************
  设备类型抽象
*********************************************************************************************************/
#define  VIDEO_MAX_CHANNEL_NUM      (32)

typedef struct lw_video_channel {
    UINT                        VCHAN_uiIndex;
    struct lw_video_device*     VCHAN_pVideoDevice;
    CHAR                        VCHAN_cDescription[32];

    struct lw_video_buffer      VCHAN_videoBuffer;
    LW_OBJECT_HANDLE            VCHAN_frameSignal;
    BOOL                        VCHAN_bIsInit;

    UINT32                      VCHAN_flags;
#define VCHAN_STREAM_OFF        (0x0)
#define VCHAN_STREAM_ON         (0x1)
#define VCHAN_CAP_ONESHOT       (0x2)

    video_pixel_format          VCHAN_format;
    UINT                        VCHAN_formatByteSize;
    UINT32                      VCHAN_width;
    UINT32                      VCHAN_height;
    UINT32                      VCHAN_x_off;
    UINT32                      VCHAN_y_off;
    UINT32                      VCHAN_x_cut;
    UINT32                      VCHAN_y_cut;

} LW_VIDEO_CHANNEL, *PLW_VIDEO_CHANNEL;
/*********************************************************************************************************
  相关操作接口
*********************************************************************************************************/
INT  API_VideoChannelInit(PLW_VIDEO_CHANNEL pVideoChannel, struct lw_video_device* pVideoDevice, UINT uiIndex);

INT  API_VideoChanRelease(PLW_VIDEO_CHANNEL pVideoChannel);

INT  API_VideoChanFormatByte(PLW_VIDEO_CHANNEL pVideoChannel);

INT  API_VideoChanStreamSet(PLW_VIDEO_CHANNEL pVideoChannel, BOOL on);

#endif                                                                  /*  __VIDEOCHANNEL_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
