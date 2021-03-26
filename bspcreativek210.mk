#*********************************************************************************************************
#
#                                    中国软件开源组织
#
#                                   嵌入式实时操作系统
#
#                                SylixOS(TM)  LW : long wing
#
#                               Copyright All Rights Reserved
#
#--------------文件信息--------------------------------------------------------------------------------
#
# 文   件   名: bspk210.mk
#
# 创   建   人: RealEvo-IDE
#
# 文件创建日期: 2018 年 10 月 17 日
#
# 描        述: 本文件由 RealEvo-IDE 生成，用于配置 Makefile 功能，请勿手动修改
#*********************************************************************************************************

#*********************************************************************************************************
# Clear setting
#*********************************************************************************************************
include $(CLEAR_VARS_MK)

#*********************************************************************************************************
# Target
#*********************************************************************************************************
LOCAL_TARGET_NAME := bspcreativek210.elf

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS :=  \
SylixOS/bsp/startup.S \
SylixOS/KendryteWare/drivers/aes.c \
SylixOS/KendryteWare/drivers/common.c \
SylixOS/KendryteWare/drivers/dmac.c \
SylixOS/KendryteWare/drivers/dvp.c \
SylixOS/KendryteWare/drivers/fft.c \
SylixOS/KendryteWare/drivers/fpioa.c \
SylixOS/KendryteWare/drivers/gpio.c \
SylixOS/KendryteWare/drivers/gpiohs.c \
SylixOS/KendryteWare/drivers/i2c.c \
SylixOS/KendryteWare/drivers/i2s.c \
SylixOS/KendryteWare/drivers/io.c \
SylixOS/KendryteWare/drivers/plic.c \
SylixOS/KendryteWare/drivers/pwm.c \
SylixOS/KendryteWare/drivers/rtc.c \
SylixOS/KendryteWare/drivers/sha256.c \
SylixOS/KendryteWare/drivers/spi.c \
SylixOS/KendryteWare/drivers/sysclock.c \
SylixOS/KendryteWare/drivers/timer.c \
SylixOS/KendryteWare/drivers/uart.c \
SylixOS/KendryteWare/drivers/uarths.c \
SylixOS/KendryteWare/drivers/wdt.c \
SylixOS/bsp/bspInit.c \
SylixOS/bsp/bspLib.c \
SylixOS/driver/aes/k210_aes.c \
SylixOS/driver/clint/k210_clint.c \
SylixOS/driver/clock/clock.c \
SylixOS/driver/clock/k210_clock.c \
SylixOS/driver/dmac/k210_dma.c \
SylixOS/driver/fft/k210_fft.c \
SylixOS/driver/gpio/k210_gpio.c \
SylixOS/driver/gpiohs/k210_gpiohs.c \
SylixOS/driver/i2c/k210_i2c.c \
SylixOS/driver/i2s/k210_i2s.c \
SylixOS/driver/irqchip/plic_irqchip.c \
SylixOS/driver/lcd/k210_lcd.c \
SylixOS/driver/lcd/lcd.c \
SylixOS/driver/lcd/nt35310.c \
SylixOS/driver/netif/netif.c \
SylixOS/driver/pinmux/pin_cfg.c \
SylixOS/driver/pwm/k210_pwm.c \
SylixOS/driver/rtc/k210_rtc.c \
SylixOS/driver/sha256/k210_sha256.c \
SylixOS/driver/sio/sio.c \
SylixOS/driver/spi/k210_spi.c \
SylixOS/driver/spi_sdi/sdcard.c \
SylixOS/driver/spi_sdi/spi_sdi.c \
SylixOS/driver/spiflash/k210_spiflash.c \
SylixOS/driver/spiflash/w25qxx.c \
SylixOS/driver/uart/k210_uart.c \
SylixOS/driver/uart/k210_uart_test.c \
SylixOS/driver/video/sccbBus/k210_sccb/k210_sccb.c \
SylixOS/driver/video/sccbBus/sccbBusLib.c \
SylixOS/driver/video/video-core/videoAsync.c \
SylixOS/driver/video/video-core/videoBufCore.c \
SylixOS/driver/video/video-core/videoChannel.c \
SylixOS/driver/video/video-core/videoDevice.c \
SylixOS/driver/video/video-core/videoFileHandle.c \
SylixOS/driver/video/video-core/videoIoctl.c \
SylixOS/driver/video/video-core/videoQueueLib.c \
SylixOS/driver/video/video-core/videoSubDev.c \
SylixOS/driver/video/video_demo/video_test.c \
SylixOS/driver/video/video_driver/k210_dvp.c \
SylixOS/driver/video/video_driver/k210_sccb.c \
SylixOS/driver/watchdog/k210_watchdog.c \
SylixOS/user/main.c

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your header files search path")
#*********************************************************************************************************
LOCAL_INC_PATH :=  \
-I"./SylixOS" \
-I"./SylixOS/bsp" \
-I"./SylixOS/KendryteWare/include" \
-I"./SylixOS/liblvgl"

#*********************************************************************************************************
# Pre-defined macro (eg. -DYOUR_MARCO=1)
#*********************************************************************************************************
LOCAL_DSYMBOL :=  \
-D__BOOT_INROM=1 \
-D__SYLIXOS_LITE

#*********************************************************************************************************
# Compiler flags
#*********************************************************************************************************
LOCAL_CFLAGS := 
LOCAL_CXXFLAGS := 

#*********************************************************************************************************
# Depend library (eg. LOCAL_DEPEND_LIB := -la LOCAL_DEPEND_LIB_PATH := -L"Your library search path")
#*********************************************************************************************************
LOCAL_DEPEND_LIB := 
LOCAL_DEPEND_LIB_PATH := 

#*********************************************************************************************************
# Link script file
#*********************************************************************************************************
LOCAL_LD_SCRIPT := SylixOSBSP.ld

#*********************************************************************************************************
# C++ config
#*********************************************************************************************************
LOCAL_USE_CXX        := no
LOCAL_USE_CXX_EXCEPT := no

#*********************************************************************************************************
# Code coverage config
#*********************************************************************************************************
LOCAL_USE_GCOV := no

#*********************************************************************************************************
# OpenMP config
#*********************************************************************************************************
LOCAL_USE_OMP := no

#*********************************************************************************************************
# Lite-BSP use extension
#*********************************************************************************************************
LOCAL_USE_EXTENSION := no

#*********************************************************************************************************
# User link command
#*********************************************************************************************************
LOCAL_PRE_LINK_CMD   := 
LOCAL_POST_LINK_CMD  := 
LOCAL_PRE_STRIP_CMD  := 
LOCAL_POST_STRIP_CMD := 

include $(LITE_BSP_MK)

#*********************************************************************************************************
# End
#*********************************************************************************************************
