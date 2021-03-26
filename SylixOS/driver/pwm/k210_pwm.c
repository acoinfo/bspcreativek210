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
** ��   ��   ��: k210_pwm.h
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 10 �� 22 ��
**
** ��        ��: PWM ����������
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "SylixOS.h"
#include "config.h"
#include <string.h>
#include "k210_pwm.h"
#include "driver/clock/k210_clock.h"
#include "driver/common.h"
#include "driver/fix_arch_def.h"
#include "KendryteWare/include/pwm.h"
/*********************************************************************************************************
  ����
*********************************************************************************************************/
#define __PWM_CHANNEL_NAME_FMT      "/dev/pwm%d"
#define __PWM_INSTANCES_NR          (3)
#define __PWM_DEFAULT_FREQ          (1000)
/*********************************************************************************************************
  PWM ͨ�����Ͷ��� (�������PWM�������Ŀ��ƼĴ����Ƕ��������ÿһ������������һ������������һ��ȫ����)
*********************************************************************************************************/
typedef struct {
    addr_t                      PWMCHAN_ulPhyAddrBase;                  /*  �����ַ����ַ              */
    enum sysctl_clock_e         PWMCHAN_clock;                          /*  ʱ��Դ���                  */
    size_t                      PWMCHAN_stChanCount;                    /*  PWM ���ͨ����              */
    UINT32                      PWMCHAN_uiFrequency;                    /*  PWM ʱ������                */
    LW_DEV_HDR                  PWMCHAN_devHdr;
    LW_LIST_LINE_HEADER         PWMCHAN_fdNodeHeader;
    time_t                      PWMCHAN_time;
    BOOL                        PWMCHAN_bIsInit;
    spinlock_t                  PWMCHAN_mutexLock;
} __K210_PWM_INSTANCE, *__PK210_PWM_INSTANCE;
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
static __K210_PWM_INSTANCE   _G_k210PwmChannels[__PWM_INSTANCES_NR] = {
        {
                .PWMCHAN_clock          =   SYSCTL_CLOCK_TIMER0,
                .PWMCHAN_ulPhyAddrBase  =   TIMER0_BASE_ADDR,
                .PWMCHAN_stChanCount    =   4,
                .PWMCHAN_uiFrequency    =   0,
        },
        {
                .PWMCHAN_clock          =   SYSCTL_CLOCK_TIMER1,
                .PWMCHAN_ulPhyAddrBase  =   TIMER1_BASE_ADDR,
                .PWMCHAN_stChanCount    =   4,
                .PWMCHAN_uiFrequency    =   0,
        },
        {
                .PWMCHAN_clock          =   SYSCTL_CLOCK_TIMER2,
                .PWMCHAN_ulPhyAddrBase  =   TIMER2_BASE_ADDR,
                .PWMCHAN_stChanCount    =   4,
                .PWMCHAN_uiFrequency    =   0,
        },
};

static INT      _G_iK210PwmDrvNum       =   PX_ERROR;
/*********************************************************************************************************
** ��������: __k210PwmHwInit
** ��������: ��ʼ�� PWM Ӳ���豸
** �䡡��  : pPwmChannel    PWM �豸
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210PwmHwInit (__PK210_PWM_INSTANCE     pPwmChannel)
{
    /*
     * pwm clock enable
     */
    sysctl_clock_enable(pPwmChannel->PWMCHAN_clock);

    pPwmChannel->PWMCHAN_uiFrequency = sysctl_clock_get_freq(pPwmChannel->PWMCHAN_clock);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __k210PwmHwDeInit
