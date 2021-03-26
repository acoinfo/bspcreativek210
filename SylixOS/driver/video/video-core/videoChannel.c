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
** ��   ��   ��: videoChannel.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 11 �� 28 ��
**
** ��        ��: ��Ƶͨ�����󼰲����ӿڶ���
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include <string.h>
#include "video.h"
#include "driver/common.h"
#include "video_debug.h"

#include "videoChannel.h"
#include "videoDevice.h"
/*********************************************************************************************************
   �����ӿ�ʵ��
*********************************************************************************************************/

INT  API_VideoChannelInit (PLW_VIDEO_CHANNEL pVideoChannel, PLW_VIDEO_DEVICE pVideoDevice, UINT uiIndex)
{
    CHAR    cBuffer[64];

    if (!pVideoChannel || !pVideoDevice) {
        printk("API_VideoBufQueueInit(): invalid parameters.\n");
        return  (PX_ERROR);
    }

    if (!pVideoChannel->VCHAN_bIsInit) {

        pVideoChannel->VCHAN_uiIndex       = uiIndex;
        pVideoChannel->VCHAN_pVideoDevice  = pVideoDevice;
        pVideoChannel->VCHAN_flags        |= VCHAN_STREAM_OFF;

        pVideoChannel->VCHAN_videoBuffer.VBUF_pVideoChannel = pVideoChannel;

        snprintf(cBuffer, sizeof(cBuffer), "video_sem%d", uiIndex);
        pVideoChannel->VCHAN_frameSignal = API_SemaphoreBCreate(cBuffer,
                                                        LW_FALSE,
                                                        LW_OPTION_OBJECT_GLOBAL | LW_OPTION_WAIT_FIFO,
                                                        LW_NULL);
        if (pVideoChannel->VCHAN_frameSignal == LW_OBJECT_HANDLE_INVALID) {
            printk("API_VideoChannelInit(): failed to create video_sem!\n");
            return  (PX_ERROR);
        }

        pVideoChannel->VCHAN_bIsInit = LW_TRUE;
    }

    return  (ERROR_NONE);
}

INT  API_VideoChanRelease (PLW_VIDEO_CHANNEL pVideoChannel)
{
    if (!pVideoChannel) {
        printk("API_VideoChanRelease(): invalid parameters.\n");
        return  (PX_ERROR);
    }

    if (pVideoChannel->VCHAN_frameSignal) {
        API_SemaphoreBDelete(&pVideoChannel->VCHAN_frameSignal);
    }

    pVideoChannel->VCHAN_bIsInit = LW_FALSE;

    return  (ERROR_NONE);
}

INT  API_VideoChanFormatByte (PLW_VIDEO_CHANNEL pVideoChannel)
{
    switch (pVideoChannel->VCHAN_format) {

    case VIDEO_PIXEL_FORMAT_RGBA_8888:
    case VIDEO_PIXEL_FORMAT_RGBX_8888:
    case VIDEO_PIXEL_FORMAT_BGRA_8888:
        return  (4);
        break;

    case VIDEO_PIXEL_FORMAT_RGB_888:
        return  (3);
        break;

    case VIDEO_PIXEL_FORMAT_RGB_565:
    case VIDEO_PIXEL_FORMAT_RGBA_5551:
    case VIDEO_PIXEL_FORMAT_RGBA_4444:
        return  (2);
        break;

    default:
        printk("API_VideoChanFormatByte(): unrecognized format calculate.\n");
        break;
    }

    return  (PX_ERROR);
}

INT  API_VideoChanStreamSet (PLW_VIDEO_CHANNEL pVideoChannel, BOOL on)
{
    if (!pVideoChannel) {
        printk("API_VideoChanStreamSet(): invalid parameters.\n");
        return  (PX_ERROR);
    }

    pVideoChannel->VCHAN_flags |= on;

    return  (ERROR_NONE);
}

/*********************************************************************************************************
  END
*********************************************************************************************************/
