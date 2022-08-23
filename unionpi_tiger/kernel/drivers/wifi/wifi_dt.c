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

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/of_irq.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/rfkill.h>
#include <linux/of.h>
#include <linux/pinctrl/consumer.h>
#include <linux/amlogic/aml_gpio_consumer.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/amlogic/cpu_version.h>
#include <linux/amlogic/iomap.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/pwm.h>
#include <linux/pci.h>
#include <linux/gpio/consumer.h>
#include "dhd_static_buf.h"
#include "wifi_dt.h"

#define OWNER_NAME "sdio_wifi"

struct wifi_plat_info {
    int interrupt_pin;
    int irq_num;
    int irq_trigger_type;

    int power_on_pin;
    int power_on_pin_level;
    int power_on_pin_OD;
    int power_on_pin2;

    struct gpio_desc *interrupt_desc;
    struct gpio_desc *power_desc;

    int plat_info_valid;
    struct pinctrl *p;
    struct device *dev;
};

#define WIFI_POWER_DRIVER_NAME "wifi_power"
#define WIFI_POWER_DEVICE_NAME "wifi_power"
#define WIFI_POWER_CLASS_NAME "wifi_power"

#define WIFI_POWER_UP _IO('m', 1)
#define WIFI_POWER_DOWN _IO('m', 2)
#define USB_POWER_UP _IO('m', 3)
#define USB_POWER_DOWN _IO('m', 4)
#define SDIO_GET_DEV_TYPE _IO('m', 5)

#define BT_BIT 0
#define WIFI_BIT 1

static struct wifi_plat_info wifi_info;
static dev_t wifi_power_devno;
static struct cdev *wifi_power_cdev;
static struct device *devp;
struct wifi_power_platform_data *pdata;
static int power_flag;

static DEFINE_MUTEX(wifi_bt_mutex);

#define WIFI_INFO(fmt, args...) dev_info(wifi_info.dev, "[%s] " fmt, __func__, ##args)

#ifdef CONFIG_OF
static const struct of_device_id wifi_match[] = {
    {.compatible = "amlogic, wifi-dev", .data = (void *)&wifi_info},
    {},
};

static struct wifi_plat_info *wifi_get_driver_data(struct platform_device *pdev)
{
    const struct of_device_id *match;

    match = of_match_node(wifi_match, pdev->dev.of_node);
    if (!match) {
        return NULL;
    }
    return (struct wifi_plat_info *)match->data;
}
#else
#define wifi_match NULL
#endif

#define SHOW_PIN_OWN(pin_str, pin_num) WIFI_INFO("%s(%d)\n", pin_str, pin_num)

static int set_power(int value)
{
    if (!wifi_info.power_on_pin_OD) {
        if (wifi_info.power_on_pin_level) {
            return gpio_direction_output(wifi_info.power_on_pin, !value);
        } else {
            return gpio_direction_output(wifi_info.power_on_pin, value);
        }
    } else {
        if (wifi_info.power_on_pin_level) {
            if (value) {
                gpio_direction_input(wifi_info.power_on_pin);
            } else {
                gpio_direction_output(wifi_info.power_on_pin, 0);
            }
        } else {
            if (value) {
                gpio_direction_output(wifi_info.power_on_pin, 0);
            } else {
                gpio_direction_input(wifi_info.power_on_pin);
            }
        }
    }
    return 0;
}

static int set_power2(int value)
{
    if (wifi_info.power_on_pin_level) {
        return gpio_direction_output(wifi_info.power_on_pin2, !value);
    } else {
        return gpio_direction_output(wifi_info.power_on_pin2, value);
    }
}

static int set_wifi_power(int is_power)
{
    int ret = 0;

    if (is_power) {
        if (wifi_info.power_on_pin) {
            ret = set_power(1);
            if (ret) {
                WIFI_INFO("power up failed(%d)\n", ret);
            }
        }
        if (wifi_info.power_on_pin2) {
            ret = set_power2(1);
            if (ret) {
                WIFI_INFO("power2 up failed(%d)\n", ret);
            }
        }
    } else {
        if (wifi_info.power_on_pin) {
            ret = set_power(0);
            if (ret) {
                WIFI_INFO("power down failed(%d)\n", ret);
            }
        }
        if (wifi_info.power_on_pin2) {
            ret = set_power2(0);
            if (ret) {
                WIFI_INFO("power2 down failed(%d)\n", ret);
            }
        }
    }
    return ret;
}

