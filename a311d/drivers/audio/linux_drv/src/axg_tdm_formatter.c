/*
 * Copyright (c) 2022 Unionman Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <linux/clk.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/regmap.h>
#include <linux/reset.h>

#include "axg_tdm_formatter.h"

struct axg_tdm_formatter {
    struct list_head list_in_all;
	struct list_head list;
	const char *name_prefix;    /* such as TDMOUT_B, TDMIN_B */
	struct device *dev;
	struct axg_tdm_stream *stream;
	const struct axg_tdm_formatter_driver *drv;
	struct clk *pclk;
	struct clk *sclk;
	struct clk *lrclk;
	struct clk *sclk_sel;
	struct clk *lrclk_sel;
	struct reset_control *reset;
	bool enabled;
	bool powerup;
	struct regmap *map;
};

static LIST_HEAD(axg_tdm_formatter_list);

static int __tdm_formatter_register(struct axg_tdm_formatter *formatter)
{
    struct axg_tdm_formatter *f;
    list_for_each_entry(f, &axg_tdm_formatter_list, list_in_all) {
        if (!strcmp(f->name_prefix, formatter->name_prefix)) {
            dev_err(formatter->dev, "duplicated dev[%s], no need to register\n",
                    formatter->name_prefix);
            return -1;
        }
    }

    list_add_tail(&formatter->list_in_all, &axg_tdm_formatter_list);
    
    dev_info(formatter->dev, "Register Snd Formatter SUCCESS: %s\n", formatter->name_prefix);
    
    return 0;
}

static struct axg_tdm_formatter *__tdm_formatter_find(const char *name_prefix)
{
    struct axg_tdm_formatter *f;
    list_for_each_entry(f, &axg_tdm_formatter_list, list_in_all) {
        if (!strcmp(f->name_prefix, name_prefix)) {
            return f;
        }
    }

    return NULL;
}

static int __tdm_formatter_enable(struct axg_tdm_formatter *formatter)
{
	struct axg_tdm_stream *ts = formatter->stream;
	bool invert;
	int ret;

	/* Do nothing if the formatter is already enabled */
	if (formatter->enabled)
		return 0;

	/*
	 * On the g12a (and possibly other SoCs), when a stream using
	 * multiple lanes is restarted, it will sometimes not start
	 * from the first lane, but randomly from another used one.
	 * The result is an unexpected and random channel shift.
	 *
	 * The hypothesis is that an HW counter is not properly reset
	 * and the formatter simply starts on the lane it stopped
	 * before. Unfortunately, there does not seems to be a way to
	 * reset this through the registers of the block.
	 *
	 * However, the g12a has indenpendent reset lines for each audio
	 * devices. Using this reset before each start solves the issue.
	 */
	ret = reset_control_reset(formatter->reset);
	if (ret)
		return ret;

	/*
	 * If sclk is inverted, it means the bit should latched on the
	 * rising edge which is what our HW expects. If not, we need to
	 * invert it before the formatter.
	 */
	invert = axg_tdm_sclk_invert(ts->iface->fmt);
	ret = clk_set_phase(formatter->sclk, invert ? 0 : 180);
	if (ret)
		return ret;

	/* Setup the stream parameter in the formatter */
	ret = formatter->drv->ops->prepare(formatter->map,
					   formatter->drv->quirks,
					   formatter->stream);
	if (ret)
		return ret;

	/* Enable the signal clocks feeding the formatter */
	ret = clk_prepare_enable(formatter->sclk);
	if (ret)
		return ret;

	ret = clk_prepare_enable(formatter->lrclk);
	if (ret) {
		clk_disable_unprepare(formatter->sclk);
		return ret;
	}

	/* Finally, actually enable the formatter */
	formatter->drv->ops->enable(formatter->map);
	formatter->enabled = true;

	return 0;
}

static void __tdm_formatter_disable(struct axg_tdm_formatter *formatter)
{
	/* Do nothing if the formatter is already disabled */
	if (!formatter->enabled)
		return;

	formatter->drv->ops->disable(formatter->map);
	clk_disable_unprepare(formatter->lrclk);
	clk_disable_unprepare(formatter->sclk);
	formatter->enabled = false;
}

