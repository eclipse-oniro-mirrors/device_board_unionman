/*
 * Copyright (C) 2022 Unionman Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/media-bus-format.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>

#include <video/display_timing.h>
#include <video/mipi_display.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

#include <linux/backlight.h>
#include <linux/device/class.h>

#define DRV_NAME "panel-chipone-icn6211"

struct icn6211 {
    struct device *dev;
    struct drm_panel panel;
    struct gpio_desc *reset_gpio;
    bool prepared;
    bool enabled;
    const struct icn6211_panel_desc *desc;
};

struct icn6211_panel_desc {
    const struct drm_display_mode *mode;
    unsigned int lanes;
    unsigned long mode_flags;
    enum mipi_dsi_pixel_format format;
    int (*init_sequence)(struct icn6211 *ctx);
};

/* I2C registers. */
#define REG_ID 0x80
#define REG_PORTA 0x81
#define REG_PORTA_HF BIT(2)
#define REG_PORTA_VF BIT(3)
#define REG_PORTB 0x82
#define REG_POWERON 0x85
#define REG_PWM 0x86

#define HACTIVE_LI 0x20
#define VACTIVE_LI 0x21
#define VACTIVE_HACTIVE_HI 0x22
#define HFP_LI 0x23
#define HSYNC_LI 0x24
#define HBP_LI 0x25
#define HFP_HSW_HBP_HI 0x26
#define VFP 0x27
#define VSYNC 0x28
#define VBP 0x29

/* DSI D-PHY Layer Registers */
#define D0W_DPHYCONTTX 0x0004
#define CLW_DPHYCONTRX 0x0020
#define D0W_DPHYCONTRX 0x0024
#define D1W_DPHYCONTRX 0x0028
#define COM_DPHYCONTRX 0x0038
#define CLW_CNTRL 0x0040
#define D0W_CNTRL 0x0044
#define D1W_CNTRL 0x0048
#define DFTMODE_CNTRL 0x0054

/* DSI PPI Layer Registers */
#define PPI_STARTPPI 0x0104
#define PPI_BUSYPPI 0x0108
#define PPI_LINEINITCNT 0x0110
#define PPI_LPTXTIMECNT 0x0114
#define PPI_CLS_ATMR 0x0140
#define PPI_D0S_ATMR 0x0144
#define PPI_D1S_ATMR 0x0148
#define PPI_D0S_CLRSIPOCOUNT 0x0164
#define PPI_D1S_CLRSIPOCOUNT 0x0168
#define CLS_PRE 0x0180
#define D0S_PRE 0x0184
#define D1S_PRE 0x0188
#define CLS_PREP 0x01A0
#define D0S_PREP 0x01A4
#define D1S_PREP 0x01A8
#define CLS_ZERO 0x01C0
#define D0S_ZERO 0x01C4
#define D1S_ZERO 0x01C8
#define PPI_CLRFLG 0x01E0
#define PPI_CLRSIPO 0x01E4
#define HSTIMEOUT 0x01F0
#define HSTIMEOUTENABLE 0x01F4

/* DSI Protocol Layer Registers */
#define DSI_STARTDSI 0x0204
#define DSI_BUSYDSI 0x0208
#define DSI_LANEENABLE 0x0210
#define DSI_LANEENABLE_CLOCK BIT(0)
#define DSI_LANEENABLE_D0 BIT(1)
#define DSI_LANEENABLE_D1 BIT(2)

#define DSI_LANESTATUS0 0x0214
#define DSI_LANESTATUS1 0x0218
#define DSI_INTSTATUS 0x0220
#define DSI_INTMASK 0x0224
#define DSI_INTCLR 0x0228
#define DSI_LPTXTO 0x0230
#define DSI_MODE 0x0260
#define DSI_PAYLOAD0 0x0268
#define DSI_PAYLOAD1 0x026C
#define DSI_SHORTPKTDAT 0x0270
#define DSI_SHORTPKTREQ 0x0274
#define DSI_BTASTA 0x0278
#define DSI_BTACLR 0x027C

