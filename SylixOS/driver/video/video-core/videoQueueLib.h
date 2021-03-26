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
** ��   ��   ��: videoQueueLib.h
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 11 �� 28 ��
**
** ��        ��: ���п�ͷ�ļ�.
*********************************************************************************************************/

#ifndef __VIDEOQUEUELIB_H
#define __VIDEOQUEUELIB_H

/*********************************************************************************************************
  ����
*********************************************************************************************************/

typedef struct lw_queue {
    UINT        QUEUE_uiSize;
    UINT        QUEUE_uiFront;
    UINT        QUEUE_uiRear;
    UINT        QUEUE_uiCount;
} LW_QUEUE, *PLW_QUEUE;

/*********************************************************************************************************
  ��������
*********************************************************************************************************/

VOID  API_QueueInit(PLW_QUEUE       pQueue, UINT   uiSize);
BOOL  API_QueueIn(PLW_QUEUE         pQueue, UINT  *pInPos);
BOOL  API_QueueUnIn(PLW_QUEUE       pQueue);
BOOL  API_QueueOut(PLW_QUEUE        pQueue, UINT  *pOutPos);
BOOL  API_QueueUnOut(PLW_QUEUE      pQueue);
BOOL  API_QueueIsFull(PLW_QUEUE     pQueue);
BOOL  API_QueueIsEmpty(PLW_QUEUE    pQueue);
UINT  API_QueueCount(PLW_QUEUE      pQueue);
UINT  API_QueueFreeSpace(PLW_QUEUE  pQueue);

#endif                                                                  /*  __VIDEOQUEUELIB_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
