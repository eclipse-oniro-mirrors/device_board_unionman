/*
 * drivers/amlogic/media/common/canvas/canvas.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/major.h>
#include <linux/io.h>
#include <linux/amlogic/cpu_version.h>
#include <linux/amlogic/media/old_cpu_version.h>
#include <linux/kernel.h>
#include <linux/irqflags.h>
#include <linux/of_address.h>
#include <linux/soc/amlogic/meson-canvas.h>

#include <linux/amlogic/media/canvas/canvas.h>


#define NUM_CANVAS 256

struct canvas_device_info {
	const char *device_name;
	spinlock_t lock;
	struct canvas_s canvasPool[NUM_CANVAS];
	struct meson_canvas *meson_canvas;
};

static struct canvas_device_info canvas_info;

#define CANVAS_VALID(n) ((n) < NUM_CANVAS && canvas_info.canvasPool[n].used)

#define canvas_lock(info, f, f2) do {\
		spin_lock_irqsave(&info->lock, f);\
		raw_local_save_flags(f2);\
		local_fiq_disable();\
	} while (0)

#define canvas_unlock(info, f, f2) do {\
		raw_local_irq_restore(f2);\
		spin_unlock_irqrestore(&info->lock, f);\
	} while (0)

static void
canvas_lut_data_build(ulong addr, u32 width, u32 height, u32 wrap, u32 blkmode,
	u32 endian, u32 *data_l, u32 *data_h)
{
	/*
	 *DMC_CAV_LUT_DATAL/DMC_CAV_LUT_DATAH
	 *high 32bits of cavnas data which need to be configured
	 *to canvas memory.
	 *64bits CANVAS look up table
	 *bit 61:58   Endian control.
	 *bit 61:  1 : switch 2 64bits data inside 128bits boundary.
	 *0 : no change.
	 *bit 60:  1 : switch 2 32bits data inside 64bits data boundary.
	 *0 : no change.
	 *bit 59:  1 : switch 2 16bits data inside 32bits data boundary.
	 *0 : no change.
	 *bit 58:  1 : switch 2 8bits data  inside 16bits data bournday.
	 *0 : no change.
	 *bit 57:56.   Canvas block mode.  2 : 64x32, 1: 32x32;
	 *0 : linear mode.
	 *bit 55:      canvas Y direction wrap control.
	 *1: wrap back in y.  0: not wrap back.
	 *bit 54:      canvas X direction wrap control.
	 *1: wrap back in X.  0: not wrap back.
	 *bit 53:41.   canvas Hight.
	 *bit 40:29.   canvas Width, unit: 8bytes. must in 32bytes boundary.
	 *that means last 2 bits must be 0.
	 *bit 28:0.    cavnas start address.   unit. 8 bytes. must be in
	 *32bytes boundary. that means last 2bits must be 0.
	 */

#define CANVAS_ADDR_LMASK	0x1fffffff
#define CANVAS_WIDTH_LMASK	0x7
#define CANVAS_WADDR_LBIT	0
#define CANVAS_WIDTH_LWID	3
#define CANVAS_WIDTH_LBIT	29
#define CANVAS_WIDTH_HBIT	0
#define CANVAS_HEIGHT_HBIT	(41 - 32)
#define CANVAS_WRAPX_HBIT	(54 - 32)
#define CANVAS_WRAPY_HBIT	(55 - 32)
#define CANVAS_BLKMODE_HBIT	(56 - 32)
#define CANVAS_ENDIAN_HBIT	(58 - 32)
#define CANVAS_HEIGHT_MASK	0x1fff
#define CANVAS_HEIGHT_BIT	9

	u32 addr_bits_l = ((addr + 7) >> 3 & CANVAS_ADDR_LMASK)
		<< CANVAS_WADDR_LBIT;

	u32 width_l = ((((width + 7) >> 3) & CANVAS_WIDTH_LMASK)
		<< CANVAS_WIDTH_LBIT);

	u32 width_h = ((((width + 7) >> 3) >> CANVAS_WIDTH_LWID)
		<< CANVAS_WIDTH_HBIT);

	u32 height_h = (height & CANVAS_HEIGHT_MASK)
		<< CANVAS_HEIGHT_BIT;

	u32 wrap_h = (wrap & (CANVAS_ADDR_WRAPX | CANVAS_ADDR_WRAPY));

	u32 blkmod_h = (blkmode & CANVAS_BLKMODE_MASK)
		<< CANVAS_BLKMODE_HBIT;

	u32 switch_bits_ctl = (endian & 0xf)
		<< CANVAS_ENDIAN_HBIT;

	*data_l = addr_bits_l | width_l;
	*data_h = width_h | height_h | wrap_h | blkmod_h | switch_bits_ctl;
}

