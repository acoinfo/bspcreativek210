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
** 文   件   名: k210_kpu.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 12 月 10 日
**
** 描        述: KPU 控制器驱动
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "SylixOS.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "k210_kpu.h"
#include "driver/clock/k210_clock.h"
#include "driver/fix_arch_def.h"
#include "driver/common.h"

#include "driver/lcd/lcd.h"
#include "driver/kpu/cnn.h"
#include "KendryteWare/include/dmac.h"
/*********************************************************************************************************
  CNN 中断处理函数
*********************************************************************************************************/
extern irqreturn_t cnn_continue(PVOID _task, ULONG ulVector);
/*********************************************************************************************************
  KPU 控制器抽象结构体定义
*********************************************************************************************************/
typedef struct kpu_instance {
    LW_DEV_HDR                  KPU_devHdr;
    LW_LIST_LINE_HEADER         KPU_fdNodeHeader;
    time_t                      KPU_time;
    BOOL                        KPU_bIsInit;

    addr_t                      KPU_ulPhyAddrBase;                      /*  AI_BASE_ADDR 物理地址基地址 */
    enum sysctl_clock_e         KPU_clock;                              /*  SYSCTL_CLOCK_AI 时钟源标记  */
    ULONG                       KPU_irqVector;                          /*  IRQN_AI_INTERRUPT 中断号    */
    UINT                        KPU_uiPrio;                             /*  KPU 中断优先级              */
    spinlock_t                  KPU_lock;                               /*  KPU AI 计算事务上下文锁     */
    LW_OBJECT_HANDLE            KPU_hlKpuAiCalDone;                     /*  KPU AI 计算事务完成信号     */
    kpu_transaction_t           *KPU_pCurTrans;
    cnn_task_t                  *KPU_pTask;

} __K210_KPU_INSTANCE,  *__PK210_KPU_INSTANCE;
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
static __K210_KPU_INSTANCE   _G_k210KpuInstance = {
        .KPU_clock          =   SYSCTL_CLOCK_AI,
        .KPU_ulPhyAddrBase  =   AI_BASE_ADDR,
        .KPU_bIsInit        =   LW_FALSE,
        .KPU_irqVector      =   KENDRYTE_PLIC_VECTOR(IRQN_AI_INTERRUPT),
        .KPU_uiPrio         =   1,
};

static INT _G_iK210KpuDrvNum   =  PX_ERROR;
/*********************************************************************************************************
** 函数名称: ai_done
** 功能描述: 完成 AI 计算后的回调函数
** 输　入  : NONE
** 输　出  : 文件节点
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static int __k210KpuAiDone(void *ctx)
{
    __PK210_KPU_INSTANCE  pKpuInstance;

    pKpuInstance = (__PK210_KPU_INSTANCE)&_G_k210KpuInstance;

//    API_SemaphoreBPost(pKpuInstance->KPU_hlKpuAiCalDone);
                                                                        /*  调用用户的回调函数          */
    if (pKpuInstance->KPU_pCurTrans && pKpuInstance->KPU_pCurTrans->ai_done_callback) {
        pKpuInstance->KPU_pCurTrans->ai_done_callback(pKpuInstance->KPU_pCurTrans->arg);
    }

    return 0;
}
/*********************************************************************************************************
** 函数名称: __k210KpuDrawBoxes
** 功能描述: 根据 KPU 的计算结果在 LCD 上画出所识别图像的框
** 输　入  : x1, y1        图像框左上角坐标
**           x2, y2        图像框右下角坐标
**           class         所识别图像分类
**           prob          (尚不明确)
** 输　出  : 文件节点
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static irqreturn_t __k210KpuIsr (__PK210_KPU_INSTANCE  pKpuInstance, ULONG ulVector)
{
    if (!pKpuInstance->KPU_pTask) {
        printk("__k210KpuIsr(): KPU_pTask not been inited firstly.\n");
    }

    cnn_continue(pKpuInstance->KPU_pTask, IRQN_AI_INTERRUPT);

    return  (LW_IRQ_HANDLED);
}
/*********************************************************************************************************
** 函数名称: __k210KpuOpen
** 功能描述: 打开 KPU 设备
** 输　入  : pDev                  设备
**           pcName                设备名字
**           iFlags                标志
**           iMode                 模式
** 输　出  : 文件节点
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LONG  __k210KpuOpen (PLW_DEV_HDR           pDev,
                            PCHAR                 pcName,
                            INT                   iFlags,
                            INT                   iMode)
{
    PLW_FD_NODE                pFdNode;
    BOOL                       bIsNew;
    __PK210_KPU_INSTANCE       pKpuInstance = container_of(pDev, __K210_KPU_INSTANCE, KPU_devHdr);

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);

    } else {
        pFdNode = API_IosFdNodeAdd(&pKpuInstance->KPU_fdNodeHeader,
                                   (dev_t)pKpuInstance, 0,
                                   iFlags, iMode, 0, 0, 0, LW_NULL, &bIsNew);
        if (pFdNode == LW_NULL) {
            printk(KERN_ERR "__k210KpuOpen(): failed to add fd node!\n");
            return  (PX_ERROR);
        }

        if (LW_DEV_INC_USE_COUNT(&pKpuInstance->KPU_devHdr) == 1) {

            /*
             * TODO:  __k210KpuOpen
             */
            LW_SPIN_INIT(&pKpuInstance->KPU_lock);

            pKpuInstance->KPU_hlKpuAiCalDone = API_SemaphoreBCreate("ai_sem", LW_TRUE,
                                               LW_OPTION_OBJECT_GLOBAL | LW_OPTION_WAIT_FIFO, LW_NULL);
            if (pKpuInstance->KPU_hlKpuAiCalDone == LW_OBJECT_HANDLE_INVALID) {
                printk(KERN_ERR "__k210KpuOpen(): failed to create kpu_signal!\n");
                goto  __error_handle;
            }

            API_InterVectorDisable(KENDRYTE_PLIC_VECTOR(IRQN_AI_INTERRUPT));
            API_InterVectorSetPriority(KENDRYTE_PLIC_VECTOR(IRQN_AI_INTERRUPT), 4);
            API_InterVectorConnect(KENDRYTE_PLIC_VECTOR(IRQN_AI_INTERRUPT), (PINT_SVR_ROUTINE)__k210KpuIsr,
                                   (PVOID)pKpuInstance, "cnn_isr");

            pKpuInstance->KPU_bIsInit = LW_TRUE;
        }

        return  ((LONG)pFdNode);
    }

