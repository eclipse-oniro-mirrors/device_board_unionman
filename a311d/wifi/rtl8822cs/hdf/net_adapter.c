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

#include <net/cfg80211.h>
#include <linux/netdevice.h>
#include "net_device.h"
#include "securec.h"
#include "hdf_base.h"
#include "hdf_wlan_utils.h"
#include "net_adapter.h"

#define OSAL_ERR_CODE_PTR_NULL 100

#ifdef _PRE_WLAN_FEATURE_MESH
#include "dmac_ext_if.h"
#include "hmac_vap.h"
#include "hmac_user.h"
#endif

#ifdef _PRE_WLAN_FEATURE_P2P
#include "wal_linux_cfg80211.h"
#endif
#include "eapol.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define HDF_LOG_TAG RTL8822CS

#define MAC_NET_DEVICE_NAME_LENGTH (16)

#define WIFI_SHIFT_BIT (8)
#define WIFI_IFNAME_MAX_SIZE (16)

struct NetDevice *g_hdf_netdev = NULL;
struct net_device *g_linux_netdev = NULL;
unsigned char g_efuseMacExist = 0;
unsigned char g_macSet = 1;

void set_krn_netdev(struct net_device *netdev)
{
    g_linux_netdev = (struct net_device *)netdev;
}

void set_efuse_mac_exist(unsigned char exist)
{
    g_efuseMacExist = exist;
}

unsigned char get_efuse_mac_exist(void)
{
    return g_efuseMacExist;
}

ProcessingResult hdf_rtl8822cs_netdev_specialethertypeprocess(const struct NetDevice *netDev, NetBuf *buff)
{
    struct EtherHeader *header = NULL;
    const struct Eapol *eapolInstance = NULL;
    int ret = HDF_SUCCESS;
    uint16_t protocol;
    uint16_t etherType;
    const int shift = 8;
    const int pidx0 = 12, pidx1 = 13;

    if (netDev == NULL || buff == NULL) {
        return PROCESSING_ERROR;
    }

    header = (struct EtherHeader *)NetBufGetAddress(buff, E_DATA_BUF);
    etherType = ntohs(header->etherType);
    protocol = (buff->data[pidx0] << shift) | buff->data[pidx1];

    if (protocol != ETHER_TYPE_PAE) {
        return PROCESSING_CONTINUE;
    }
    if (netDev->specialProcPriv == NULL) {
        HDF_LOGE("%s: return PROCESSING_ERROR", __func__);
        return PROCESSING_ERROR;
    }

    eapolInstance = EapolGetInstance();
    ret = eapolInstance->eapolOp->writeEapolToQueue(netDev, buff);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: writeEapolToQueue failed", __func__);
        NetBufFree(buff);
    }

    return PROCESSING_COMPLETE;
}

void hdf_rtl8822cs_netdev_linkstatuschanged(struct NetDevice *netDev)
{
    HDF_LOGE("%s: start...", __func__);
    (void)netDev;
}

int32_t hdf_rtl8822cs_netdev_changemtu(struct NetDevice *netDev, int32_t mtu)
{
    int32_t retVal = 0;
    struct net_device *netdev = GetLinuxInfByNetDevice(netDev);
    HDF_LOGE("%s: start...", __func__);

    if (netdev == NULL) {
        HDF_LOGE("%s: netdev null!", __func__);
        return HDF_FAILURE;
    }
    HDF_LOGE("%s: change mtu to %d\n", __FUNCTION__, mtu);

    return retVal;
}

uint32_t hdf_rtl8822cs_netdev_netifnotify(struct NetDevice *netDev, NetDevNotify *notify)
{
    HDF_LOGE("%s: start...", __func__);
    (void)netDev;
    (void)notify;
    return HDF_SUCCESS;
}

void hdf_rtl8822cs_netdev_setnetifstats(struct NetDevice *netDev, NetIfStatus status)
{
    HDF_LOGE("%s: start...", __func__);
    (void)netDev;
    (void)status;
}

uint16_t hdf_rtl8822cs_netdev_selectqueue(struct NetDevice *netDev, NetBuf *netBuff)
{
    HDF_LOGE("%s: start...", __func__);
    (void)netDev;
    (void)netBuff;
    return HDF_SUCCESS;
}

