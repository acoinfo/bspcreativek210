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
** ��   ��   ��: videoBufCore.h
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 11 �� 28 ��
**
** ��        ��: ������й���ӿ�ͷ�ļ�
*********************************************************************************************************/

#ifndef __VIDEOBUF_H
#define __VIDEOBUF_H

/*********************************************************************************************************
  ͷ�ļ���������ǰ����
*********************************************************************************************************/
struct lw_video_buffer;
struct lw_video_channel;
#include "videoQueueLib.h"
/*********************************************************************************************************
  �궨�弰���Ͷ���
*********************************************************************************************************/
#define VIDOE_BUFFER_ALING_SIZE         (8)
#define VIDEO_BUFFER_MAX_PLANE_NR       (4)

typedef enum vb_buf_mem_type {
    VB_MMAP     =   (1 << 0),
    VB_USERPTR  =   (1 << 1),
    VB_READ     =   (1 << 2),
    VB_WRITE    =   (1 << 3),
    VB_DMABUF   =   (1 << 4),
} vb_buf_mem_type;

typedef enum vb_buffer_state {
    VB_BUF_STATE_IDLE,
    VB_BUF_STATE_PREPARING,
    VB_BUF_STATE_READY,
} vb_buffer_state;

/*********************************************************************************************************
  �豸���ͳ���
*********************************************************************************************************/
typedef struct lw_vbuf_plane {
    UINT                       PLANE_uiIndex;
    vb_buffer_state            PLANE_state;
    UINT64                     PLANE_timestamp;
} LW_VBUF_PLANE, *PLW_VBUF_PLANE;

typedef struct lw_video_buffer {
    struct lw_video_channel*   VBUF_pVideoChannel;
    vb_buf_mem_type            VBUF_uiMemType;
    UINT                       VBUF_uiMemSize;
    addr_t                     VBUF_ulMemAddr;
    spinlock_t                 VBUF_slBufLock;
    BOOL                       VBUF_bIsInit;

    UINT                       VBUF_uiPlaneNum;
    UINT                       VBUF_uiFrameSize;
    UINT                       VBUF_uiCapPlaneId;                       /*   ���ڽ���ͼ��ɼ�ͼ���֡ID */
    struct lw_queue            VBUF_planesQueue;                        /*   ���л��ζ���               */
    PLW_VBUF_PLANE             VBUF_planesArray;

    UINT                       VBUF_uiRecvFrameCount;

} LW_VIDEO_BUFFER, *PLW_VIDEO_BUFFER;
/*********************************************************************************************************
  ��ز����ӿ�
*********************************************************************************************************/
INT  API_VideoBufQueueInit(PLW_VIDEO_BUFFER  pVideoBuf, UINT  uiFrameSize, UINT  uiPlaneNum);
INT  API_VideoBufMemAlloc(PLW_VIDEO_BUFFER  pVideoBuf, UINT  uiMemType, addr_t *pMemAddr, UINT  uiMemSize);
INT  API_VideoBufMemPrep(PLW_VIDEO_BUFFER  pVideoBuf, UINT  uiMemType, addr_t *pMemAddr);

INT  API_VideoBufNextPlaneGet(PLW_VIDEO_BUFFER  pVideoBuf, addr_t *pMemAddr, UINT *pPlaneID);
INT  API_VideoBufImageFrameGet(PLW_VIDEO_BUFFER  pVideoBuf, addr_t *pMemAddr, UINT *pPlaneID);

INT  API_VideoBufMemFree(PLW_VIDEO_BUFFER  pVideoBuf);
INT  API_VideoBufQueueRelease(PLW_VIDEO_BUFFER  pVideoBuf);

#endif                                                                  /*       __VIDEOBUF_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