static void wifi_power_control(int is_power, int shift)
{
    mutex_lock(&wifi_bt_mutex);
    if (is_power) {
        if (!power_flag) {
            set_wifi_power(is_power);
            WIFI_INFO("Set %s power on !\n", (shift ? "WiFi" : "BT"));
            msleep(200L);
            sdio_reinit();
        }
        power_flag |= (1 << shift);
        WIFI_INFO("Set %s power on !\n", (shift ? "WiFi" : "BT"));
    } else {
        power_flag &= ~(1 << shift);
        if (!power_flag) {
            set_wifi_power(is_power);
            WIFI_INFO("Set %s power down\n", (shift ? "WiFi" : "BT"));
        }
    }
    mutex_unlock(&wifi_bt_mutex);
}

void aml_set_bt_power(int is_power)
{
    wifi_power_control(is_power, BT_BIT);
}
EXPORT_SYMBOL(aml_set_bt_power);

void aml_set_wifi_power(int is_power)
{
    wifi_power_control(is_power, WIFI_BIT);
}
EXPORT_SYMBOL(aml_set_wifi_power);

static int wifi_power_open(struct inode *inode, struct file *file)
{
    struct cdev *cdevp = inode->i_cdev;
    file->private_data = cdevp;
    return 0;
}

static int wifi_power_release(struct inode *inode, struct file *file)
{
    return 0;
}

static long wifi_power_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    char dev_type[10] = {'\0'};

    switch (cmd) {
        case WIFI_POWER_UP:
            aml_set_wifi_power(0);
            mdelay(200L);
            aml_set_wifi_power(1);
            mdelay(200L);
            WIFI_INFO("ioctl wifi power up!\n");
            break;
        case WIFI_POWER_DOWN:
            aml_set_wifi_power(0);
            WIFI_INFO("ioctl wifi power down!\n");
            break;
        case USB_POWER_UP:
            WIFI_INFO(KERN_INFO "ioctl usb wifi power up!\n");
            break;
        case USB_POWER_DOWN:
            WIFI_INFO(KERN_INFO "ioctl usb wifi power down!\n");
            break;
        case SDIO_GET_DEV_TYPE: /* Now only support sdio */
            if (strlen("sdio") >= sizeof(dev_type)) {
                memcpy(dev_type, "sdio", (sizeof(dev_type) - 1));
            } else {
                memcpy(dev_type, "sdio", strlen("sdio"));
            }
            WIFI_INFO("wifi interface dev type: %s, length = %d\n", dev_type, (int)strlen(dev_type));
            if (copy_to_user((char __user *)arg, dev_type, strlen(dev_type))) {
                return -ENOTTY;
            }
            break;
        default:
            WIFI_INFO("usb wifi_power_ioctl: default !!!\n");
            return -EINVAL;
    }

    return 0;
}

static const struct file_operations wifi_power_fops = {
    .unlocked_ioctl = wifi_power_ioctl,
    .compat_ioctl = wifi_power_ioctl,
    .open = wifi_power_open,
    .release = wifi_power_release,
};

static struct class wifi_power_class = {
    .name = WIFI_POWER_CLASS_NAME,
    .owner = THIS_MODULE,
};

int wifi_set_up_power(void)
{
    int ret;
    /* setup power */
    if (wifi_info.power_on_pin) {
        ret = gpio_request(wifi_info.power_on_pin, OWNER_NAME);
        if (ret) {
            WIFI_INFO("power_on_pin request failed(%d)\n", ret);
        }
        if (wifi_info.power_on_pin_level) {
            ret = set_power(1);
        } else {
            ret = set_power(0);
        }
        if (ret) {
            WIFI_INFO("power_on_pin output failed(%d)\n", ret);
        }
        SHOW_PIN_OWN("WIFI: power_on_pin ", wifi_info.power_on_pin);
    }

    if (wifi_info.power_on_pin2) {
        ret = gpio_request(wifi_info.power_on_pin2, OWNER_NAME);
        if (ret) {
            WIFI_INFO("power_on_pin2 request failed(%d)\n", ret);
        }
        if (wifi_info.power_on_pin_level) {
            ret = set_power2(1);
        } else {
            ret = set_power2(0);
        }
        if (ret) {
            WIFI_INFO("power_on_pin2 output failed(%d)\n", ret);
        }
        SHOW_PIN_OWN("WIFI: power_on_pin2 ", wifi_info.power_on_pin2);
    }
    return ret;
}

