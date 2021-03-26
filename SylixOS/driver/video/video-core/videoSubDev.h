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
** ��   ��   ��: videoSubDev.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 11 �� 28 ��
**
** ��        ��: ��Ƶ�����豸ͷ�ļ�
*********************************************************************************************************/

#ifndef __VIDEOSUBDEV_H
#define __VIDEOSUBDEV_H

/*********************************************************************************************************
   ��ǰ����
*********************************************************************************************************/
struct lw_video_card_subdev;
struct lw_video_device;
struct lw_video_event;
/*********************************************************************************************************

*********************************************************************************************************/

typedef enum lw_card_client_bus_type{
    SUBDEV_BUS_TYPE_SCCB  =  0,
    SUBDEV_BUS_TYPE_I2C   =  1,
    SUBDEV_BUS_TYPE_SPI   =  2,
    SUBDEV_BUS_TYPE_USB   =  3,
    SUBDEV_BUS_TYPE_MAX
} LW_CARD_CLIENT_BUS_TYPE;

struct subdev_camera_isp_ops {                                                                 //�ýӿڸ���Linux��OV5640�����Ĳ��ֽӿڶ��壬�ο����� ov5640_ctrl_ops
    INT        (*set_ctrl_gain)(struct lw_video_card_subdev*  pSubDevice, INT auto_gain);      //������Ƶ�ɼ�ģʽΪ�Զ���ȡ�������ֶ�����
    INT        (*set_ctrl_exposure)(struct lw_video_card_subdev*  pSubDevice, INT exp);        //��������ͷ���ع����
    INT        (*set_ctrl_white_balance)(struct lw_video_card_subdev*  pSubDevice, INT awb);   //��������ͷ�İ�ƽ��
    INT        (*set_ctrl_hue)(struct lw_video_card_subdev*  pSubDevice, int value);           //��������ͷ��ɫ��
    INT        (*set_ctrl_contrast)(struct lw_video_card_subdev*  pSubDevice, int value);      //��������ͷ�ĶԱȶ�
    INT        (*set_ctrl_saturation)(struct lw_video_card_subdev*  pSubDevice, int value);    //��������ͷ�ɼ�ͼ��ı��Ͷ�
};

typedef struct lw_subdev_common_ops {
    const struct subdev_camera_isp_ops   *cameraIspOps;                 /*  ����ͷ�� Image sensor processor һЩͨ�ù��ܵĽӿ� */
    const uint32_t                       *otherIC[8];                   /*  Ԥ������������ IC ��ͨ�ù��̽ӿ� */
} LW_SUBDEV_COMMON_OPS,  *PLW_SUBDEV_COMMON_OPS;

typedef struct lw_subdev_internal_ops {
    INT         (*bind)(struct lw_video_card_subdev*  pSubDevice);             /*  ����video_cardƥ��ʱ������  */
    INT         (*unbind)(struct lw_video_card_subdev*  pSubDevice);           /*  ����video_cardƥ����ж��ʱ������  */

    INT         (*init)(struct lw_video_card_subdev*  pSubDevice);             /*  ���ڳ�ʼ�����豸�Ĺ���ģʽ  */
    INT         (*release)(struct lw_video_card_subdev*  pSubDevice);          /*  �����ͷ�Ϊ���豸���������Դ  */
} LW_SUBDEV_INTERNAL_OPS,  *PLW_SUBDEV_INTERNAL_OPS;
/*********************************************************************************************************

*********************************************************************************************************/
typedef struct lw_video_card_subdev {
    LW_LIST_LINE                    SUBDEV_lineNode;                    /*  �������ӵ��첽ƥ��������ƥ�䵽�� video_card_devcie �� */
    CHAR                            SUBDEV_bindCardName[64];            /*  ������Ҫƥ�����Ƶ��������  */
    struct lw_video_device*         SUBDEV_pCardDevice;                 /*  ָ����ƥ�䵽����Ƶ���ṹ��  */

    LW_CARD_CLIENT_BUS_TYPE         SUBDEV_clientBusType;               /*  ���� Coder, Decoder, ISP ��ʹ���õ��������� */
    CHAR                            SUBDEV_clientBusName[64];           /*  ���ߵ��豸�ļ�·������: /dev/i2c */
    PVOID                           SUBDEV_pBusClient;                  /*  ��: i2c_device, spi_device �� */
    INT32                           SUBDEV_iClientAddr;                 /*  �ӻ��ĵ�ַ */

    spinlock_t                      SUBDEV_lock;                        /*  ��Ҫ���ڹ��� ops �Ļ������ */
    PLW_SUBDEV_COMMON_OPS           SUBDEV_pCommonOps;                  /*  �����豸��һЩͨ�ù���ʵ�ֽӿڣ������豸���ͺ;����֧�����ѡ��ʵ�� */
    PLW_SUBDEV_INTERNAL_OPS         SUBDEV_pInternalOps;                /*  �������ʹ�õ�һЩͨ�ýӿ� */

    PVOID                           SUBDEV_pSubDevPri;                  /*  ˽������ָ�룬һ������ָ������ýṹ������� */

} LW_VIDEO_CARD_SUBDEV, *PLW_VIDEO_CARD_SUBDEV;
/*********************************************************************************************************

*********************************************************************************************************/
INT    API_VideoCardSubDevInit(PLW_VIDEO_CARD_SUBDEV       pSubDevice,
                               PCHAR                       pBindCardName,
                               LW_CARD_CLIENT_BUS_TYPE     busType,
                               PCHAR                       pBusAdapterName,
                               INT32                       iClientAddr);

INT    API_VideoCardSubDevOpsSet(PLW_VIDEO_CARD_SUBDEV     pSubDevice,
                                 PLW_SUBDEV_COMMON_OPS     pCommonOps,
                                 PLW_SUBDEV_INTERNAL_OPS   pInternalOps);

VOID   API_VideoSubDevPriDataGet(PLW_VIDEO_CARD_SUBDEV  pSubDevice, PVOID  *pSubDevPriData);

VOID   API_VideoSubDevPriDataSet(PLW_VIDEO_CARD_SUBDEV  pSubDevice, PVOID  pSubDevPriData);

VOID   API_VideoSubDevEntryGet(PLW_LIST_LINE  pSubDevNode, PLW_VIDEO_CARD_SUBDEV  *pSubDev);

INT    API_VideoSubDevEventAdd(struct lw_video_device*  pVideoDevice, struct lw_video_event*  pVideoEvent);

INT    API_VideoSubDevNotifyEvent(PLW_VIDEO_CARD_SUBDEV  pSubDevice, struct lw_video_event*  pVideoEvent);

#endif                                                                  /*      __VIDEOSUBDEV_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
