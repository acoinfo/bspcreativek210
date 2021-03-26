/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "SylixOS.h"
#include "config.h"
#include "driver/clock/k210_clock.h"

#include <stdint.h>
#include <stdio.h>
#include "uarths.h"

volatile uarths_t *const uarths = (volatile uarths_t *)UARTHS_BASE_ADDR;

static inline int uarths_putc(char c)
{
    while (uarths->txdata.full)
        continue;
    uarths->txdata.data = (uint8_t)c;

    return 0;
}

int uarths_getc(void)
{
    /* while not empty */
    uarths_rxdata_t recv = uarths->rxdata;

    if (recv.empty)
        return EOF;
    else
        return recv.data;
}

int uarths_putchar(char c)
{
    return uarths_putc(c);
}

int uarths_puts(const char *s)
{
    while (*s)
        if (uarths_putc(*s++) != 0)
            return -1;
    return 0;
}

void uarths_init(void)
{
    uint32_t freq = sysctl_clock_get_freq(SYSCTL_CLOCK_CPU);
    uint16_t div = freq / 115200 - 1;

    /* Set UART registers */
    uarths->div.div = div;
    uarths->txctrl.txen = 1;
    uarths->rxctrl.rxen = 1;
    uarths->txctrl.txcnt = 0;
    uarths->rxctrl.rxcnt = 0;
    uarths->ip.txwm = 1;
    uarths->ip.rxwm = 1;
    uarths->ie.txwm = 0;
    uarths->ie.rxwm = 1;
}
