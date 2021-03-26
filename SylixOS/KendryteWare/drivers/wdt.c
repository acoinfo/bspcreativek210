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

#include "wdt.h"
#include "stddef.h"
#include "plic.h"
#include "math.h"

volatile wdt_t *const wdt[2] =
{
    (volatile wdt_t *)WDT0_BASE_ADDR,
    (volatile wdt_t *)WDT1_BASE_ADDR
};

static void wdt_enable(wdt_device_number_t id)
{
    wdt[id]->crr = WDT_CRR_MASK;
    wdt[id]->cr |= WDT_CR_ENABLE;
}

static void wdt_disable(wdt_device_number_t id)
{
    wdt[id]->crr = WDT_CRR_MASK;
    wdt[id]->cr &= (~WDT_CR_ENABLE);
}

static void wdt_set_timeout(wdt_device_number_t id, uint8_t timeout)
{
    wdt[id]->torr = WDT_TORR_TOP(timeout);
}

static void wdt_response_mode(wdt_device_number_t id, uint8_t mode)
{
    wdt[id]->cr &= (~WDT_CR_RMOD_MASK);
    wdt[id]->cr |= mode;
}

static uint64_t wdt_get_pclk(wdt_device_number_t id)
{
    return id ? sysctl_clock_get_freq(SYSCTL_CLOCK_WDT1) : sysctl_clock_get_freq(SYSCTL_CLOCK_WDT0);
}

static uint8_t wdt_get_top(wdt_device_number_t id, uint64_t timeout_ms)
{
    uint64_t wdt_clk = wdt_get_pclk(id);
    uint64_t ret = (timeout_ms * wdt_clk / 1000) >> 16;
    if (ret)
        ret = (uint32_t)log2(ret);
    if (ret > 0xf)
        ret = 0xf;
    return (uint8_t)ret;
}

void wdt_feed(wdt_device_number_t id)
{
    wdt[id]->crr = WDT_CRR_MASK;
}

void wdt_clear_interrupt(wdt_device_number_t id)
{
    wdt[id]->eoi = wdt[id]->eoi;
}

void wdt_start(wdt_device_number_t id, uint64_t time_out_ms, plic_irq_callback_t on_irq)
{
    wdt_disable(id);
    wdt_clear_interrupt(id);

#if 0
    /* -------- fix irq setting --------*/
    plic_irq_register(id ? IRQN_WDT1_INTERRUPT : IRQN_WDT0_INTERRUPT, on_irq, NULL);
    plic_set_priority(id ? IRQN_WDT1_INTERRUPT : IRQN_WDT0_INTERRUPT, 1);
    plic_irq_enable(id ? IRQN_WDT1_INTERRUPT : IRQN_WDT0_INTERRUPT);
#endif

    sysctl_reset(id ? SYSCTL_RESET_WDT1 : SYSCTL_RESET_WDT0);
    sysctl_clock_set_threshold(id ? SYSCTL_THRESHOLD_WDT1 : SYSCTL_THRESHOLD_WDT0, 0);
    sysctl_clock_enable(id ? SYSCTL_CLOCK_WDT1 : SYSCTL_CLOCK_WDT0);

    wdt_response_mode(id, WDT_CR_RMOD_INTERRUPT);

    uint8_t m_top = wdt_get_top(id, time_out_ms);
    wdt_set_timeout(id, m_top);
    wdt_enable(id);
}

void wdt_stop(wdt_device_number_t id)
{
    wdt_disable(id);
}

/*********************************************************************************************************
  follow interface exported for rebooting
**********************************************************************************************************/

void k210_wdt_enable(wdt_device_number_t id)
{
    wdt_enable(id);
}

void k210_wdt_disable(wdt_device_number_t id)
{
    wdt_disable(id);
}

void k210_wdt_set_timeout(wdt_device_number_t id, uint8_t timeout)
{
    wdt_set_timeout(id, timeout);
}

void k210_wdt_response_mode(wdt_device_number_t id, uint8_t mode)
{
    wdt_response_mode(id, mode);
}

uint8_t k210_wdt_get_top(wdt_device_number_t id, uint64_t timeout_ms)
{
    return wdt_get_top(id, timeout_ms);
}