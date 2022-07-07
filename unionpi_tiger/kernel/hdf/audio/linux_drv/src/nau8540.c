/*
 * Copyright (c) 2022 Unionman Technology Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <linux/slab.h>
#include <linux/of_device.h>
#include "sound/soc.h"

#include "axg_common.h"
#include "nau8540.h"

#define NAU_FREF_MAX 13500000
#define NAU_FVCO_MAX 100000000
#define NAU_FVCO_MIN 90000000

/* the maximum frequency of CLK_ADC */
#define CLK_ADC_MAX 6144000

/* scaling for mclk from sysclk_src output */
static const struct nau8540_fll_attr mclk_src_scaling[] = {
    {1, 0x0},
    {2, 0x2},
    {4, 0x3},
    {8, 0x4},
    {16, 0x5},
    {32, 0x6},
    {3, 0x7},
    {6, 0xa},
    {12, 0xb},
    {24, 0xc},
};

/* ratio for input clk freq */
static const struct nau8540_fll_attr fll_ratio[] = {
    {512000, 0x01},
    {256000, 0x02},
    {128000, 0x04},
    {64000, 0x08},
    {32000, 0x10},
    {8000, 0x20},
    {4000, 0x40},
};

static const struct nau8540_fll_attr fll_pre_scalar[] = {
    {1, 0x0},
    {2, 0x1},
    {4, 0x2},
    {8, 0x3},
};

/* over sampling rate */
static const struct nau8540_osr_attr osr_adc_sel[] = {
    {32, 3},  /* OSR 32, SRC 1/8 */
    {64, 2},  /* OSR 64, SRC 1/4 */
    {128, 1}, /* OSR 128, SRC 1/2 */
    {256, 0}, /* OSR 256, SRC 1 */
};

static const struct reg_default nau8540_reg_defaults[] = {
    {NAU8540_REG_POWER_MANAGEMENT, 0x0000},
    {NAU8540_REG_CLOCK_CTRL, 0x0000},
    {NAU8540_REG_CLOCK_SRC, 0x0000},
    {NAU8540_REG_FLL1, 0x0001},
    {NAU8540_REG_FLL2, 0x3126},
    {NAU8540_REG_FLL3, 0x0008},
    {NAU8540_REG_FLL4, 0x0010},
    {NAU8540_REG_FLL5, 0xC000},
    {NAU8540_REG_FLL6, 0x6000},
    {NAU8540_REG_FLL_VCO_RSV, 0xF13C},
    {NAU8540_REG_PCM_CTRL0, 0x000B},
    {NAU8540_REG_PCM_CTRL1, 0x3010},
    {NAU8540_REG_PCM_CTRL2, 0x0800},
    {NAU8540_REG_PCM_CTRL3, 0x0000},
    {NAU8540_REG_PCM_CTRL4, 0x000F},
    {NAU8540_REG_ALC_CONTROL_1, 0x0000},
    {NAU8540_REG_ALC_CONTROL_2, 0x700B},
    {NAU8540_REG_ALC_CONTROL_3, 0x0022},
    {NAU8540_REG_ALC_CONTROL_4, 0x1010},
    {NAU8540_REG_ALC_CONTROL_5, 0x1010},
    {NAU8540_REG_NOTCH_FIL1_CH1, 0x0000},
    {NAU8540_REG_NOTCH_FIL2_CH1, 0x0000},
    {NAU8540_REG_NOTCH_FIL1_CH2, 0x0000},
    {NAU8540_REG_NOTCH_FIL2_CH2, 0x0000},
    {NAU8540_REG_NOTCH_FIL1_CH3, 0x0000},
    {NAU8540_REG_NOTCH_FIL2_CH3, 0x0000},
    {NAU8540_REG_NOTCH_FIL1_CH4, 0x0000},
    {NAU8540_REG_NOTCH_FIL2_CH4, 0x0000},
    {NAU8540_REG_HPF_FILTER_CH12, 0x0000},
    {NAU8540_REG_HPF_FILTER_CH34, 0x0000},
    {NAU8540_REG_ADC_SAMPLE_RATE, 0x0002},
    {NAU8540_REG_DIGITAL_GAIN_CH1, 0x0400},
    {NAU8540_REG_DIGITAL_GAIN_CH2, 0x0400},
    {NAU8540_REG_DIGITAL_GAIN_CH3, 0x0400},
    {NAU8540_REG_DIGITAL_GAIN_CH4, 0x0400},
    {NAU8540_REG_DIGITAL_MUX, 0x00E4},
    {NAU8540_REG_GPIO_CTRL, 0x0000},
    {NAU8540_REG_MISC_CTRL, 0x0000},
    {NAU8540_REG_I2C_CTRL, 0xEFFF},
    {NAU8540_REG_VMID_CTRL, 0x0000},
    {NAU8540_REG_MUTE, 0x0000},
    {NAU8540_REG_ANALOG_ADC1, 0x0011},
    {NAU8540_REG_ANALOG_ADC2, 0x0020},
    {NAU8540_REG_ANALOG_PWR, 0x0000},
    {NAU8540_REG_MIC_BIAS, 0x0004},
    {NAU8540_REG_REFERENCE, 0x0000},
    {NAU8540_REG_FEPGA1, 0x0000},
    {NAU8540_REG_FEPGA2, 0x0000},
    {NAU8540_REG_FEPGA3, 0x0101},
    {NAU8540_REG_FEPGA4, 0x0101},
    {NAU8540_REG_PWR, 0x0000},
};