static int __tdm_formatter_attach(struct axg_tdm_formatter *formatter)
{
	struct axg_tdm_stream *ts = formatter->stream;
	int ret = 0;

	mutex_lock(&ts->lock);

	/* Catch up if the stream is already running when we attach */
	if (ts->ready) {
		ret = __tdm_formatter_enable(formatter);
		if (ret) {
			pr_err("failed to enable formatter\n");
			goto out;
		}
	}

	list_add_tail(&formatter->list, &ts->formatter_list);
out:
	mutex_unlock(&ts->lock);
	return ret;
}

static void __tdm_formatter_dettach(struct axg_tdm_formatter *formatter)
{
	struct axg_tdm_stream *ts = formatter->stream;

	mutex_lock(&ts->lock);
	list_del(&formatter->list);
	mutex_unlock(&ts->lock);

	__tdm_formatter_disable(formatter);
}

int axg_tdm_formatter_set_channel_masks(struct regmap *map,
					struct axg_tdm_stream *ts,
					unsigned int offset)
{
	unsigned int val, ch = ts->channels;
	unsigned long mask;
	int i, j;

	/*
	 * Distribute the channels of the stream over the available slots
	 * of each TDM lane
	 */
	for (i = 0; i < AXG_TDM_NUM_LANES; i++) {
		val = 0;
		mask = ts->mask[i];

		for (j = find_first_bit(&mask, 32);
		     (j < 32) && ch;
		     j = find_next_bit(&mask, 32, j + 1)) {
			val |= 1 << j;
			ch -= 1;
		}

		regmap_write(map, offset, val);
		offset += regmap_get_reg_stride(map);
	}

	/*
	 * If we still have channel left at the end of the process, it means
	 * the stream has more channels than we can accommodate and we should
	 * have caught this earlier.
	 */
	if (WARN_ON(ch != 0)) {
		pr_err("channel mask error\n");
		return -EINVAL;
	}

	return 0;
}

int axg_tdm_formatter_power_up(struct axg_tdm_formatter *formatter,
				      struct axg_tdm_stream *ts)
{
    int ret;

    if (formatter->powerup)
        return 0;
        
    /* Clock our device */
    ret = clk_prepare_enable(formatter->pclk);
    if (ret)
        return ret;

    /* Reparent the bit clock to the TDM interface */
    ret = clk_set_parent(formatter->sclk_sel, ts->iface->sclk);
    if (ret)
        goto disable_pclk;

    /* Reparent the sample clock to the TDM interface */
    ret = clk_set_parent(formatter->lrclk_sel, ts->iface->lrclk);
    if (ret)
        goto disable_pclk;

    formatter->stream = ts;
    ret = __tdm_formatter_attach(formatter);
    if (ret)
        goto disable_pclk;

    formatter->powerup = true;
    return 0;

disable_pclk:
    clk_disable_unprepare(formatter->pclk);
    return ret;
}

void axg_tdm_formatter_power_down(struct axg_tdm_formatter *formatter)
{
    if (!formatter->powerup)
        return;

    __tdm_formatter_dettach(formatter);
    clk_disable_unprepare(formatter->pclk);
    formatter->stream = NULL;
    formatter->powerup = false;
}

int axg_tdm_formatter_src_in_sel(struct axg_tdm_formatter *formatter,
                        unsigned int index)
{
    int ret = -1;
    if (formatter->drv->ops->src_in_sel) {
        ret = formatter->drv->ops->src_in_sel(formatter->map, index);
    }
    return ret;
}

int axg_tdm_formatter_sink_out_sel(struct axg_tdm_formatter *formatter,
                        unsigned int index)
{
    int ret = -1;
    if (formatter->drv->ops->sink_out_sel) {
        ret = formatter->drv->ops->sink_out_sel(formatter->map, index);
    }
    return ret;
}

struct axg_tdm_formatter *axg_tdm_formatter_get(const char *name_prefix)
{
    return __tdm_formatter_find(name_prefix);
}

