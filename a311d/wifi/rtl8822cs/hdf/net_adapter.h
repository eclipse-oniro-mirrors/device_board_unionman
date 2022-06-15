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

#ifndef NET_ADAPTER_H
#define NET_ADAPTER_H

#include <drv_types.h>
#include "net_device.h"
#include "osdep_intf.h"

/* * Ask if a driver is ready to send */
#define WAL_SIOCDEVPRIVATE (0x89F0) /* SIOCDEVPRIVATE */
#define WAL_ADDR_MAX (16)

#define ETHER_ADDR_LEN MAC_ADDR_SIZE
#define WLAN_MAC_ADDR_LEN (6) /* MAC地址长度宏 */

typedef enum {
    WAL_PHY_MODE_11N = 0,
    WAL_PHY_MODE_11G = 1,
    WAL_PHY_MODE_11B = 2,
    WAL_PHY_MODE_BUTT
} wal_phy_mode;

typedef enum {
    WAL_ADDR_IDX_STA0 = 0,
    WAL_ADDR_IDX_AP0  = 1,
    WAL_ADDR_IDX_STA1 = 2,
    WAL_ADDR_IDX_STA2 = 3,
    WAL_ADDR_IDX_BUTT
} wal_addr_idx;

typedef struct {
    unsigned char  ac_addr[WLAN_MAC_ADDR_LEN];
    uint16_t us_status;
} wal_dev_addr_stru;

struct net_device* GetLinuxInfByNetDevice(const struct NetDevice *netDevice);

int32_t InitNetdev(struct NetDevice *netDevice);
int32_t DeinitNetdev(void);

int InitP2pNetDev(struct NetDevice *netDevice);
int DeinitP2pNetDev(void);

int32_t wal_init_drv_wlan_netdev(enum nl80211_iftype type, wal_phy_mode mode, NetDevice *netdev);
uint32_t wal_get_dev_addr(unsigned char *pc_addr, unsigned char addr_len, unsigned char type);
uint32_t rtl_macaddr_check(const unsigned char *mac_addr);
struct NetDevice* get_rtl_netdev(void);
void* get_rtl_priv_data(void);

int rtw_drv_entry(void);
int rtw_ndev_init(struct net_device *dev);
int rtw_xmit_entry(struct sk_buff *pkt, struct net_device *pnetdev);

int netdev_open(struct net_device *pnetdev);
int netdev_close(struct net_device *pnetdev);

#endif