static struct nau8540 *g_nau8540 = NULL;

static bool nau8540_readable_reg(struct device *dev, unsigned int reg)
{
    switch (reg) {
        case NAU8540_REG_POWER_MANAGEMENT ... NAU8540_REG_FLL_VCO_RSV:
        case NAU8540_REG_PCM_CTRL0 ... NAU8540_REG_PCM_CTRL4:
        case NAU8540_REG_ALC_CONTROL_1 ... NAU8540_REG_ALC_CONTROL_5:
        case NAU8540_REG_ALC_GAIN_CH12 ... NAU8540_REG_ADC_SAMPLE_RATE:
        case NAU8540_REG_DIGITAL_GAIN_CH1 ... NAU8540_REG_DIGITAL_MUX:
        case NAU8540_REG_P2P_CH1 ... NAU8540_REG_I2C_CTRL:
        case NAU8540_REG_I2C_DEVICE_ID:
        case NAU8540_REG_VMID_CTRL ... NAU8540_REG_MUTE:
        case NAU8540_REG_ANALOG_ADC1 ... NAU8540_REG_PWR:
            return true;
        default:
            return false;
    }
}

static bool nau8540_writeable_reg(struct device *dev, unsigned int reg)
{
    switch (reg) {
        case NAU8540_REG_SW_RESET ... NAU8540_REG_FLL_VCO_RSV:
        case NAU8540_REG_PCM_CTRL0 ... NAU8540_REG_PCM_CTRL4:
        case NAU8540_REG_ALC_CONTROL_1 ... NAU8540_REG_ALC_CONTROL_5:
        case NAU8540_REG_NOTCH_FIL1_CH1 ... NAU8540_REG_ADC_SAMPLE_RATE:
        case NAU8540_REG_DIGITAL_GAIN_CH1 ... NAU8540_REG_DIGITAL_MUX:
        case NAU8540_REG_GPIO_CTRL ... NAU8540_REG_I2C_CTRL:
        case NAU8540_REG_RST:
        case NAU8540_REG_VMID_CTRL ... NAU8540_REG_MUTE:
        case NAU8540_REG_ANALOG_ADC1 ... NAU8540_REG_PWR:
            return true;
        default:
            return false;
    }
}

static bool nau8540_volatile_reg(struct device *dev, unsigned int reg)
{
    switch (reg) {
        case NAU8540_REG_SW_RESET:
        case NAU8540_REG_ALC_GAIN_CH12 ... NAU8540_REG_ALC_STATUS:
        case NAU8540_REG_P2P_CH1 ... NAU8540_REG_PEAK_CH4:
        case NAU8540_REG_I2C_DEVICE_ID:
        case NAU8540_REG_RST:
            return true;
        default:
            return false;
    }
}

static void nau8540_powerup(struct nau8540 *nau8540)
{
    /* MICBIAS1 */
    regmap_update_bits(nau8540->regmap, NAU8540_REG_MIC_BIAS,
                       BIT(10U), BIT(10U));
    /* ADC CH1 */
    regmap_update_bits(nau8540->regmap, NAU8540_REG_ANALOG_PWR,
                       BIT(0), BIT(0));
    /* Frontend PGA1 */
    regmap_update_bits(nau8540->regmap, NAU8540_REG_PWR,
                       BIT(12U), BIT(12U));
    /* ADC1 */
    regmap_update_bits(nau8540->regmap, NAU8540_REG_POWER_MANAGEMENT,
                       BIT(0), BIT(0));
}