/* DSI General Registers */
#define DSIERRCNT 0x0300
#define DSISIGMOD 0x0304

/* DSI Application Layer Registers */
#define APLCTRL 0x0400
#define APLSTAT 0x0404
#define APLERR 0x0408
#define PWRMOD 0x040C
#define RDPKTLN 0x0410
#define PXLFMT 0x0414
#define MEMWRCMD 0x0418

/* LCDC/DPI Host Registers */
#define LCDCTRL 0x0420
#define HSR 0x0424
#define HDISPR 0x0428
#define VSR 0x042C
#define VDISPR 0x0430
#define VFUEN 0x0434

/* DBI-B Host Registers */
#define DBIBCTRL 0x0440

/* SPI Master Registers */
#define SPICMR 0x0450
#define SPITCR 0x0454

/* System Controller Registers */
#define SYSSTAT 0x0460
#define SYSCTRL 0x0464
#define SYSPLL1 0x0468
#define SYSPLL2 0x046C
#define SYSPLL3 0x0470
#define SYSPMCTRL 0x047C

#define BRIGHTNESS_DEVICE_NAME "icn6211"
#define BRIGHTNESS_CLASS_NAME "brightness"

#define ICN6211_DSI(dsi, seq...)                                                                                       \
    do {                                                                                                               \
        const u8 d[] = {seq};                                                                                          \
        mipi_dsi_generic_write(dsi, d, ARRAY_SIZE(d));                                                                 \
    }while (0)

struct panel_icn6211_i2c {
    int brightness;
    struct i2c_client *i2c;
};

static struct panel_icn6211_i2c *icn6211_i2c = NULL;
static bool panel_icn6211_enable = false;

static int panel_icn6211_i2c_read(struct panel_icn6211_i2c *ts, u8 reg)
{
    return i2c_smbus_read_byte_data(ts->i2c, reg);
}

static void panel_icn6211_i2c_write(struct panel_icn6211_i2c *ts, u8 reg, u8 val)
{
    int ret;

    ret = i2c_smbus_write_byte_data(ts->i2c, reg, val);
    if (ret)
        dev_err(&ts->i2c->dev, "I2C write failed: %d\n", ret);
}

static int panel_icn6211_dsi_write(struct mipi_dsi_device *dsi, u16 reg, u32 val)
{
    u8 msg[] = {
        reg, reg >> 8L, val, val >> 8L, val >> 16L, val >> 24L,
    };

    mipi_dsi_generic_write(dsi, msg, sizeof(msg));

    return 0;
}

static inline struct icn6211 *panel_to_icn6211(struct drm_panel *panel)
{
    return container_of(panel, struct icn6211, panel);
}

static const struct drm_display_mode icn6211_mode = {
    .clock = 28344600L / 1000L,
    .hdisplay = 800L,
    .hsync_start = 800L + 16L,
    .hsync_end = 800L + 16L + 1,
    .htotal = 800L + 16L + 1 + 88L,
    .vdisplay = 480L,
    .vsync_start = 480L + 7L,
    .vsync_end = 480L + 7L + 3L,
    .vtotal = 480L + 7L + 3L + 32L,
    .width_mm = 105L,
    .height_mm = 67L,
};

struct icn6211_panel_desc icn6211_panel_desc = {
    .mode = &icn6211_mode,
    .lanes = 1,
    .mode_flags = (MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST | MIPI_DSI_MODE_LPM),
    .format = MIPI_DSI_FMT_RGB888,
};

