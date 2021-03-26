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
** ��   ��   ��: videoDevice.h
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 11 �� 28 ��
**
** ��        ��: ��Ƶ�豸���󼰲����ӿڶ���ͷ�ļ�
*********************************************************************************************************/

#ifndef __VIDEODEVICE_H
#define __VIDEODEVICE_H

/*********************************************************************************************************
   ͷ�ļ���������ǰ����
*********************************************************************************************************/
struct lw_video_device;
struct lw_video_device_drv;
struct lw_video_card_subdev;
/*********************************************************************************************************
  �궨�弰���Ͷ���
*********************************************************************************************************/
#define  VIDEO_CARD_MAX_CHANNEL_NUM    (8)
#define  VIDEO_MAX_INPUT_VQUEUE        (16)

typedef struct video_frame_msg {
    int32_t             source;
    int32_t             channel;
    int32_t             mem_addr;
    int32_t             plane_id;
} video_frame_msg;

typedef enum lw_video_format_cap {
    VIDEO_CAP_FORMAT_RGB_888      =   0x1,
    VIDEO_CAP_FORMAT_RGB_565      =   0x2,
    VIDEO_CAP_FORMAT_YUV_422_SP   =   0x4,
} LW_VIDEO_FORMAT_CAP;
#define LW_FORMATS_DESC_MAX  3

typedef enum lw_video_effect_cap {
    VIDEO_CAP_EFFECT_BRIGHTNESS   =   0x1,
    VIDEO_CAP_EFFECT_CONTRAST     =   0x2,
    VIDEO_CAP_EFFECT_RED_BALANCE  =   0x4,
} LW_VIDEO_EFFECT_CAP;
#define LW_EFFECTS_DESC_MAX  3

typedef struct lw_video_event {
    LW_LIST_LINE                    lineNode;
    struct lw_video_card_subdev*    pSubDevice;
    UINT32                          type;
    UINT                            prio;
    struct timespec                 timestamp;
    UINT32                          data;
    UINT32                          reserved[8];
} LW_VIDEO_EVENT, *PLW_VIDEO_EVENT;

typedef VOID (*PLW_VIDEO_NOTIFIER)(struct lw_video_card_subdev* pSubDevice, PLW_VIDEO_EVENT pVideoEvent);

/*********************************************************************************************************
  �豸���ͳ���
*********************************************************************************************************/

typedef struct lw_video_device_drv {
    LW_LIST_MONO                    DEVICEDRV_node;
    INT                             DEVICEDRV_iVideoDrvNum;
    CHAR                            DEVICEDRV_descBuffer[32];
                                                                        /* ��������������ʵ�ֲ����     */
    struct lw_video_ioctl_ops*      DEVICEDRV_pvideoIoctls;
    struct lw_video_file_handles*   DEVICEDRV_pvideoFileOps;

} LW_VIDEO_DEVICE_DRV, *PLW_VIDEO_DEVICE_DRV;

typedef struct lw_video_device {
    LW_DEV_HDR                      VIDEO_devHdr;
    LW_LIST_LINE_HEADER             VIDEO_fdNodeHeader;
    time_t                          VIDEO_time;
    atomic_t                        VIDEO_atomicRefer;

    LW_OBJECT_HANDLE                VIDEO_hlMsgQueue;
    LW_SEL_WAKEUPLIST               VIDEO_waitList;

    spinlock_t                      VIDEO_lock;
    PLW_VIDEO_DEVICE_DRV            VIDEO_pVideoDriver;

    spinlock_t                      VIDEO_slSubDevLock;
    UINT32                          VIDEO_uiSubDevNr;
    LW_LIST_LINE_HEADER             VIDEO_subDevList;
    CHAR                            VIDEO_cDeviceName[32];

    spinlock_t                      VIDEO_slEventLock;
    UINT32                          VIDEO_uiEventNr;
    LW_LIST_LINE_HEADER             VIDEO_pEventListHdr;
    PLW_VIDEO_NOTIFIER              VIDEO_pfuncNotify;

    UINT                            VIDEO_uiChannels;
    struct lw_video_channel*        VIDEO_channelArray;
                                                                        /* ����Ӳ����Ϣ�������������   */
    UINT                            VIDEO_uiMaxImageWidth;
    UINT                            VIDEO_uiMaxImageHeight;
    UINT32                          VIDEO_formatCapability;             /* ÿһλ��ʾ�Ƿ�֧��ĳһ�ָ�ʽ */
    PCHAR                           VIDEO_pFormatsIdArray;              /* ��������֧�ָ�ʽ����������ID */
    UINT32                          VIDEO_effectCapability;             /* ÿһλ��ʾ�Ƿ�֧��ĳһ����Ч */
    PCHAR                           VIDEO_pEffectsIdArray;              /* ��������֧����Ч����������ID */

    PVOID                           VIDEO_pVideoDevPri;

    UINT32                          VIDEO_flags;
#define VIDEO_DEV_REGISTED          (0x1)

} LW_VIDEO_DEVICE,  *PLW_VIDEO_DEVICE;

/*********************************************************************************************************
  ��ز����ӿ�
*********************************************************************************************************/
INT  API_VideoDeviceInit(PLW_VIDEO_DEVICE  pVideoDevice, UINT   uiChannels);

INT  API_VideoDevImageAttrSet(PLW_VIDEO_DEVICE  pVideoDevice, UINT  uiMaxWidth,  UINT  uiMaxHeight);
INT  API_VideoDevCapAttrSet(PLW_VIDEO_DEVICE  pVideoDevice, UINT32  uiFormatCap, UINT32  uiEffectCap);

INT  API_VideoDeviceNotifierInstall(PLW_VIDEO_DEVICE  pVideoDevice, PLW_VIDEO_NOTIFIER pfuncNotifier);

INT  API_VideoDeviceDrvInstall(PLW_VIDEO_DEVICE pVideoDevice, PLW_VIDEO_DEVICE_DRV  pVideoDeviceDrv);

INT  API_VideoDeviceCreate(PLW_VIDEO_DEVICE pVideoDevice, CPCHAR  pcDeviceNameName);

INT  API_VideoDeviceSubDevInstall(PLW_VIDEO_DEVICE   pVideoDevice, struct lw_video_card_subdev* pSubDevice);

INT  API_VideoDeviceSubDevUninstall(struct lw_video_card_subdev* pSubDevice);

INT  API_VideoDevFormatsNrGet(PLW_VIDEO_DEVICE pVideoDevice);

INT  API_VideoDevEffectsNrGet(PLW_VIDEO_DEVICE pVideoDevice);

INT  API_VideoDeviceClear(PLW_VIDEO_DEVICE  pVideoDevice);

#endif                                                                  /*      __VIDEODEVICE_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
