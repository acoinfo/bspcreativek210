/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: sccbBusLib.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 11 �� 19 ��
**
** ��        ��: sccb  ���ߺ��豸������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "sccbBus.h"
#include "sccbBusDev.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_OBJECT_HANDLE             _G_hVideoBusListLock = LW_OBJECT_HANDLE_INVALID;
/*********************************************************************************************************
  �����
*********************************************************************************************************/
#define __VIDEOBUS_LIST_LOCK()      API_SemaphoreBPend(_G_hVideoBusListLock, LW_OPTION_WAIT_INFINITE)
#define __VIDEOBUS_LIST_UNLOCK()    API_SemaphoreBPost(_G_hVideoBusListLock)
/*********************************************************************************************************
** ��������: __videoBusAdapterFind
** ��������: ��ѯ VIDEO BUS ������
** �䡡��  : pcName        ����������
** �䡡��  : ������ָ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_VIDEOBUS_ADAPTER  __videoBusAdapterFind (CPCHAR  pcName)
{
    REGISTER PLW_BUS_ADAPTER        pbusAdapter;
    
    pbusAdapter = __busAdapterGet(pcName);                              /*  �������߲�������            */

    return  ((PLW_VIDEOBUS_ADAPTER)pbusAdapter);                        /*  ���߽ṹΪ��������Ԫ��      */
}
/*********************************************************************************************************
** ��������: API_VideoBusLibInit
** ��������: ��ʼ�� VIDEO BUS �����
** �䡡��  : NONE
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_VideoBusLibInit (VOID)
{
    if (_G_hVideoBusListLock == LW_OBJECT_HANDLE_INVALID) {
        _G_hVideoBusListLock = API_SemaphoreBCreate("videobus_listlock",
                                                    LW_TRUE,
                                                    LW_OPTION_WAIT_FIFO | LW_OPTION_OBJECT_GLOBAL,
                                                    LW_NULL);
    }
    
    if (_G_hVideoBusListLock) {
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_VideoBusAdapterCreate
** ��������: ����һ�� VIDEO BUS ������
** �䡡��  : pcName            ����������
**           pvideoBusfunc     ����������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_VideoBusAdapterCreate (CPCHAR              pcName,
                                PLW_VIDEOBUS_FUNCS  pvideoBusfunc)
{
    REGISTER PLW_VIDEOBUS_ADAPTER       pvideoBusAdapter;

    if (!pcName || !pvideoBusfunc) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (_Object_Name_Invalid(pcName)) {                                 /*  ���������Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (PX_ERROR);
    }
    
    /*
     *  �������ƿ�
     */
    pvideoBusAdapter = (PLW_VIDEOBUS_ADAPTER)__SHEAP_ALLOC(sizeof(LW_VIDEOBUS_ADAPTER));
    if (pvideoBusAdapter == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    pvideoBusAdapter->VIDEOBUSADAPTER_pvideoBusfunc  = pvideoBusfunc;
    pvideoBusAdapter->VIDEOBUSADAPTER_plineDevHeader = LW_NULL;         /*  Ŀǰ�������� VIDEO BUS �豸 */
    
    /*
     *  ���������
     */
    pvideoBusAdapter->VIDEOBUSADAPTER_hBusLock = API_SemaphoreMCreate("videobus_lock",
                                                                      LW_PRIO_DEF_CEILING,
                                                                      LW_OPTION_WAIT_PRIORITY |
                                                                      LW_OPTION_INHERIT_PRIORITY |
                                                                      LW_OPTION_DELETE_SAFE |
                                                                      LW_OPTION_OBJECT_GLOBAL,
                                                                      LW_NULL);
    if (pvideoBusAdapter->VIDEOBUSADAPTER_hBusLock == LW_OBJECT_HANDLE_INVALID) {
        __SHEAP_FREE(pvideoBusAdapter);
        return  (PX_ERROR);
    }
    
    /*
     *  �������߲�
     */
    if (__busAdapterCreate(&pvideoBusAdapter->VIDEOBUSADAPTER_pbusAdapter, pcName) != ERROR_NONE) {
        API_SemaphoreMDelete(&pvideoBusAdapter->VIDEOBUSADAPTER_hBusLock);
        __SHEAP_FREE(pvideoBusAdapter);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_VideoBusAdapterDelete
** ��������: �Ƴ�һ�� VIDEO BUS ������
** �䡡��  : pcName        ����������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���ں��е���������ȷ��������ʹ�������������, ����ɾ�����������.
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_VideoBusAdapterDelete (CPCHAR  pcName)
{
    REGISTER PLW_VIDEOBUS_ADAPTER       pvideoBusAdapter = __videoBusAdapterFind(pcName);
    
    if (pvideoBusAdapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (PX_ERROR);
    }
    
    /*
     *  ����Ӳ����豸, �����е�����, ��û���κα���!
     */
    if (API_SemaphoreMPend(pvideoBusAdapter->VIDEOBUSADAPTER_hBusLock,
                           LW_OPTION_WAIT_INFINITE) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    if (pvideoBusAdapter->VIDEOBUSADAPTER_plineDevHeader) {             /*  ����Ƿ����豸���ӵ�������  */
        API_SemaphoreMPost(pvideoBusAdapter->VIDEOBUSADAPTER_hBusLock);
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }
    
    if (__busAdapterDelete(pcName) != ERROR_NONE) {                     /*  �����߲��Ƴ�                */
        API_SemaphoreMPost(pvideoBusAdapter->VIDEOBUSADAPTER_hBusLock);
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }
    
    API_SemaphoreMDelete(&pvideoBusAdapter->VIDEOBUSADAPTER_hBusLock);  /*  ɾ���������ź���            */
    __SHEAP_FREE(pvideoBusAdapter);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_VideoBusAdapterGet
** ��������: ͨ�����ֻ�ȡһ�� VIDEO BUS ������
** �䡡��  : pcName        ����������
** �䡡��  : videoBus ���������ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PLW_VIDEOBUS_ADAPTER  API_VideoBusAdapterGet (CPCHAR  pcName)
{
    REGISTER PLW_VIDEOBUS_ADAPTER       pvideoBusAdapter = __videoBusAdapterFind(pcName);
    
    if (pvideoBusAdapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (LW_NULL);
    
    } else {
        return  (pvideoBusAdapter);
    }
}
/*********************************************************************************************************
** ��������: API_VideoBusDeviceCreate
** ��������: ��ָ�� VIDEO BUS ��������, ����һ�� VIDEO BUS �豸
** �䡡��  : pcAdapterName       ����������
**           pcDeviceName        �豸����
** �䡡��  : videoBus �豸���ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PLW_VIDEOBUS_DEVICE  API_VideoBusDeviceCreate (CPCHAR   pcAdapterName,
                                               CPCHAR   pcDeviceName)
{
    REGISTER PLW_VIDEOBUS_ADAPTER    pvideoBusAdapter = __videoBusAdapterFind(pcAdapterName);
    REGISTER PLW_VIDEOBUS_DEVICE     pvideoBusDevice;
    
    if (pvideoBusAdapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (LW_NULL);
    }
    
    if (_Object_Name_Invalid(pcDeviceName)) {                           /*  ���������Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_NULL);
    }
    
    pvideoBusDevice = (PLW_VIDEOBUS_DEVICE)__SHEAP_ALLOC(sizeof(LW_VIDEOBUS_DEVICE));
    if (pvideoBusDevice == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    
    pvideoBusDevice->VIDEOBUSDEV_pvideoBusAdapter = pvideoBusAdapter;
    pvideoBusDevice->VIDEOBUSDEV_atomicUsageCnt.counter = 0;            /*  Ŀǰ��û�б�ʹ�ù�          */
    lib_strcpy(pvideoBusDevice->VIDEOBUSDEV_cName, pcDeviceName);
    
    __VIDEOBUS_LIST_LOCK();                                             /*  ���� VIDEO BUS ����         */
    _List_Line_Add_Ahead(&pvideoBusDevice->VIDEOBUSDEV_lineManage,
                         &pvideoBusAdapter->VIDEOBUSADAPTER_plineDevHeader);/*  �����������豸����      */
    __VIDEOBUS_LIST_UNLOCK();                                           /*  ���� VIDEO BUS ����         */
    
    LW_BUS_INC_DEV_COUNT(&pvideoBusAdapter->VIDEOBUSADAPTER_pbusAdapter);   /*  �����豸++              */
    
    return  (pvideoBusDevice);
}
/*********************************************************************************************************
** ��������: API_VideoBusDeviceDelete
** ��������: ɾ��ָ���� VIDEO BUS �豸
** �䡡��  : pvideoBusDevice       ָ���� VIDEO BUS �豸���ƿ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_VideoBusDeviceDelete (PLW_VIDEOBUS_DEVICE  pvideoBusDevice)
{
    REGISTER PLW_VIDEOBUS_ADAPTER       pvideoBusAdapter;

    if (pvideoBusDevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pvideoBusAdapter = pvideoBusDevice->VIDEOBUSDEV_pvideoBusAdapter;
    if (pvideoBusAdapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (PX_ERROR);
    }
    
    if (API_AtomicGet(&pvideoBusDevice->VIDEOBUSDEV_atomicUsageCnt)) {
        _ErrorHandle(EBUSY);
        return  (PX_ERROR);
    }
    
    __VIDEOBUS_LIST_LOCK();                                             /*  ���� VIDEO BUS ����         */
    _List_Line_Del(&pvideoBusDevice->VIDEOBUSDEV_lineManage,
                   &pvideoBusAdapter->VIDEOBUSADAPTER_plineDevHeader);  /*  �����������豸����          */
    __VIDEOBUS_LIST_UNLOCK();                                           /*  ���� VIDEO BUS ����         */
    
    LW_BUS_DEC_DEV_COUNT(&pvideoBusAdapter->VIDEOBUSADAPTER_pbusAdapter);   /*  �����豸--              */
    
    __SHEAP_FREE(pvideoBusDevice);                                      /*  �ͷ��ڴ�                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_VideoBusDeviceUsageInc
** ��������: ��ָ�� VIDEO BUS �豸ʹ�ü���++
** �䡡��  : pvideoBusDevice       ָ���� VIDEO BUS �豸���ƿ�
** �䡡��  : ��ǰ��ʹ�ü���ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_VideoBusDeviceUsageInc (PLW_VIDEOBUS_DEVICE    pvideoBusDevice)
{
    if (pvideoBusDevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (API_AtomicInc(&pvideoBusDevice->VIDEOBUSDEV_atomicUsageCnt));
}
/*********************************************************************************************************
** ��������: API_VideoBusDeviceUsageDec
** ��������: ��ָ�� VIDEO BUS �豸ʹ�ü���--
** �䡡��  : pvideoBusDevice       ָ���� VIDEO BUS �豸���ƿ�
** �䡡��  : ��ǰ��ʹ�ü���ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_VideoBusDeviceUsageDec (PLW_VIDEOBUS_DEVICE    pvideoBusDevice)
{
    if (pvideoBusDevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (API_AtomicDec(&pvideoBusDevice->VIDEOBUSDEV_atomicUsageCnt));
}
/*********************************************************************************************************
** ��������: API_VideoBusDeviceUsageGet
** ��������: ���ָ�� VIDEO BUS �豸ʹ�ü���
** �䡡��  : pvideoBusDevice       ָ���� VIDEO BUS �豸���ƿ�
** �䡡��  : ��ǰ��ʹ�ü���ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_VideoBusDeviceUsageGet (PLW_VIDEOBUS_DEVICE    pvideoBusDevice)
{
    if (pvideoBusDevice == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (API_AtomicGet(&pvideoBusDevice->VIDEOBUSDEV_atomicUsageCnt));
}
/*********************************************************************************************************
** ��������: API_VideoBusDeviceBusRequest
** ��������: ���ָ�� VIDEO BUS �豸������ʹ��Ȩ
** �䡡��  : pvideoBusDevice       ָ���� VIDEO BUS �豸���ƿ�
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_VideoBusDeviceBusRequest (PLW_VIDEOBUS_DEVICE   pvideoBusDevice)
{
    REGISTER PLW_VIDEOBUS_ADAPTER        pvideoBusAdapter;

    if (!pvideoBusDevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pvideoBusAdapter = pvideoBusDevice->VIDEOBUSDEV_pvideoBusAdapter;
    if (pvideoBusAdapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (PX_ERROR);
    }
    
    if (API_SemaphoreMPend(pvideoBusAdapter->VIDEOBUSADAPTER_hBusLock,
                           LW_OPTION_WAIT_INFINITE) != ERROR_NONE) {    /*  ��������                    */
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_VideoBusDeviceBusRelease
** ��������: �ͷ�ָ�� VIDEO BUS �豸������ʹ��Ȩ
** �䡡��  : pvideoBusDevice        ָ���� VIDEO BUS �豸���ƿ�
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_VideoBusDeviceBusRelease (PLW_VIDEOBUS_DEVICE   pvideoBusDevice)
{
    REGISTER PLW_VIDEOBUS_ADAPTER        pvideoBusAdapter;

    if (!pvideoBusDevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pvideoBusAdapter = pvideoBusDevice->VIDEOBUSDEV_pvideoBusAdapter;
    if (pvideoBusAdapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (PX_ERROR);
    }
    
    if (API_SemaphoreMPost(pvideoBusAdapter->VIDEOBUSADAPTER_hBusLock) !=
        ERROR_NONE) {                                                   /*  ��������                    */
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_VideoBusDeviceTx
** ��������: ʹ��ָ�� VIDEO BUS �豸����һ�δ���
** �䡡��  : pvideoBusDevice       ָ���� VIDEO BUS �豸���ƿ�
**           pvideoBusmsg          ������Ϣ���ƿ���
**           iNum                  ������Ϣ������Ϣ������
** �䡡��  : ������ VIDEO MSG ����, ���󷵻� -1
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_VideoBusDeviceTx (PLW_VIDEOBUS_DEVICE   pvideoBusDevice,
                           PLW_VIDEOBUS_MESSAGE  pvideoBusmsg,
                           INT                   iNum)
{
    REGISTER PLW_VIDEOBUS_ADAPTER   pvideoBusAdapter;
             INT                    iRet;

    if (!pvideoBusDevice || !pvideoBusmsg || (iNum < 1)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pvideoBusAdapter = pvideoBusDevice->VIDEOBUSDEV_pvideoBusAdapter;
    if (pvideoBusAdapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (PX_ERROR);
    }
    
    if (pvideoBusAdapter->VIDEOBUSADAPTER_pvideoBusfunc->VIDEOBUSFUNC_pfuncMasterTx) {
        if (API_SemaphoreMPend(pvideoBusAdapter->VIDEOBUSADAPTER_hBusLock,
                               LW_OPTION_WAIT_INFINITE) != ERROR_NONE) {/*  ��������                    */
            return  (PX_ERROR);
        }
        iRet = pvideoBusAdapter->VIDEOBUSADAPTER_pvideoBusfunc->VIDEOBUSFUNC_pfuncMasterTx(
               pvideoBusAdapter, pvideoBusmsg, iNum);
        API_SemaphoreMPost(pvideoBusAdapter->VIDEOBUSADAPTER_hBusLock); /*  �ͷ�����                    */
    } else {
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_VideoBusDeviceRx
** ��������: ʹ��ָ�� VIDEO BUS �豸����һ�δ���
** �䡡��  : pvideoBusDevice       ָ���� VIDEO BUS �豸���ƿ�
**           pvideoBusmsg          ������Ϣ���ƿ���
**           iNum                  ������Ϣ������Ϣ������
** �䡡��  : ������ VIDEO MSG ����, ���󷵻� -1
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_VideoBusDeviceRx (PLW_VIDEOBUS_DEVICE   pvideoBusDevice,
                                 PLW_VIDEOBUS_MESSAGE  pvideoBusmsg,
                                 INT                   iNum)
{
    REGISTER PLW_VIDEOBUS_ADAPTER   pvideoBusAdapter;
             INT                    iRet;

    if (!pvideoBusDevice || !pvideoBusmsg || (iNum < 1)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pvideoBusAdapter = pvideoBusDevice->VIDEOBUSDEV_pvideoBusAdapter;
    if (pvideoBusAdapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (PX_ERROR);
    }

    if (pvideoBusAdapter->VIDEOBUSADAPTER_pvideoBusfunc->VIDEOBUSFUNC_pfuncMasterRx) {
        if (API_SemaphoreMPend(pvideoBusAdapter->VIDEOBUSADAPTER_hBusLock,
                               LW_OPTION_WAIT_INFINITE) != ERROR_NONE) {/*  ��������                    */
            return  (PX_ERROR);
        }
        iRet = pvideoBusAdapter->VIDEOBUSADAPTER_pvideoBusfunc->VIDEOBUSFUNC_pfuncMasterRx(
               pvideoBusAdapter, pvideoBusmsg, iNum);
        API_SemaphoreMPost(pvideoBusAdapter->VIDEOBUSADAPTER_hBusLock); /*  �ͷ�����                    */
    } else {
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
    }

    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_VideoBusDeviceCtrl
** ��������: ָ�� VIDEO BUS �豸����ָ������
** �䡡��  : pvideoBusDevice   ָ���� VIDEO BUS �豸���ƿ�
**           iCmd              ������
**           lArg              �������
** �䡡��  : ����ִ�н��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_VideoBusDeviceCtrl (PLW_VIDEOBUS_DEVICE   pvideoBusDevice, INT  iCmd, LONG  lArg)
{
    REGISTER PLW_VIDEOBUS_ADAPTER   pvideoBusAdapter;

    if (!pvideoBusDevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pvideoBusAdapter = pvideoBusDevice->VIDEOBUSDEV_pvideoBusAdapter;
    if (pvideoBusAdapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  δ�ҵ�������                */
        return  (PX_ERROR);
    }
    
    if (pvideoBusAdapter->VIDEOBUSADAPTER_pvideoBusfunc->VIDEOBUSFUNC_pfuncMasterCtrl) {
        return  (pvideoBusAdapter->VIDEOBUSADAPTER_pvideoBusfunc->VIDEOBUSFUNC_pfuncMasterCtrl(
                 pvideoBusAdapter, iCmd, lArg));
    } else {
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