int wifi_setup_dt(void)
{
    int ret;

    WIFI_INFO("wifi_setup_dt\n");
    if (!wifi_info.plat_info_valid) {
        WIFI_INFO("wifi_setup_dt : invalid device tree setting\n");
        return -1;
    }

    /* setup irq */
    if (wifi_info.interrupt_pin) {
        ret = gpio_request(wifi_info.interrupt_pin, OWNER_NAME);
        if (ret) {
            WIFI_INFO("interrupt_pin request failed(%d)\n", ret);
        }

        ret = gpio_direction_input(wifi_info.interrupt_pin);
        if (ret) {
            WIFI_INFO("set interrupt_pin input failed(%d)\n", ret);
        }

        wifi_info.irq_num = gpio_to_irq(wifi_info.interrupt_pin);
        if (wifi_info.irq_num) {
            WIFI_INFO("irq num is:(%d)\n", wifi_info.irq_num);
        }

        SHOW_PIN_OWN("interrupt_pin", wifi_info.interrupt_pin);
    }

    /* setup power */
    wifi_set_up_power();

    return 0;
}

void wifi_teardown_dt(void)
{
    WIFI_INFO("wifi_teardown_dt\n");

    if (!wifi_info.plat_info_valid) {
        WIFI_INFO("wifi_teardown_dt : invalid device tree setting\n");
        return;
    }

    if (wifi_info.power_on_pin) {
        gpio_free(wifi_info.power_on_pin);
    }

    if (wifi_info.power_on_pin2) {
        gpio_free(wifi_info.power_on_pin2);
    }

    if (wifi_info.interrupt_pin) {
        gpio_free(wifi_info.interrupt_pin);
    }
}

static int wifi_dev_probe(struct platform_device *pdev)
{
    int ret;
#ifdef CONFIG_OF
    struct wifi_plat_info *plat;
    const char *value;
    int desc;
#else
    struct wifi_plat_info *plat = (struct wifi_plat_info *)(pdev->dev.platform_data);
#endif

#ifdef CONFIG_OF
    if (pdev->dev.of_node) {
        plat = wifi_get_driver_data(pdev);
        plat->plat_info_valid = 0;
        plat->dev = &pdev->dev;

        ret = of_property_read_string(pdev->dev.of_node, "interrupt_pin", &value);
        if (ret) {
            plat->interrupt_pin = 0;
        } else {
            desc = of_get_named_gpio_flags(pdev->dev.of_node, "interrupt_pin", 0, NULL);
            plat->interrupt_desc = gpio_to_desc(desc);
            plat->interrupt_pin = desc;
    
            ret = of_property_read_string(pdev->dev.of_node, "irq_trigger_type", &value);
            if (ret) {
                WIFI_INFO("no irq_trigger_type");
                plat->irq_trigger_type = 0;
                return -1;
            }
    
            if (strcmp(value, "GPIO_IRQ_HIGH") == 0) {
                plat->irq_trigger_type = GPIO_IRQ_HIGH;
            } else if (strcmp(value, "GPIO_IRQ_LOW") == 0) {
                plat->irq_trigger_type = GPIO_IRQ_LOW;
            } else if (strcmp(value, "GPIO_IRQ_RISING") == 0) {
                plat->irq_trigger_type = GPIO_IRQ_RISING;
            } else if (strcmp(value, "GPIO_IRQ_FALLING") == 0) {
                plat->irq_trigger_type = GPIO_IRQ_FALLING;
            } else {
                WIFI_INFO("unknown irq trigger type-%s\n", value);
                return -1;
            }

            WIFI_INFO("interrupt_pin=%d\n", plat->interrupt_pin);
            WIFI_INFO("irq_num=%d, irq_trigger_type=%d\n", plat->irq_num, plat->irq_trigger_type);
        }

        ret = of_property_read_string(pdev->dev.of_node, "power_on_pin", &value);
        if (ret) {
            WIFI_INFO("no power_on_pin");
            plat->power_on_pin = 0;
            plat->power_on_pin_OD = 0;
        } else {
            desc = of_get_named_gpio_flags(pdev->dev.of_node, "power_on_pin", 0, NULL);
            plat->power_desc = gpio_to_desc(desc);
            plat->power_on_pin = desc;
        }

        ret = of_property_read_u32(pdev->dev.of_node, "power_on_pin_level", &plat->power_on_pin_level);
        if (ret) {
            plat->power_on_pin_level = 0;
        }

        ret = of_property_read_u32(pdev->dev.of_node, "power_on_pin_OD", &plat->power_on_pin_OD);
        if (ret) {
            plat->power_on_pin_OD = 0;
        }

        ret = of_property_read_string(pdev->dev.of_node, "power_on_pin2", &value);
        if (ret) {
            plat->power_on_pin2 = 0;
        } else {
            desc = of_get_named_gpio_flags(pdev->dev.of_node, "power_on_pin2", 0, NULL);
            plat->power_on_pin2 = desc;
        }

        if (of_get_property(pdev->dev.of_node, "dhd_static_buf", NULL)) {
            WIFI_INFO("dhd_static_buf setup\n");
        }

        plat->plat_info_valid = 1;
    }
#endif

    ret = alloc_chrdev_region(&wifi_power_devno, 0, 1, WIFI_POWER_DRIVER_NAME);
    if (ret < 0) {
        ret = -ENODEV;
        goto out;
    }

    ret = class_register(&wifi_power_class);
    if (ret < 0) {
        goto error1;
    }

    wifi_power_cdev = cdev_alloc();
    if (!wifi_power_cdev) {
        goto error2;
    }
    cdev_init(wifi_power_cdev, &wifi_power_fops);
    wifi_power_cdev->owner = THIS_MODULE;
    ret = cdev_add(wifi_power_cdev, wifi_power_devno, 1);
    if (ret) {
        goto error3;
    }

    devp = device_create(&wifi_power_class, NULL, wifi_power_devno, NULL, WIFI_POWER_DEVICE_NAME);
    if (IS_ERR(devp)) {
        ret = PTR_ERR(devp);
        goto error3;
    }
    devp->platform_data = pdata;

    wifi_setup_dt();

    return 0;
error3:
    cdev_del(wifi_power_cdev);
error2:
    class_unregister(&wifi_power_class);
error1:
    unregister_chrdev_region(wifi_power_devno, 1);
out:
    return ret;
}