static void nau8540_powerdown(struct nau8540 *nau8540)
{
    /* ADC1 */
    regmap_update_bits(nau8540->regmap, NAU8540_REG_POWER_MANAGEMENT,
                       BIT(0), 0);
    /* ADC CH1 */
    regmap_update_bits(nau8540->regmap, NAU8540_REG_ANALOG_PWR,
                       BIT(0), 0);
    /* Frontend PGA1 */
    regmap_update_bits(nau8540->regmap, NAU8540_REG_PWR,
                       BIT(12U), 0);
    /* MICBIAS1 */
    regmap_update_bits(nau8540->regmap, NAU8540_REG_MIC_BIAS,
                       BIT(10U), 0);
}

int nau8540_adc_enable(bool enable)
{
    struct nau8540 *nau8540 = g_nau8540;

    if (!g_nau8540) {
        pr_err("FATAL: g_nau8540 is NULL.\n");
        return -1;
    }

    if (enable) {
        nau8540_powerup(nau8540);
        msleep(300U);
        /* DO12 and DO34 pad output enable */
        regmap_update_bits(nau8540->regmap, NAU8540_REG_PCM_CTRL1,
                           NAU8540_I2S_DO12_TRI, 0);
        regmap_update_bits(nau8540->regmap, NAU8540_REG_PCM_CTRL2,
                           NAU8540_I2S_DO34_TRI, 0);
    } else {
        regmap_update_bits(nau8540->regmap, NAU8540_REG_PCM_CTRL1,
                           NAU8540_I2S_DO12_TRI, NAU8540_I2S_DO12_TRI);
        regmap_update_bits(nau8540->regmap, NAU8540_REG_PCM_CTRL2,
                           NAU8540_I2S_DO34_TRI, NAU8540_I2S_DO34_TRI);
        nau8540_powerdown(nau8540);
        /* reset */
        regmap_write(nau8540->regmap, NAU8540_REG_RST, 0x0001);
        regmap_write(nau8540->regmap, NAU8540_REG_RST, 0x0000);
    }

    pr_info("%s(%d) SUCCESS\n", __FUNCTION__, enable);
    return 0;
}

static int nau8540_clock_check(struct nau8540 *nau8540, int rate, int osr)
{
    if (osr >= ARRAY_SIZE(osr_adc_sel)) {
        return -EINVAL;
    }

    if (rate * osr > CLK_ADC_MAX) {
        dev_err(nau8540->dev, "exceed the maximum frequency of CLK_ADC\n");
        return -EINVAL;
    }

    return 0;
}

static int nau8540_set_sysclk(int clk_id)
{
    struct nau8540 *nau8540 = g_nau8540;

    switch (clk_id) {
        case NAU8540_CLK_DIS:
        case NAU8540_CLK_MCLK:
            regmap_update_bits(nau8540->regmap, NAU8540_REG_CLOCK_SRC,
                               NAU8540_CLK_SRC_MASK, NAU8540_CLK_SRC_MCLK);
            regmap_update_bits(nau8540->regmap, NAU8540_REG_FLL6,
                               NAU8540_DCO_EN, 0);
            break;
        case NAU8540_CLK_INTERNAL:
            regmap_update_bits(nau8540->regmap, NAU8540_REG_FLL6,
                               NAU8540_DCO_EN, NAU8540_DCO_EN);
            regmap_update_bits(nau8540->regmap, NAU8540_REG_CLOCK_SRC,
                               NAU8540_CLK_SRC_MASK, NAU8540_CLK_SRC_VCO);
            break;
        default:
            dev_err(nau8540->dev, "Invalid clock id (%d)\n", clk_id);
            return -EINVAL;
    }

    dev_dbg(nau8540->dev, "clock id is %d\n", clk_id);

    return 0;
}

