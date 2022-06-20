/*
 * Copyright (c) 2022 Unionman Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <linux/clk.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/module.h>
#include <linux/reset.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>

#include "axg_fifo.h"

#define DMA_SIZE_MAX            (1*1024*1024)

// FRDDR definition
#define CTRL0_FRDDR_PP_MODE		BIT(30)
#define CTRL0_SEL1_EN_SHIFT		3
#define CTRL0_SEL2_SHIFT		4
#define CTRL0_SEL2_EN_SHIFT		7
#define CTRL0_SEL3_SHIFT		8
#define CTRL0_SEL3_EN_SHIFT		11
#define CTRL1_FRDDR_FORCE_FINISH	BIT(12)
#define CTRL2_SEL1_SHIFT		0
#define CTRL2_SEL1_EN_SHIFT		4
#define CTRL2_SEL2_SHIFT		8
#define CTRL2_SEL2_EN_SHIFT		12
#define CTRL2_SEL3_SHIFT		16
#define CTRL2_SEL3_EN_SHIFT		20

// TODDR definition
#define CTRL0_TODDR_SEL_RESAMPLE	BIT(30)
#define CTRL0_TODDR_EXT_SIGNED		BIT(29)
#define CTRL0_TODDR_PP_MODE		BIT(28)
#define CTRL0_TODDR_SYNC_CH		BIT(27)
#define CTRL0_TODDR_TYPE_MASK		GENMASK(15, 13)
#define CTRL0_TODDR_TYPE(x)		((x) << 13)
#define CTRL0_TODDR_MSB_POS_MASK	GENMASK(12, 8)
#define CTRL0_TODDR_MSB_POS(x)		((x) << 8)
#define CTRL0_TODDR_LSB_POS_MASK	GENMASK(7, 3)
#define CTRL0_TODDR_LSB_POS(x)		((x) << 3)
#define CTRL1_TODDR_FORCE_FINISH	BIT(25)
#define CTRL1_SEL_SHIFT			28
#define TODDR_MSB_POS	31

static LIST_HEAD(axg_fifo_list);

static void __dma_enable(struct axg_fifo *fifo,  bool enable)
{
    regmap_update_bits(fifo->map, FIFO_CTRL0, CTRL0_DMA_EN,
                enable ? CTRL0_DMA_EN : 0);
}

/* allocate the coherent DMA pages */
static int __fifo_pages_alloc(struct axg_fifo *fifo, size_t size)
{
    gfp_t gfp_flags;

    gfp_flags = GFP_KERNEL
                    | __GFP_COMP	/* compound page lets parts be mapped */
                    | __GFP_NORETRY /* don't trigger OOM-killer */
                    | __GFP_NOWARN; /* no stack trace print - this call is non-critical */
    fifo->dma_vaddr = dma_alloc_coherent(fifo->dev, size, &fifo->dma_addr,
                                        gfp_flags);

    if (!fifo->dma_vaddr)
        return -1;

    fifo->dma_area = size;

    dev_info(fifo->dev, "%s: alloc dma success, size=%lu", fifo->name_prefix, size);
    
    return 0;
}

#if 0
/* free the coherent DMA pages */
static void __fifo_pages_free(struct axg_fifo *fifo)
{
    if (fifo->dma_vaddr) {
        dma_free_coherent(fifo->dev, fifo->dma_area, fifo->dma_vaddr, fifo->dma_addr);
    }

    fifo->dma_vaddr = NULL;
    fifo->dma_area = 0;
    fifo->dma_addr = 0;
}
#endif
static int __fifo_update_bits(struct axg_fifo *fifo,
        unsigned int reg, unsigned int mask, unsigned int val)
{
    int ret;
    
    ret = regmap_update_bits(fifo->map, reg, mask, val);

    dev_info(fifo->dev, "%s(name=%s, reg=0x%x, mask=0x%x, val=0x%x): %d\n", 
            __FUNCTION__, fifo->name_prefix, reg, mask, val, ret);
        
    return ret;
}

