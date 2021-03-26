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
** 文   件   名: k210_fft.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 10 月 22 日
**
** 描        述: FFT 快速傅里叶变换加速器 驱动头文件
*********************************************************************************************************/
#define __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "config.h"
#include "SylixOS.h"
#include <string.h>
#include "k210_fft.h"
#include "driver/common.h"
#include "driver/clock/k210_clock.h"

#include "KendryteWare/include/fft.h"
#include "KendryteWare/include/dmac.h"
#include "KendryteWare/include/plic.h"
/*********************************************************************************************************
  定义
*********************************************************************************************************/
#define __FFT_CHANNEL_NAME_FMT    "/dev/fft"
/*********************************************************************************************************
  FFT 类型定义
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR                  FFT_devHdr;
    LW_LIST_LINE_HEADER         FFT_fdNodeHeader;
    time_t                      FFT_time;
    ULONG                       FFT_ulIrqVector;                        /*  FFT 中断向量                */

    dmac_channel_number_t       FFT_DmaSendChannel;
    dmac_channel_number_t       FFT_DmaReceiveChannel;
    uintptr_t                   FFT_ulPhyBaseAddr;
    spinlock_t                  FFT_mutex;
    BOOL                        FFT_bIsInit;
} __K210_FFT_INSTANCE,  *__PK210_FFT_INSTANCE;
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
static __K210_FFT_INSTANCE   _G_k210FftINSTANCE = {
    .FFT_ulPhyBaseAddr      =  FFT_BASE_ADDR,
    .FFT_DmaSendChannel     =  DMAC_CHANNEL4,
    .FFT_DmaReceiveChannel  =  DMAC_CHANNEL5,
    .FFT_bIsInit            =  LW_FALSE,
    .FFT_ulIrqVector        =  KENDRYTE_PLIC_VECTOR(IRQN_FFT_INTERRUPT),
};

static INT   _G_iK210FftDrvNum  =  PX_ERROR;
/*********************************************************************************************************
** 函数名称: __k210FftOpen
** 功能描述: 打开 Fft 设备
** 输　入  : pDev                  设备
**           pcName                设备名字
**           iFlags                标志
**           iMode                 模式
** 输　出  : 文件节点
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LONG  __k210FftOpen (PLW_DEV_HDR    pDev,
                            PCHAR          pcName,
                            INT            iFlags,
                            INT            iMode)
{
    __PK210_FFT_INSTANCE    pFftChannel = container_of(pDev, __K210_FFT_INSTANCE, FFT_devHdr);
    PLW_FD_NODE             pFdNode;
    BOOL                    bIsNew;
    UINT                    uiRet = 0;

    volatile fft_t 	*fft = (volatile fft_t*)pFftChannel->FFT_ulPhyBaseAddr;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);

    } else {
        pFdNode = API_IosFdNodeAdd(&pFftChannel->FFT_fdNodeHeader,
                                   (dev_t)pFftChannel, 0,
                                   iFlags, iMode, 0, 0, 0, LW_NULL, &bIsNew);
        if (pFdNode == LW_NULL) {
            printk(KERN_ERR "__k210FftOpen(): failed to add fd node!\n");
            return  (PX_ERROR);
        }

        if (LW_DEV_INC_USE_COUNT(&pFftChannel->FFT_devHdr) == 1) {

            LW_SPIN_INIT(&pFftChannel->FFT_mutex);                      /*       初始化自旋锁           */

            /*
             * Hardware initialize
             */

            uiRet = sysctl_clock_enable(SYSCTL_CLOCK_FFT);
            if (uiRet != ERROR_NONE) {
                printk(KERN_ERR "__k210FftOpen(): failed to sysctl_clock_enable!\n");
                goto  __error_handle;
            }

            sysctl_reset(SYSCTL_RESET_FFT);
            fft->intr_clear.fft_done_clear = 1;
            fft->intr_mask.fft_done_mask   = 0;

            pFftChannel->FFT_bIsInit = LW_TRUE;
        }

        return  ((LONG)pFdNode);
    }

__error_handle:

    if (pFdNode) {
        API_IosFdNodeDec(&pFftChannel->FFT_fdNodeHeader, pFdNode, LW_NULL);
        pFdNode = LW_NULL;
    }

    LW_DEV_DEC_USE_COUNT(&pFftChannel->FFT_devHdr);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __k210FftClose