int nau8540_hw_params(unsigned int rate, unsigned int bit_width)
{
    int ret;
    struct nau8540 *nau8540 = g_nau8540;
    unsigned int val_len = 0, osr;

    if (!g_nau8540) {
        pr_err("FATAL: g_nau8540 is NULL.\n");
        return -1;
    }

    ret = nau8540_set_sysclk(NAU8540_CLK_DIS);
    if (ret) {
        return ret;
    }

    /* CLK_ADC = OSR * FS
     * ADC clock frequency is defined as Over Sampling Rate (OSR)
     * multiplied by the audio sample rate (Fs). Note that the OSR and Fs
     * values must be selected such that the maximum frequency is less
     * than 6.144 MHz.
     */
    regmap_read(nau8540->regmap, NAU8540_REG_ADC_SAMPLE_RATE, &osr);
    osr &= NAU8540_ADC_OSR_MASK;
    if (nau8540_clock_check(nau8540, rate, osr)) {
        return -EINVAL;
    }

    regmap_update_bits(nau8540->regmap, NAU8540_REG_CLOCK_SRC,
                       NAU8540_CLK_ADC_SRC_MASK,
                       osr_adc_sel[osr].clk_src << NAU8540_CLK_ADC_SRC_SFT);

    switch (bit_width) {
        case AXG_BIT_WIDTH16:
            val_len |= NAU8540_I2S_DL_16;
            break;
        case AXG_BIT_WIDTH20:
            val_len |= NAU8540_I2S_DL_20;
            break;
        case AXG_BIT_WIDTH24:
            val_len |= NAU8540_I2S_DL_24;
            break;
        case AXG_BIT_WIDTH32:
            val_len |= NAU8540_I2S_DL_32;
            break;
        default:
            return -EINVAL;
    }

    regmap_update_bits(nau8540->regmap, NAU8540_REG_PCM_CTRL0,
                       NAU8540_I2S_DL_MASK, val_len);

    pr_info("%s(rate=%u, bit_width=%u) SUCCESS\n",
            __FUNCTION__, rate, bit_width);
    return 0;
}

static int nau8540_fmt_ctrl_val_determine(unsigned int fmt,
                                          unsigned int *pctrl1_val,
                                          unsigned int *pctrl2_val)
{
    *pctrl1_val = 0;
    *pctrl2_val = 0;

    switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
        case SND_SOC_DAIFMT_CBM_CFM:
            *pctrl2_val |= NAU8540_I2S_MS_MASTER;
            break;
        case SND_SOC_DAIFMT_CBS_CFS:
            break;
        default:
            return -EINVAL;
    }

    switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
        case SND_SOC_DAIFMT_NB_NF:
            break;
        case SND_SOC_DAIFMT_IB_NF:
            *pctrl1_val |= NAU8540_I2S_BP_INV;
            break;
        default:
            return -EINVAL;
    }

    switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
        case SND_SOC_DAIFMT_I2S:
            *pctrl1_val |= NAU8540_I2S_DF_I2S;
            break;
        case SND_SOC_DAIFMT_LEFT_J:
            *pctrl1_val |= NAU8540_I2S_DF_LEFT;
            break;
        case SND_SOC_DAIFMT_RIGHT_J:
            *pctrl1_val |= NAU8540_I2S_DF_RIGTH;
            break;
        case SND_SOC_DAIFMT_DSP_A:
            *pctrl1_val |= NAU8540_I2S_DF_PCM_AB;
            break;
        case SND_SOC_DAIFMT_DSP_B:
            *pctrl1_val |= NAU8540_I2S_DF_PCM_AB;
            *pctrl1_val |= NAU8540_I2S_PCMB_EN;
            break;
        default:
            return -EINVAL;
    }

    return 0;
}

int nau8540_set_fmt(unsigned int fmt)
{
    struct nau8540 *nau8540 = g_nau8540;
    unsigned int ctrl1_val = 0, ctrl2_val = 0;
    int ret;

    if (!g_nau8540) {
        pr_err("FATAL: g_nau8540 is NULL.\n");
        return -1;
    }

    ret = nau8540_fmt_ctrl_val_determine(fmt, &ctrl1_val, &ctrl2_val);
    if (ret) {
        pr_err("nau8540_fmt_ctrl_val_determine failed: %d\n", ret);
        return ret;
    }

    regmap_update_bits(nau8540->regmap, NAU8540_REG_PCM_CTRL0,
                       NAU8540_I2S_DL_MASK | NAU8540_I2S_DF_MASK |
                           NAU8540_I2S_BP_INV | NAU8540_I2S_PCMB_EN,
                       ctrl1_val);
    regmap_update_bits(nau8540->regmap, NAU8540_REG_PCM_CTRL1,
                       NAU8540_I2S_MS_MASK | NAU8540_I2S_DO12_OE,
                       ctrl2_val);
    regmap_update_bits(nau8540->regmap, NAU8540_REG_PCM_CTRL2,
                       NAU8540_I2S_DO34_OE,
                       0);

    pr_info("%s(fmt=0x%x) SUCCESS\n", __FUNCTION__, fmt);
    return 0;
}