struct NetDevStats *hdf_rtl8822cs_netdev_getstats(struct NetDevice *netDev)
{
    static struct NetDevStats devStat = {0};
    struct net_device_stats *kdevStat = NULL;
    struct net_device *netdev = GetLinuxInfByNetDevice(netDev);

    HDF_LOGE("%s: start...", __func__);

    if (netdev == NULL) {
        HDF_LOGE("%s: netDev null!", __func__);
        return NULL;
    }

    kdevStat = rtw_net_get_stats(netdev);
    if (kdevStat == NULL) {
        HDF_LOGE("%s: ndo_get_stats return null!", __func__);
        return NULL;
    }

    devStat.rxPackets = kdevStat->rx_packets;
    devStat.txPackets = kdevStat->tx_packets;
    devStat.rxBytes = kdevStat->rx_bytes;
    devStat.txBytes = kdevStat->tx_bytes;
    devStat.rxErrors = kdevStat->rx_errors;
    devStat.txErrors = kdevStat->tx_errors;
    devStat.rxDropped = kdevStat->rx_dropped;
    devStat.txDropped = kdevStat->tx_dropped;

    return &devStat;
}

int32_t hdf_rtl8822cs_netdev_setmacaddr(struct NetDevice *netDev, void *addr)
{
    int32_t retVal = 0;
    struct net_device *netdev = GetLinuxInfByNetDevice(netDev);

    HDF_LOGE("%s: start...", __func__);

    if (netdev == NULL || addr == NULL) {
        HDF_LOGE("%s: netDev or addr null!", __func__);
        return HDF_FAILURE;
    }

    retVal = (int32_t)rtw_cfg80211_monitor_if_set_mac_address(netdev, addr);
    if (retVal < 0) {
        HDF_LOGE("%s: hdf net device setmacaddr failed! ret = %d", __func__, retVal);
    }

    return retVal;
}

int32_t hdf_rtl8822cs_netdev_ioctl(struct NetDevice *netDev, IfReq *req, int32_t cmd)
{
    int32_t retVal = 0;
    struct ifreq dhd_req = {0};
    struct net_device *netdev = GetLinuxInfByNetDevice(netDev);

    HDF_LOGE("%s: start...", __func__);

    if (netdev == NULL || req == NULL) {
        HDF_LOGE("%s: netdev or req null!", __func__);
        return HDF_FAILURE;
    }

    dhd_req.ifr_ifru.ifru_data = req->ifrData;

    retVal = (int32_t)rtw_ioctl(netdev, &dhd_req, cmd);
    if (retVal < 0) {
        HDF_LOGE("%s: hdf net device ioctl failed! ret = %d", __func__, retVal);
    }

    return retVal;
}

int32_t hdf_rtl8822cs_netdev_xmit(struct NetDevice *netDev, NetBuf *netBuff)
{
    int32_t retVal = 0;
    struct net_device *netdev = GetLinuxInfByNetDevice(netDev);

    if (netdev == NULL || netBuff == NULL) {
        HDF_LOGE("%s: netdev or netBuff null!", __func__);
        return HDF_FAILURE;
    }

    retVal = (int32_t)rtw_xmit_entry((struct sk_buff *)netBuff, netdev);
    if (retVal < 0) {
        HDF_LOGE("%s: hdf net device xmit failed! ret = %d", __func__, retVal);
    }

    return retVal;
}

int32_t hdf_rtl8822cs_netdev_stop(struct NetDevice *netDev)
{
    int32_t retVal = 0;
    struct net_device *netdev = GetLinuxInfByNetDevice(netDev);

    HDF_LOGE("%s: start...", __func__);

    if (netdev == NULL) {
        HDF_LOGE("%s: netDev null!", __func__);
        return HDF_FAILURE;
    }

    retVal = (int32_t)netdev_close(netdev);
    if (retVal < 0) {
        HDF_LOGE("%s: hdf net device stop failed! ret = %d", __func__, retVal);
    }

    return retVal;
}