static void canvas_config_locked(u32 index, struct canvas_s *p)
{
	struct canvas_device_info *info = &canvas_info;
	u32 datal, datah;

	canvas_lut_data_build(p->addr,
			p->width,
			p->height,
			p->wrap,
			p->blkmode,
			p->endian, &datal, &datah);

	meson_canvas_config(info->meson_canvas, (u8)index,
			p->addr, p->width, p->height,
			p->wrap, p->blkmode,
			p->endian);

	p->dataL = datal;
	p->dataH = datah;
}

void canvas_config_ex(u32 index, ulong addr, u32 width, u32 height, u32 wrap,
	u32 blkmode, u32 endian)
{
	struct canvas_device_info *info = &canvas_info;
	struct canvas_s *canvas;
	unsigned long flags, fiqflags;
	if (!CANVAS_VALID(index))
		return;

	canvas_lock(info, flags, fiqflags);
	canvas = &info->canvasPool[index];
	canvas->addr = addr;
	canvas->width = width;
	canvas->height = height;
	canvas->wrap = wrap;
	canvas->blkmode = blkmode;
	canvas->endian = endian;
	canvas_config_locked(index, canvas);
	canvas_unlock(info, flags, fiqflags);
}
EXPORT_SYMBOL(canvas_config_ex);

void canvas_config_config(u32 index, struct canvas_config_s *cfg)
{
	canvas_config_ex(index, cfg->phy_addr,
		cfg->width, cfg->height, CANVAS_ADDR_NOWRAP,
		cfg->block_mode, cfg->endian);
}
EXPORT_SYMBOL(canvas_config_config);

void canvas_config(u32 index, ulong addr, u32 width, u32 height, u32 wrap,
	u32 blkmode)
{
	return canvas_config_ex(index, addr, width, height, wrap, blkmode, 0);
}
EXPORT_SYMBOL(canvas_config);

void canvas_read(u32 index, struct canvas_s *p)
{
	struct canvas_device_info *info = &canvas_info;

	if (CANVAS_VALID(index))
		*p = info->canvasPool[index];
}
EXPORT_SYMBOL(canvas_read);

void canvas_copy(u32 src, u32 dst)
{
	struct canvas_device_info *info = &canvas_info;
	struct canvas_s *canvas_src = &info->canvasPool[src];
	struct canvas_s *canvas_dst = &info->canvasPool[dst];
	unsigned long flags, fiqflags;

	if (!CANVAS_VALID(src) || !CANVAS_VALID(dst))
		return;

	canvas_lock(info, flags, fiqflags);
	canvas_dst->addr = canvas_src->addr;
	canvas_dst->width = canvas_src->width;
	canvas_dst->height = canvas_src->height;
	canvas_dst->wrap = canvas_src->wrap;
	canvas_dst->blkmode = canvas_src->blkmode;
	canvas_dst->endian = canvas_src->endian;
	canvas_dst->dataH = canvas_src->dataH;
	canvas_dst->dataL = canvas_src->dataL;
	canvas_config_locked(dst, canvas_dst);
	canvas_unlock(info, flags, fiqflags);
}
EXPORT_SYMBOL(canvas_copy);

void canvas_update_addr(u32 index, u32 addr)
{
	struct canvas_device_info *info = &canvas_info;
	struct canvas_s *canvas;
	unsigned long flags, fiqflags;

	if (!CANVAS_VALID(index))
		return;
	canvas = &info->canvasPool[index];

	canvas_lock(info, flags, fiqflags);
	canvas->addr = addr;
	canvas_config_locked(index, canvas);
	canvas_unlock(info, flags, fiqflags);

	return;
}
EXPORT_SYMBOL(canvas_update_addr);

