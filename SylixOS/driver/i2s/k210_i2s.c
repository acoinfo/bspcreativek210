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
** ��   ��   ��: k210_i2s.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 10 �� 24 ��
**
** ��        ��: ���ɵ�·������Ƶ����(I2S)����ͷ�ļ�
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "SylixOS.h"
#include "config.h"
#include <string.h>
#include "k210_i2s.h"
#include "driver/clock/k210_clock.h"
#include "driver/common.h"
#include "driver/fix_arch_def.h"

#include "KendryteWare/include/i2s.h"
#include "KendryteWare/include/dmac.h"
/*********************************************************************************************************
  ����
*********************************************************************************************************/
#define __I2S_CONTROLLER_NR           (3)
#define __I2S_DEVICE_NAME_FMT         "/dev/i2s_%d"
#define __I2S_STAGE_SEM_NAME_FMT      "i2s_ssem%d"
#define __I2S_COMPLETE_SEM_NAME_FMT   "i2s_csem%d"

/*********************************************************************************************************
  I2S ���������Ͷ���
*********************************************************************************************************/
typedef struct {

    enum sysctl_clock_e         I2SC_clock;
    enum sysctl_dma_select_e    I2SC_dma_req_base;
    enum sysctl_threshold_e     I2SC_clock_threshold;
    addr_t                      I2SC_ulPhyAddrBase;                     /*  �����ַ����ַ              */

    i2s_transmit_t              I2SC_rxtx_mode;
    UINT32                      I2SC_channel_mask;
    size_t                      I2SC_stTxSingleLen;
    UINT                        I2SC_uiChannel;

    BOOL                        I2SC_bIsInit;
    UINT                        I2SC_uiDevId;
    UINT32                      I2SC_uiPeriods;

    LW_DEV_HDR                  I2SC_devHdr;
    LW_LIST_LINE_HEADER         I2SC_fdNodeHeader;
    time_t                      I2SC_time;

    spinlock_t                  I2SC_slLock;
    LW_OBJECT_HANDLE            I2SC_stage_event;
    LW_OBJECT_HANDLE            I2SC_completion_event;

} __K210_I2S_CONTROLLER,  *__PK210_I2S_CONTROLLER;
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
static __K210_I2S_CONTROLLER   _G_k210I2sController[__I2S_CONTROLLER_NR] = {
        {
                .I2SC_ulPhyAddrBase     =   I2S0_BASE_ADDR,
                .I2SC_clock             =   SYSCTL_CLOCK_I2S0,
                .I2SC_dma_req_base      =   SYSCTL_DMA_SELECT_I2S0_RX_REQ,
                .I2SC_clock_threshold   =   SYSCTL_THRESHOLD_I2S0,
                .I2SC_stTxSingleLen     =   16,
                .I2SC_uiDevId           =   0,
        },
        {
                .I2SC_ulPhyAddrBase     =   I2S1_BASE_ADDR,
                .I2SC_clock             =   SYSCTL_CLOCK_I2S1,
                .I2SC_dma_req_base      =   SYSCTL_DMA_SELECT_I2S1_RX_REQ,
                .I2SC_clock_threshold   =   SYSCTL_THRESHOLD_I2S1,
                .I2SC_stTxSingleLen     =   16,
                .I2SC_uiDevId           =   1,
        },
        {
                .I2SC_ulPhyAddrBase     =   I2S2_BASE_ADDR,
                .I2SC_clock             =   SYSCTL_CLOCK_I2S2,
                .I2SC_dma_req_base      =   SYSCTL_DMA_SELECT_I2S2_RX_REQ,
                .I2SC_clock_threshold   =   SYSCTL_THRESHOLD_I2S2,
                .I2SC_stTxSingleLen     =   16,
                .I2SC_uiDevId           =   2,
        }
};