static int __fifo_register(struct axg_fifo *fifo)
{
    struct axg_fifo *f;
    list_for_each_entry(f, &axg_fifo_list, list) {
        if (!strcmp(f->name_prefix, fifo->name_prefix)) {
            dev_err(fifo->dev, "duplicated dev[%s], no need to register\n",
                    fifo->name_prefix);
            return -1;
        }
    }

    list_add_tail(&fifo->list, &axg_fifo_list);
    
    dev_info(fifo->dev, "Register Snd FIFO SUCCESS: %s\n", fifo->name_prefix);
    
    return 0;
}

static struct axg_fifo *__fifo_find(const char *name_prefix)
{
    struct axg_fifo *f;
    list_for_each_entry(f, &axg_fifo_list, list) {
        if (!strcmp(f->name_prefix, name_prefix)) {
            return f;
        }
    }

    return NULL;
}

static int __frddr_pointer_reset(struct axg_fifo *fifo)
{
    if (!fifo->is_g12a)
        return 0; // do nothing for axg

    /* Reset the read pointer to the FIFO_INIT_ADDR */
    regmap_update_bits(fifo->map, FIFO_CTRL1,
                    CTRL1_FRDDR_FORCE_FINISH, 0);
    regmap_update_bits(fifo->map, FIFO_CTRL1,
                    CTRL1_FRDDR_FORCE_FINISH, CTRL1_FRDDR_FORCE_FINISH);
    regmap_update_bits(fifo->map, FIFO_CTRL1,
                    CTRL1_FRDDR_FORCE_FINISH, 0);

    return 0;
}

static int __toddr_pointer_reset(struct axg_fifo *fifo)
{
    if (!fifo->is_g12a) {
        return 0; // do nothing for axg
    }
    
    /* Reset the write pointer to the FIFO_INIT_ADDR */
    regmap_update_bits(fifo->map, FIFO_CTRL1,
                CTRL1_TODDR_FORCE_FINISH, 0);
    regmap_update_bits(fifo->map, FIFO_CTRL1,
                CTRL1_TODDR_FORCE_FINISH, CTRL1_TODDR_FORCE_FINISH);
    regmap_update_bits(fifo->map, FIFO_CTRL1,
                CTRL1_TODDR_FORCE_FINISH, 0);

    return 0;
}

static int __frddr_dai_startup(struct axg_fifo *fifo)
{
    unsigned int val;

    /* Apply single buffer mode to the interface */
    regmap_update_bits(fifo->map, FIFO_CTRL0, CTRL0_FRDDR_PP_MODE, 0);

    /* Use all fifo depth */
    val = (fifo->depth / AXG_FIFO_BURST) - 1;
    regmap_update_bits(fifo->map, FIFO_CTRL1, CTRL1_FRDDR_DEPTH_MASK,
                CTRL1_FRDDR_DEPTH(val));

    return 0;
}

static int __toddr_dai_startup(struct axg_fifo *fifo)
{
    /* Select orginal data - resampling not supported ATM */
    regmap_update_bits(fifo->map, FIFO_CTRL0, CTRL0_TODDR_SEL_RESAMPLE, 0);

    /* Only signed format are supported ATM */
    regmap_update_bits(fifo->map, FIFO_CTRL0, CTRL0_TODDR_EXT_SIGNED,
                CTRL0_TODDR_EXT_SIGNED);

    /* Apply single buffer mode to the interface */
    regmap_update_bits(fifo->map, FIFO_CTRL0, CTRL0_TODDR_PP_MODE, 0);


    if (fifo->is_g12a) {
        /*
         * Make sure the first channel ends up in the at beginning of the output
         * As weird as it looks, without this the first channel may be misplaced
         * in memory, with a random shift of 2 channels.
         */
        regmap_update_bits(fifo->map, FIFO_CTRL0, CTRL0_TODDR_SYNC_CH,
                    CTRL0_TODDR_SYNC_CH);
    }

    return 0;
}

