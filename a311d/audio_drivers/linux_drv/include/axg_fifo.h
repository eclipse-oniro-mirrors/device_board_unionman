/*
 * Copyright (c) 2022 Unionman Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef MESON_AXG_FIFO_H
#define MESON_AXG_FIFO_H

#include <linux/regmap.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

struct clk;
struct platform_device;
struct reg_field;
struct regmap;
struct regmap_field;
struct reset_control;

#define AXG_FIFO_CH_MAX         128
#define AXG_FIFO_RATES          (SNDRV_PCM_RATE_5512 |      \
                        SNDRV_PCM_RATE_8000_192000)
#define AXG_FIFO_FORMATS        (SNDRV_PCM_FMTBIT_S8 |      \
                        SNDRV_PCM_FMTBIT_S16_LE |   \
                        SNDRV_PCM_FMTBIT_S20_LE |   \
                        SNDRV_PCM_FMTBIT_S24_LE |   \
                        SNDRV_PCM_FMTBIT_S32_LE |   \
                        SNDRV_PCM_FMTBIT_IEC958_SUBFRAME_LE)

#define AXG_FIFO_BURST          8

#define FIFO_INT_ADDR_FINISH    BIT(0)
#define FIFO_INT_ADDR_INT       BIT(1)
#define FIFO_INT_COUNT_REPEAT   BIT(2)
#define FIFO_INT_COUNT_ONCE     BIT(3)
#define FIFO_INT_FIFO_ZERO      BIT(4)
#define FIFO_INT_FIFO_DEPTH     BIT(5)
#define FIFO_INT_MASK           GENMASK(7, 0)

#define FIFO_CTRL0              0x00
#define  CTRL0_DMA_EN           BIT(31)
#define  CTRL0_INT_EN(x)        ((x) << 16)
#define  CTRL0_SEL_MASK         GENMASK(2, 0)
#define  CTRL0_SEL_SHIFT        0
#define FIFO_CTRL1              0x04
#define  CTRL1_INT_CLR(x)       ((x) << 0)
#define  CTRL1_STATUS2_SEL_MASK     GENMASK(11, 8)
#define  CTRL1_STATUS2_SEL(x)       ((x) << 8)
#define   STATUS2_SEL_DDR_READ      0
#define  CTRL1_FRDDR_DEPTH_MASK     GENMASK(31, 24)
#define  CTRL1_FRDDR_DEPTH(x)       ((x) << 24)
#define FIFO_START_ADDR         0x08
#define FIFO_FINISH_ADDR        0x0c
#define FIFO_INT_ADDR           0x10
#define FIFO_STATUS1            0x14
#define  STATUS1_INT_STS(x)     ((x) << 0)
#define FIFO_STATUS2            0x18
#define FIFO_INIT_ADDR          0x24
#define FIFO_CTRL2              0x28

struct axg_fifo {
    const char *name_prefix;    /* such as FRDDR_B, TODDR_B */
    struct device *dev;
    struct regmap *map;
    struct clk *pclk;
    struct reset_control *arb;
    struct regmap_field *field_threshold;
    unsigned int depth;
    int irq;
    dma_addr_t dma_addr;        /* physical bus address (no cached) */
    unsigned char *dma_vaddr;   /* virtual pointer */
    size_t dma_area;            /* size of DMA area */
    size_t dma_cir_size;        /* size of DMA circle buffer */
    int is_frddr;               /* 1 for from-ddr, 0 for to-ddr */
    int is_g12a;                /* 1 for g12a, 0 for axg */
    bool pcm_opened;
    bool dai_startup;
    
    struct list_head list;
};

struct axg_fifo_match_data {
    struct reg_field field_threshold;
    int is_frddr;   /* 1 for from-ddr, 0 for to-ddr */
    int is_g12a;    /* 1 for g12a, 0 for axg */
};

int meson_axg_fifo_probe(struct platform_device *pdev);

// name_prefix: such as FRDDR_B, TODDR_B...
struct axg_fifo *meson_axg_fifo_get(const char *name_prefix);

int meson_axg_fifo_update_bits(struct axg_fifo *fifo,
                               unsigned int reg, unsigned int mask, unsigned int val);

int meson_axg_fifo_pcm_open(struct axg_fifo *fifo);

int meson_axg_fifo_pcm_close(struct axg_fifo *fifo);

int meson_axg_fifo_dai_hw_params(struct axg_fifo *fifo,
                                 unsigned int bit_width, unsigned int phys_width);

int meson_axg_fifo_pcm_hw_params(struct axg_fifo *fifo,
                                 unsigned int period, unsigned int cir_buf_size);

int meson_axg_fifo_pcm_hw_free(struct axg_fifo *fifo);

int meson_axg_fifo_pcm_prepare(struct axg_fifo *fifo);

uint32_t meson_axg_fifo_pcm_pointer(struct axg_fifo *fifo);

int meson_axg_fifo_pcm_enable(struct axg_fifo *fifo, bool enable);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* MESON_AXG_FIFO_H */