static int wifi_dev_remove(struct platform_device *pdev)
{
    WIFI_INFO("wifi_dev_remove\n");
    wifi_teardown_dt();
    return 0;
}

static struct platform_driver wifi_plat_driver = {
    .probe = wifi_dev_probe,
    .remove = wifi_dev_remove,
    .driver = {.name = "wifi", .owner = THIS_MODULE, .of_match_table = wifi_match},
};

static int __init wifi_dt_init(void)
{
    int ret;

    ret = platform_driver_register(&wifi_plat_driver);
    return ret;
}

device_initcall_sync(wifi_dt_init);

static void __exit wifi_dt_exit(void)
{
    platform_driver_unregister(&wifi_plat_driver);
}
module_exit(wifi_dt_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("AlgoIdeas <yu19881234@163.com>");
MODULE_DESCRIPTION("Wifi device tree driver for Amlogic");

/**************** wifi mac *****************/
u8 WIFI_MAC[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static unsigned char chartonum(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
        return (c - 'A') + 10L;
    }
    if (c >= 'a' && c <= 'f') {
        return (c - 'a') + 10L;
    }
    return 0;
}

static int __init mac_addr_set(char *line)
{
    unsigned char mac[6];
    int i = 0;

    WIFI_INFO("try to wifi mac from emmc key!\n");
    for (i = 0; i < 6L && line[0] != '\0' && line[1] != '\0'; i++) {
        mac[i] = chartonum(line[0]) << 4L | chartonum(line[1]);
        line += 3L;
    }
    memcpy(WIFI_MAC, mac, 6L);
    WIFI_INFO("uboot setup mac-addr: %x:%x:%x:%x:%x:%x\n", WIFI_MAC[0], WIFI_MAC[1], WIFI_MAC[2L], WIFI_MAC[3L],
              WIFI_MAC[4L], WIFI_MAC[5L]);

    return 1;
}

__setup("mac_wifi=", mac_addr_set);

u8 *wifi_get_mac(void)
{
    return WIFI_MAC;
}
EXPORT_SYMBOL(wifi_get_mac);

void extern_wifi_set_enable(int is_on)
{
    if (is_on) {
        set_wifi_power(1);
        WIFI_INFO("WIFI  Enable! %d\n", wifi_info.power_on_pin);
    } else {
        set_wifi_power(0);
        WIFI_INFO("WIFI  Disable! %d\n", wifi_info.power_on_pin);
    }
}
EXPORT_SYMBOL(extern_wifi_set_enable);

int wifi_irq_num(void)
{
    return wifi_info.irq_num;
}
EXPORT_SYMBOL(wifi_irq_num);

int wifi_irq_trigger_level(void)
{
    return wifi_info.irq_trigger_type;
}
EXPORT_SYMBOL(wifi_irq_trigger_level);
MODULE_DESCRIPTION("Amlogic WIFI device tree driver");
MODULE_AUTHOR("AlgoIdeas <yu19881234@163.com>");
MODULE_LICENSE("GPL");
