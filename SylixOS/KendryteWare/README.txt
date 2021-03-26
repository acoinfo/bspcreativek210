
/*********************************************************************************************************
 * KendryteWare下的源文件是从官方的 standalone-sdk-v0.2 的 driver 目录移植过来的，源码下载地址:
 * https://github.com/kendryte/kendryte-standalone-sdk. SylxiOS中的驱动仅使用部分功能, 并且对
 * 源码进行了如下修改；这里还是保留其全部的代码文件, 方便查看。
 ********************************************************************************************************/

MODIFY_1: 
# 向 bspKendryteK210 工程中添加 逻辑驱动源码包
# 配置 bspKendryteK210，添加头文件路径 /KendryteWare/include
# 改 sysctl.c 为 freertos-sdk 的

MODIFY_2: 
# 添加逻辑库，将裸机库的include的文件夹加入到工程的头文件路径，向裸机库的源码文件中添加 
# #define  __SYLIXOS_KERNEL
# #define  __SYLIXOS_STDIO
# #include "SylixOS.h"
# #include "config.h"
# 删除有黄线标记的头文件(找到不到的头文件): 主要有#include "encoding.h" #include "syscalls.h" #include "Platform.h" #include "atomic.h"
# 删除裸机库中的 sysctl.c 和 sysctl.h 文件 (这里使用从freertos-sdk中移植过来的 sysctl.c 和 sysctl.h)
# 将裸机库中使用的 sysctl.h 头文件替换成 #include "driver/clock/k210_clock.h"
# 将裸机库中使用的 utils 和 clint 
# 添加 common.h 
# 删除 uart 使用我们自己的 uart 驱动
# 删除 i2c 使用我们自己的 i2c 驱动 

