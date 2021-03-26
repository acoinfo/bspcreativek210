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
** ��   ��   ��: videoQueueLib.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 11 �� 28 ��
**
** ��        ��: ���п�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "video_debug.h"

#include "videoQueueLib.h"
/*********************************************************************************************************
** ��������: API_QueueInit
** ��������: ��ʼ������
** �䡡��  : pQueue            ����
**           uiSize            ���д�С
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  API_QueueInit (PLW_QUEUE  pQueue, UINT  uiSize)
{
    pQueue->QUEUE_uiFront = 0;
    pQueue->QUEUE_uiRear  = 0;
    pQueue->QUEUE_uiSize  = uiSize;
    pQueue->QUEUE_uiCount = 0;
}
/*********************************************************************************************************
** ��������: API_QueueIn
** ��������: �����
** �䡡��  : pQueue            ����
**           pInPos            ��ӵ�
** �䡡��  : TRUE OR FALSE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL  API_QueueIn (PLW_QUEUE  pQueue, UINT  *pInPos)
{
    if (API_QueueIsFull(pQueue)) {
        return  (LW_FALSE);
    } else {
        pQueue->QUEUE_uiCount++;
        *pInPos = pQueue->QUEUE_uiRear;
        pQueue->QUEUE_uiRear = (pQueue->QUEUE_uiRear + 1) % pQueue->QUEUE_uiSize;
        return  (LW_TRUE);
    }
}
/*********************************************************************************************************
** ��������: API_QueueUnIn
** ��������: ȡ�������
** �䡡��  : pQueue            ����
** �䡡��  : TRUE OR FALSE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL  API_QueueUnIn (PLW_QUEUE  pQueue)
{
    if (API_QueueIsEmpty(pQueue)) {
        return  (LW_FALSE);
    } else {
        pQueue->QUEUE_uiCount--;
        pQueue->QUEUE_uiRear--;
        if (pQueue->QUEUE_uiRear >= pQueue->QUEUE_uiSize) {
            pQueue->QUEUE_uiRear =  pQueue->QUEUE_uiSize - 1;
        }
        return  (LW_TRUE);
    }
}
/*********************************************************************************************************
** ��������: API_QueueOut
** ��������: ������
** �䡡��  : pQueue            ����
**           pOutPos           ���ӵ�
** �䡡��  : TRUE OR FALSE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL  API_QueueOut (PLW_QUEUE  pQueue, UINT  *pOutPos)
{
    if (API_QueueIsEmpty(pQueue)) {
        return  (LW_FALSE);
    } else {
        pQueue->QUEUE_uiCount--;
        *pOutPos = pQueue->QUEUE_uiFront;
        pQueue->QUEUE_uiFront = (pQueue->QUEUE_uiFront + 1) % pQueue->QUEUE_uiSize;
        return  (LW_TRUE);
    }
}
/*********************************************************************************************************
** ��������: API_QueueUnOut
** ��������: ȡ��������
** �䡡��  : pQueue            ����
** �䡡��  : TRUE OR FALSE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL  API_QueueUnOut (PLW_QUEUE  pQueue)
{
    if (API_QueueIsFull(pQueue)) {
        return  (LW_FALSE);
    } else {
        pQueue->QUEUE_uiCount++;
        pQueue->QUEUE_uiFront--;
        if (pQueue->QUEUE_uiFront >= pQueue->QUEUE_uiSize) {
            pQueue->QUEUE_uiFront =  pQueue->QUEUE_uiSize - 1;
        }
        return  (LW_TRUE);
    }
}
/*********************************************************************************************************
** ��������: API_QueueIsFull
** ��������: �ж϶����Ƿ���
** �䡡��  : pQueue            ����
** �䡡��  : TRUE OR FALSE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL  API_QueueIsFull (PLW_QUEUE  pQueue)
{
    if (pQueue->QUEUE_uiCount >= pQueue->QUEUE_uiSize)
        return  (LW_TRUE);
    else
        return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: API_QueueIsEmpty
** ��������: �ж϶����Ƿ��
** �䡡��  : pQueue            ����
** �䡡��  : TRUE OR FALSE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL  API_QueueIsEmpty (PLW_QUEUE  pQueue)
{
    if (pQueue->QUEUE_uiCount == 0)
        return  (LW_TRUE);
    else
        return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: API_QueueCount
** ��������: ��ö���Ԫ�ظ���
** �䡡��  : pQueue            ����
** �䡡��  : Ԫ�ظ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT  API_QueueCount (PLW_QUEUE  pQueue)
{
    return  (pQueue->QUEUE_uiCount);
}
/*********************************************************************************************************
** ��������: API_QueueFreeSpace
** ��������: ��ö��п��пռ��С
** �䡡��  : pQueue            ����
** �䡡��  : ���пռ��С
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT  API_QueueFreeSpace (PLW_QUEUE  pQueue)
{
    return  (pQueue->QUEUE_uiSize - pQueue->QUEUE_uiCount);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