int32_t hdf_rtl8822cs_netdev_open(struct NetDevice *netDev)
{
    int32_t retVal = 0;
    const int idx0 = 0, idx1 = 1, idx2 = 2, idx3 = 3, idx4 = 4, idx5 = 5;
    struct net_device *netdev = GetLinuxInfByNetDevice(netDev);

    if (netdev == NULL) {
        HDF_LOGE("%s: netDev null!", __func__);
        return HDF_FAILURE;
    }

    HDF_LOGE("%s: ndo_open...", __func__);

    retVal = (int32_t)netdev_open(netdev);
    if (retVal < 0) {
        HDF_LOGE("%s: netdev_open fail!", __func__);
    }

    netDev->ieee80211Ptr = netdev->ieee80211_ptr;
    if (netDev->ieee80211Ptr == NULL) {
        HDF_LOGE("%s: NULL == netDev->ieee80211Ptr", __func__);
    }

    // update mac addr to NetDevice object
    memcpy_s(netDev->macAddr, MAC_ADDR_SIZE, netdev->dev_addr, netdev->addr_len);
    HDF_LOGE("%s: %02x:%02x:%02x:%02x:%02x:%02x", __func__,
        netDev->macAddr[idx0], netDev->macAddr[idx1], netDev->macAddr[idx2],
        netDev->macAddr[idx3], netDev->macAddr[idx4], netDev->macAddr[idx5]);

    return retVal;
}

void hdf_rtl8822cs_netdev_deinit(struct NetDevice *netDev)
{
    HDF_LOGE("%s: start...", __func__);
    (void)netDev;
}

int32_t hdf_rtl8822cs_netdev_init(struct NetDevice *netDev);

struct NetDeviceInterFace g_wal_rtl_net_dev_ops = {
    .init = hdf_rtl8822cs_netdev_init,
    .deInit = hdf_rtl8822cs_netdev_deinit,
    .open = hdf_rtl8822cs_netdev_open,
    .stop = hdf_rtl8822cs_netdev_stop,
    .xmit = hdf_rtl8822cs_netdev_xmit,
    .ioctl = hdf_rtl8822cs_netdev_ioctl,
    .setMacAddr = hdf_rtl8822cs_netdev_setmacaddr,
    .getStats = hdf_rtl8822cs_netdev_getstats,
    .setNetIfStatus = hdf_rtl8822cs_netdev_setnetifstats,
    .selectQueue = hdf_rtl8822cs_netdev_selectqueue,
    .netifNotify = hdf_rtl8822cs_netdev_netifnotify,
    .changeMtu = hdf_rtl8822cs_netdev_changemtu,
    .linkStatusChanged = hdf_rtl8822cs_netdev_linkstatuschanged,
    .specialEtherTypeProcess = hdf_rtl8822cs_netdev_specialethertypeprocess,
};

struct NetDeviceInterFace *rtl_get_net_dev_ops(void)
{
    return &g_wal_rtl_net_dev_ops;
}

int32_t hdf_rtl8822cs_netdev_init(struct NetDevice *netDev)
{
    HDF_LOGE("%s: start...", __func__);
    if (netDev == NULL) {
        HDF_LOGE("%s: netDev null!", __func__);
        return HDF_FAILURE;
    }

    HDF_LOGE("%s: netDev->name:%s\n", __func__, netDev->name);
    netDev->netDeviceIf = rtl_get_net_dev_ops();
    CreateEapolData(netDev);

    return HDF_SUCCESS;
}

void* get_rtl_priv_data(void)
{
    return g_hdf_netdev->mlPriv;
}

struct NetDevice* get_rtl_netdev(void)
{
    return g_hdf_netdev;
}