static  INT     _G_iK210I2sDrvNum       =   PX_ERROR;
/*********************************************************************************************************
** ��������: __k210I2sRead
** ��������: �� I2S �豸
** �䡡��  : pFdEntry              �ļ��ṹ
**           pcBuffer              ������
**           stMaxBytes            ����������
** �䡡��  : �ɹ���ȡ���ֽ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  __k210I2sRead (PLW_FD_ENTRY      pFdEntry,
                               PCHAR             pcBuffer,
                               size_t            stMaxBytes)
{
    __PK210_I2S_CONTROLLER      pI2sController = container_of(pFdEntry->FDENTRY_pdevhdrHdr,
                                                 __K210_I2S_CONTROLLER, I2SC_devHdr);

    if (!pFdEntry || !pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!stMaxBytes) {
        return  (0);
    }

    stMaxBytes = stMaxBytes / sizeof(INT);

    i2s_receive_data(pI2sController->I2SC_uiDevId,
                     pI2sController->I2SC_uiChannel,
                     (UINT64 *)pcBuffer, stMaxBytes);

    return  (stMaxBytes);
}
/*********************************************************************************************************
** ��������: __k210I2sWrite
** ��������: д I2S �豸
** �䡡��  : pFdEntry              �ļ��ṹ
**           pcBuffer              ������
**           stMaxBytes            ����������
** �䡡��  : �ɹ�д����ֽ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  __k210I2sWrite (PLW_FD_ENTRY      pFdEntry,
                                PCHAR             pcBuffer,
                                size_t            stMaxBytes)
{
    BOOL                    bNonBlock;
    __PK210_I2S_CONTROLLER  pI2sController = container_of(pFdEntry->FDENTRY_pdevhdrHdr,
                                                            __K210_I2S_CONTROLLER, I2SC_devHdr);

    if (!pFdEntry || !pcBuffer) {                                       /*  ������Ч                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!stMaxBytes) {
        return  (0);
    }

    /*
     * �Ƿ������
     */
    if (pFdEntry->FDENTRY_iFlag & O_NONBLOCK) {
        bNonBlock = LW_TRUE;
    } else {
        bNonBlock = LW_FALSE;
    }

    i2s_send_data(pI2sController->I2SC_uiDevId,
                  pI2sController->I2SC_uiChannel,
                  (uint8_t *)pcBuffer, stMaxBytes,
                  pI2sController->I2SC_stTxSingleLen);

    return  (stMaxBytes);
}
/*********************************************************************************************************
** ��������: __k210I2sOpen
** ��������: �� PWM �豸
** �䡡��  : pDev                  �豸
**           pcName                �豸����
**           iFlags                ��־
**           iMode                 ģʽ
** �䡡��  : �ļ��ڵ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LONG  __k210I2sOpen (PLW_DEV_HDR           pDev,
                            PCHAR                 pcName,
                            INT                   iFlags,
                            INT                   iMode)
{
    PLW_FD_NODE                pFdNode;
    BOOL                       bIsNew;
    CHAR                       cBuffer[64];
    __PK210_I2S_CONTROLLER     pI2sController = container_of(pDev, __K210_I2S_CONTROLLER, I2SC_devHdr);

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);

    } else {
        pFdNode = API_IosFdNodeAdd(&pI2sController->I2SC_fdNodeHeader,
                                   (dev_t)pI2sController, 0,
                                   iFlags, iMode, 0, 0, 0, LW_NULL, &bIsNew);
        if (pFdNode == LW_NULL) {
            printk(KERN_ERR "__k210I2sOpen(): failed to add fd node!\n");
            return  (PX_ERROR);
        }

        if (LW_DEV_INC_USE_COUNT(&pI2sController->I2SC_devHdr) == 1) {

            LW_SPIN_INIT(&pI2sController->I2SC_slLock);                  /*       ��ʼ��������          */

            snprintf(cBuffer, sizeof(cBuffer), __I2S_STAGE_SEM_NAME_FMT, pI2sController->I2SC_uiDevId);
            pI2sController->I2SC_stage_event = API_SemaphoreBCreate(cBuffer, 0,
                                               LW_OPTION_OBJECT_GLOBAL | LW_OPTION_WAIT_FIFO, LW_NULL);
                                                                        /*        ��ʼ���ź���          */
            if (pI2sController->I2SC_stage_event == LW_OBJECT_HANDLE_INVALID) {
                printk(KERN_ERR "__k210AesOpen(): failed to create dvp_signal!\n");
                goto  __error_handle;
            }

            snprintf(cBuffer, sizeof(cBuffer), __I2S_COMPLETE_SEM_NAME_FMT, pI2sController->I2SC_uiDevId);
            pI2sController->I2SC_completion_event = API_SemaphoreBCreate(cBuffer, 0,
                                               LW_OPTION_OBJECT_GLOBAL | LW_OPTION_WAIT_FIFO, LW_NULL);
                                                                        /*        ��ʼ���ź���          */
            if (pI2sController->I2SC_completion_event == LW_OBJECT_HANDLE_INVALID) {
                printk(KERN_ERR "__k210AesOpen(): failed to create dvp_signal!\n");
                goto  __error_handle;
            }

            /*
             * Hardware initialize
             */

            pI2sController->I2SC_bIsInit = LW_TRUE;
        }

        return  ((LONG)pFdNode);
    }