static int __toddr_dai_hw_params(struct axg_fifo *fifo, 
            unsigned int bit_width, unsigned int phys_width)
{
    unsigned int type, width;

    switch (phys_width) {
    case 8:
        type = 0; /* 8 samples of 8 bits */
        break;
    case 16:
        type = 2; /* 4 samples of 16 bits - right justified */
        break;
    case 32:
        type = 4; /* 2 samples of 32 bits - right justified */
        break;
    default:
        return -EINVAL;
    }

    width = bit_width;

    regmap_update_bits(fifo->map, FIFO_CTRL0,
                CTRL0_TODDR_TYPE_MASK |
                CTRL0_TODDR_MSB_POS_MASK |
                CTRL0_TODDR_LSB_POS_MASK,
                CTRL0_TODDR_TYPE(type) |
                CTRL0_TODDR_MSB_POS(TODDR_MSB_POS) |
                CTRL0_TODDR_LSB_POS(TODDR_MSB_POS - (width - 1)));

    return 0;
}

uint32_t meson_axg_fifo_pcm_pointer(struct axg_fifo *fifo)
{
    if (!fifo->pcm_opened)
        return 0;

    unsigned int addr;

    regmap_read(fifo->map, FIFO_STATUS2, &addr);

    if (addr > fifo->dma_addr)
        return (uint32_t)(addr - fifo->dma_addr);
    else
        return 0;
}

int meson_axg_fifo_pcm_hw_params(struct axg_fifo *fifo, 
        unsigned int period, unsigned int cir_buf_size)
{
	unsigned int burst_num, threshold;
	dma_addr_t end_ptr;

    if (!fifo->pcm_opened)
        return -1;

    if (cir_buf_size > fifo->dma_area) {
        dev_err(fifo->dev, "invalid param: cir_buf_size(%d) > dma_area(%d)",
                cir_buf_size, fifo->dma_area);
        return -EINVAL;
    }
    
	/* Setup dma memory pointers */
	end_ptr = fifo->dma_addr + cir_buf_size - AXG_FIFO_BURST;
	regmap_write(fifo->map, FIFO_START_ADDR, fifo->dma_addr);
	regmap_write(fifo->map, FIFO_FINISH_ADDR, end_ptr);

	/* Setup interrupt periodicity */
	burst_num = period / AXG_FIFO_BURST;
	regmap_write(fifo->map, FIFO_INT_ADDR, burst_num);

	/*
	 * Start the fifo request on the smallest of the following:
	 * - Half the fifo size
	 * - Half the period size
	 */
	threshold = min(period / 2, fifo->depth / 2);

	/*
	 * With the threshold in bytes, register value is:
	 * V = (threshold / burst) - 1
	 */
	threshold /= AXG_FIFO_BURST;
	regmap_field_write(fifo->field_threshold,
			   threshold ? threshold - 1 : 0);

	/* Enable block count irq */
	regmap_update_bits(fifo->map, FIFO_CTRL0,
			   CTRL0_INT_EN(FIFO_INT_COUNT_REPEAT),
			   CTRL0_INT_EN(FIFO_INT_COUNT_REPEAT));

    if (fifo->is_g12a) {
    	/* Set the initial memory address of the DMA */
    	regmap_write(fifo->map, FIFO_INIT_ADDR, fifo->dma_addr);
    }

    fifo->dma_cir_size = cir_buf_size;
    
    dev_info(fifo->dev, "%s: %s(period=%u, cir_buf_size=%u) SUCCESS.\n ", 
            fifo->name_prefix, __FUNCTION__, period, cir_buf_size);
            
	return 0;
}

int meson_axg_fifo_pcm_hw_free(struct axg_fifo *fifo)
{
	/* Disable the block count irq */
	regmap_update_bits(fifo->map, FIFO_CTRL0,
			   CTRL0_INT_EN(FIFO_INT_COUNT_REPEAT), 0);

	return 0;
}

int meson_axg_fifo_pcm_prepare(struct axg_fifo *fifo)
{
    int ret = 0;
    
    if (!fifo->pcm_opened)
        return -1;

    /* reset FIFO pointer */
    if (fifo->is_frddr)
        ret = __frddr_pointer_reset(fifo);
    else
        ret = __toddr_pointer_reset(fifo);

    /* clear dma buffer */
    if (fifo->dma_cir_size) {
        memset(fifo->dma_vaddr, 0, fifo->dma_cir_size);
    }

    return ret;
}

