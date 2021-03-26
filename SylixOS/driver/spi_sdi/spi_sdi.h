/*********************************************************************************************************
**
**                                    �й�������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: spi_sdi.c
**
** ��   ��   ��: Zeng.Bo(����)
**
** �ļ���������: 2016 �� 01 �� 20 ��
**
** ��        ��: ʹ��SPI���ߵ�SD����׼����Դ�ļ�
**               SylixOS��SD�����, һ����������һ��SD��һһ��Ӧ.
                 ���, ��һ��SPI�Ϲҽ���n��SD�豸, �������n��SDM�������HOST.
** BUG:
*********************************************************************************************************/
#ifndef __SPI_SDI_H
#define __SPI_SDI_H
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define SPI_SDI_PIN_NONE        ((UINT)(~0))                            /* ��ʾ��Ч���ű��             */
/*********************************************************************************************************
  ƽ̨ʵ����ض���
*********************************************************************************************************/
typedef struct __spi_platform_ops {
    VOID  (*SPIPO_pfuncChipSelect) (UINT  uiMasterID, UINT uiDeviceID, BOOL bEnable);
} SPI_PLATFORM_OPS;
/*********************************************************************************************************
  SPI�ӿ����͵�SD����������ע�� API
*********************************************************************************************************/
INT   spisdiLibInit(SPI_PLATFORM_OPS *pspiplatformops);

INT   spisdiDrvInstall(UINT   uiMasterID,
                       UINT   uiDeviceID,
                       UINT   uiCdPin,
                       UINT   uiWpPin);

#endif                                                                  /* __SPI_SDI_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/