** 功能描述: 关闭 Fft 设备
** 输　入  : pFdEntry              文件结构
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __k210FftClose (PLW_FD_ENTRY   pFdEntry)
{
    __PK210_FFT_INSTANCE  pFftChannel = container_of(pFdEntry->FDENTRY_pdevhdrHdr,
                                                     __K210_FFT_INSTANCE, FFT_devHdr);
    PLW_FD_NODE           pFdNode     = (PLW_FD_NODE)pFdEntry->FDENTRY_pfdnode;

    if (pFdEntry && pFdNode) {

        API_IosFdNodeDec(&pFftChannel->FFT_fdNodeHeader, pFdNode, LW_NULL);

        if (LW_DEV_DEC_USE_COUNT(&pFftChannel->FFT_devHdr) == 0) {

            /*
             * TODO: release system resource
             */
            pFftChannel->FFT_bIsInit = LW_FALSE;
        }

        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __k210FftLstat
** 功能描述: 获得 Fft 设备状态
** 输　入  : pDev                  设备
**           pcName                设备名字
**           pStat                 stat 结构指针
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210FftLstat (PLW_DEV_HDR     pDev,
                            PCHAR           pcName,
                            struct stat    *pStat)
{
    __PK210_FFT_INSTANCE  pFftChannel = container_of(pDev, __K210_FFT_INSTANCE, FFT_devHdr);

    if (pStat) {
        pStat->st_dev     = (dev_t)pFftChannel;
        pStat->st_ino     = (ino_t)0;
        pStat->st_mode    = (S_IFCHR | S_IRUSR | S_IRGRP | S_IROTH);
        pStat->st_nlink   = 1;
        pStat->st_uid     = 0;
        pStat->st_gid     = 0;
        pStat->st_rdev    = 0;
        pStat->st_size    = 0;
        pStat->st_blksize = 0;
        pStat->st_blocks  = 0;
        pStat->st_atime   = pFftChannel->FFT_time;
        pStat->st_mtime   = pFftChannel->FFT_time;
        pStat->st_ctime   = pFftChannel->FFT_time;
        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __k210FftIoctl
** 功能描述: 控制 Fft 设备
** 输　入  : pFdEntry              文件结构
**           iCmd                  命令
**           lArg                  参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210FftIoctl (PLW_FD_ENTRY    pFdEntry,
                            INT             iCmd,
                            LONG            lArg)
{
    __PK210_FFT_INSTANCE     pFftChannel = container_of(pFdEntry->FDENTRY_pdevhdrHdr,
                                                        __K210_FFT_INSTANCE, FFT_devHdr);
    struct stat             *pStat;
    fft_transaction_t       *pTransaction;

    switch (iCmd) {

    case FIOFSTATGET:
        pStat = (struct stat *)lArg;
        if (pStat) {
            pStat->st_dev     = (dev_t)pFftChannel;
            pStat->st_ino     = (ino_t)0;
            pStat->st_mode    = (S_IRUSR | S_IRGRP | S_IROTH);
            pStat->st_nlink   = 1;
            pStat->st_uid     = 0;
            pStat->st_gid     = 0;
            pStat->st_rdev    = 0;
            pStat->st_size    = 0;
            pStat->st_blksize = 0;
            pStat->st_blocks  = 0;
            pStat->st_atime   = pFftChannel->FFT_time;
            pStat->st_mtime   = pFftChannel->FFT_time;
            pStat->st_ctime   = pFftChannel->FFT_time;
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

    case START_FFT_TRANSACTION:
        /*
         * FFT_DmaSendChannel       发送数据使用的DMA 通道号                  输入
         *
         * FFT_DmaReceiveChannel    接收数据使用的DMA 通道号                  输入
         *
         * direction                FFT 正变换或是逆变换                      输入
         * input                    输入的数据序列，格式为RIRI.., 实部
         *
         *                          与虚部的精度都为16bit。                   输入
         *
         * point_num                待运算的数据点数，只能为512/256/128/64    输入
         * output                   运算后结果。格式为RIRI.., 实部与
         *
         *                          虚部的精度都为16bit。                     输出
         */
        pTransaction = (fft_transaction_t *)lArg;

        API_SpinLock(&pFftChannel->FFT_mutex);

        fft_complex_uint16_dma(pFftChannel->FFT_DmaSendChannel,
                               pFftChannel->FFT_DmaReceiveChannel,
                               pTransaction->direction,
                               pTransaction->input,
                               pTransaction->point_num,
                               pTransaction->output);

        API_SpinUnlock(&pFftChannel->FFT_mutex);

        return  (ERROR_NONE);

    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: k210FFTDrv
** 功能描述: 安装 FFT 驱动
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  k210FFTDrv (VOID)
{
    struct file_operations  fileOper;

    if (_G_iK210FftDrvNum >= 0) {
        return  (ERROR_NONE);
    }

    lib_memset(&fileOper, 0, sizeof(struct file_operations));

    fileOper.owner     = THIS_MODULE;
    fileOper.fo_create = __k210FftOpen;
    fileOper.fo_open   = __k210FftOpen;
    fileOper.fo_close  = __k210FftClose;
    fileOper.fo_lstat  = __k210FftLstat;
    fileOper.fo_ioctl  = __k210FftIoctl;

    _G_iK210FftDrvNum = iosDrvInstallEx2(&fileOper, LW_DRV_TYPE_NEW_1);

    DRIVER_LICENSE(_G_iK210FftDrvNum,     "Dual BSD/GPL->Ver 1.0");
    DRIVER_AUTHOR(_G_iK210FftDrvNum,      "Yu.KangZhi");
    DRIVER_DESCRIPTION(_G_iK210FftDrvNum, "k210 fft driver.");

    return  ((_G_iK210FftDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** 函数名称: k210FFTDevAdd
** 功能描述: 创建 FFT 设备
** 输　入  : uiChannel         通道号
**           pPinMux           管脚复用
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  k210FFTDevAdd(VOID)
{
    __PK210_FFT_INSTANCE  pPwmChannel;

    pPwmChannel = (__PK210_FFT_INSTANCE)&_G_k210FftINSTANCE;

    pPwmChannel->FFT_time = time(LW_NULL);

    if (API_IosDevAddEx(&pPwmChannel->FFT_devHdr,
                        __FFT_CHANNEL_NAME_FMT,
                        _G_iK210FftDrvNum,
                        DT_CHR) != ERROR_NONE) {
        printk(KERN_ERR "k210PwmDevAdd(): can not add device : %s.\n", strerror(errno));
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
