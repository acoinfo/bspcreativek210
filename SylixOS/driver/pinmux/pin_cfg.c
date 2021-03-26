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

#include "pin_cfg.h"
#include "fpioa.h"

#include "driver/common.h"

const fpioa_cfg_t __attribute__((weak)) g_fpioa_cfg =
{
    .version = PIN_CFG_VERSION,
    .functions_count = 0
};

const power_bank_cfg_t __attribute__((weak)) g_power_bank_cfg =
{
    .version = PIN_CFG_VERSION,
    .power_banks_count = 0
};

const pin_cfg_t __attribute__((weak)) g_pin_cfg =
{
    .version = PIN_CFG_VERSION,
    .set_spi0_dvp_data = 0
};

static void fpioa_setup()
{
    configASSERT(g_fpioa_cfg.version == PIN_CFG_VERSION);

    uint32_t i;
    for (i = 0; i < g_fpioa_cfg.functions_count; i++)
    {
        fpioa_cfg_item_t item = g_fpioa_cfg.functions[i];
        fpioa_set_function(item.number, item.function);
    }
}

static void power_bank_setup()
{
    configASSERT(g_power_bank_cfg.version == PIN_CFG_VERSION);

    uint32_t i;
    for (i = 0; i < g_power_bank_cfg.power_banks_count; i++)
    {
        power_bank_item_t item = g_power_bank_cfg.power_banks[i];
        sysctl_power_mode_sel(item.power_bank, item.io_power_mode);
    }
}

static void pin_setup()
{
    configASSERT(g_pin_cfg.version == PIN_CFG_VERSION);

    if (g_fpioa_cfg.version != PIN_CFG_VERSION) {
        _PrintFormat("[fpioa_setup]: invalid version detected!\r\n");
        return;
    }

    sysctl_spi0_dvp_data_set(g_pin_cfg.set_spi0_dvp_data);
}

/*********************************************************************************************************
 * 对以上裸机接口重新进行封装，建议使用以下接口
 ********************************************************************************************************/

INT  API_FpioaSetup (fpioa_cfg_t  *pFpioaPinCfg)
{
    uint32_t  i;

    if (!pFpioaPinCfg) {
        _PrintFormat("[API_FpioaSetup]: invalid parameter detected!\r\n");
        return  (PX_ERROR);
    }

    if (pFpioaPinCfg->version != PIN_CFG_VERSION) {
        _PrintFormat("[API_FpioaSetup]: invalid version detected!\r\n");
        return  (PX_ERROR);
    }

    for (i = 0; i < pFpioaPinCfg->functions_count; i++) {
        fpioa_cfg_item_t item = pFpioaPinCfg->functions[i];
        fpioa_set_function(item.number, item.function);
    }

    return  (ERROR_NONE);
}

INT  API_FpioaPowerBankSetup (power_bank_cfg_t  *pFpioaPowerBankCfg)
{
    uint32_t  i;

    if (!pFpioaPowerBankCfg) {
        _PrintFormat("[API_FpioaPowerBankSetup]: invalid parameter detected!\r\n");
        return  (PX_ERROR);
    }

    if (pFpioaPowerBankCfg->version != PIN_CFG_VERSION) {
        _PrintFormat("[API_FpioaPowerBankSetup]: invalid version detected!\r\n");
        return  (PX_ERROR);
    }

    for (i = 0; i < pFpioaPowerBankCfg->power_banks_count; i++) {
        power_bank_item_t item = pFpioaPowerBankCfg->power_banks[i];
        sysctl_power_mode_sel(item.power_bank, item.io_power_mode);
    }

    return  (ERROR_NONE);
}

INT  API_FpioaDvpPinSetup (pin_cfg_t  *pFpioaPinCfg)
{
    if (!pFpioaPinCfg) {
        _PrintFormat("[API_FpioaDvpPinSetup]: invalid parameter detected!\r\n");
        return  (PX_ERROR);
    }

    if (pFpioaPinCfg->version != PIN_CFG_VERSION) {
        _PrintFormat("[API_FpioaDvpPinSetup]: invalid version detected!\r\n");
        return  (PX_ERROR);
    }

    sysctl_spi0_dvp_data_set(pFpioaPinCfg->set_spi0_dvp_data);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
