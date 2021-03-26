/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: sccbBus.h
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 11 月 19 日
**
** 描        述: sccb 总线模型.
*********************************************************************************************************/

#ifndef __VIDEOBUS_H
#define __VIDEOBUS_H

/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

/*********************************************************************************************************
  VIDEO 总线传输控制消息:  完整的数据传输包括两个或三个阶段，每一个阶段包含9位数据，其中
                           高8位为所要传输的数据，最低位根据器件所处情况有不同的取值.
*********************************************************************************************************/

typedef struct lw_videobus_message {
    UINT8               VIDEOMSG_uiSlaveAddress;                        /*  SCCB 总线上从机的地址       */
    UINT                VIDEOMSG_uiIdRegByteWidth;                      /*  从机上寄存器编号的字节宽    */
    UINT                VIDEOMSG_uiValueRegByteWidth;                   /*  从机上寄存器的字节宽        */
    UINT32              VIDEOMSG_uiLen;                                 /*  长度(缓冲区大小)            */
    UINT8              *VIDEOMSG_pucRegIdTable;                         /*  读写缓冲区                  */
    UINT8              *VIDEOMSG_pucRegValueTable;                      /*  读写缓冲区                  */
    VOIDFUNCPTR         VIDEOMSG_pfuncComplete;                         /*  传输结束后的回调函数        */
    PVOID               VIDEOMSG_pvContext;                             /*  回调函数参数                */
} LW_VIDEOBUS_MESSAGE;
typedef LW_VIDEOBUS_MESSAGE    *PLW_VIDEOBUS_MESSAGE;

/*********************************************************************************************************
  VIDEO 总线适配器
*********************************************************************************************************/

struct lw_videobus_funcs;
typedef struct lw_videobus_adapter {
    LW_BUS_ADAPTER              VIDEOBUSADAPTER_pbusAdapter;            /*  总线节点                    */
    struct lw_videobus_funcs   *VIDEOBUSADAPTER_pvideoBusfunc;          /*  总线适配器操作函数          */
    
    LW_OBJECT_HANDLE            VIDEOBUSADAPTER_hBusLock;               /*  总线操作锁                  */
    
    LW_LIST_LINE_HEADER         VIDEOBUSADAPTER_plineDevHeader;         /*  设备链表                    */
} LW_VIDEOBUS_ADAPTER;
typedef LW_VIDEOBUS_ADAPTER    *PLW_VIDEOBUS_ADAPTER;

/*********************************************************************************************************
  VIDEO 总线传输函数集
*********************************************************************************************************/

typedef struct lw_videobus_funcs {
    INT             (*VIDEOBUSFUNC_pfuncMasterTx)(PLW_VIDEOBUS_ADAPTER   pvideoBusAdapter,
                                                  PLW_VIDEOBUS_MESSAGE   pvideoBusmsg,
                                                  INT                    iNum);
                                                                        /*  适配器数据传输              */
    INT             (*VIDEOBUSFUNC_pfuncMasterRx)(PLW_VIDEOBUS_ADAPTER   pvideoBusAdapter,
                                                  PLW_VIDEOBUS_MESSAGE   pvideoBusmsg,
                                                  INT                    iNum);
                                                                        /*  适配器数据传输              */
    INT             (*VIDEOBUSFUNC_pfuncMasterCtrl)(PLW_VIDEOBUS_ADAPTER   pvideoBusAdapter,
                                                    INT                    iCmd,
                                                    LONG                   lArg);
                                                                        /*  适配器控制                  */
} LW_VIDEOBUS_FUNCS;
typedef LW_VIDEOBUS_FUNCS       *PLW_VIDEOBUS_FUNCS;

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
#endif                                                                  /*  __VIDEOBUS_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