static int icn6211_enable(struct drm_panel *panel)
{
    struct icn6211 *ctx = panel_to_icn6211(panel);
    struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
    int i;

    if (!panel_icn6211_enable) {
        return -1;
    }

    if (ctx->enabled) {
        dev_info(ctx->dev, "panel enabled\n");
        return 0;
    }

    dev_info(ctx->dev, "panel enable\n");

    panel_icn6211_i2c_write(icn6211_i2c, REG_POWERON, 1);

    /* Wait for nPWRDWN to go low to indicate poweron is done. */
    for (i = 0; i < 100L; i++) {
        if (panel_icn6211_i2c_read(icn6211_i2c, REG_PORTB) & 1)
            break;
    }

    panel_icn6211_dsi_write(dsi, DSI_LANEENABLE, DSI_LANEENABLE_CLOCK | DSI_LANEENABLE_D0);
    panel_icn6211_dsi_write(dsi, PPI_D0S_CLRSIPOCOUNT, 0x05);
    panel_icn6211_dsi_write(dsi, PPI_D1S_CLRSIPOCOUNT, 0x05);
    panel_icn6211_dsi_write(dsi, PPI_D0S_ATMR, 0x00);
    panel_icn6211_dsi_write(dsi, PPI_D1S_ATMR, 0x00);
    panel_icn6211_dsi_write(dsi, PPI_LPTXTIMECNT, 0x03);

    panel_icn6211_dsi_write(dsi, SPICMR, 0x00);
    panel_icn6211_dsi_write(dsi, LCDCTRL, 0x00100150);
    panel_icn6211_dsi_write(dsi, SYSCTRL, 0x040f);
    msleep(100L);

    panel_icn6211_dsi_write(dsi, PPI_STARTPPI, 0x01);
    panel_icn6211_dsi_write(dsi, DSI_STARTDSI, 0x01);
    msleep(100L);

    /* Turn on the backlight. */
    panel_icn6211_i2c_write(icn6211_i2c, REG_PWM, 255L);
    icn6211_i2c->brightness = 255L;

    /* Default to the same orientation as the closed source
     * firmware used for the panel.
     */
    panel_icn6211_i2c_write(icn6211_i2c, REG_PORTA, BIT(2L));

    ctx->enabled = true;

    return 0;
}

static int icn6211_disable(struct drm_panel *panel)
{
    struct icn6211 *ctx = panel_to_icn6211(panel);

    if (!panel_icn6211_enable) {
        return -1;
    }

    dev_info(ctx->dev, "panel disable\n");

    panel_icn6211_i2c_write(icn6211_i2c, REG_PWM, 0);
    panel_icn6211_i2c_write(icn6211_i2c, REG_POWERON, 0);

    ctx->enabled = false;

    return 0;
}

static int icn6211_unprepare(struct drm_panel *panel)
{
    if (!panel_icn6211_enable)
        return 0;

    struct icn6211 *ctx = panel_to_icn6211(panel);

    if (!ctx->prepared)
        return 0;

    if (ctx->reset_gpio != NULL)
        gpiod_set_value_cansleep(ctx->reset_gpio, 0);

    ctx->prepared = false;

    return 0;
}

static int icn6211_prepare(struct drm_panel *panel)
{
    int ret;
    struct icn6211 *ctx = panel_to_icn6211(panel);
    struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);

    if (!panel_icn6211_enable) {
        return -1;
    }

    if (ctx->prepared)
        return 0;

    ret = mipi_dsi_turn_on_peripheral(dsi);
    if (ret < 0) {
        dev_err(panel->dev, "failed to turn on peripheral: %d\n", ret);
        return ret;
    }

    ctx->prepared = true;

    return 0;
}

static int icn6211_get_modes(struct drm_panel *panel, struct drm_connector *connector)
{
    struct icn6211 *ctx = panel_to_icn6211(panel);
    struct drm_display_mode *mode;
    static const u32 bus_format = MEDIA_BUS_FMT_RGB888_1X24;

    if (!panel_icn6211_enable) {
        connector->status = connector_status_disconnected;
        return 0;
    }

    mode = drm_mode_duplicate(connector->dev, ctx->desc->mode);
    if (!mode) {
        dev_err(ctx->dev, "Failed to add mode %ux%u@%u\n", ctx->desc->mode->hdisplay, ctx->desc->mode->vdisplay,
                drm_mode_vrefresh(ctx->desc->mode));
        return -ENOMEM;
    }

    drm_mode_set_name(mode);

    mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
    connector->display_info.bpc = 8L;
    connector->display_info.width_mm = mode->width_mm;
    connector->display_info.height_mm = mode->height_mm;
    drm_mode_probed_add(connector, mode);

    drm_display_info_set_bus_formats(&connector->display_info, &bus_format, 1);

    return 1;
}