/**
 * nau8540_set_tdm_slot - configure DAI TX TDM.
 * @dai: DAI
 * @tx_mask: bitmask representing active TX slots. Ex.
 *                 0xf for normal 4 channel TDM.
 *                 0xf0 for shifted 4 channel TDM
 * @rx_mask: no used.
 * @slots: Number of slots in use.
 * @slot_width: Width in bits for each slot.
 *
 * Configures a DAI for TDM operation. Only support 4 slots TDM.
 */
int nau8540_set_tdm_slot(unsigned int tx_mask,
                         unsigned int rx_mask, int slots, int slot_width)
{
    struct nau8540 *nau8540 = g_nau8540;
    unsigned int ctrl2_val = 0, ctrl4_val = 0;

    if (!g_nau8540) {
        pr_err("FATAL: g_nau8540 is NULL.\n");
        return -1;
    }

    if (slots > 4U || ((tx_mask & 0xf0) && (tx_mask & 0xf))) {
        return -EINVAL;
    }

    ctrl4_val |= (NAU8540_TDM_MODE | NAU8540_TDM_OFFSET_EN);
    if (tx_mask & 0xf0) {
        ctrl2_val = 4U * slot_width;
        ctrl4_val |= (tx_mask >> 4U);
    } else {
        ctrl4_val |= tx_mask;
    }
    regmap_update_bits(nau8540->regmap, NAU8540_REG_PCM_CTRL4,
                       NAU8540_TDM_MODE | NAU8540_TDM_OFFSET_EN |
                           NAU8540_TDM_TX_MASK,
                       ctrl4_val);
    regmap_update_bits(nau8540->regmap, NAU8540_REG_PCM_CTRL1,
                       NAU8540_I2S_DO12_OE, NAU8540_I2S_DO12_OE);
    regmap_update_bits(nau8540->regmap, NAU8540_REG_PCM_CTRL2,
                       NAU8540_I2S_DO34_OE | NAU8540_I2S_TSLOT_L_MASK,
                       NAU8540_I2S_DO34_OE | ctrl2_val);

    pr_info("%s(tx_mask=0x%x, rx_mask=0x%x, slots=%d, slot_width=%d) SUCCESS\n",
            __FUNCTION__, tx_mask, rx_mask, slots, slot_width);
    return 0;
}

/**
 * nau8540_calc_fll_param - Calculate FLL parameters.
 * @fll_in: external clock provided to codec.
 * @fs: sampling rate.
 * @fll_param: Pointer to structure of FLL parameters.
 *
 * Calculate FLL parameters to configure codec.
 *
 * Returns 0 for success or negative error code.
 */
static int nau8540_calc_fll_param(unsigned int fll_in,
                                  unsigned int fs, struct nau8540_fll *fll_param)
{
    u64 fvco, fvco_max;
    unsigned int fref, i, fvco_sel;

    /* Ensure the reference clock frequency (FREF) is <= 13.5MHz by dividing
     * freq_in by 1, 2, 4, or 8 using FLL pre-scalar.
     * FREF = freq_in / NAU8540_FLL_REF_DIV_MASK
     */
    for (i = 0; i < ARRAY_SIZE(fll_pre_scalar); i++) {
        fref = fll_in / fll_pre_scalar[i].param;
        if (fref <= NAU_FREF_MAX) {
            break;
        }
    }
    if (i == ARRAY_SIZE(fll_pre_scalar)) {
        return -EINVAL;
    }

    fll_param->clk_ref_div = fll_pre_scalar[i].val;

    /* Choose the FLL ratio based on FREF */
    for (i = 0; i < ARRAY_SIZE(fll_ratio); i++) {
        if (fref >= fll_ratio[i].param) {
            break;
        }
    }
    if (i == ARRAY_SIZE(fll_ratio)) {
        return -EINVAL;
    }

    fll_param->ratio = fll_ratio[i].val;

    /* Calculate the frequency of DCO (FDCO) given freq_out = 256 * Fs.
     * FDCO must be within the 90MHz - 124MHz or the FFL cannot be
     * guaranteed across the full range of operation.
     * FDCO = freq_out * 2 * mclk_src_scaling
     */
    fvco_max = 0;
    fvco_sel = ARRAY_SIZE(mclk_src_scaling);
    for (i = 0; i < ARRAY_SIZE(mclk_src_scaling); i++) {
        fvco = 256ULL * fs * 2ULL * mclk_src_scaling[i].param;
        if (fvco > NAU_FVCO_MIN && fvco < NAU_FVCO_MAX &&
            fvco_max < fvco) {
            fvco_max = fvco;
            fvco_sel = i;
        }
    }
    if (ARRAY_SIZE(mclk_src_scaling) == fvco_sel) {
        return -EINVAL;
    }

    fll_param->mclk_src = mclk_src_scaling[fvco_sel].val;

    /* Calculate the FLL 10-bit integer input and the FLL 16-bit fractional
     * input based on FDCO, FREF and FLL ratio.
     */
    fvco = div_u64(fvco_max << 16U, fref * fll_param->ratio);
    fll_param->fll_int = (fvco >> 16U) & 0x3FF;
    fll_param->fll_frac = fvco & 0xFFFF;
    return 0;
}

