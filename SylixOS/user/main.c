#include "unistd.h"
#include "sys/utsname.h"
#include <stdio.h>

extern int  videoFrameWorkTestStart (void);

extern int  uartTestStart (void);

int  t_main (void)
{
    struct utsname  name;
    
    uname(&name);
    
    printf("sysname  : %s\n", name.sysname);
    printf("nodename : %s\n", name.nodename);
    printf("release  : %s\n", name.release);
    printf("version  : %s\n", name.version);
    printf("machine  : %s\n", name.machine);
    
    Lw_TShell_Create(STDOUT_FILENO, LW_OPTION_TSHELL_PROMPT_FULL | LW_OPTION_TSHELL_VT100);
    
    API_TShellKeywordAdd("test_video", (PCOMMAND_START_ROUTINE)videoFrameWorkTestStart);

    API_TShellKeywordAdd("test_uart", (PCOMMAND_START_ROUTINE)uartTestStart);

    return  (0);
}