int32_t InitNetdev(struct NetDevice *netDevice)
{
    uint32_t i;
    int8_t vap_netdev_name[2][MAC_NET_DEVICE_NAME_LENGTH] = {"wlan0", "p2p0"};
    enum nl80211_iftype vap_types[2] = {NL80211_IFTYPE_STATION, NL80211_IFTYPE_P2P_DEVICE};
    struct NetDevice *netdev = NULL;
    struct HdfWifiNetDeviceData *data = NULL;
    void *netdevLinux = NULL;

    if (netDevice == NULL) {
        HDF_LOGE("%s:para is null!", __func__);
        return HDF_FAILURE;
    }

    netdevLinux = GetLinuxInfByNetDevice(netDevice);
    if (netdevLinux == NULL) {
        HDF_LOGE("%s net_device is null!", __func__);
        return HDF_FAILURE;
    }
    set_krn_netdev(netdevLinux);

    data = GetPlatformData(netDevice);
    if (data == NULL) {
        HDF_LOGE("%s:netdevice data null!", __func__);
        return HDF_FAILURE;
    }

    hdf_rtl8822cs_netdev_init(netDevice);

    for (i = 0; i < (sizeof(vap_netdev_name) / sizeof(vap_netdev_name[0])); i++) {
        netdev = NetDeviceGetInstByName((const int8_t *)vap_netdev_name[i]);
        if (netdev == NULL) {
            HDF_LOGE("%s:get dev by name failed! %s", __func__, vap_netdev_name[i]);
            continue;
        }

        netdev->classDriverName = netDevice->classDriverName;
        netdev->classDriverPriv = data;
    }

    g_hdf_netdev = netDevice;
    NetDeviceAdd(get_rtl_netdev());

    rtw_drv_entry();

    return HDF_SUCCESS;
}

int32_t DeinitNetdev(void)
{
    HDF_LOGE("DeinitNetdev rtl8822cs.\n");
    return HDF_SUCCESS;
}

struct NetDeviceInterFace *rtl_get_net_p2p_dev_ops(void);

static int32_t hdf_p2p_netdev_init(struct NetDevice *netDev)
{
    HDF_LOGE("%s: start %s...", __func__, netDev->name);

    if (netDev == NULL) {
        HDF_LOGE("%s: netDev null!", __func__);
        return HDF_FAILURE;
    }

    HDF_LOGE("%s: netDev->name:%s\n", __func__, netDev->name);
    netDev->netDeviceIf = rtl_get_net_p2p_dev_ops();
    CreateEapolData(netDev);

    return HDF_SUCCESS;
}

static void hdf_p2p_netdev_deinit(struct NetDevice *netDev)
{
    HDF_LOGE("%s: start...", __func__);
    (void)netDev;
}

static int32_t hdf_p2p_netdev_open(struct NetDevice *netDevice)
{
    int32_t retVal = 0;
    struct net_device *netdev = GetLinuxInfByNetDevice(netDevice);

    if (netdev == NULL) {
        HDF_LOGE("%s: netDev null!", __func__);
        return HDF_FAILURE;
    }

    HDF_LOGE("%s: ndo_open %s...", __func__, netDevice->name);

    retVal = (int32_t)netdev_open(netdev);
    if (retVal < 0) {
        HDF_LOGE("%s: hdf net device open failed! ret = %d", __func__, retVal);
    }

    netDevice->ieee80211Ptr = netdev->ieee80211_ptr;
    if (netDevice->ieee80211Ptr == NULL) {
        HDF_LOGE("%s: NULL == netDevice->ieee80211Ptr", __func__);
    }

    // update mac addr to NetDevice object
    memcpy_s(netDevice->macAddr, MAC_ADDR_SIZE, netdev->dev_addr, netdev->addr_len);

    return retVal;
}

static int32_t hdf_p2p_netdev_stop(struct NetDevice *netDev)
{
    HDF_LOGE("%s: stop %s...", __func__, netDev->name);
    return HDF_SUCCESS;
}

static int32_t hdf_p2p_netdev_xmit(struct NetDevice *netDev, NetBuf *netBuff)
{
    HDF_LOGI("%s: start %s...", __func__, netDev->name);

    if (netBuff) {
        dev_kfree_skb_any(netBuff);
    }

    return HDF_SUCCESS;
}

static int32_t hdf_p2p_netdev_ioctl(struct NetDevice *netDev, IfReq *req, int32_t cmd)
{
    HDF_LOGE("%s: start %s...", __func__, netDev->name);
    return HDF_SUCCESS;
}