static void nau8540_fll_apply(struct regmap *regmap,
                              struct nau8540_fll *fll_param)
{
    regmap_update_bits(regmap, NAU8540_REG_CLOCK_SRC,
                       NAU8540_CLK_SRC_MASK | NAU8540_CLK_MCLK_SRC_MASK,
                       NAU8540_CLK_SRC_MCLK | fll_param->mclk_src);
    regmap_update_bits(regmap, NAU8540_REG_FLL1,
                       NAU8540_FLL_RATIO_MASK | NAU8540_ICTRL_LATCH_MASK,
                       fll_param->ratio | (0x6 << NAU8540_ICTRL_LATCH_SFT));
    /* FLL 16-bit fractional input */
    regmap_write(regmap, NAU8540_REG_FLL2, fll_param->fll_frac);
    /* FLL 10-bit integer input */
    regmap_update_bits(regmap, NAU8540_REG_FLL3,
                       NAU8540_FLL_INTEGER_MASK, fll_param->fll_int);
    /* FLL pre-scaler */
    regmap_update_bits(regmap, NAU8540_REG_FLL4,
                       NAU8540_FLL_REF_DIV_MASK,
                       fll_param->clk_ref_div << NAU8540_FLL_REF_DIV_SFT);
    regmap_update_bits(regmap, NAU8540_REG_FLL5,
                       NAU8540_FLL_CLK_SW_MASK, NAU8540_FLL_CLK_SW_REF);
    regmap_update_bits(regmap,
                       NAU8540_REG_FLL6, NAU8540_DCO_EN, 0);
    if (fll_param->fll_frac) {
        regmap_update_bits(regmap, NAU8540_REG_FLL5,
                           NAU8540_FLL_PDB_DAC_EN | NAU8540_FLL_LOOP_FTR_EN |
                               NAU8540_FLL_FTR_SW_MASK,
                           NAU8540_FLL_PDB_DAC_EN | NAU8540_FLL_LOOP_FTR_EN |
                               NAU8540_FLL_FTR_SW_FILTER);
        regmap_update_bits(regmap, NAU8540_REG_FLL6,
                           NAU8540_SDM_EN | NAU8540_CUTOFF500,
                           NAU8540_SDM_EN | NAU8540_CUTOFF500);
    } else {
        regmap_update_bits(regmap, NAU8540_REG_FLL5,
                           NAU8540_FLL_PDB_DAC_EN | NAU8540_FLL_LOOP_FTR_EN |
                               NAU8540_FLL_FTR_SW_MASK,
                           NAU8540_FLL_FTR_SW_ACCU);
        regmap_update_bits(regmap, NAU8540_REG_FLL6,
                           NAU8540_SDM_EN | NAU8540_CUTOFF500, 0);
    }
}

