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
** 文   件   名: k210_watchdog.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 10 月 30 日
**
** 描        述: K210 处理器 WATCH_DOG 驱动
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "SylixOS.h"
#include "config.h"
#include <math.h>
#include <string.h>
#include <linux/compat.h>
#include "driver/clock/k210_clock.h"
#include "driver/fix_arch_def.h"
#include "driver/watchdog/k210_watchdog.h"

#include "KendryteWare/include/common.h"
#include "KendryteWare/include/wdt.h"
/*********************************************************************************************************
  定义
*********************************************************************************************************/
#define __WATCH_DOG_CONTROLER_ISR_FMT           "wdog%d_isr"
#define __WATCH_DOG_CONTROLER_NAME_FMT          "/dev/wdt%d"
#define __WATCH_DOG_CONTROLER_NR                (2)

#define COMMON_ENTRY(userdata)                                                      \
        __PK210_WATCH_DOG_CONTROLER data = (__PK210_WATCH_DOG_CONTROLER)userdata;   \
        volatile wdt_t *wdt = (volatile wdt_t *)data->WDTC_ulPhyAddrBase;           \
        (void)wdt;
/*********************************************************************************************************
  WATCH_DOG 控制器类型定义  (参考 freertos-sdk wdt.c 中的 wdt_data)
*********************************************************************************************************/
typedef struct {
    addr_t                      WDTC_ulPhyAddrBase;                     /*  物理地址基地址              */
    sysctl_clock_t              WDTC_clock;
    sysctl_threshold_t          WDTC_threshold;
    sysctl_reset_t              WDTC_reset;
    ULONG                       WDTC_ulIrqVector;
    INT                         WDTC_uiChannel;
    BOOL                        WDTC_bIsInit;
    LW_DEV_HDR                  WDTC_devHdr;
    LW_LIST_LINE_HEADER         WDTC_fdNodeHeader;
    time_t                      WDTC_time;
} __K210_WATCH_DOG_CONTROLER, *__PK210_WATCH_DOG_CONTROLER;
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
static __K210_WATCH_DOG_CONTROLER     _G_k210WatchDogControlers[__WATCH_DOG_CONTROLER_NR] = {
        {
                .WDTC_ulPhyAddrBase =  WDT0_BASE_ADDR,
                .WDTC_clock         =  SYSCTL_CLOCK_WDT0,
                .WDTC_threshold     =  SYSCTL_THRESHOLD_WDT0,
                .WDTC_reset         =  SYSCTL_RESET_WDT0,
                .WDTC_uiChannel     =  0,
                .WDTC_ulIrqVector   =  KENDRYTE_PLIC_VECTOR(IRQN_WDT0_INTERRUPT),
        },
        {
                .WDTC_ulPhyAddrBase =  WDT1_BASE_ADDR,
                .WDTC_clock         =  SYSCTL_CLOCK_WDT1,
                .WDTC_threshold     =  SYSCTL_THRESHOLD_WDT1,
                .WDTC_reset         =  SYSCTL_RESET_WDT1,
                .WDTC_uiChannel     =  1,
                .WDTC_ulIrqVector   =  KENDRYTE_PLIC_VECTOR(IRQN_WDT1_INTERRUPT),
        },
};

