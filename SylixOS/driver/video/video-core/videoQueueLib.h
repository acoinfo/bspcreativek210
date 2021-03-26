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
** 文   件   名: videoQueueLib.h
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 11 月 28 日
**
** 描        述: 队列库头文件.
*********************************************************************************************************/

#ifndef __VIDEOQUEUELIB_H
#define __VIDEOQUEUELIB_H

/*********************************************************************************************************
  队列
*********************************************************************************************************/

typedef struct lw_queue {
    UINT        QUEUE_uiSize;
    UINT        QUEUE_uiFront;
    UINT        QUEUE_uiRear;
    UINT        QUEUE_uiCount;
} LW_QUEUE, *PLW_QUEUE;

/*********************************************************************************************************
  函数声明
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