static int32_t hdf_p2p_netdev_setmacaddr(struct NetDevice *netDev, void *addr)
{
    int32_t retVal = 0;
    struct net_device *netdev = GetLinuxInfByNetDevice(netDev);

    HDF_LOGE("%s: start %s...", __func__, netDev->name);

    if (netdev == NULL || addr == NULL) {
        HDF_LOGE("%s: netDev or addr null!", __func__);
        return HDF_FAILURE;
    }

    memcpy_s(netdev->dev_addr, netdev->addr_len, addr, netdev->addr_len);
    memcpy_s(netDev->macAddr, MAC_ADDR_SIZE, netdev->dev_addr, netdev->addr_len);

    return retVal;
}

static struct NetDevStats *hdf_p2p_netdev_getstats(struct NetDevice *netDev)
{
    static struct NetDevStats devStat = {0};
    struct net_device *netdev = GetLinuxInfByNetDevice(netDev);

    HDF_LOGE("%s: start %s...", __func__, netDev->name);

    if (netdev == NULL) {
        HDF_LOGE("%s: netDev null!", __func__);
        return NULL;
    }

    return &devStat;
}

static int32_t hdf_p2p_netdev_changemtu(struct NetDevice *netDev, int32_t mtu)
{
    struct net_device *netdev = GetLinuxInfByNetDevice(netDev);
    HDF_LOGE("%s: start %s...", __func__, netDev->name);
    netdev->mtu = mtu;
    return HDF_SUCCESS;
}

static void hdf_p2p_netdev_setnetifstats(struct NetDevice *netDev, NetIfStatus status)
{
    HDF_LOGE("%s: start...", __func__);
    (void)netDev;
    (void)status;
}

static uint16_t hdf_p2p_netdev_selectqueue(struct NetDevice *netDev, NetBuf *netBuff)
{
    HDF_LOGE("%s: start...", __func__);
    (void)netDev;
    (void)netBuff;
    return HDF_SUCCESS;
}

static uint32_t hdf_p2p_netdev_netifnotify(struct NetDevice *netDev, NetDevNotify *notify)
{
    HDF_LOGE("%s: start...", __func__);
    (void)netDev;
    (void)notify;
    return HDF_SUCCESS;
}

static void hdf_p2p_netdev_linkstatuschanged(struct NetDevice *netDev)
{
    HDF_LOGE("%s: start...", __func__);
    (void)netDev;
}

static ProcessingResult hdf_p2p_netdev_specialethertypeprocess(const struct NetDevice *netDev, NetBuf *buff)
{
    const struct Eapol *eapolInstance = NULL;
    int ret;
    uint16_t protocol;
    const int pidx0 = 12, pidx1 = 13;

    HDF_LOGE("%s: start...", __func__);

    if (netDev == NULL || buff == NULL) {
        return PROCESSING_ERROR;
    }

    protocol = (buff->data[pidx0] << WIFI_SHIFT_BIT) | buff->data[pidx1];
    if (protocol != ETHER_TYPE_PAE) {
        HDF_LOGE("%s: return PROCESSING_CONTINUE", __func__);
        return PROCESSING_CONTINUE;
    }

    if (netDev->specialProcPriv == NULL) {
        HDF_LOGE("%s: return PROCESSING_ERROR", __func__);
        return PROCESSING_ERROR;
    }

    eapolInstance = EapolGetInstance();
    ret = eapolInstance->eapolOp->writeEapolToQueue(netDev, buff);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: writeEapolToQueue failed", __func__);
        NetBufFree(buff);
    }

    return PROCESSING_COMPLETE;
}

struct NetDeviceInterFace g_wal_p2p_net_dev_ops = {
    .init = hdf_p2p_netdev_init,
    .deInit = hdf_p2p_netdev_deinit,
    .open = hdf_p2p_netdev_open,
    .stop = hdf_p2p_netdev_stop,
    .xmit = hdf_p2p_netdev_xmit,
    .ioctl = hdf_p2p_netdev_ioctl,
    .setMacAddr = hdf_p2p_netdev_setmacaddr,
    .getStats = hdf_p2p_netdev_getstats,
    .setNetIfStatus = hdf_p2p_netdev_setnetifstats,
    .selectQueue = hdf_p2p_netdev_selectqueue,
    .netifNotify = hdf_p2p_netdev_netifnotify,
    .changeMtu = hdf_p2p_netdev_changemtu,
    .linkStatusChanged = hdf_p2p_netdev_linkstatuschanged,
    .specialEtherTypeProcess = hdf_p2p_netdev_specialethertypeprocess,
};