static const struct drm_panel_funcs icn6211_drm_funcs = {
    .disable = icn6211_disable,
    .unprepare = icn6211_unprepare,
    .prepare = icn6211_prepare,
    .enable = icn6211_enable,
    .get_modes = icn6211_get_modes,
};

static int icn6211_probe(struct mipi_dsi_device *dsi)
{
    struct device *dev = &dsi->dev;
    struct icn6211 *ctx;
    int ret;

    ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
    if (!ctx)
        return -ENOMEM;

    mipi_dsi_set_drvdata(dsi, ctx);

    ctx->dev = dev;
    ctx->desc = of_device_get_match_data(dev);

    dsi->mode_flags = ctx->desc->mode_flags;
    dsi->format = ctx->desc->format;
    dsi->lanes = ctx->desc->lanes;

    drm_panel_init(&ctx->panel, dev, &icn6211_drm_funcs, DRM_MODE_CONNECTOR_DSI);

    drm_panel_add(&ctx->panel);

    ret = mipi_dsi_attach(dsi);
    if (ret < 0) {
        dev_err(dev, "mipi_dsi_attach failed (%d). Is host ready?\n", ret);
        drm_panel_remove(&ctx->panel);
        return ret;
    }

    dev_info(dev, "%ux%u@%u %ubpp dsi %ulanes\n", ctx->desc->mode->hdisplay, ctx->desc->mode->vdisplay,
             drm_mode_vrefresh(ctx->desc->mode), mipi_dsi_pixel_format_to_bpp(dsi->format), dsi->lanes);

    return 0;
}

static void icn6211_shutdown(struct mipi_dsi_device *dsi)
{
    struct icn6211 *ctx = mipi_dsi_get_drvdata(dsi);
    int ret;

    ret = drm_panel_unprepare(&ctx->panel);
    if (ret < 0)
        dev_err(&dsi->dev, "Failed to unprepare panel: %d\n", ret);

    ret = drm_panel_disable(&ctx->panel);
    if (ret < 0)
        dev_err(&dsi->dev, "Failed to disable panel: %d\n", ret);
}

static int icn6211_remove(struct mipi_dsi_device *dsi)
{
    struct icn6211 *ctx = mipi_dsi_get_drvdata(dsi);
    int ret;

    icn6211_shutdown(dsi);

    ret = mipi_dsi_detach(dsi);
    if (ret < 0)
        dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

    drm_panel_remove(&ctx->panel);

    return 0;
}

static const struct of_device_id icn6211_of_match[] = {{.compatible = "chipone,icn6211", .data = &icn6211_panel_desc},
                                                       {}};
MODULE_DEVICE_TABLE(of, icn6211_of_match);

static struct mipi_dsi_driver icn6211_panel_driver = {
    .probe = icn6211_probe,
    .remove = icn6211_remove,
    .shutdown = icn6211_shutdown,
    .driver =
        {
            .name = DRV_NAME,
            .of_match_table = icn6211_of_match,
        },
};
module_mipi_dsi_driver(icn6211_panel_driver);

static ssize_t brightness_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    int brightness = 0;

    if (panel_icn6211_enable) {
        brightness = icn6211_i2c->brightness;
    }

    if ((brightness >= 0) && (brightness <= 255L)) {
        brightness = (brightness * 100L) / 255L;
    } else {
        brightness = 100L;
    }

    return snprintf(buf, 8L, "%d\n", brightness);
}