static void __axg_fifo_ack_irq(struct axg_fifo *fifo, u8 mask)
{
	regmap_update_bits(fifo->map, FIFO_CTRL1,
			   CTRL1_INT_CLR(FIFO_INT_MASK),
			   CTRL1_INT_CLR(mask));

	/* Clear must also be cleared */
	regmap_update_bits(fifo->map, FIFO_CTRL1,
			   CTRL1_INT_CLR(FIFO_INT_MASK),
			   0);
}

static irqreturn_t __axg_fifo_pcm_irq_block(int irq, void *dev_id)
{
	struct axg_fifo *fifo = dev_id;
	unsigned int status;

    //dev_info(fifo->dev, "%s\n", __FUNCTION__);
    
	regmap_read(fifo->map, FIFO_STATUS1, &status);

	status = STATUS1_INT_STS(status) & FIFO_INT_MASK;
	if (status & FIFO_INT_COUNT_REPEAT)
		;/* DO nothing here */
	else
		dev_dbg(fifo->dev, "unexpected irq - STS 0x%02x\n",
			status);

	/* Ack irqs */
	__axg_fifo_ack_irq(fifo, status);

	return IRQ_RETVAL(status);
}

int meson_axg_fifo_dai_hw_params(struct axg_fifo *fifo,
                unsigned int bit_width, unsigned int phys_width)
{
    if (!fifo->pcm_opened)
        return -1;

    /* Do nothing with FRDDR */
    if (fifo->is_frddr)
        return 0;

    return __toddr_dai_hw_params(fifo, bit_width, phys_width);
}

struct axg_fifo *meson_axg_fifo_get(const char *name_prefix)
{
    return __fifo_find(name_prefix);
}


int meson_axg_fifo_update_bits(struct axg_fifo *fifo,
        unsigned int reg, unsigned int mask, unsigned int val)
{
    return __fifo_update_bits(fifo, reg, mask, val);
}

int meson_axg_fifo_pcm_open(struct axg_fifo *fifo)
{
	int ret;

    if (fifo->pcm_opened)
        return 0;
        
    ret = request_irq(fifo->irq, __axg_fifo_pcm_irq_block, 0,
                dev_name(fifo->dev), fifo);
    if (ret)
        return ret;

    /* Enable pclk to access registers and clock the fifo ip */
    ret = clk_prepare_enable(fifo->pclk);
    if (ret)
        goto free_irq;

    /* Setup status2 so it reports the memory pointer */
    regmap_update_bits(fifo->map, FIFO_CTRL1,
                CTRL1_STATUS2_SEL_MASK,
                CTRL1_STATUS2_SEL(STATUS2_SEL_DDR_READ));

    /* Make sure the dma is initially disabled */
    __dma_enable(fifo, false);

    /* Disable irqs until params are ready */
    regmap_update_bits(fifo->map, FIFO_CTRL0,
                CTRL0_INT_EN(FIFO_INT_MASK), 0);

    /* Clear any pending interrupt */
    __axg_fifo_ack_irq(fifo, FIFO_INT_MASK);

    /* Take memory arbitror out of reset */
    ret = reset_control_deassert(fifo->arb);
    if (ret)
        goto free_clk;

    if (fifo->is_frddr)
        ret = __frddr_dai_startup(fifo);
    else
        ret = __toddr_dai_startup(fifo);

    if (ret)
        goto free_clk;

    fifo->pcm_opened = true;
    
    return 0;

free_clk:
    clk_disable_unprepare(fifo->pclk);
free_irq:
    free_irq(fifo->irq, fifo);
    return ret;
}

int meson_axg_fifo_pcm_close(struct axg_fifo *fifo)
{
    int ret;

    if (!fifo->pcm_opened)
        return 0;
        
    /* Put the memory arbitror back in reset */
    ret = reset_control_assert(fifo->arb);

    /* Disable fifo ip and register access */
    clk_disable_unprepare(fifo->pclk);

    /* remove IRQ */
    free_irq(fifo->irq, fifo);

    fifo->pcm_opened = false;
    return ret;
}