struct NetDeviceInterFace *rtl_get_net_p2p_dev_ops(void)
{
    return &g_wal_p2p_net_dev_ops;
}

int32_t p2p_netdev_init(struct NetDevice *netDev)
{
    HDF_LOGE("%s: start %s...", __func__, netDev->name);

    if (netDev == NULL) {
        HDF_LOGE("%s: netDev null!", __func__);
        return HDF_FAILURE;
    }

    HDF_LOGE("%s: netDev->name:%s\n", __func__, netDev->name);
    netDev->netDeviceIf = rtl_get_net_p2p_dev_ops();
    CreateEapolData(netDev);

    return HDF_SUCCESS;
}

int InitP2pNetDev(struct NetDevice *netDevice)
{
    struct NetDevice *hnetdev = NULL;
    struct HdfWifiNetDeviceData *data = NULL;
    struct net_device *netdev = NULL;
    char *ifname = "p2p0";
    
    if (netDevice == NULL) {
        HDF_LOGE("%s:para is null!", __func__);
        return -1;
    }

    hnetdev = NetDeviceInit(ifname, strlen(ifname), WIFI_LINK, FULL_OS);
    if (hnetdev == NULL) {
        HDF_LOGE("%s:netdev is null!", __func__);
        return -1;
    }

    data = GetPlatformData(netDevice);
    if (data == NULL) {
        HDF_LOGE("%s:netdevice data null!", __func__);
        return -1;
    }

    p2p_netdev_init(hnetdev); // set net_dev_ops
    hnetdev->classDriverPriv = data;
    hnetdev->classDriverName = netDevice->classDriverName;

    NetDeviceAdd(hnetdev);

    return HDF_SUCCESS;
}

int DeinitP2pNetDev(void)
{
    HDF_LOGE("DeinitP2pNetDev rtl8822cs.\n");
    return HDF_SUCCESS;
}

/* ****************************************************************************
 功能描述  : 初始化wlan设备
 输入参数  : [1]type 设备类型
             [2]mode 模式
             [3]NetDevice
 输出参数  : [1]ifname 设备名
             [2]len 设备名长度
 返 回 值  : 错误码
**************************************************************************** */
wal_dev_addr_stru g_dev_addr = { 0 };

/* 根据设备类型分配mac地址索引 */
static wal_addr_idx wal_get_dev_addr_idx(unsigned char type)
{
    wal_addr_idx addr_idx = WAL_ADDR_IDX_BUTT;

    switch (type) {
        case PROTOCOL_80211_IFTYPE_STATION:
            addr_idx = WAL_ADDR_IDX_STA0;
            break;
        case PROTOCOL_80211_IFTYPE_AP:
        case PROTOCOL_80211_IFTYPE_P2P_CLIENT:
        case PROTOCOL_80211_IFTYPE_P2P_GO:
        case PROTOCOL_80211_IFTYPE_MESH_POINT:
            addr_idx = WAL_ADDR_IDX_AP0;
            break;
        case PROTOCOL_80211_IFTYPE_P2P_DEVICE:
            addr_idx = WAL_ADDR_IDX_STA2;
            break;
        default:
            HDF_LOGE("wal_get_dev_addr_idx:: dev type [%d] is not supported !", type);
            break;
    }

    return addr_idx;
}