/* freq_out must be 256*Fs in order to achieve the best performance */
int nau8540_set_pll(int pll_id, unsigned int freq_in,
                    unsigned int freq_out)
{
    struct nau8540 *nau8540 = g_nau8540;
    struct nau8540_fll fll_param;
    int ret, fs;

    if (!g_nau8540) {
        pr_err("FATAL: g_nau8540 is NULL.\n");
        return -1;
    }

    switch (pll_id) {
        case NAU8540_CLK_FLL_MCLK:
            regmap_update_bits(nau8540->regmap, NAU8540_REG_FLL3,
                               NAU8540_FLL_CLK_SRC_MASK | NAU8540_GAIN_ERR_MASK,
                               NAU8540_FLL_CLK_SRC_MCLK | 0);
            break;
        case NAU8540_CLK_FLL_BLK:
            regmap_update_bits(nau8540->regmap, NAU8540_REG_FLL3,
                               NAU8540_FLL_CLK_SRC_MASK | NAU8540_GAIN_ERR_MASK,
                               NAU8540_FLL_CLK_SRC_BLK |
                                   (0xf << NAU8540_GAIN_ERR_SFT));
            break;
        case NAU8540_CLK_FLL_FS:
            regmap_update_bits(nau8540->regmap, NAU8540_REG_FLL3,
                               NAU8540_FLL_CLK_SRC_MASK | NAU8540_GAIN_ERR_MASK,
                               NAU8540_FLL_CLK_SRC_FS |
                                   (0xf << NAU8540_GAIN_ERR_SFT));
            break;

        default:
            dev_err(nau8540->dev, "Invalid clock id (%d)\n", pll_id);
            return -EINVAL;
    }
    dev_dbg(nau8540->dev, "Sysclk is %dHz and clock id is %d\n",
            freq_out, pll_id);

    fs = freq_out / 256U;
    ret = nau8540_calc_fll_param(freq_in, fs, &fll_param);
    if (ret < 0) {
        dev_err(nau8540->dev, "Unsupported input clock %d\n", freq_in);
        return ret;
    }
    dev_info(nau8540->dev, "mclk_src=%x ratio=%x fll_frac=%x fll_int=%x clk_ref_div=%x\n",
             fll_param.mclk_src, fll_param.ratio, fll_param.fll_frac,
             fll_param.fll_int, fll_param.clk_ref_div);

    nau8540_fll_apply(nau8540->regmap, &fll_param);
    mdelay(2U);
    regmap_update_bits(nau8540->regmap, NAU8540_REG_CLOCK_SRC,
                       NAU8540_CLK_SRC_MASK, NAU8540_CLK_SRC_VCO);

    return 0;
}

static void nau8540_reset_chip(struct regmap *regmap)
{
    regmap_write(regmap, NAU8540_REG_SW_RESET, 0x00);
    regmap_write(regmap, NAU8540_REG_SW_RESET, 0x00);
}

static void nau8540_init_regs(struct nau8540 *nau8540)
{
    struct regmap *regmap = nau8540->regmap;

    /* Enable Bias/VMID/VMID Tieoff */
    regmap_update_bits(regmap, NAU8540_REG_VMID_CTRL,
                       NAU8540_VMID_EN | NAU8540_VMID_SEL_MASK,
                       NAU8540_VMID_EN | (0x2 << NAU8540_VMID_SEL_SFT));
    regmap_update_bits(regmap, NAU8540_REG_REFERENCE,
                       NAU8540_PRECHARGE_DIS | NAU8540_GLOBAL_BIAS_EN,
                       NAU8540_PRECHARGE_DIS | NAU8540_GLOBAL_BIAS_EN);
    mdelay(2U);
    regmap_update_bits(regmap, NAU8540_REG_MIC_BIAS,
                       NAU8540_PU_PRE, NAU8540_PU_PRE);
    regmap_update_bits(regmap, NAU8540_REG_CLOCK_CTRL,
                       NAU8540_CLK_ADC_EN | NAU8540_CLK_I2S_EN,
                       NAU8540_CLK_ADC_EN | NAU8540_CLK_I2S_EN);
    /* ADC OSR selection, CLK_ADC = Fs * OSR;
     * Channel time alignment enable.
     */
    regmap_update_bits(regmap, NAU8540_REG_ADC_SAMPLE_RATE,
                       NAU8540_CH_SYNC | NAU8540_ADC_OSR_MASK,
                       NAU8540_CH_SYNC | NAU8540_ADC_OSR_128);
    /* PGA input mode selection */
    regmap_update_bits(regmap, NAU8540_REG_FEPGA1,
                       NAU8540_FEPGA1_MODCH2_SHT | NAU8540_FEPGA1_MODCH1_SHT,
                       NAU8540_FEPGA1_MODCH2_SHT | NAU8540_FEPGA1_MODCH1_SHT);
    regmap_update_bits(regmap, NAU8540_REG_FEPGA2,
                       NAU8540_FEPGA2_MODCH4_SHT | NAU8540_FEPGA2_MODCH3_SHT,
                       NAU8540_FEPGA2_MODCH4_SHT | NAU8540_FEPGA2_MODCH3_SHT);
    /* DO12 and DO34 pad output disable */
    regmap_update_bits(regmap, NAU8540_REG_PCM_CTRL1,
                       NAU8540_I2S_DO12_TRI, NAU8540_I2S_DO12_TRI);
    regmap_update_bits(regmap, NAU8540_REG_PCM_CTRL2,
                       NAU8540_I2S_DO34_TRI, NAU8540_I2S_DO34_TRI);
    /* Update 14th bit always out */
    regmap_update_bits(nau8540->regmap, NAU8540_REG_PCM_CTRL1,
                       0x4000, 0X4000);
    regmap_write(nau8540->regmap, NAU8540_REG_FEPGA3, 0x1717);
    regmap_write(nau8540->regmap, NAU8540_REG_DIGITAL_GAIN_CH1, 0x0520);
    regmap_write(nau8540->regmap, NAU8540_REG_DIGITAL_GAIN_CH2, 0x0520);
    regmap_write(nau8540->regmap, NAU8540_REG_DIGITAL_GAIN_CH3, 0x0520);
    regmap_write(nau8540->regmap, NAU8540_REG_DIGITAL_GAIN_CH4, 0x0520);
    regmap_write(nau8540->regmap, NAU8540_REG_DIGITAL_MUX, 0x0000);
    regmap_write(nau8540->regmap, NAU8540_REG_HPF_FILTER_CH12, 0x1F1F);
    regmap_write(nau8540->regmap, NAU8540_REG_HPF_FILTER_CH34, 0x1F1F);
}

