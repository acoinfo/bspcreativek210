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
** 文   件   名: sccbBusLib.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 11 月 19 日
**
** 描        述: sccb  总线和设备操作库.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "sccbBus.h"
#include "sccbBusDev.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_OBJECT_HANDLE             _G_hVideoBusListLock = LW_OBJECT_HANDLE_INVALID;
/*********************************************************************************************************
  宏操作
*********************************************************************************************************/
#define __VIDEOBUS_LIST_LOCK()      API_SemaphoreBPend(_G_hVideoBusListLock, LW_OPTION_WAIT_INFINITE)
#define __VIDEOBUS_LIST_UNLOCK()    API_SemaphoreBPost(_G_hVideoBusListLock)
/*********************************************************************************************************
** 函数名称: __videoBusAdapterFind
** 功能描述: 查询 VIDEO BUS 适配器
** 输　入  : pcName        适配器名称
** 输　出  : 适配器指针
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PLW_VIDEOBUS_ADAPTER  __videoBusAdapterFind (CPCHAR  pcName)
{
    REGISTER PLW_BUS_ADAPTER        pbusAdapter;
    
    pbusAdapter = __busAdapterGet(pcName);                              /*  查找总线层适配器            */

    return  ((PLW_VIDEOBUS_ADAPTER)pbusAdapter);                        /*  总线结构为适配器首元素      */
}
/*********************************************************************************************************
** 函数名称: API_VideoBusLibInit
** 功能描述: 初始化 VIDEO BUS 组件库
** 输　入  : NONE
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
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
** 函数名称: API_VideoBusAdapterCreate
** 功能描述: 创建一个 VIDEO BUS 适配器
** 输　入  : pcName            适配器名称
**           pvideoBusfunc     操作函数组
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
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
    
    if (_Object_Name_Invalid(pcName)) {                                 /*  检查名字有效性              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (PX_ERROR);
    }
    
    /*
     *  创建控制块
     */
    pvideoBusAdapter = (PLW_VIDEOBUS_ADAPTER)__SHEAP_ALLOC(sizeof(LW_VIDEOBUS_ADAPTER));
    if (pvideoBusAdapter == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    pvideoBusAdapter->VIDEOBUSADAPTER_pvideoBusfunc  = pvideoBusfunc;
    pvideoBusAdapter->VIDEOBUSADAPTER_plineDevHeader = LW_NULL;         /*  目前还不存在 VIDEO BUS 设备 */
    
    /*
     *  创建相关锁
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
     *  加入总线层
     */
    if (__busAdapterCreate(&pvideoBusAdapter->VIDEOBUSADAPTER_pbusAdapter, pcName) != ERROR_NONE) {
        API_SemaphoreMDelete(&pvideoBusAdapter->VIDEOBUSADAPTER_hBusLock);
        __SHEAP_FREE(pvideoBusAdapter);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_VideoBusAdapterDelete
** 功能描述: 移除一个 VIDEO BUS 适配器
** 输　入  : pcName        适配器名称
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
** 注  意  : 在内核中的驱动必须确保永不再使用这个适配器了, 才能删除这个适配器.
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_VideoBusAdapterDelete (CPCHAR  pcName)
{
    REGISTER PLW_VIDEOBUS_ADAPTER       pvideoBusAdapter = __videoBusAdapterFind(pcName);
    
    if (pvideoBusAdapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (PX_ERROR);
    }
    
    /*
     *  程序从查找设备, 到运行到这里, 并没有任何保护!
     */
    if (API_SemaphoreMPend(pvideoBusAdapter->VIDEOBUSADAPTER_hBusLock,
                           LW_OPTION_WAIT_INFINITE) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    if (pvideoBusAdapter->VIDEOBUSADAPTER_plineDevHeader) {             /*  检查是否有设备链接到适配器  */
        API_SemaphoreMPost(pvideoBusAdapter->VIDEOBUSADAPTER_hBusLock);
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }
    
    if (__busAdapterDelete(pcName) != ERROR_NONE) {                     /*  从总线层移除                */
        API_SemaphoreMPost(pvideoBusAdapter->VIDEOBUSADAPTER_hBusLock);
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }
    
    API_SemaphoreMDelete(&pvideoBusAdapter->VIDEOBUSADAPTER_hBusLock);  /*  删除总线锁信号量            */
    __SHEAP_FREE(pvideoBusAdapter);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_VideoBusAdapterGet
** 功能描述: 通过名字获取一个 VIDEO BUS 适配器
** 输　入  : pcName        适配器名称
** 输　出  : videoBus 适配器控制块
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PLW_VIDEOBUS_ADAPTER  API_VideoBusAdapterGet (CPCHAR  pcName)
{
    REGISTER PLW_VIDEOBUS_ADAPTER       pvideoBusAdapter = __videoBusAdapterFind(pcName);
    
    if (pvideoBusAdapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (LW_NULL);
    
    } else {
        return  (pvideoBusAdapter);
    }
}
/*********************************************************************************************************
** 函数名称: API_VideoBusDeviceCreate
** 功能描述: 在指定 VIDEO BUS 适配器上, 创建一个 VIDEO BUS 设备
** 输　入  : pcAdapterName       适配器名称
**           pcDeviceName        设备名称
** 输　出  : videoBus 设备控制块
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PLW_VIDEOBUS_DEVICE  API_VideoBusDeviceCreate (CPCHAR   pcAdapterName,
                                               CPCHAR   pcDeviceName)
{
    REGISTER PLW_VIDEOBUS_ADAPTER    pvideoBusAdapter = __videoBusAdapterFind(pcAdapterName);
    REGISTER PLW_VIDEOBUS_DEVICE     pvideoBusDevice;
    
    if (pvideoBusAdapter == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (LW_NULL);
    }
    
    if (_Object_Name_Invalid(pcDeviceName)) {                           /*  检查名字有效性              */
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
    pvideoBusDevice->VIDEOBUSDEV_atomicUsageCnt.counter = 0;            /*  目前还没有被使用过          */
    lib_strcpy(pvideoBusDevice->VIDEOBUSDEV_cName, pcDeviceName);
    
    __VIDEOBUS_LIST_LOCK();                                             /*  锁定 VIDEO BUS 链表         */
    _List_Line_Add_Ahead(&pvideoBusDevice->VIDEOBUSDEV_lineManage,
                         &pvideoBusAdapter->VIDEOBUSADAPTER_plineDevHeader);/*  链入适配器设备链表      */
    __VIDEOBUS_LIST_UNLOCK();                                           /*  解锁 VIDEO BUS 链表         */
    
    LW_BUS_INC_DEV_COUNT(&pvideoBusAdapter->VIDEOBUSADAPTER_pbusAdapter);   /*  总线设备++              */
    
    return  (pvideoBusDevice);
}
/*********************************************************************************************************
** 函数名称: API_VideoBusDeviceDelete
** 功能描述: 删除指定的 VIDEO BUS 设备
** 输　入  : pvideoBusDevice       指定的 VIDEO BUS 设备控制块
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
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
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (PX_ERROR);
    }
    
    if (API_AtomicGet(&pvideoBusDevice->VIDEOBUSDEV_atomicUsageCnt)) {
        _ErrorHandle(EBUSY);
        return  (PX_ERROR);
    }
    
    __VIDEOBUS_LIST_LOCK();                                             /*  锁定 VIDEO BUS 链表         */
    _List_Line_Del(&pvideoBusDevice->VIDEOBUSDEV_lineManage,
                   &pvideoBusAdapter->VIDEOBUSADAPTER_plineDevHeader);  /*  链入适配器设备链表          */
    __VIDEOBUS_LIST_UNLOCK();                                           /*  解锁 VIDEO BUS 链表         */
    
    LW_BUS_DEC_DEV_COUNT(&pvideoBusAdapter->VIDEOBUSADAPTER_pbusAdapter);   /*  总线设备--              */
    
    __SHEAP_FREE(pvideoBusDevice);                                      /*  释放内存                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_VideoBusDeviceUsageInc
** 功能描述: 将指定 VIDEO BUS 设备使用计数++
** 输　入  : pvideoBusDevice       指定的 VIDEO BUS 设备控制块
** 输　出  : 当前的使用计数值
** 全局变量: 
** 调用模块: 
                                           API 函数
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
** 函数名称: API_VideoBusDeviceUsageDec
** 功能描述: 将指定 VIDEO BUS 设备使用计数--
** 输　入  : pvideoBusDevice       指定的 VIDEO BUS 设备控制块
** 输　出  : 当前的使用计数值
** 全局变量: 
** 调用模块: 
                                           API 函数
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
** 函数名称: API_VideoBusDeviceUsageGet
** 功能描述: 获得指定 VIDEO BUS 设备使用计数
** 输　入  : pvideoBusDevice       指定的 VIDEO BUS 设备控制块
** 输　出  : 当前的使用计数值
** 全局变量: 
** 调用模块: 
                                           API 函数
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
** 函数名称: API_VideoBusDeviceBusRequest
** 功能描述: 获得指定 VIDEO BUS 设备的总线使用权
** 输　入  : pvideoBusDevice       指定的 VIDEO BUS 设备控制块
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
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
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (PX_ERROR);
    }
    
    if (API_SemaphoreMPend(pvideoBusAdapter->VIDEOBUSADAPTER_hBusLock,
                           LW_OPTION_WAIT_INFINITE) != ERROR_NONE) {    /*  锁定总线                    */
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_VideoBusDeviceBusRelease
** 功能描述: 释放指定 VIDEO BUS 设备的总线使用权
** 输　入  : pvideoBusDevice        指定的 VIDEO BUS 设备控制块
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
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
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (PX_ERROR);
    }
    
    if (API_SemaphoreMPost(pvideoBusAdapter->VIDEOBUSADAPTER_hBusLock) !=
        ERROR_NONE) {                                                   /*  锁定总线                    */
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_VideoBusDeviceTx
** 功能描述: 使用指定 VIDEO BUS 设备进行一次传输
** 输　入  : pvideoBusDevice       指定的 VIDEO BUS 设备控制块
**           pvideoBusmsg          传输消息控制块组
**           iNum                  控制消息组中消息的数量
** 输　出  : 操作的 VIDEO MSG 数量, 错误返回 -1
** 全局变量: 
** 调用模块: 
                                           API 函数
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
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (PX_ERROR);
    }
    
    if (pvideoBusAdapter->VIDEOBUSADAPTER_pvideoBusfunc->VIDEOBUSFUNC_pfuncMasterTx) {
        if (API_SemaphoreMPend(pvideoBusAdapter->VIDEOBUSADAPTER_hBusLock,
                               LW_OPTION_WAIT_INFINITE) != ERROR_NONE) {/*  锁定总线                    */
            return  (PX_ERROR);
        }
        iRet = pvideoBusAdapter->VIDEOBUSADAPTER_pvideoBusfunc->VIDEOBUSFUNC_pfuncMasterTx(
               pvideoBusAdapter, pvideoBusmsg, iNum);
        API_SemaphoreMPost(pvideoBusAdapter->VIDEOBUSADAPTER_hBusLock); /*  释放总线                    */
    } else {
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_VideoBusDeviceRx
** 功能描述: 使用指定 VIDEO BUS 设备进行一次传输
** 输　入  : pvideoBusDevice       指定的 VIDEO BUS 设备控制块
**           pvideoBusmsg          传输消息控制块组
**           iNum                  控制消息组中消息的数量
** 输　出  : 操作的 VIDEO MSG 数量, 错误返回 -1
** 全局变量:
** 调用模块:
                                           API 函数
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
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
        return  (PX_ERROR);
    }

    if (pvideoBusAdapter->VIDEOBUSADAPTER_pvideoBusfunc->VIDEOBUSFUNC_pfuncMasterRx) {
        if (API_SemaphoreMPend(pvideoBusAdapter->VIDEOBUSADAPTER_hBusLock,
                               LW_OPTION_WAIT_INFINITE) != ERROR_NONE) {/*  锁定总线                    */
            return  (PX_ERROR);
        }
        iRet = pvideoBusAdapter->VIDEOBUSADAPTER_pvideoBusfunc->VIDEOBUSFUNC_pfuncMasterRx(
               pvideoBusAdapter, pvideoBusmsg, iNum);
        API_SemaphoreMPost(pvideoBusAdapter->VIDEOBUSADAPTER_hBusLock); /*  释放总线                    */
    } else {
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_VideoBusDeviceCtrl
** 功能描述: 指定 VIDEO BUS 设备处理指定命令
** 输　入  : pvideoBusDevice   指定的 VIDEO BUS 设备控制块
**           iCmd              命令编号
**           lArg              命令参数
** 输　出  : 命令执行结果
** 全局变量: 
** 调用模块: 
                                           API 函数
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
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);                         /*  未找到适配器                */
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