static ssize_t brightness_store(struct class *cla, struct class_attribute *attr, const char *buf, size_t count)
{
    int brightness;

    brightness = simple_strtoul(buf, NULL, 10L);
    if ((brightness >= 0) && (brightness <= 100L)) {
        brightness = (brightness * 255L) / 100L;
    } else {
        brightness = 255L;
    }

    if (panel_icn6211_enable) {
        if (brightness == 0) {
            if (icn6211_i2c->brightness != 0) {
                panel_icn6211_i2c_write(icn6211_i2c, REG_PWM, 0);
                panel_icn6211_i2c_write(icn6211_i2c, REG_POWERON, 0);
            }
        } else {
            if (icn6211_i2c->brightness == 0) {
                panel_icn6211_i2c_write(icn6211_i2c, REG_POWERON, 1);
                udelay(120L);
            }

            panel_icn6211_i2c_write(icn6211_i2c, REG_PWM, brightness);
            panel_icn6211_i2c_write(icn6211_i2c, REG_PORTA, BIT(2L));
        }

        icn6211_i2c->brightness = brightness;
    }

    return count;
}

static CLASS_ATTR_RW(brightness);

static struct attribute *icn6211_class_attrs[] = {&class_attr_brightness.attr, NULL};
ATTRIBUTE_GROUPS(icn6211_class);

static struct class icn6211_class = {
    .name = BRIGHTNESS_CLASS_NAME,
    .class_groups = icn6211_class_groups,
};

static int panel_icn6211_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
    struct device *dev = &i2c->dev;
    struct device *brightness_dev;
    int ret;
    int ver;

    icn6211_i2c = devm_kzalloc(dev, sizeof(*icn6211_i2c), GFP_KERNEL);
    if (!icn6211_i2c) {
        dev_err(dev, "devm_kzalloc failed!\n");
        return -ENOMEM;
    }

    i2c_set_clientdata(i2c, icn6211_i2c);

    icn6211_i2c->i2c = i2c;
    panel_icn6211_enable = false;

    ver = panel_icn6211_i2c_read(icn6211_i2c, 0x80);
    if (ver < 0) {
        dev_err(dev, "I2C read failed: %d\n", ver);
        return -ENODEV;
    }

    dev_info(dev, "icn6211 reg id 0x%x\n", ver);

    switch (ver) {
        case 0xde: /* ver 1 */
        case 0xc3: /* ver 2 */
            break;
        default:
            dev_err(dev, "Unknown firmware revision: 0x%02x\n", ver);
            return -ENODEV;
    }

    /* Turn off at boot, so we can cleanly sequence powering on. */
    panel_icn6211_enable = true;
    panel_icn6211_i2c_write(icn6211_i2c, REG_POWERON, 0);

    ret = class_register(&icn6211_class);
    if (ret < 0) {
        dev_warn(dev, "register icn6211 class fail! %d\n", ret);
    }

    brightness_dev = device_create(&icn6211_class, NULL, 0, NULL, BRIGHTNESS_DEVICE_NAME);
    if (IS_ERR_OR_NULL(brightness_dev)) {
        dev_err(dev, "create brightness device error\n");
        class_unregister(&icn6211_class);
    }

    return 0;
}

static int panel_icn6211_i2c_remove(struct i2c_client *i2c)
{
    return 0;
}

static const struct of_device_id panel_icn6211_i2c_of_ids[] = {
    {.compatible = "chipone,icn6211-i2c"}, {} /* sentinel */
};
MODULE_DEVICE_TABLE(of, panel_icn6211_i2c_of_ids);

static struct i2c_driver panel_icn6211_i2c_driver = {
    .driver =
        {
            .name = "panel-icn6211-i2c",
            .of_match_table = panel_icn6211_i2c_of_ids,
        },
    .probe = panel_icn6211_i2c_probe,
    .remove = panel_icn6211_i2c_remove,
};

static int __init panel_icn6211_i2c_init(void)
{
    return i2c_add_driver(&panel_icn6211_i2c_driver);
}
module_init(panel_icn6211_i2c_init);

static void __exit panel_icn6211_i2c_exit(void)
{
    i2c_del_driver(&panel_icn6211_i2c_driver);
}
module_exit(panel_icn6211_i2c_exit);

MODULE_AUTHOR("AlgoIdeas <yu19881234@163.com>");
MODULE_DESCRIPTION("DRM driver for chipone icn6211 based MIPI DSI panels");
MODULE_LICENSE("GPL v2");