** ��������: �ر� PWM �豸
** �䡡��  : pPwmChannel     PWM �豸
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __k210PwmHwDeInit (__PK210_PWM_INSTANCE     pPwmChannel)
{
    /*
     * pwm clock disable
     */
    sysctl_clock_disable(pPwmChannel->PWMCHAN_clock);
}
/*********************************************************************************************************
** ��������: __k210PwmOpen
** ��������: �� PWM �豸
** �䡡��  : pDev       �豸
**           pcName     �豸����
**           iFlags     ��־
**           iMode      ģʽ
** �䡡��  : �ļ��ڵ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LONG  __k210PwmOpen (PLW_DEV_HDR     pDev,
                            PCHAR           pcName,
                            INT             iFlags,
                            INT             iMode)
{
    PLW_FD_NODE                pFdNode;
    BOOL                       bIsNew;
    __PK210_PWM_INSTANCE       pPwmChannel = container_of(pDev, __K210_PWM_INSTANCE, PWMCHAN_devHdr);

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);

    } else {
        pFdNode = API_IosFdNodeAdd(&pPwmChannel->PWMCHAN_fdNodeHeader,
                                   (dev_t)pPwmChannel, 0,
                                   iFlags, iMode, 0, 0, 0, LW_NULL, &bIsNew);
        if (pFdNode == LW_NULL) {
            printk(KERN_ERR "__k210PwmOpen(): failed to add fd node!\n");
            return  (PX_ERROR);
        }

        if (LW_DEV_INC_USE_COUNT(&pPwmChannel->PWMCHAN_devHdr) == 1) {

            if (__k210PwmHwInit(pPwmChannel) != ERROR_NONE) {
                printk(KERN_ERR "__k210PwmOpen(): failed to init!\n");
                goto  __error_handle;
            }

            pPwmChannel->PWMCHAN_bIsInit = LW_TRUE;
        }

        return  ((LONG)pFdNode);
    }

__error_handle:

    if (pFdNode) {
        API_IosFdNodeDec(&pPwmChannel->PWMCHAN_fdNodeHeader, pFdNode, LW_NULL);
        pFdNode = LW_NULL;
    }

    LW_DEV_DEC_USE_COUNT(&pPwmChannel->PWMCHAN_devHdr);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __k210PwmClose
** ��������: �ر� PWM �豸
** �䡡��  : pFdEntry     �ļ��ṹ
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __k210PwmClose (PLW_FD_ENTRY   pFdEntry)
{
    __PK210_PWM_INSTANCE  pPwmChannel = container_of(pFdEntry->FDENTRY_pdevhdrHdr,
                                                          __K210_PWM_INSTANCE, PWMCHAN_devHdr);
    PLW_FD_NODE           pFdNode     = (PLW_FD_NODE)pFdEntry->FDENTRY_pfdnode;

    if (pFdEntry && pFdNode) {
        API_IosFdNodeDec(&pPwmChannel->PWMCHAN_fdNodeHeader, pFdNode, LW_NULL);

        if (LW_DEV_DEC_USE_COUNT(&pPwmChannel->PWMCHAN_devHdr) == 0) {

            __k210PwmHwDeInit(pPwmChannel);

            pPwmChannel->PWMCHAN_bIsInit = LW_FALSE;
        }

        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __k210PwmLstat
** ��������: ��� PWM �豸״̬
** �䡡��  : pDev       �豸
**           pcName     �豸����
**           pStat      stat �ṹָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210PwmLstat (PLW_DEV_HDR     pDev,
                            PCHAR           pcName,
                            struct stat    *pStat)
{
    __PK210_PWM_INSTANCE  pPwmChannel = container_of(pDev, __K210_PWM_INSTANCE, PWMCHAN_devHdr);

    if (pStat) {
        pStat->st_dev     = (dev_t)pPwmChannel;
        pStat->st_ino     = (ino_t)0;
        pStat->st_mode    = (S_IFCHR | S_IRUSR | S_IRGRP | S_IROTH);
        pStat->st_nlink   = 1;
        pStat->st_uid     = 0;
        pStat->st_gid     = 0;
        pStat->st_rdev    = 0;
        pStat->st_size    = 0;
        pStat->st_blksize = 0;
        pStat->st_blocks  = 0;
        pStat->st_atime   = pPwmChannel->PWMCHAN_time;
        pStat->st_mtime   = pPwmChannel->PWMCHAN_time;
        pStat->st_ctime   = pPwmChannel->PWMCHAN_time;
        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __k210PwmIoctl
** ��������: ���� PWM �豸
** �䡡��  : pFdEntry   �ļ��ṹ
**           iCmd       ����
**           lArg       ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210PwmIoctl (PLW_FD_ENTRY      pFdEntry,
                            INT               iCmd,
                            LONG              lArg)
{
    __PK210_PWM_INSTANCE       pPwmChannel = container_of(pFdEntry->FDENTRY_pdevhdrHdr,
                                                          __K210_PWM_INSTANCE, PWMCHAN_devHdr);
    UINT                       uiInstance  = pPwmChannel - &_G_k210PwmChannels[0];
    struct stat               *pStat;
    pwm_ctrl_msg_t            *pCmd;

    switch (iCmd) {

    case FIOFSTATGET:
        pStat = (struct stat *)lArg;
        if (pStat) {
            pStat->st_dev     = (dev_t)pPwmChannel;
            pStat->st_ino     = (ino_t)0;
            pStat->st_mode    = (S_IRUSR | S_IRGRP | S_IROTH);
            pStat->st_nlink   = 1;
            pStat->st_uid     = 0;
            pStat->st_gid     = 0;
            pStat->st_rdev    = 0;
            pStat->st_size    = 0;
            pStat->st_blksize = 0;
            pStat->st_blocks  = 0;
            pStat->st_atime   = pPwmChannel->PWMCHAN_time;
            pStat->st_mtime   = pPwmChannel->PWMCHAN_time;
            pStat->st_ctime   = pPwmChannel->PWMCHAN_time;
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

    case SET_PWM_FREQUENCY:
        pCmd = (pwm_ctrl_msg_t *)lArg;
        pwm_set_frequency(uiInstance, pCmd->uiChannel, pCmd->frequency, 0);
        return  (ERROR_NONE);

    case GET_PWM_FREQUENCY:
        if (pPwmChannel->PWMCHAN_bIsInit == LW_TRUE) {
            *(UINT32 *)lArg = pPwmChannel->PWMCHAN_uiFrequency;
            return  (ERROR_NONE);
        }
        return  (PX_ERROR);

    case SET_PWM_CHANNEL_ENABLE:
        pwm_set_enable(uiInstance, (pwm_channel_number_t)lArg, LW_TRUE);
        return  (ERROR_NONE);

    case SET_PWM_CHANNEL_DISABLE:
        pwm_set_enable(uiInstance, (pwm_channel_number_t)lArg, LW_FALSE);
        return  (ERROR_NONE);

    case SET_PWM_ACTIVE_PERCENT:
        pCmd = (pwm_ctrl_msg_t *)lArg;
        pwm_set_frequency(uiInstance, pCmd->uiChannel, pCmd->frequency, pCmd->duty);
        return  (ERROR_NONE);

    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: k210PwmDrv
** ��������: ��װ PWM ����
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  k210PwmDrv (VOID)
{
    struct file_operations   fileOper;

    if (_G_iK210PwmDrvNum >= 0) {
        return  (ERROR_NONE);
    }

    lib_memset(&fileOper, 0, sizeof(struct file_operations));

    fileOper.owner     = THIS_MODULE;
    fileOper.fo_create = __k210PwmOpen;
    fileOper.fo_open   = __k210PwmOpen;
    fileOper.fo_close  = __k210PwmClose;
    fileOper.fo_lstat  = __k210PwmLstat;
    fileOper.fo_ioctl  = __k210PwmIoctl;

    _G_iK210PwmDrvNum = iosDrvInstallEx2(&fileOper, LW_DRV_TYPE_NEW_1);

    DRIVER_LICENSE(_G_iK210PwmDrvNum,     "Dual BSD/GPL->Ver 1.0");
    DRIVER_AUTHOR(_G_iK210PwmDrvNum,      "Yu.KangZhi");
    DRIVER_DESCRIPTION(_G_iK210PwmDrvNum, "k210 pwm driver.");

    return  ((_G_iK210PwmDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** ��������: k210PwmDevAdd
** ��������: ���� PWM �豸
** �䡡��  : uiChannel         ͨ����
**           pPinMux           �ܽŸ���
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  k210PwmDevAdd(UINT  uiPwmInstance)
{
    __PK210_PWM_INSTANCE       pPwmChannel;
    CHAR                       cBuffer[64];

    if (uiPwmInstance >= __PWM_INSTANCES_NR) {
        printk(KERN_ERR "k210PwmDevAdd(): PWM channel or instance invalid!\n");
        return  (PX_ERROR);
    }

    pPwmChannel = (__PK210_PWM_INSTANCE)&_G_k210PwmChannels[uiPwmInstance];

    pPwmChannel->PWMCHAN_time = time(LW_NULL);

    snprintf(cBuffer, sizeof(cBuffer), __PWM_CHANNEL_NAME_FMT, uiPwmInstance);

    if (API_IosDevAddEx(&pPwmChannel->PWMCHAN_devHdr,
                        cBuffer,
                        _G_iK210PwmDrvNum,
                        DT_CHR) != ERROR_NONE) {
        printk(KERN_ERR "k210PwmDevAdd(): can not add device : %s.\n", strerror(errno));
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