__error_handle:

    if (pFdNode) {
        API_IosFdNodeDec(&pKpuInstance->KPU_fdNodeHeader, pFdNode, LW_NULL);
        pFdNode = LW_NULL;
    }

    LW_DEV_DEC_USE_COUNT(&pKpuInstance->KPU_devHdr);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __k210KpuClose
** 功能描述: 关闭 KPU 设备
** 输　入  : pFdEntry              文件结构
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __k210KpuClose (PLW_FD_ENTRY   pFdEntry)
{
    __PK210_KPU_INSTANCE  pKpuInstance = container_of(pFdEntry->FDENTRY_pdevhdrHdr,
                                                          __K210_KPU_INSTANCE, KPU_devHdr);
    PLW_FD_NODE           pFdNode     = (PLW_FD_NODE)pFdEntry->FDENTRY_pfdnode;

    if (pFdEntry && pFdNode) {
        API_IosFdNodeDec(&pKpuInstance->KPU_fdNodeHeader, pFdNode, LW_NULL);

        if (LW_DEV_DEC_USE_COUNT(&pKpuInstance->KPU_devHdr) == 0) {

            /*
             * TODO:  __k210KpuClose
             */

            pKpuInstance->KPU_bIsInit = LW_FALSE;
        }

        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __k210KpuLstat
** 功能描述: 获得 KPU 设备状态
** 输　入  : pDev                  设备
**           pcName                设备名字
**           pStat                 stat 结构指针
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210KpuLstat (PLW_DEV_HDR           pDev,
                            PCHAR                 pcName,
                            struct stat          *pStat)
{
    __PK210_KPU_INSTANCE  pKpuInstance = container_of(pDev, __K210_KPU_INSTANCE, KPU_devHdr);

    if (pStat) {
        pStat->st_dev     = (dev_t)pKpuInstance;
        pStat->st_ino     = (ino_t)0;
        pStat->st_mode    = (S_IFCHR | S_IRUSR | S_IRGRP | S_IROTH);
        pStat->st_nlink   = 1;
        pStat->st_uid     = 0;
        pStat->st_gid     = 0;
        pStat->st_rdev    = 0;
        pStat->st_size    = 0;
        pStat->st_blksize = 0;
        pStat->st_blocks  = 0;
        pStat->st_atime   = pKpuInstance->KPU_time;
        pStat->st_mtime   = pKpuInstance->KPU_time;
        pStat->st_ctime   = pKpuInstance->KPU_time;
        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __k210KpuIoctl
** 功能描述: 控制 KPU 设备
** 输　入  : pFdEntry              文件结构
**           iCmd                  命令
**           lArg                  参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210KpuIoctl (PLW_FD_ENTRY      pFdEntry,
                            INT               iCmd,
                            LONG              lArg)
{
    struct stat             *pStat;
    __PK210_KPU_INSTANCE     pKpuInstance = container_of(pFdEntry->FDENTRY_pdevhdrHdr, __K210_KPU_INSTANCE, KPU_devHdr);

    kpu_transaction_t       *pKpuTransArg;

    switch (iCmd) {

    case FIOFSTATGET:
        pStat = (struct stat *)lArg;
        if (pStat) {
            pStat->st_dev     = (dev_t)pKpuInstance;
            pStat->st_ino     = (ino_t)0;
            pStat->st_mode    = (S_IRUSR | S_IRGRP | S_IROTH);
            pStat->st_nlink   = 1;
            pStat->st_uid     = 0;
            pStat->st_gid     = 0;
            pStat->st_rdev    = 0;
            pStat->st_size    = 0;
            pStat->st_blksize = 0;
            pStat->st_blocks  = 0;
            pStat->st_atime   = pKpuInstance->KPU_time;
            pStat->st_mtime   = pKpuInstance->KPU_time;
            pStat->st_ctime   = pKpuInstance->KPU_time;
            return  (ERROR_NONE);
        }
        return  (PX_ERROR);

    case FIOSETFL:
        if ((int)lArg & O_NONBLOCK) {
            pFdEntry->FDENTRY_iFlag |= O_NONBLOCK;
        } else {
            pFdEntry->FDENTRY_iFlag &= ~O_NONBLOCK;
        }
        return  (ERROR_NONE);

    /*
     * TODO:  __k210KpuIoctl
     */
    case IOCTRL_KPU_TASK_INIT:
        pKpuTransArg = (kpu_transaction_t *)lArg;

        pKpuInstance->KPU_pTask = (cnn_task_t *)pKpuTransArg->task;     /*  用于中断处理时的参数        */
                                                                        /*  必须在 cnn_run 之前调用     */
        /* init kpu task*/
        cnn_task_init((cnn_task_t*)pKpuTransArg->task);

        return  (ERROR_NONE);

    case IOCTRL_KPU_CALCULATE_RUN:
        pKpuTransArg = (kpu_transaction_t *)lArg;

        API_SpinLock(&pKpuInstance->KPU_lock);

        /* recored kpu_transaction_t */
        pKpuInstance->KPU_pCurTrans = pKpuTransArg;

//        API_SemaphoreBClear(pKpuInstance->KPU_hlKpuAiCalDone);

        /* start to calculate */
        cnn_run((cnn_task_t*)pKpuTransArg->task,
                pKpuTransArg->dma_ch,
                pKpuTransArg->src,
                pKpuTransArg->dst,
                __k210KpuAiDone);

        API_SpinUnlock(&pKpuInstance->KPU_lock);

//        API_SemaphoreBPend(pKpuInstance->KPU_hlKpuAiCalDone, LW_OPTION_WAIT_INFINITE);
        return  (ERROR_NONE);

    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: k210KpuDrv
** 功能描述: 安装 KPU 驱动
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  k210KpuDrv (VOID)
{
    struct file_operations      fileOper;

    if (_G_iK210KpuDrvNum >= 0) {
        return  (ERROR_NONE);
    }

    lib_memset(&fileOper, 0, sizeof(struct file_operations));

    fileOper.owner     = THIS_MODULE;
    fileOper.fo_create = __k210KpuOpen;
    fileOper.fo_open   = __k210KpuOpen;
    fileOper.fo_close  = __k210KpuClose;
    fileOper.fo_lstat  = __k210KpuLstat;
    fileOper.fo_ioctl  = __k210KpuIoctl;

    _G_iK210KpuDrvNum = iosDrvInstallEx2(&fileOper, LW_DRV_TYPE_NEW_1);

    DRIVER_LICENSE(_G_iK210KpuDrvNum,     "Dual BSD/GPL->Ver 1.0");
    DRIVER_AUTHOR(_G_iK210KpuDrvNum,      "Yu.KangZhi");
    DRIVER_DESCRIPTION(_G_iK210KpuDrvNum, "k210 kpu driver.");

    return  ((_G_iK210KpuDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** 函数名称: k210KpuDevAdd
** 功能描述: 创建 KPU 设备
** 输　入  : uiChannel         通道号
**           pPinMux           管脚复用
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  k210KpuDevAdd(VOID)
{
    __PK210_KPU_INSTANCE  pKpuInstance;

    pKpuInstance = (__PK210_KPU_INSTANCE)&_G_k210KpuInstance;

    pKpuInstance->KPU_time = time(LW_NULL);

    if (API_IosDevAddEx(&pKpuInstance->KPU_devHdr,
                        "/dev/k210_kpu",
                        _G_iK210KpuDrvNum,
                        DT_CHR) != ERROR_NONE) {
        printk(KERN_ERR "k210KpuDevAdd(): can not add device : %s.\n", strerror(errno));
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
