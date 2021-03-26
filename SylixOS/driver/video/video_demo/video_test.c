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
** ��   ��   ��: k210_lcd_test.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 10 �� 29 ��
**
** ��        ��: K210 ������ LCD �������Գ���
*********************************************************************************************************/
#include "pthread.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "mman.h"
#include "video.h"

#include "driver/clock/k210_clock.h"                                    /* �����ڲ���ʱ,�ṩLCD����ӿ� */
#include "driver/lcd/lcd.h"
#include "driver/lcd/nt35310.h"
#include "KendryteWare/include/fpioa.h"
/*********************************************************************************************************
** ��������: prepare_test_lcd
** ��������: ׼������ͼ��֡��ʾ���Ե�LCD
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
int prepare_test_lcd()
{
    INT  iFbFd;

    sysctl_spi0_dvp_data_set(1);
    sysctl_power_mode_sel(SYSCTL_POWER_BANK1, POWER_V18);

    iFbFd = open("/dev/fb0", O_RDWR, 0666);
    if (iFbFd < 0) {
        printf("failed to open /dev/fb0\n");
        return  (-1);
    }

    lcd_set_direction(DIR_YX_LRUD);
    lcd_clear(BLACK);

    return  (iFbFd);
}
/*********************************************************************************************************
** ��������: pthread_test
** ��������: Video Framework ���� pthread �߳�
** �䡡��  : pvArg         �̲߳���
** �䡡��  : �̷߳���ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  *pthread_test(VOID  *pvArg)
{
  int  fd;
  INT  iFbFd;
  int  i, j;
  void *pcapmem;
  void *imagemem;
//  int  frameCount = 0;

  video_dev_desc      dev;
  video_channel_desc  chan_desc;
  video_format_desc   format;

  video_channel_ctl   channel;
  video_buf_cal       cal;
  video_buf_ctl       buf;
  video_cap_ctl       cap;

  video_cap_stat      stat;

  fd = open("/dev/video0", O_RDWR);                                     /* �����Ӳ����������ʼ��       */

  iFbFd = prepare_test_lcd();                                           /* ׼������ͼ��֡��ʾ���Ե�LCD  */

  ioctl(fd, VIDIOC_DEVDESC, &dev);                                      /* ��ø��豸��ͨ������         */
//  printf("DriverName: %s\n", dev.driver);
//  printf("CardName: %s\n", dev.card);
//  printf("BusName: %s\n", dev.bus);
//  printf("Channels: %d\n", dev.channels);
//  printf("\n");

  for (i = 0; i < dev.channels; i++) {
      chan_desc.channel = i;
      ioctl(fd, VIDIOC_CHANDESC, &chan_desc);                           /* ���ָ��ͨ��֧�ֵ�֡��ʽ���� */
//      printf("Channel: %d\n", chan_desc.channel);
//      printf("xsize_max: %d\n", chan_desc.xsize_max);
//      printf("ysize_max: %d\n", chan_desc.ysize_max);
//      printf("queue_max: %d\n", chan_desc.queue_max);
//      printf("formats: %d\n", chan_desc.formats);
//      printf("\n");

      for (j = 0; j < chan_desc.formats; j++) {
          format.channel = i;
          format.index = j;
          ioctl(fd, VIDIOC_FORMATDESC, &format);                        /* ���ָ��֡(index)��ʽ������  */
//          printf("format: %d\n", format.format);
//          printf("order: %d\n", format.order);
//          printf("description: %s\n", format.description);
//          printf("\n");
      }
  }

  channel.channel = 0;
  channel.xsize   = 320;
  channel.ysize   = 240;
  channel.x_off   = 0;
  channel.y_off   = 0;
  channel.queue   = 3;

  channel.source = 0;
  channel.format = VIDEO_PIXEL_FORMAT_RGB_565;
  channel.order  = VIDEO_LSB_CRCB;

  ioctl(fd, VIDIOC_SCHANCTL, &channel);                                 /* ����ָ��ͨ��֡��ʽ�ͻ����С */

  cal.channel = 0;
  ioctl(fd, VIDIOC_MAPCAL, &cal);                                       /* ��ȡͼ��֡�ڴ����ʹ�С��Ϣ */
  printf(">> show channel %d info.\n", cal.channel);
  printf("VideoBuf Size: %ld Byte\n", cal.size);
  printf("Frame Align: %ld Byte\n", cal.align);
  printf("Frame Size: %ld Byte\n", cal.size_per_fq);
  printf("\n");

  buf.channel = 0;
  buf.mem     = NULL;
  buf.size    = cal.size;
  buf.mtype   = VIDEO_MEMORY_AUTO;

  ioctl(fd, VIDIOC_MAPPREPAIR, &buf);                                   /* �����ڴ�(�������͵Ĵ���)���� */

  //pcapmem = mmap(NULL, buf.size, PROT_READ, MAP_SHARED, fd, 0);       /* �ڴ�ӳ��, ��ܸ���ʵʱ����   */

  //���ʹ�� read() ����, ��ÿ�ζ�ȡ�����ݶ��������Ч��һ֡����.       /* ��ܸ��𿽱������Ч֡���û� */

  cap.channel = 0;
  cap.on      = 1;
  cap.flags   = 0;
  ioctl(fd, VIDIOC_SCAPCTL, &cap);                                      /* ��ʼ�ɼ�ͼ��֡����           */

  pcapmem = buf.mem;
  if (pcapmem == LW_NULL) {
      printf("cmd: VIDIOC_MAPPREPAIR execute error.\n");
      while(1);
  }

  imagemem = pcapmem;
  while (1) {
        lcd_draw_picture(0, 0, 320, 240, imagemem);                     /* ʹ������ӿڻ����ɼ�����ͼ�� */
        //VIDIOC_GCAPCTL
        stat.channel = 0;
        ioctl(fd, VIDIOC_CAPSTAT, &stat);
        printf("qindex_vaild: %d\n", stat.qindex_vaild);
        printf("qindex_cur: %d\n", stat.qindex_cur);
        imagemem = pcapmem + (2 * 320 * 240) * stat.qindex_vaild;
  }

  cap.on = 0;
  ioctl(fd, VIDIOC_SCAPCTL, &cap);                                      /* ֹͣ�ɼ�ͼ��֡����           */

  //munmap(pcapmem, buf.size);                                          /* ȡ���ڴ�ӳ��                 */

  close(fd);                                                            /* ���ͷ����д򿪺����������Դ */
  close(iFbFd);

  while(1);

  return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: videoFrameWorkTestStart
** ��������: Video Framework ����
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  videoFrameWorkTestStart (VOID)
{
    pthread_t   tid;
    INT         iError;

    iError  = pthread_create(&tid, NULL, pthread_test, NULL);

    return  (iError == 0 ? ERROR_NONE : PX_ERROR);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
