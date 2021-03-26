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
** ��   ��   ��: videoIoctl.h
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 11 �� 28 ��
**
** ��        ��: video device �ļ�������ܲ�ӿ�ͷ�ļ�
*********************************************************************************************************/

#ifndef __VIDEOIOCTL_H
#define __VIDEOIOCTL_H

/*********************************************************************************************************
   ��ǰ����
*********************************************************************************************************/
struct lw_video_device;
/*********************************************************************************************************

*********************************************************************************************************/
typedef struct lw_video_ioctl_ops {

    /* VIDIOC_DEVDESC       :   ����豸������Ϣ */
    /* VIDIOC_CHANDESC      :   ���ָ��ͨ������Ϣ */
    /* VIDIOC_FORMATDESC    :   ���ָ��ͨ��֧�ֵĸ�ʽ��Ϣ */
    INT               (*vidioc_g_dev_desc)(struct lw_video_device* pVideoDevice, video_dev_desc *pVideoDevDesc);
    INT               (*vidioc_g_chan_desc)(struct lw_video_device* pVideoDevice, video_channel_desc *pVideoChanDesc);
    INT               (*vidioc_g_format_desc)(struct lw_video_device* pVideoDevice, video_format_desc *pVideoFormatDesc);

    /* VIDIOC_GCHANCTL      :   ��ȡͨ�������� */
    /* VIDIOC_SCHANCTL      :   ����ͨ�������� */
    INT               (*vidioc_g_chan_config)(struct lw_video_device* pVideoDevice, video_channel_ctl *pVideoChanCtl);
    INT               (*vidioc_s_chan_config)(struct lw_video_device* pVideoDevice, video_channel_ctl *pVideoChanCtl);   //===> ����ʵ��

    /* VIDIOC_MAPCAL        :   ��ȡ�ڴ�������� */
    /* VIDIOC_MAPPREPAIR    :   ׼��ӳ��ָ��ͨ���ڴ� */
    INT               (*vidioc_g_vbuf_info)(struct lw_video_device* pVideoDevice, video_buf_cal *pVideoBufInfo);
    INT               (*vidioc_prepare_vbuf)(struct lw_video_device* pVideoDevice, video_buf_ctl *pVideoBufConfig);      //===> ����ʵ��

    /* VIDIOC_CAPSTAT        :  ��ѯ��ǰ������Ϣ */
    INT               (*vidioc_querycap_stat)(struct lw_video_device* pVideoDevice, video_cap_stat *pVideoCapStat);

    /* VIDIOC_GCAPCTL        :  ��õ�ǰ��Ƶ����״̬ */
    /* VIDIOC_SCAPCTL        :  ������Ƶ����״̬ 1:ON 0:OFF */
    INT               (*vidioc_g_cap_ctl)(struct lw_video_device* pVideoDevice, video_cap_ctl *pVideoCaptureCtl);
    INT               (*vidioc_s_cap_ctl)(struct lw_video_device* pVideoDevice, video_cap_ctl *pVideoCaptureCtl);        //===> ����ʵ��

    /* VIDIOC_GEFFCTL        :  ���ͼ����Ч��Ϣ */
    /* VIDIOC_SEFFCTL        :  ��������ͼ����Ч */
    INT               (*vidioc_g_image_effect)(struct lw_video_device* pVideoDevice, video_effect_info *pVideoImageEffect);
    INT               (*vidioc_s_image_effect)(struct lw_video_device* pVideoDevice, video_effect_ctl *pVideoEffectConfig);
} LW_VIDEO_IOCTL_OPS, *PLW_VIDEO_IOCTL_OPS;
/*********************************************************************************************************

*********************************************************************************************************/
INT  API_VideoIoctlManager(PLW_FD_ENTRY  pFdEntry, INT  iCmd, LONG  lArg);

#endif                                                                  /*       __VIDEOIOCTL_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