static INT      _G_iK210WatchDogDrvNum  =   PX_ERROR;
/*********************************************************************************************************
** 函数名称: __k210WatchDogIsr
** 功能描述: WatchDog 中断服务例程
** 输　入  : pDmaControler     WatchDog 控制器
**           ulVector          中断向量
** 输　出  : 中断返回值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static irqreturn_t __k210WatchDogIsr (__PK210_WATCH_DOG_CONTROLER pWatchDogControler, ULONG ulVector)
{
    wdt_clear_interrupt(pWatchDogControler->WDTC_uiChannel);

    return  (LW_IRQ_HANDLED);
}
/*********************************************************************************************************
** 函数名称: __k210WatchDogHwInit
** 功能描述: 初始化 WATCH_DOG 硬件设备
** 输　入  : pWatchDogControler    WATCH_DOG 设备
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210WatchDogHwInit (__PK210_WATCH_DOG_CONTROLER     pWatchDogControler)
{
    CHAR    cBuffer[64];

    sysctl_reset(pWatchDogControler->WDTC_reset);
    sysctl_clock_set_threshold(pWatchDogControler->WDTC_threshold, 0);
    sysctl_clock_enable(pWatchDogControler->WDTC_clock);

    API_InterVectorSetPriority(pWatchDogControler->WDTC_ulIrqVector, BSP_CFG_TIMER0_INT_PRIO);
    snprintf(cBuffer, sizeof(cBuffer), __WATCH_DOG_CONTROLER_ISR_FMT, pWatchDogControler->WDTC_uiChannel);
    API_InterVectorConnect(pWatchDogControler->WDTC_ulIrqVector, (PINT_SVR_ROUTINE)__k210WatchDogIsr,
                           (PVOID)pWatchDogControler, cBuffer);
    API_InterVectorEnable(pWatchDogControler->WDTC_ulIrqVector);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __k210PwmHwDeInit
** 功能描述: 关闭 WATCH_DOG 硬件设备
** 输　入  : pWatchDogControler    WATCH_DOG 设备
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __k210WatchDogDeInit (__PK210_WATCH_DOG_CONTROLER     pWatchDogControler)
{
    /*
     * TODO: __k210WatchDogDeInit
     */
    pWatchDogControler->WDTC_bIsInit = LW_FALSE;
}
/*********************************************************************************************************
** 函数名称: __k210WatchDogOpen
** 功能描述: 打开 WATCH_DOG 设备
** 输　入  : pDev                  设备
**           pcName                设备名字
**           iFlags                标志
**           iMode                 模式
** 输　出  : 文件节点
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LONG  __k210WatchDogOpen (PLW_DEV_HDR     pDev,
                                 PCHAR           pcName,
                                 INT             iFlags,
                                 INT             iMode)
{
    PLW_FD_NODE                     pFdNode;
    BOOL                            bIsNew;
    __PK210_WATCH_DOG_CONTROLER     pWatchDogControler = container_of(pDev,
                                                         __K210_WATCH_DOG_CONTROLER, WDTC_devHdr);

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    } else {
        pFdNode = API_IosFdNodeAdd(&pWatchDogControler->WDTC_fdNodeHeader,
                                   (dev_t)pWatchDogControler, 0,
                                   iFlags, iMode, 0, 0, 0, LW_NULL, &bIsNew);
        if (pFdNode == LW_NULL) {
            printk(KERN_ERR "__k210WatchDogOpen(): failed to add fd node!\n");
            return  (PX_ERROR);
        }

        if (LW_DEV_INC_USE_COUNT(&pWatchDogControler->WDTC_devHdr) == 1) {
            if (__k210WatchDogHwInit(pWatchDogControler) != ERROR_NONE) {
                printk(KERN_ERR "__k210WatchDogOpen(): failed to init!\n");
                goto  __error_handle;
            }

            pWatchDogControler->WDTC_bIsInit = LW_TRUE;
        }

        return  ((LONG)pFdNode);
    }

__error_handle:

    if (pFdNode) {
        API_IosFdNodeDec(&pWatchDogControler->WDTC_fdNodeHeader, pFdNode, LW_NULL);
        pFdNode = LW_NULL;
    }

    LW_DEV_DEC_USE_COUNT(&pWatchDogControler->WDTC_devHdr);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __k210WatchDogClose
** 功能描述: 关闭 WATCH_DOG 设备
** 输　入  : pFdEntry              文件结构
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __k210WatchDogClose (PLW_FD_ENTRY   pFdEntry)
{
    __PK210_WATCH_DOG_CONTROLER   pWatchDogControler = container_of(pFdEntry->FDENTRY_pdevhdrHdr,
                                                         __K210_WATCH_DOG_CONTROLER, WDTC_devHdr);
    PLW_FD_NODE                   pFdNode = (PLW_FD_NODE)pFdEntry->FDENTRY_pfdnode;

    if (pFdEntry && pFdNode) {
        API_IosFdNodeDec(&pWatchDogControler->WDTC_fdNodeHeader, pFdNode, LW_NULL);

        if (LW_DEV_DEC_USE_COUNT(&pWatchDogControler->WDTC_devHdr) == 0) {
            __k210WatchDogDeInit(pWatchDogControler);
        }

        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __k210WatchDogLstat
** 功能描述: 获得 WATCH_DOG 设备状态
** 输　入  : pDev                  设备
**           pcName                设备名字
**           pStat                 stat 结构指针
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210WatchDogLstat (PLW_DEV_HDR     pDev,
                                 PCHAR           pcName,
                                 struct stat    *pStat)
{
    __PK210_WATCH_DOG_CONTROLER   pWatchDogControler = container_of(pDev,
                                                            __K210_WATCH_DOG_CONTROLER, WDTC_devHdr);

    if (pStat) {
        pStat->st_dev     = (dev_t)pWatchDogControler;
        pStat->st_ino     = (ino_t)0;
        pStat->st_mode    = (S_IFCHR | S_IRUSR | S_IRGRP | S_IROTH);
        pStat->st_nlink   = 1;
        pStat->st_uid     = 0;
        pStat->st_gid     = 0;
        pStat->st_rdev    = 0;
        pStat->st_size    = 0;
        pStat->st_blksize = 0;
        pStat->st_blocks  = 0;
        pStat->st_atime   = pWatchDogControler->WDTC_time;
        pStat->st_mtime   = pWatchDogControler->WDTC_time;
        pStat->st_ctime   = pWatchDogControler->WDTC_time;
        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __k210WatchDogIoctl
** 功能描述: 控制 WATCH_DOG 设备
** 输　入  : pFdEntry              文件结构
**           iCmd                  命令
**           lArg                  参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210WatchDogIoctl (PLW_FD_ENTRY     pFdEntry,
                                 INT              iCmd,
                                 LONG             lArg)
{
    __PK210_WATCH_DOG_CONTROLER   pWatchDogControler = container_of(pFdEntry->FDENTRY_pdevhdrHdr,
                                                         __K210_WATCH_DOG_CONTROLER, WDTC_devHdr);
    struct stat                  *pStat;

    switch (iCmd) {

    case FIOFSTATGET:
        pStat = (struct stat *)lArg;
        if (pStat) {
            pStat->st_dev     = (dev_t)pWatchDogControler;
            pStat->st_ino     = (ino_t)0;
            pStat->st_mode    = (S_IRUSR | S_IRGRP | S_IROTH);
            pStat->st_nlink   = 1;
            pStat->st_uid     = 0;
            pStat->st_gid     = 0;
            pStat->st_rdev    = 0;
            pStat->st_size    = 0;
            pStat->st_blksize = 0;
            pStat->st_blocks  = 0;
            pStat->st_atime   = pWatchDogControler->WDTC_time;
            pStat->st_mtime   = pWatchDogControler->WDTC_time;
            pStat->st_ctime   = pWatchDogControler->WDTC_time;
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

    case WDT_SET_TIMEOUT:
        wdt_start(pWatchDogControler->WDTC_uiChannel, lArg, LW_NULL);
        return  (ERROR_NONE);

    case WDT_SET_STOP:
        wdt_stop(pWatchDogControler->WDTC_uiChannel);
        return  (ERROR_NONE);

    case WDT_FEED_DOG:
        wdt_feed(pWatchDogControler->WDTC_uiChannel);
        return  (ERROR_NONE);

    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: k210WatchDogDrv
** 功能描述: 安装 WATCH_DOG 驱动
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  k210WatchDogDrv (VOID)
{
    struct file_operations   fileOper;

    if (_G_iK210WatchDogDrvNum >= 0) {
        return  (ERROR_NONE);
    }

    lib_memset(&fileOper, 0, sizeof(struct file_operations));

    fileOper.owner     = THIS_MODULE;
    fileOper.fo_create = __k210WatchDogOpen;
    fileOper.fo_open   = __k210WatchDogOpen;
    fileOper.fo_close  = __k210WatchDogClose;
    fileOper.fo_lstat  = __k210WatchDogLstat;
    fileOper.fo_ioctl  = __k210WatchDogIoctl;

    _G_iK210WatchDogDrvNum = iosDrvInstallEx2(&fileOper, LW_DRV_TYPE_NEW_1);

    DRIVER_LICENSE(_G_iK210WatchDogDrvNum,     "Dual BSD/GPL->Ver 1.0");
    DRIVER_AUTHOR(_G_iK210WatchDogDrvNum,      "Yu.KangZhi");
    DRIVER_DESCRIPTION(_G_iK210WatchDogDrvNum, "k210 watch dog driver.");

    return  ((_G_iK210WatchDogDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** 函数名称: k210WatchDogDevAdd
** 功能描述: 创建 WATCH_DOG 设备
** 输　入  : uiIndex           ID
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  k210WatchDogDevAdd (UINT  uiIndex)
{
    __PK210_WATCH_DOG_CONTROLER     pWatchDogControler;
    CHAR                            cBuffer[64];

    if (uiIndex >= __WATCH_DOG_CONTROLER_NR) {
        printk(KERN_ERR "k210WatchDogDevAdd(): watch dog index invalid!\n");
        return  (PX_ERROR);
    }

    pWatchDogControler = (__PK210_WATCH_DOG_CONTROLER)&_G_k210WatchDogControlers[uiIndex];

    pWatchDogControler->WDTC_time = time(LW_NULL);

    snprintf(cBuffer, sizeof(cBuffer), __WATCH_DOG_CONTROLER_NAME_FMT, uiIndex);

    if (API_IosDevAddEx(&pWatchDogControler->WDTC_devHdr,
                        cBuffer,
                        _G_iK210WatchDogDrvNum,
                        DT_CHR) != ERROR_NONE) {
        printk(KERN_ERR "k210WatchDogDevAdd(): can not add device : %s.\n", strerror(errno));
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