int axg_tdm_formatter_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	const struct axg_tdm_formatter_driver *drv;
	struct axg_tdm_formatter *formatter;
	void __iomem *regs;
	int ret;

	drv = of_device_get_match_data(dev);
	if (!drv) {
		dev_err(dev, "failed to match device\n");
		return -ENODEV;
	}

	formatter = devm_kzalloc(dev, sizeof(*formatter), GFP_KERNEL);
	if (!formatter)
		return -ENOMEM;
	platform_set_drvdata(pdev, formatter);
	formatter->drv = drv;

	regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(regs))
		return PTR_ERR(regs);

	formatter->map = devm_regmap_init_mmio(dev, regs, drv->regmap_cfg);
	if (IS_ERR(formatter->map)) {
		dev_err(dev, "failed to init regmap: %ld\n",
			PTR_ERR(formatter->map));
		return PTR_ERR(formatter->map);
	}

	/* Peripharal clock */
	formatter->pclk = devm_clk_get(dev, "pclk");
	if (IS_ERR(formatter->pclk)) {
		ret = PTR_ERR(formatter->pclk);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "failed to get pclk: %d\n", ret);
		return ret;
	}

	/* Formatter bit clock */
	formatter->sclk = devm_clk_get(dev, "sclk");
	if (IS_ERR(formatter->sclk)) {
		ret = PTR_ERR(formatter->sclk);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "failed to get sclk: %d\n", ret);
		return ret;
	}

	/* Formatter sample clock */
	formatter->lrclk = devm_clk_get(dev, "lrclk");
	if (IS_ERR(formatter->lrclk)) {
		ret = PTR_ERR(formatter->lrclk);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "failed to get lrclk: %d\n", ret);
		return ret;
	}

	/* Formatter bit clock input multiplexer */
	formatter->sclk_sel = devm_clk_get(dev, "sclk_sel");
	if (IS_ERR(formatter->sclk_sel)) {
		ret = PTR_ERR(formatter->sclk_sel);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "failed to get sclk_sel: %d\n", ret);
		return ret;
	}

	/* Formatter sample clock input multiplexer */
	formatter->lrclk_sel = devm_clk_get(dev, "lrclk_sel");
	if (IS_ERR(formatter->lrclk_sel)) {
		ret = PTR_ERR(formatter->lrclk_sel);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "failed to get lrclk_sel: %d\n", ret);
		return ret;
	}

	/* Formatter dedicated reset line */
	formatter->reset = devm_reset_control_get_optional_exclusive(dev, NULL);
	if (IS_ERR(formatter->reset)) {
		ret = PTR_ERR(formatter->reset);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "failed to get reset: %d\n", ret);
		return ret;
	}

    ret = of_property_read_string(dev->of_node, "sound-name-prefix", &formatter->name_prefix);
    if (ret) {
        dev_err(dev, "failed to get sound-name-prefix\n");
        return ret;
    }

    formatter->dev = dev;

	__tdm_formatter_register(formatter);

	return 0;
}

int axg_tdm_stream_start(struct axg_tdm_stream *ts)
{
	struct axg_tdm_formatter *formatter;
	int ret = 0;

	mutex_lock(&ts->lock);
	ts->ready = true;

	/* Start all the formatters attached to the stream */
	list_for_each_entry(formatter, &ts->formatter_list, list) {
		ret = __tdm_formatter_enable(formatter);
		if (ret) {
			pr_err("failed to start tdm stream\n");
			goto out;
		}
	}

out:
	mutex_unlock(&ts->lock);
	return ret;
}

void axg_tdm_stream_stop(struct axg_tdm_stream *ts)
{
	struct axg_tdm_formatter *formatter;

	mutex_lock(&ts->lock);
	ts->ready = false;

	/* Stop all the formatters attached to the stream */
	list_for_each_entry(formatter, &ts->formatter_list, list) {
		__tdm_formatter_disable(formatter);
	}

	mutex_unlock(&ts->lock);
}

struct axg_tdm_stream *axg_tdm_stream_alloc(struct axg_tdm_iface *iface)
{
	struct axg_tdm_stream *ts;

	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (ts) {
		INIT_LIST_HEAD(&ts->formatter_list);
		mutex_init(&ts->lock);
		ts->iface = iface;
	}

	return ts;
}

void axg_tdm_stream_free(struct axg_tdm_stream *ts)
{
	/*
	 * If the list is not empty, it would mean that one of the formatter
	 * widget is still powered and attached to the interface while we
	 * are removing the TDM DAI. It should not be possible
	 */
	WARN_ON(!list_empty(&ts->formatter_list));
	mutex_destroy(&ts->lock);
	kfree(ts);
}

