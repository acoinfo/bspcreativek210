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
** ��   ��   ��: videoAsync.h
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 11 �� 28 ��
**
** ��        ��: �첽ƥ���ͷ�ļ�
*********************************************************************************************************/

#ifndef __VIDEOASYNC_H
#define __VIDEOASYNC_H

/*********************************************************************************************************
   ��ǰ����
*********************************************************************************************************/
struct lw_video_card_subdev;
struct lw_video_device;
/*********************************************************************************************************

*********************************************************************************************************/
typedef enum lw_async_amtch_type{
    VIDEO_ASYNC_MATCH_DEVNAME = 0
} LW_ASYNC_AMTCH_TYPE;

typedef struct lw_match_info {
    UINT32       uiMatchType;
    CPCHAR       pMatchName;    //pSubDevice->SUBDEV_bindCardName
} LW_MATCH_INFO, *PLW_MATCH_INFO;

/*********************************************************************************************************

*********************************************************************************************************/
typedef struct lw_async_notifier {
    LW_LIST_LINE                NOTIFY_lineNode;
    LW_OBJECT_HANDLE            NOTIFY_hListSignal;

    UINT                        NOTIFY_uiMatchedDevNr;
    UINT                        NOTIFY_uiSubDevNumbers;
    PLW_MATCH_INFO              NOTIFY_pMatchInfoArray;

    struct lw_video_device*     NOTIFY_pVideoDevice;
    VOID                       (*NOTIFY_pfuncComplete)(struct lw_async_notifier* pNotifier);
} LW_ASYNC_NOTIFIER,  *PLW_ASYNC_NOTIFIER;

typedef VOID  (*PLW_ASYNC_COMPLETE_FUNC)(struct lw_async_notifier* pNotifier);
/*********************************************************************************************************

*********************************************************************************************************/
INT API_AsyncSystemLibInit(VOID);

INT API_AsyncNotifierInit(PLW_ASYNC_NOTIFIER  pNotifier);

INT API_AsyncCompleteFunSet(PLW_ASYNC_NOTIFIER  pNotifier, PLW_ASYNC_COMPLETE_FUNC pFunComplete);

INT API_VideoCardSubDevInstall(struct lw_video_card_subdev*  pSubDevice);

INT API_VideoCardProberInstall(struct lw_video_device*  pVideoDevice, PLW_ASYNC_NOTIFIER  pNotifier);

#endif                                                                  /*         __VIDEOASYNC_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
