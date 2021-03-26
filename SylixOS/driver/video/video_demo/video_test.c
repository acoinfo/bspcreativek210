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
** 文   件   名: k210_lcd_test.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 10 月 29 日
**
** 描        述: K210 处理器 LCD 驱动测试程序
*********************************************************************************************************/
#include "pthread.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "mman.h"
#include "video.h"

#include "driver/clock/k210_clock.h"                                    /* 仅用于测试时,提供LCD裸机接口 */
#include "driver/lcd/lcd.h"
#include "driver/lcd/nt35310.h"
#include "KendryteWare/include/fpioa.h"
/*********************************************************************************************************
** 函数名称: prepare_test_lcd
** 功能描述: 准备用于图像帧显示测试的LCD
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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
** 函数名称: pthread_test
** 功能描述: Video Framework 测试 pthread 线程
** 输　入  : pvArg         线程参数
** 输　出  : 线程返回值
** 全局变量:
** 调用模块:
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

  fd = open("/dev/video0", O_RDWR);                                     /* 将完成硬件和驱动初始化       */

  iFbFd = prepare_test_lcd();                                           /* 准备用于图像帧显示测试的LCD  */

  ioctl(fd, VIDIOC_DEVDESC, &dev);                                      /* 获得该设备的通道个数         */
//  printf("DriverName: %s\n", dev.driver);
//  printf("CardName: %s\n", dev.card);
//  printf("BusName: %s\n", dev.bus);
//  printf("Channels: %d\n", dev.channels);
//  printf("\n");

  for (i = 0; i < dev.channels; i++) {
      chan_desc.channel = i;
      ioctl(fd, VIDIOC_CHANDESC, &chan_desc);                           /* 获得指定通道支持的帧格式个数 */
//      printf("Channel: %d\n", chan_desc.channel);
//      printf("xsize_max: %d\n", chan_desc.xsize_max);
//      printf("ysize_max: %d\n", chan_desc.ysize_max);
//      printf("queue_max: %d\n", chan_desc.queue_max);
//      printf("formats: %d\n", chan_desc.formats);
//      printf("\n");

      for (j = 0; j < chan_desc.formats; j++) {
          format.channel = i;
          format.index = j;
          ioctl(fd, VIDIOC_FORMATDESC, &format);                        /* 获得指定帧(index)格式的描述  */
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

  ioctl(fd, VIDIOC_SCHANCTL, &channel);                                 /* 设置指定通道帧格式和缓冲大小 */

  cal.channel = 0;
  ioctl(fd, VIDIOC_MAPCAL, &cal);                                       /* 获取图像帧内存对齐和大小信息 */
  printf(">> show channel %d info.\n", cal.channel);
  printf("VideoBuf Size: %ld Byte\n", cal.size);
  printf("Frame Align: %ld Byte\n", cal.align);
  printf("Frame Size: %ld Byte\n", cal.size_per_fq);
  printf("\n");

  buf.channel = 0;
  buf.mem     = NULL;
  buf.size    = cal.size;
  buf.mtype   = VIDEO_MEMORY_AUTO;

  ioctl(fd, VIDIOC_MAPPREPAIR, &buf);                                   /* 缓冲内存(各种类型的处理)分配 */

  //pcapmem = mmap(NULL, buf.size, PROT_READ, MAP_SHARED, fd, 0);       /* 内存映射, 框架负责实时更新   */

  //如果使用 read() 调用, 则每次读取的数据都是最近有效的一帧数据.       /* 框架负责拷贝最近有效帧到用户 */

  cap.channel = 0;
  cap.on      = 1;
  cap.flags   = 0;
  ioctl(fd, VIDIOC_SCAPCTL, &cap);                                      /* 开始采集图像帧数据           */

  pcapmem = buf.mem;
  if (pcapmem == LW_NULL) {
      printf("cmd: VIDIOC_MAPPREPAIR execute error.\n");
      while(1);
  }

  imagemem = pcapmem;
  while (1) {
        lcd_draw_picture(0, 0, 320, 240, imagemem);                     /* 使用裸机接口画出采集到的图像 */
        //VIDIOC_GCAPCTL
        stat.channel = 0;
        ioctl(fd, VIDIOC_CAPSTAT, &stat);
        printf("qindex_vaild: %d\n", stat.qindex_vaild);
        printf("qindex_cur: %d\n", stat.qindex_cur);
        imagemem = pcapmem + (2 * 320 * 240) * stat.qindex_vaild;
  }

  cap.on = 0;
  ioctl(fd, VIDIOC_SCAPCTL, &cap);                                      /* 停止采集图像帧数据           */

  //munmap(pcapmem, buf.size);                                          /* 取消内存映射                 */

  close(fd);                                                            /* 将释放所有打开后所分配的资源 */
  close(iFbFd);

  while(1);

  return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: videoFrameWorkTestStart
** 功能描述: Video Framework 测试
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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
