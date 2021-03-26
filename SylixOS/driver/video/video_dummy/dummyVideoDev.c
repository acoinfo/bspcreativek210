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
** ��   ��   ��: dummyVideoDev.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 12 �� 7 ��
**
** ��        ��: ��������ͷ�ӿ�(DVP)����
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
  ����
*********************************************************************************************************/
#define __DVP_CONTROLLER_CHANNEL_NR     (2)
#define __DVP_LCD_X_MAX                 (320)
#define __DVP_LCD_Y_MAX                 (240)
/*********************************************************************************************************
  DVP ���Ͷ���
*********************************************************************************************************/
typedef struct {
    LW_VIDEO_DEVICE             DVP_videoEntry;                         /*  ��Ƶ���ַ��豸ʵ��          */
    LW_ASYNC_NOTIFIER           DVP_subDevNotify;
    LW_VIDEO_CARD_SUBDEV        DVP_vdieoSubDev;                        /*  ����ͷISP����               */

    UINT32                      DVP_uiOtherInfo;                        /*  ���������������Ϣ          */

} __DUMMY_DEV_INSTANCE,  *__PDUMMY_DEV_INSTANCE;
/*********************************************************************************************************
  ȫ�ֱ�������
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
** ��������: k210DvpDrv
** ��������: ��װ DVP ����
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  dummyVideoDrv(VOID)
{
    INT                 iRet = ERROR_NONE;
    PLW_VIDEO_DEVICE    pVideoDevice;

    pVideoDevice = &_G_dummyDevInstance.DVP_videoEntry;
                                                                        /* ��ʼ��Video�ṹ������ͨ��    */
    API_VideoDeviceInit(pVideoDevice, __DVP_CONTROLLER_CHANNEL_NR);
                                                                        /* ��װ video device ����       */
    iRet = API_VideoDeviceDrvInstall(pVideoDevice, &_G_VideoDeviceDriver);

    return  (iRet);
}
/*********************************************************************************************************
  �첽ƥ�����
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
** ��������: k210DvpDevAdd
** ��������: ���� DVP �豸
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
                                                                        /* ��ʼ��Video�豸ͼ�����Բ���  */
    API_VideoDevImageAttrSet(pVideoDevice, __DVP_LCD_X_MAX, __DVP_LCD_Y_MAX);

    API_VideoDevCapAttrSet(pVideoDevice,                                /* ��ʼ��Video�豸�������Բ���  */
                           VIDEO_CAP_FORMAT_RGB_888 | VIDEO_CAP_FORMAT_RGB_565,
                           VIDEO_CAP_EFFECT_BRIGHTNESS | VIDEO_CAP_EFFECT_CONTRAST);
                                                                        /* ���û���ʼ��ͨ��������Բ��� */
    pVideoChannel = &(pVideoDevice->VIDEO_channelArray[0]);
    strncpy(pVideoChannel->VCHAN_cDescription, "display_channel", strlen("display_channel"));
    pVideoChannel = &(pVideoDevice->VIDEO_channelArray[1]);
    strncpy(pVideoChannel->VCHAN_cDescription, "ai_caculate_channel", strlen("ai_caculate_channel"));
                                                                        /* ���� video device ��˽������ */
    pVideoDevice->VIDEO_pVideoDevPri =  &_G_dummyDevInstance;

    /* Video device preparing finished here. */


    /* To prepare our video notifier for matching sub device async here. */

    API_AsyncNotifierInit(pAsyncNotifier);                              /* ��ʼ�� AsyncNotifier �ṹ��  */

    pAsyncNotifier->NOTIFY_uiSubDevNumbers  =  1;                       /* ������Ҫƥ���sub device��Ϣ */
    pAsyncNotifier->NOTIFY_pMatchInfoArray  =  &_G_matchInfo;

    API_AsyncCompleteFunSet(pAsyncNotifier, dummyAsyncMatchComplete);   /* ����ƥ��ɹ�ʱ�Ļص�����     */

    API_VideoCardProberInstall(pVideoDevice, pAsyncNotifier);           /* ��װAsyncNotifier���첽ƥ���*/

    /* Video notifier preparing finished here. */

    return  (iRet);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