__error_handle:

    if (pI2sController->I2SC_stage_event) {
        API_SemaphoreBDelete(&pI2sController->I2SC_stage_event);
    }

    if (pI2sController->I2SC_completion_event) {
        API_SemaphoreBDelete(&pI2sController->I2SC_completion_event);
    }

    if (pFdNode) {
        API_IosFdNodeDec(&pI2sController->I2SC_fdNodeHeader, pFdNode, LW_NULL);
        pFdNode = LW_NULL;
    }

    LW_DEV_DEC_USE_COUNT(&pI2sController->I2SC_devHdr);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __k210I2sClose
** ��������: �ر� PWM �豸
** �䡡��  : pFdEntry              �ļ��ṹ
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __k210I2sClose (PLW_FD_ENTRY   pFdEntry)
{
    __PK210_I2S_CONTROLLER  pI2sController = container_of(pFdEntry->FDENTRY_pdevhdrHdr,
                                                          __K210_I2S_CONTROLLER, I2SC_devHdr);
    PLW_FD_NODE           pFdNode     = (PLW_FD_NODE)pFdEntry->FDENTRY_pfdnode;

    if (pFdEntry && pFdNode) {
        API_IosFdNodeDec(&pI2sController->I2SC_fdNodeHeader, pFdNode, LW_NULL);

        if (LW_DEV_DEC_USE_COUNT(&pI2sController->I2SC_devHdr) == 0) {

            /*
             * Release system resource
             */
            if (pI2sController->I2SC_stage_event) {
                API_SemaphoreBDelete(&pI2sController->I2SC_stage_event);
            }

            if (pI2sController->I2SC_completion_event) {
                API_SemaphoreBDelete(&pI2sController->I2SC_completion_event);
            }

            pI2sController->I2SC_bIsInit = LW_FALSE;
        }

        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __k210I2sLstat
** ��������: ��� PWM �豸״̬
** �䡡��  : pDev                  �豸
**           pcName                �豸����
**           pStat                 stat �ṹָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210I2sLstat (PLW_DEV_HDR     pDev,
                            PCHAR           pcName,
                            struct stat    *pStat)
{
    __PK210_I2S_CONTROLLER  pI2sController = container_of(pDev, __K210_I2S_CONTROLLER, I2SC_devHdr);

    if (pStat) {
        pStat->st_dev     = (dev_t)pI2sController;
        pStat->st_ino     = (ino_t)0;
        pStat->st_mode    = (S_IFCHR | S_IRUSR | S_IRGRP | S_IROTH);
        pStat->st_nlink   = 1;
        pStat->st_uid     = 0;
        pStat->st_gid     = 0;
        pStat->st_rdev    = 0;
        pStat->st_size    = 0;
        pStat->st_blksize = 0;
        pStat->st_blocks  = 0;
        pStat->st_atime   = pI2sController->I2SC_time;
        pStat->st_mtime   = pI2sController->I2SC_time;
        pStat->st_ctime   = pI2sController->I2SC_time;
        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __k210I2sIoctl
** ��������: ���� PWM �豸
** �䡡��  : pFdEntry              �ļ��ṹ
**           iCmd                  ����
**           lArg                  ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210I2sIoctl (PLW_FD_ENTRY      pFdEntry,
                            INT               iCmd,
                            LONG              lArg)
{
    INT                        uiRet = 0;
    __PK210_I2S_CONTROLLER     pI2sController = container_of(pFdEntry->FDENTRY_pdevhdrHdr,
                                                          __K210_I2S_CONTROLLER, I2SC_devHdr);
    struct stat               *pStat;
    i2s_dev_mode_t            *pMode;
    i2s_dev_config_t          *pConfig;


    switch (iCmd) {

    case FIOFSTATGET:
        pStat = (struct stat *)lArg;
        if (pStat) {
            pStat->st_dev     = (dev_t)pI2sController;
            pStat->st_ino     = (ino_t)0;
            pStat->st_mode    = (S_IRUSR | S_IRGRP | S_IROTH);
            pStat->st_nlink   = 1;
            pStat->st_uid     = 0;
            pStat->st_gid     = 0;
            pStat->st_rdev    = 0;
            pStat->st_size    = 0;
            pStat->st_blksize = 0;
            pStat->st_blocks  = 0;
            pStat->st_atime   = pI2sController->I2SC_time;
            pStat->st_mtime   = pI2sController->I2SC_time;
            pStat->st_ctime   = pI2sController->I2SC_time;
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
         * TODO ���û�ȡƵ�ʡ�ռ�ñȡ���������ʵ��
         */

    case IOCTL_I2S_INSTALL_DEV:
        pMode = (i2s_dev_mode_t *)lArg;

        API_SpinLock(&pI2sController->I2SC_slLock);

        pI2sController->I2SC_rxtx_mode    = pMode->rxtx_mode;
        pI2sController->I2SC_uiChannel    = pMode->data_channel;
        pI2sController->I2SC_channel_mask = pMode->channel_mask;

        i2s_init(pI2sController->I2SC_uiDevId,
                 pI2sController->I2SC_rxtx_mode,
                 pI2sController->I2SC_channel_mask);

        API_SpinUnlock(&pI2sController->I2SC_slLock);
        break;

    case IOCTL_I2S_CHAN_CONFIG:
        pConfig = (i2s_dev_config_t *)lArg;

        API_SpinLock(&pI2sController->I2SC_slLock);

        if (pI2sController->I2SC_rxtx_mode == I2S_RECEIVER) {
            i2s_rx_channel_config(pI2sController->I2SC_uiDevId, pConfig->channel_num, pConfig->word_length,
                                  pConfig->word_select_size, pConfig->trigger_level, pConfig->word_mode);
        } else if (pI2sController->I2SC_rxtx_mode == I2S_TRANSMITTER) {
            i2s_tx_channel_config(pI2sController->I2SC_uiDevId, pConfig->channel_num, pConfig->word_length,
                                  pConfig->word_select_size, pConfig->trigger_level, pConfig->word_mode);
        } else {
            printk("__k210I2sIoctl: i2s controller has not been init!\r\n");
            uiRet = PX_ERROR;
        }

        API_SpinUnlock(&pI2sController->I2SC_slLock);
        break;

    case IOCTL_I2S_SET_SAMPLE_RATE:
        i2s_set_sample_rate(pI2sController->I2SC_uiDevId, (UINT32)lArg);
        break;

    case IOCTL_SINGLE_LENGTH:
        API_SpinLock(&pI2sController->I2SC_slLock);
        pI2sController->I2SC_stTxSingleLen  =  (size_t)lArg;
        API_SpinUnlock(&pI2sController->I2SC_slLock);
        break;

    case IOCTL_I2S_STOP:
        i2s_transmit_set_enable(pI2sController->I2SC_rxtx_mode, 0,
                                (void *)pI2sController->I2SC_ulPhyAddrBase);
        break;

    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }

    return  (uiRet);
}
/*********************************************************************************************************
** ��������: k210I2sDrv
** ��������: ��װ PWM ����
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  k210I2sDrv (VOID)
{
    struct file_operations      fileOper;

    if (_G_iK210I2sDrvNum >= 0) {
        return  (ERROR_NONE);
    }

    lib_memset(&fileOper, 0, sizeof(struct file_operations));

    fileOper.owner     = THIS_MODULE;
    fileOper.fo_create = __k210I2sOpen;
    fileOper.fo_open   = __k210I2sOpen;
    fileOper.fo_close  = __k210I2sClose;
    fileOper.fo_lstat  = __k210I2sLstat;
    fileOper.fo_ioctl  = __k210I2sIoctl;
    fileOper.fo_read   = __k210I2sRead;
    fileOper.fo_write  = __k210I2sWrite;

    _G_iK210I2sDrvNum = iosDrvInstallEx2(&fileOper, LW_DRV_TYPE_NEW_1);

    DRIVER_LICENSE(_G_iK210I2sDrvNum,     "Dual BSD/GPL->Ver 1.0");
    DRIVER_AUTHOR(_G_iK210I2sDrvNum,      "Yu.KangZhi");
    DRIVER_DESCRIPTION(_G_iK210I2sDrvNum, "k210 i2s driver.");

    return  ((_G_iK210I2sDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** ��������: k210I2sDevAdd
** ��������: ���� PWM �豸
** �䡡��  : uiChannel         ͨ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  k210I2sDevAdd (UINT  uiChannel)
{
    __PK210_I2S_CONTROLLER       pI2sDevData;
    CHAR                         cBuffer[64];

    if (uiChannel >= __I2S_CONTROLLER_NR) {
        printk(KERN_ERR "sifivePwmDevAdd(): PWM channel or instance invalid!\n");
        return  (PX_ERROR);
    }

    pI2sDevData = (__PK210_I2S_CONTROLLER)&_G_k210I2sController[uiChannel];

    pI2sDevData->I2SC_time = time(LW_NULL);

    snprintf(cBuffer, sizeof(cBuffer), __I2S_DEVICE_NAME_FMT, uiChannel);

    if (API_IosDevAddEx(&pI2sDevData->I2SC_devHdr,
                        cBuffer,
                        _G_iK210I2sDrvNum,
                        DT_CHR) != ERROR_NONE) {
        printk(KERN_ERR "sifivePwmDevAdd(): can not add device : %s.\n", strerror(errno));
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