uint32_t wal_get_dev_addr(unsigned char *pc_addr, unsigned char addr_len, unsigned char type) /* 建议5.5误报，166行有元素赋值 */
{
    uint16_t us_addr[ETHER_ADDR_LEN];
    uint32_t tmp;
    wal_addr_idx addr_idx;

    if (pc_addr == NULL) {
        HDF_LOGE("wal_get_dev_addr:: pc_addr is NULL!");
        return HDF_FAILURE;
    }

    addr_idx = wal_get_dev_addr_idx(type);
    if (addr_idx >= WAL_ADDR_IDX_BUTT) {
        return HDF_FAILURE;
    }

    for (tmp = 0; tmp < ETHER_ADDR_LEN; tmp++) {
        us_addr[tmp] = (uint16_t)g_dev_addr.ac_addr[tmp];
    }

    /* 1.低位自增 2.高位取其进位 3.低位将进位位置0 */
    us_addr[5] += addr_idx;                      /* 5 地址第6位 */
    us_addr[4] += ((us_addr[5] & (0x100)) >> 8); /* 4 地址第5位 5 地址第6位 8 右移8位 */
    us_addr[5] = us_addr[5] & (0xff);            /* 5 地址第6位 */
    /* 最低位运算完成,下面类似 */
    us_addr[3] += ((us_addr[4] & (0x100)) >> 8); /* 3 地址第4位 4 地址第5位 8 右移8位 */
    us_addr[4] = us_addr[4] & (0xff);            /* 4 地址第5位 */
    us_addr[2] += ((us_addr[3] & (0x100)) >> 8); /* 2 地址第3位 3 地址第4位 8 右移8位 */
    us_addr[3] = us_addr[3] & (0xff);            /* 3 地址第4位 */
    us_addr[1] += ((us_addr[2] & (0x100)) >> 8); /* 1 地址第2位 2 地址第3位 8 右移8位 */
    us_addr[2] = us_addr[2] & (0xff);            /* 2 地址第3位 */
    us_addr[0] += ((us_addr[1] & (0x100)) >> 8); /* 8 右移8位 */
    us_addr[1] = us_addr[1] & (0xff);
    if (us_addr[0] > 0xff) {
        us_addr[0] = 0;
    }
    us_addr[0] &= 0xFE;

    for (tmp = 0; tmp < addr_len; tmp++) {
        pc_addr[tmp] = (unsigned char)us_addr[tmp];
    }

    return HDF_SUCCESS;
}

unsigned char mac_addr_is_zero(const unsigned char *mac_addr)
{
    unsigned char zero_mac_addr[6] = {0};

    if (mac_addr == NULL) {
        return true;
    }

    return (memcmp(zero_mac_addr, mac_addr, 6) == 0);
}

uint32_t rtl_macaddr_check(const unsigned char *mac_addr)
{
    if ((mac_addr_is_zero(mac_addr) == 1) || ((mac_addr[0] & 0x1) == 0x1)) {
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

uint32_t rtl_set_dev_addr_from_efuse(const char *pc_addr, unsigned char mac_len)
{
    if (pc_addr == NULL) {
        HDF_LOGE("rtl_set_dev_addr:: pc_addr is NULL!");
        return HDF_FAILURE;
    }

    if (rtl_macaddr_check((unsigned char *)pc_addr) != HDF_SUCCESS) {
        g_macSet = 2;
        HDF_LOGE("rtl_set_dev_addr:: mac from efuse is zero!");
        return HDF_FAILURE;
    }

    if (memcpy_s(g_dev_addr.ac_addr, ETHER_ADDR_LEN, pc_addr, mac_len) != EOK) {
        HDF_LOGE("rtl_set_dev_addr:: memcpy_s FAILED");
        return HDF_FAILURE;
    }

    set_efuse_mac_exist(1);
    g_macSet = 0;

    return HDF_SUCCESS;
}

uint32_t rtl_set_dev_addr(const char *pc_addr, unsigned char mac_len)
{
    uint32_t count = 0;

    if (pc_addr == NULL) {
        HDF_LOGE("rtl_set_dev_addr:: pc_addr is NULL!");
        return HDF_FAILURE;
    }

    count = NetDevGetRegisterCount();
    if (count > 1) {
        HDF_LOGE("rtl_set_dev_addr::vap exist, could not set mac address!");
        return HDF_FAILURE;
    }

    if (memcpy_s(g_dev_addr.ac_addr, ETHER_ADDR_LEN, pc_addr, mac_len) != EOK) {
        HDF_LOGE("{rtl_set_dev_addr::mem safe function err!}");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