int meson_axg_fifo_pcm_enable(struct axg_fifo *fifo, bool enable)
{
    if (!fifo->pcm_opened)
        return -1;

    __dma_enable(fifo, enable);
    return 0;
}

static const struct regmap_config axg_fifo_regmap_cfg = {
    .reg_bits	= 32,
    .val_bits	= 32,
    .reg_stride	= 4,
    .max_register	= FIFO_CTRL2,
};

int meson_axg_fifo_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    const struct axg_fifo_match_data *data;
    struct axg_fifo *fifo;
    void __iomem *regs;
    int ret;

    data = of_device_get_match_data(dev);
    if (!data) {
        dev_err(dev, "failed to match device\n");
        return -ENODEV;
    }

    fifo = devm_kzalloc(dev, sizeof(*fifo), GFP_KERNEL);
    if (!fifo)
        return -ENOMEM;
    platform_set_drvdata(pdev, fifo);

    fifo->is_frddr = data->is_frddr;
    fifo->is_g12a = data->is_g12a;
    
    regs = devm_platform_ioremap_resource(pdev, 0);
    if (IS_ERR(regs))
        return PTR_ERR(regs);

    fifo->map = devm_regmap_init_mmio(dev, regs, &axg_fifo_regmap_cfg);
    if (IS_ERR(fifo->map)) {
        dev_err(dev, "failed to init regmap: %ld\n",
                PTR_ERR(fifo->map));
        return PTR_ERR(fifo->map);
    }

    fifo->pclk = devm_clk_get(dev, NULL);
    if (IS_ERR(fifo->pclk)) {
        if (PTR_ERR(fifo->pclk) != -EPROBE_DEFER)
            dev_err(dev, "failed to get pclk: %ld\n",
                    PTR_ERR(fifo->pclk));
        return PTR_ERR(fifo->pclk);
    }

    fifo->arb = devm_reset_control_get_exclusive(dev, NULL);
    if (IS_ERR(fifo->arb)) {
        if (PTR_ERR(fifo->arb) != -EPROBE_DEFER)
            dev_err(dev, "failed to get arb reset: %ld\n",
                    PTR_ERR(fifo->arb));
        return PTR_ERR(fifo->arb);
    }

    fifo->irq = of_irq_get(dev->of_node, 0);
    if (fifo->irq <= 0) {
        dev_err(dev, "failed to get irq: %d\n", fifo->irq);
        return fifo->irq;
    }

    fifo->field_threshold =
            devm_regmap_field_alloc(dev, fifo->map, data->field_threshold);
    if (IS_ERR(fifo->field_threshold))
        return PTR_ERR(fifo->field_threshold);

    ret = of_property_read_string(dev->of_node, "sound-name-prefix", &fifo->name_prefix);
    if (ret) {
        dev_err(dev, "failed to get sound-name-prefix\n");
        return ret;
    }
    
    ret = of_property_read_u32(dev->of_node, "amlogic,fifo-depth",
                    &fifo->depth);
    if (ret) {
        /* Error out for anything but a missing property */
        if (ret != -EINVAL)
            return ret;
        /*
         * If the property is missing, it might be because of an old
         * DT. In such case, assume the smallest known fifo depth
         */
        fifo->depth = 256;
        dev_warn(dev, "fifo depth not found, assume %u bytes\n",
                fifo->depth);
    }
    
    fifo->dev = dev;
    
    ret = __fifo_pages_alloc(fifo, DMA_SIZE_MAX);
    if (ret) {
        dev_err(dev, "failed to alloc dma for %s\n", fifo->name_prefix);
        return ret;
    }
    
    if (fifo->is_frddr) {
        // clear "FRDDR_x SRC x EN"
        __fifo_update_bits(fifo, FIFO_CTRL0, 0x888, 0);
    }

    __fifo_register(fifo);

    dev_info(dev, "meson_axg_fifo_probe(%s, is_frddr=%d, is_g12a=%d) SUCCESS.\n", 
            fifo->name_prefix, fifo->is_frddr, fifo->is_g12a);
    return 0;
}