unsigned int canvas_get_addr(u32 index)
{
	struct canvas_device_info *info = &canvas_info;

	return info->canvasPool[index].addr;
}
EXPORT_SYMBOL(canvas_get_addr);

unsigned int canvas_get_width(u32 index)
{
	struct canvas_device_info *info = &canvas_info;

	if (!CANVAS_VALID(index))
		return 0;

	return info->canvasPool[index].width;
}
EXPORT_SYMBOL(canvas_get_width);

unsigned int canvas_get_height(u32 index)
{
	struct canvas_device_info *info = &canvas_info;

	if (!CANVAS_VALID(index))
		return 0;

	return info->canvasPool[index].height;
}
EXPORT_SYMBOL(canvas_get_height);


static int alloc_canvas_range(struct canvas_device_info *info, uint8_t start, uint8_t end)
{
	uint8_t i, j;
	for (i = start; i <= end; i++) {
		if (meson_canvas_alloc_spec(info->meson_canvas, i)) {
			// free before fail out.
			for (j = start; j < i; j++) {
				meson_canvas_free(info->meson_canvas, j);
				info->canvasPool[j].used = 0;
			}
			return -1;
		}
		info->canvasPool[i].used = 1;
	}

	pr_info("Alloc Canvas: [%u, %u]\n", start, end);
	
	return 0;
}

static void free_canvas_range(struct canvas_device_info *info, uint8_t start, uint8_t end)
{
	uint8_t i;
	for (i = start; i <= end; i++) {
		meson_canvas_free(info->meson_canvas, i);
		info->canvasPool[i].used = 0;
	}
}

static int canvas_probe(struct platform_device *pdev)
{
	struct canvas_device_info *info = &canvas_info;
	struct device *dev = &pdev->dev;
	
	memset(info, 0, sizeof(*info));

	info->meson_canvas = meson_canvas_get(dev);
	if (IS_ERR(info->meson_canvas)) {
		dev_err(dev, "meson_canvas_get() failed. check your dts\n");
		return -1;
	}
	
	spin_lock_init(&info->lock);

	// Pre-Alloc canvas for "VDec" : [0x78, 0xbf]
	if (alloc_canvas_range(info, 0x78, 0xbf)) {
		dev_err(dev, "alloc_canvas_range(0x78, 0xbf) failed\n");
		return -EBUSY;
	}
	
	// Pre-Alloc canvas for "VEnc" : [0xe4, 0xef]
	if (alloc_canvas_range(info, 0xe4, 0xef)) {
		dev_err(dev, "alloc_canvas_range(0xe4, 0xef) failed\n");
		return -EBUSY;
	}

	dev_info(dev, "canvas_legacy probe success\n");
	
	return 0;
}

/* static int __devexit canvas_remove(struct platform_device *pdev) */
static int canvas_remove(struct platform_device *pdev)
{
	struct canvas_device_info *info = &canvas_info;
	struct device *dev = &pdev->dev;

	// Free canvas
	free_canvas_range(info, 0x78, 0xbf);
	free_canvas_range(info, 0xe4, 0xef);
	
	dev_info(dev, "Canvas driver removed.\n");
	
	return 0;
}

static const struct of_device_id canvas_dt_match[] = {
	{
		.compatible = "amlogic, meson, canvas_legacy", 
	},
	{},
};

static struct platform_driver canvas_driver = {
	.probe = canvas_probe,
	.remove = canvas_remove,
	.driver = {
		.name = "amlogic-canvas-legacy",
		.of_match_table = canvas_dt_match,
	},
};

static int __init amcanvas_init(void)
{
	int r;
	
	r = platform_driver_register(&canvas_driver);
	if (r) {
			pr_err("Unable to register canvas driver\n");
			return r;
	}
	pr_info("register canvas platform driver\n");
	
	return 0;
}

static void __exit amcanvas_exit(void)
{
}

module_init(amcanvas_init);
module_exit(amcanvas_exit);

MODULE_DESCRIPTION("AMLOGIC Canvas management driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tim Yao <timyao@amlogic.com>");