int nau8540_suspend(void)
{
    struct nau8540 *nau8540 = g_nau8540;

    if (!g_nau8540) {
        pr_err("FATAL: g_nau8540 is NULL.\n");
        return -1;
    }

    regcache_cache_only(nau8540->regmap, true);
    regcache_mark_dirty(nau8540->regmap);

    return 0;
}

int nau8540_resume(void)
{
    struct nau8540 *nau8540 = g_nau8540;

    if (!g_nau8540) {
        pr_err("FATAL: g_nau8540 is NULL.\n");
        return -1;
    }

    regcache_cache_only(nau8540->regmap, false);
    regcache_sync(nau8540->regmap);

    return 0;
}

static const struct regmap_config nau8540_regmap_config = {
    .val_bits = 16,
    .reg_bits = 16,

    .max_register = NAU8540_REG_MAX,
    .readable_reg = nau8540_readable_reg,
    .writeable_reg = nau8540_writeable_reg,
    .volatile_reg = nau8540_volatile_reg,

    .cache_type = REGCACHE_RBTREE,
    .reg_defaults = nau8540_reg_defaults,
    .num_reg_defaults = ARRAY_SIZE(nau8540_reg_defaults),
};

static int nau8540_i2c_probe(struct i2c_client *i2c,
                             const struct i2c_device_id *id)
{
    struct device *dev = &i2c->dev;
    struct nau8540 *nau8540 = dev_get_platdata(dev);
    int ret, value;

    if (!nau8540) {
        nau8540 = devm_kzalloc(dev, sizeof(*nau8540), GFP_KERNEL);
        if (!nau8540) {
            return -ENOMEM;
        }
    }
    i2c_set_clientdata(i2c, nau8540);

    nau8540->regmap = devm_regmap_init_i2c(i2c, &nau8540_regmap_config);
    if (IS_ERR(nau8540->regmap)) {
        return PTR_ERR(nau8540->regmap);
    }

    ret = regmap_read(nau8540->regmap, NAU8540_REG_I2C_DEVICE_ID, &value);
    if (ret < 0) {
        dev_err(dev, "Failed to read device id from the NAU85L40: %d\n", ret);
        return ret;
    }

    nau8540->dev = dev;
    nau8540_reset_chip(nau8540->regmap);
    nau8540_init_regs(nau8540);

    g_nau8540 = nau8540;

    dev_info(dev, "nau8540 probe SUCCESS\n");
    return 0;
}

static const struct i2c_device_id nau8540_i2c_ids[] = {
    { "nau8540", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, nau8540_i2c_ids);

static const struct of_device_id nau8540_of_ids[] = {
    { .compatible = "nuvoton,nau8540", },
    {}
};
MODULE_DEVICE_TABLE(of, nau8540_of_ids);

static struct i2c_driver nau8540_i2c_driver = {
    .driver = {
        .name = "nau8540",
        .of_match_table = of_match_ptr(nau8540_of_ids),
    },
    .probe = nau8540_i2c_probe,
    .id_table = nau8540_i2c_ids,
};
module_i2c_driver(nau8540_i2c_driver);

