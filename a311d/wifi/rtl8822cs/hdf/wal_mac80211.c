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

#include <drv_types.h>
#include <net/cfg80211.h>
#include "securec.h"
#include "wifi_module.h"
#include "wifi_mac80211_ops.h"
#include "net_adapter.h"
#include "net_device.h"
#include "hdf_wlan_utils.h"
#include "osal_mem.h"
#include "ioctl_cfg80211.h"

#define RTK_FLAGS_AP (0x00000040)
#define RTK_FLAGS_P2P_DEDICATED_INTERFACE (0x00000400)
#define RTK_FLAGS_P2P_CONCURRENT (0x00000200)
#define RTK_FLAGS_P2P_CAPABLE (0x00000800)

#define RTK_POINT_CHANNEL_SIZE (8)

/* HT Capabilities Info field within HT Capabilities element */
#define HT_CAP_INFO_SUPP_CHANNEL_WIDTH_SET ((u16) BIT(1))
#define HT_CAP_INFO_GREEN_FIELD ((u16) BIT(4))
#define HT_CAP_INFO_SHORT_GI20MHZ ((u16) BIT(5))
#define HT_CAP_INFO_SHORT_GI40MHZ ((u16) BIT(6))
#define HT_CAP_INFO_MAX_AMSDU_SIZE ((u16) BIT(11))
#define HT_CAP_INFO_DSSS_CCK40MHZ ((u16) BIT(12))

#define WIFI_5G_CHANNEL_NUMS (24)

struct MacStorage {
    unsigned char isStorage;
    unsigned char mac[ETHER_ADDR_LEN];
};

typedef enum {
    WLAN_BAND_2G,
    WLAN_BAND_5G,
    WLAN_BAND_BUTT
} wlan_channel_band_enum;

#define HDF_LOG_TAG RTL8822CS

static struct MacStorage g_macStorage = { 0 };
#define ADAPTER_MHZ_TO_KHZ (1000)
#define WIFI_24G_CHANNEL_NUMS (14)
#define WAL_FREQ_2G_INTERVAL (5)
#define WIFI_SCAN_EXTRA_IE_LEN_MAX 512
#define WIFI_24G_CHANNEL_NUMS   (14)
#define WAL_MIN_CHANNEL_2G      (1)
#define WAL_MAX_CHANNEL_2G      (14)
#define WAL_MIN_FREQ_2G         (2412 + 5*(WAL_MIN_CHANNEL_2G - 1))
#define WAL_MAX_FREQ_2G         (2484)
#define WAL_FREQ_2G_INTERVAL    (5)

#define MAX_ACTION_DATA_LEN (1024)
#define MAC_CONTRY_CODE_LEN (3)

struct wiphy* get_linux_wiphy_ndev(struct net_device *ndev)
{
    if (ndev == NULL || ndev->ieee80211_ptr == NULL) {
        return NULL;
    }

    return ndev->ieee80211_ptr->wiphy;
}

struct wiphy* get_linux_wiphy_hdfdev(NetDevice *netDev)
{
    struct net_device *ndev = GetLinuxInfByNetDevice(netDev);
    return get_linux_wiphy_ndev(ndev);
}

int32_t WalDisconnect(NetDevice *netDev, uint16_t reasonCode)
{
    struct net_device *ndev = NULL;
    struct wiphy *wiphy = NULL;

    if (netDev == NULL) {
        HDF_LOGE("%s:NULL ptr!", __func__);
        return HDF_FAILURE;
    }
    ndev = GetLinuxInfByNetDevice(netDev);
    if (ndev == NULL) {
        HDF_LOGE("%s:NULL ptr!", __func__);
        return HDF_FAILURE;
    }
    wiphy = oal_wiphy_get();
    if (!wiphy) {
        HDF_LOGE("%s: wiphy is NULL", __func__);
        return HDF_FAILURE;
    }

    return cfg80211_rtw_disconnect(wiphy, ndev, reasonCode);
}

static int32_t WifiScanSetSsid(const struct WlanScanRequest *params, struct cfg80211_scan_request *request)
{
    int32_t count = 0;
    int32_t loop;

    if (params->ssidCount > WPAS_MAX_SCAN_SSIDS) {
        HDF_LOGE("%s:unexpected numSsids!numSsids=%u", __func__, params->ssidCount);
        return HDF_FAILURE;
    }

    if (params->ssidCount == 0) {
        HDF_LOGE("%s:ssid number is 0!", __func__);
        return HDF_SUCCESS;
    }

    request->ssids = (struct cfg80211_ssid *)OsalMemCalloc(params->ssidCount * sizeof(struct cfg80211_ssid));
    if (request->ssids == NULL) {
        HDF_LOGE("%s:oom", __func__);
        return HDF_FAILURE;
    }

    for (loop = 0; loop < params->ssidCount; loop++) {
        if (count >= DRIVER_MAX_SCAN_SSIDS) {
            break;
        }

        if (params->ssids[loop].ssidLen > IEEE80211_MAX_SSID_LEN) {
            continue;
        }

        request->ssids[count].ssid_len = params->ssids[loop].ssidLen;
        if (memcpy_s(request->ssids[count].ssid, 32, params->ssids[loop].ssid, \
            params->ssids[loop].ssidLen) != EOK) {
            continue;
        }
        count++;
    }
    request->n_ssids = count;
    return HDF_SUCCESS;
}

static int32_t WifiScanSetUserIe(const struct WlanScanRequest *params, struct cfg80211_scan_request *request)
{
    uint8_t *ie = NULL;
    if (params->extraIEsLen > WIFI_SCAN_EXTRA_IE_LEN_MAX) {
        HDF_LOGE("%s:unexpected extra len!extraIesLen=%d", __func__, params->extraIEsLen);
        return HDF_FAILURE;
    }
    if ((params->extraIEs != NULL) && (params->extraIEsLen != 0)) {
        ie = (uint8_t *)OsalMemCalloc(params->extraIEsLen);
        if (ie == NULL) {
            HDF_LOGE("%s:oom", __func__);
            if (request->ie != NULL) {
                ie = (uint8_t *)request->ie;
                OsalMemFree(ie);
                request->ie = NULL;
            }
             return HDF_FAILURE;
        }
        (void)memcpy_s(ie, params->extraIEsLen, params->extraIEs, params->extraIEsLen);
        request->ie = ie;
        request->ie_len = params->extraIEsLen;
    }

    return HDF_SUCCESS;
}

static struct ieee80211_channel *GetChannelByFreq(const struct wiphy *wiphy, uint16_t center_freq)
{
    enum Ieee80211Band band;
    struct ieee80211_supported_band *currentBand = NULL;
    int32_t loop;
    for (band = (enum Ieee80211Band)0; band < IEEE80211_NUM_BANDS; band++) {
        currentBand = wiphy->bands[band];
        if (currentBand == NULL) {
            continue;
        }
        for (loop = 0; loop < currentBand->n_channels; loop++) {
            if (currentBand->channels[loop].center_freq == center_freq) {
                return &currentBand->channels[loop];
            }
        }
    }
    return NULL;
}

static int32_t WifiScanSetChannel(const struct wiphy *wiphy, const struct WlanScanRequest *params, \
    struct cfg80211_scan_request *request)
{
    int32_t loop;
    int32_t count = 0;
    enum Ieee80211Band band = IEEE80211_BAND_2GHZ;
    struct ieee80211_channel *chan = NULL;

    int32_t channelTotal = ieee80211_get_num_supported_channels((struct wiphy *)wiphy);

    if ((params->freqs == NULL) || (params->freqsCount == 0)) {
        for (band = IEEE80211_BAND_2GHZ; band <= IEEE80211_BAND_5GHZ; band++) {
            if (wiphy->bands[band] == NULL) {
                HDF_LOGE("%s: wiphy->bands[band] = NULL!\n", __func__);
                continue;
            }

            for (loop = 0; loop < (int32_t)wiphy->bands[band]->n_channels; loop++) {
                if(count >= channelTotal) {
                    break;
                } 

                chan = &wiphy->bands[band]->channels[loop];
                if ((chan->flags & WIFI_CHAN_DISABLED) != 0) {
                    continue;
                }

                request->channels[count++] = chan;
            }
        }
    } else {
        for (loop = 0; loop < params->freqsCount; loop++) {
            chan = GetChannelByFreq(wiphy, (uint16_t)(params->freqs[loop]));
            if (chan == NULL) {
                HDF_LOGE("%s: freq not found!freq=%d!\n", __func__, params->freqs[loop]);
                continue;
            }

            if (count >= channelTotal) {
                break;
            }
            
            request->channels[count++] = chan;
        }
    }

    if (count == 0) {
        HDF_LOGE("%s: invalid freq info!\n", __func__);
        return HDF_FAILURE;
    }
    request->n_channels = count;

    return HDF_SUCCESS;
}

static int32_t WifiScanSetRequest(struct NetDevice *netdev, const struct WlanScanRequest *params, \
    struct cfg80211_scan_request *request)
{
    if (netdev == NULL || netdev->ieee80211Ptr == NULL) {
        return HDF_FAILURE;
    }
    request->wiphy = GET_NET_DEV_CFG80211_WIRELESS(netdev)->wiphy;
    request->wdev = GET_NET_DEV_CFG80211_WIRELESS(netdev);
    request->n_ssids = params->ssidCount;
    request->flags = 0;

    if (WifiScanSetChannel(GET_NET_DEV_CFG80211_WIRELESS(netdev)->wiphy, params, request)) {
        HDF_LOGE("%s:set channel failed!", __func__);
        return HDF_FAILURE;
    }
    if (WifiScanSetSsid(params, request)) {
        HDF_LOGE("%s:set ssid failed!", __func__);
        return HDF_FAILURE;
    }
    if (WifiScanSetUserIe(params, request)) {
        HDF_LOGE("%s:set user ie failed!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

void WifiScanFree(struct cfg80211_scan_request **request)
{
    if (*request != NULL) {
        if ((*request)->ie != NULL) {
            OsalMemFree((uint8_t *)(*request)->ie);
            (*request)->ie = NULL;
        }
        if ((*request)->ssids != NULL) {
            OsalMemFree((*request)->ssids);
            (*request)->ssids = NULL;
        }
        OsalMemFree(*request);
        *request = NULL;
    }
}

static int32_t WalStartScan(NetDevice *netdev, struct WlanScanRequest *scanParam)
{
    int32_t ret = 0;
    static int32_t is_p2p_complete = 0;
    int32_t channelTotal;
    struct cfg80211_scan_request *request = NULL;
    struct wiphy *wiphy = NULL;

    struct net_device *ndev = GetLinuxInfByNetDevice(netdev);
    if (ndev == NULL) {
        HDF_LOGE("%s: ndev is NULL!", __func__);
        return HDF_FAILURE;
    }

    wiphy = get_linux_wiphy_ndev(ndev);
    if (wiphy == NULL) {
        HDF_LOGE("%s: wiphy is NULL!", __func__);
        return HDF_FAILURE;
    }

    channelTotal = ieee80211_get_num_supported_channels(wiphy);

    request = (struct cfg80211_scan_request *)OsalMemCalloc(sizeof(struct cfg80211_scan_request) \
                            + RTK_POINT_CHANNEL_SIZE * channelTotal);
    if (request == NULL) {
        HDF_LOGE("%s: request is NULL!", __func__);
        return HDF_FAILURE;
    }

    HDF_LOGE("start scan %s", ndev->name);

    if (WifiScanSetRequest(netdev, scanParam, request) != HDF_SUCCESS) {
        HDF_LOGE("WifiScanSetRequest failed!");
        WifiScanFree(&request);
        return HDF_FAILURE;
    }

    ret = cfg80211_rtw_scan(wiphy, request);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("scan failed %d!", ret);
        WifiScanFree(&request);
    }

    return ret;
}

static struct ieee80211_channel *WalGetChannel(struct wiphy *wiphy, int32_t freq)
{
    enum Ieee80211Band band;
    struct ieee80211_supported_band *currentBand = NULL;
    int32_t loop;

    if (wiphy == NULL) {
        HDF_LOGE("%s: capality is NULL!", __func__);
        return NULL;
    }

    for (band = (enum Ieee80211Band)0; band < IEEE80211_NUM_BANDS; band++) {
        currentBand = wiphy->bands[band];
        if (currentBand == NULL) {
            continue;
        }

        for (loop = 0; loop < currentBand->n_channels; loop++) {
            if (currentBand->channels[loop].center_freq == freq) {
                return &currentBand->channels[loop];
            }
        }
    }

    return NULL;
}

static int32_t WalConnect(NetDevice *netDev, WlanConnectParams *param)
{
    struct net_device *ndev = NULL;
    struct wiphy *wiphy = NULL;
    struct cfg80211_connect_params cfg80211_params = { 0 };

    if (netDev == NULL || param == NULL) {
        HDF_LOGE("%s:NULL ptr!", __func__);
        return HDF_FAILURE;
    }

    ndev = GetLinuxInfByNetDevice(netDev);
    if (ndev == NULL) {
        HDF_LOGE("%s:NULL ptr!", __func__);
        return HDF_FAILURE;
    }

    wiphy = oal_wiphy_get();
    if (!wiphy) {
        HDF_LOGE("%s: wiphy is NULL", __func__);
        return HDF_FAILURE;
    }

    if (param->centerFreq != WLAN_FREQ_NOT_SPECFIED) {
        cfg80211_params.channel = WalGetChannel(wiphy, param->centerFreq);
        if ((cfg80211_params.channel == NULL) || (cfg80211_params.channel->flags & WIFI_CHAN_DISABLED)) {
            HDF_LOGE("%s:illegal channel.flags=%u", __func__,
                (cfg80211_params.channel == NULL) ? 0 : cfg80211_params.channel->flags);
            return HDF_FAILURE;
        }
    }
    cfg80211_params.bssid = param->bssid;
    cfg80211_params.ssid = param->ssid;
    cfg80211_params.ie = param->ie;
    cfg80211_params.ssid_len = param->ssidLen;
    cfg80211_params.ie_len = param->ieLen;
    int ret = memcpy_s(&cfg80211_params.crypto, sizeof(cfg80211_params.crypto), &param->crypto, sizeof(param->crypto));
    if (ret != EOK) {
        HDF_LOGE("%s:Copy crypto info failed!ret=%d", __func__, ret);
        return HDF_FAILURE;
    }
    cfg80211_params.key = param->key;
    cfg80211_params.auth_type = (unsigned char)param->authType;
    cfg80211_params.privacy = param->privacy;
    cfg80211_params.key_len = param->keyLen;
    cfg80211_params.key_idx = param->keyIdx;
    cfg80211_params.mfp = (unsigned char)param->mfp;

    return cfg80211_rtw_connect(wiphy, ndev, &cfg80211_params);
}

static int32_t SetupWireLessDev(struct NetDevice *netDev, struct WlanAPConf *apSettings)
{
    return HDF_SUCCESS;
}

int32_t WalSetSsid(NetDevice *netDev, const uint8_t *ssid, uint32_t ssidLen)
{
    return HDF_SUCCESS;
}

int32_t WalChangeBeacon(NetDevice *netDev, struct WlanBeaconConf *param)
{
    return HDF_SUCCESS;
}

int32_t WalSetMeshId(NetDevice *netDev, const char *meshId, uint32_t meshIdLen)
{
#ifdef _PRE_WLAN_FEATURE_MESH
    return wal_cfg80211_set_meshid((NetDevice *)netDev, meshId, meshIdLen);
#else
    (void)netDev;
    (void)meshId;
    (void)meshIdLen;
    return HDF_SUCCESS;
#endif
}

int32_t WalStartAp(NetDevice *netDev)
{
    HDF_LOGI("%s: start... ", __func__);
    return HDF_SUCCESS;
}

int32_t WalStopAp(NetDevice *netDev)
{
    HDF_LOGI("%s: start... ", __func__);
    return HDF_SUCCESS;
}

int32_t WalDelStation(NetDevice *netDev, const uint8_t *macAddr)
{
    HDF_LOGI("%s: start... ", __func__);
    return HDF_SUCCESS;
}

int32_t WalAddKey(struct NetDevice *netDev, uint8_t keyIndex, bool pairwise, const uint8_t *macAddr, \
    struct KeyParams *params)
{
    struct key_params keyParm = { 0 };
    struct net_device *netdev = NULL;
    struct wiphy *wiphy = NULL;

    HDF_LOGI("%s: start... ", __func__);
    netdev = GetLinuxInfByNetDevice(netDev);
    if (!netdev) {
        HDF_LOGE("%s: net_device is NULL", __func__);
        return HDF_FAILURE;
    }

    wiphy = oal_wiphy_get();
    if (!wiphy) {
        HDF_LOGE("%s: wiphy is NULL", __func__);
        return HDF_FAILURE;
    }

    keyParm.key = params->key;
    keyParm.key_len = params->keyLen;
    keyParm.seq_len = params->seqLen;
    keyParm.seq = params->seq;
    keyParm.cipher = params->cipher;
    return cfg80211_rtw_add_key(wiphy, netdev, keyIndex, pairwise, macAddr, &keyParm);
}

int32_t WalDelKey(struct NetDevice *netDev, uint8_t keyIndex, bool pairwise, const uint8_t *macAddr)
{
    int32_t retVal = 0;
    struct net_device *netdev = NULL;
    struct wiphy *wiphy = NULL;

    (void)netDev;

    netdev = GetLinuxInfByNetDevice(netDev);
    if (!netdev) {
        HDF_LOGE("%s: net_device is NULL", __func__);
        return HDF_FAILURE;
    }

    wiphy = oal_wiphy_get();
    if (!wiphy) {
        HDF_LOGE("%s: wiphy is NULL", __func__);
        return HDF_FAILURE;
    }

    HDF_LOGE("%s: start..., mac=%p, keyIndex=%u,pairwise=%d", __func__, macAddr, keyIndex, pairwise);

    retVal = (int32_t)cfg80211_rtw_del_key(wiphy, netdev, keyIndex, pairwise, macAddr);
    if (retVal < 0) {
        HDF_LOGE("%s: delete key failed!", __func__);
    }

    return retVal;
}

int32_t WalSetDefaultKey(struct NetDevice *netDev, uint8_t keyIndex, bool unicast, bool multicas)
{
    int32_t retVal = 0;
    struct net_device *netdev = NULL;
    struct wiphy *wiphy = NULL;

    netdev = GetLinuxInfByNetDevice(netDev);
    if (!netdev) {
        HDF_LOGE("%s: net_device is NULL", __func__);
        return HDF_FAILURE;
    }

    wiphy = oal_wiphy_get();
    if (!wiphy) {
        HDF_LOGE("%s: wiphy is NULL", __func__);
        return HDF_FAILURE;
    }

    HDF_LOGE("%s: start..., keyIndex=%u,unicast=%d, multicas=%d", __func__, \
         keyIndex, unicast, multicas);

    retVal = (int32_t)cfg80211_rtw_set_default_key(wiphy, netdev, keyIndex, unicast, multicas);
    if (retVal < 0) {
        HDF_LOGE("%s: set default key failed!", __func__);
    }

    return retVal;
}

int32_t WalSetMacAddr(NetDevice *netDev, uint8_t *mac, uint8_t len)
{
    uint32_t ret;
    (void)netDev;

    HDF_LOGI("%s: start... ", __func__);

    ret = rtl_macaddr_check(mac);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE( "mac addr is unavailable!");
        return ret;
    }

    ret = memcpy_s(g_macStorage.mac, ETHER_ADDR_LEN, mac, len);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("memcpy_s failed!");
        return ret;
    }
    g_macStorage.isStorage = true;

    return HDF_SUCCESS;
}

int32_t WalSetMode(NetDevice *netDev, enum WlanWorkMode iftype)
{
    int32_t retVal = 0;
    struct net_device *netdev = NULL;
    struct wiphy *wiphy = NULL;

    netdev = GetLinuxInfByNetDevice(netDev);
    if (!netdev) {
        HDF_LOGE("%s: net_device is NULL", __func__);
        return HDF_FAILURE;
    }

    wiphy = oal_wiphy_get();
    if (!wiphy) {
        HDF_LOGE("%s: wiphy is NULL", __func__);
        return HDF_FAILURE;
    }

    HDF_LOGI("%s: start... iftype=%d name is %s", __func__, iftype, netdev->name);
    retVal = (int32_t) cfg80211_rtw_change_iface(wiphy, netdev, \
        (enum nl80211_iftype) iftype, NULL);
    if (retVal < 0) {
        HDF_LOGE("%s: set mode failed!", __func__);
    }

    return retVal;
}

int32_t WalAbortScan(NetDevice *netDev)
{
    HDF_LOGI("%s: start... ", __func__);
    return HDF_ERR_NOT_SUPPORT;
}

int32_t WalGetDeviceMacAddr(NetDevice *netDev, int32_t type, uint8_t *mac, uint8_t len)
{
    (void)netDev;

    if (mac == NULL || len != ETHER_ADDR_LEN) {
        HDF_LOGE("{WalGetDeviceMacAddr::input param error!}");
        return HDF_FAILURE;
    }

    if (!get_efuse_mac_exist()) {
        /* if there is no data in efuse */
        HDF_LOGE("wal_get_efuse_mac_addr:: no data in efuse!");
        return HDF_ERR_NOT_SUPPORT;
    }

    if (wal_get_dev_addr(mac, len, type) != HDF_SUCCESS) {
        HDF_LOGE("{set_mac_addr_by_type::GetDeviceMacAddr failed!}");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t WalGetValidFreqsWithBand(NetDevice *netDev, int32_t band, int32_t *freqs, uint32_t *num)
{
    uint32_t freqIndex = 0, channelNumber, freqTmp, minFreq, maxFreq;
    struct ieee80211_supported_band *band5g = NULL;
    int32_t max5GChNum = 0;
    struct wiphy *wiphy = NULL;

    (void)netDev;

    HDF_LOGE("%s: start band [%d]", __func__, band);

    wiphy = get_linux_wiphy_hdfdev(netDev);
    if (!wiphy) {
        HDF_LOGE("%s: wiphy is NULL", __func__);
        return HDF_FAILURE;
    }

    switch (band) {
        case WLAN_BAND_2G: {
                minFreq = 2412;
                maxFreq = 2484;

                for (channelNumber = 1; channelNumber <= WIFI_24G_CHANNEL_NUMS; channelNumber++) {
                    if (channelNumber < WAL_MAX_CHANNEL_2G) {
                        freqTmp = WAL_MIN_FREQ_2G + (channelNumber - 1) * WAL_FREQ_2G_INTERVAL;
                    } else if (channelNumber == WAL_MAX_CHANNEL_2G) {
                        freqTmp = WAL_MAX_FREQ_2G;
                    }
                    if (freqTmp < minFreq || freqTmp > maxFreq) {
                        continue;
                    }
                    freqs[freqIndex] = freqTmp;
                    freqIndex++;
                }
                *num = freqIndex;
            }
            break;
        case WLAN_BAND_5G: {
                band5g = wiphy->bands[IEEE80211_BAND_5GHZ];
                if (band5g == NULL) {
                    HDF_LOGE("%s: band5g is null, error!", __func__);
                    return HDF_ERR_NOT_SUPPORT;
                }

                max5GChNum = min(band5g->n_channels, WIFI_5G_CHANNEL_NUMS);
                for (freqIndex = 0; freqIndex < max5GChNum; freqIndex++) {
                    freqs[freqIndex] = band5g->channels[freqIndex].center_freq;
                }
                *num = freqIndex;
            }
            break;
        default:
            HDF_LOGE("not support this band!");
            return HDF_ERR_NOT_SUPPORT;
    }

    return HDF_SUCCESS;
}

int32_t WalSetTxPower(NetDevice *netDev, int32_t power)
{
    int retVal = 0;
    struct wiphy *wiphy = NULL;
    struct wireless_dev *wdev = GET_NET_DEV_CFG80211_WIRELESS(netDev);

    wiphy = get_linux_wiphy_hdfdev(netDev);
    if (!wiphy) {
        HDF_LOGE("%s: wiphy is NULL", __func__);
        return HDF_FAILURE;
    }

    HDF_LOGE("%s: start...", __func__);
    retVal = (int32_t)cfg80211_rtw_set_txpower(wiphy, wdev, NL80211_TX_POWER_FIXED, power);
    if (retVal < 0) {
        HDF_LOGE("%s: set_tx_power failed!", __func__);
    }

    return HDF_SUCCESS;
}

int32_t WalGetAssociatedStasCount(NetDevice *netDev, uint32_t *num)
{
    HDF_LOGI("%s: start... ", __func__);
    return HDF_SUCCESS;
}

int32_t WalGetAssociatedStasInfo(NetDevice *netDev, WifiStaInfo *staInfo, uint32_t num)
{
    HDF_LOGI("%s: start... ", __func__);
    return HDF_SUCCESS;
}

int32_t WalSetCountryCode(NetDevice *netDev, const char *code, uint32_t len)
{
    HDF_LOGI("%s: start... ", __func__);
    return HDF_SUCCESS;
}

int32_t WalSetScanningMacAddress(NetDevice *netDev, unsigned char *mac, uint32_t len)
{
    (void)netDev;
    (void)mac;
    (void)len;
    HDF_LOGI("%s: start... ", __func__);
    return HDF_ERR_NOT_SUPPORT;
}

int32_t WalConfigAp(NetDevice *netDev, struct WlanAPConf *apConf)
{
    HDF_LOGI("%s: start... ", __func__);
    return HDF_SUCCESS;
}

void WalReleaseHwCapability(struct WlanHwCapability *self)
{
    uint8_t i;
    if (self == NULL) {
        return;
    }
    for (i = 0; i < IEEE80211_NUM_BANDS; i++) {
        if (self->bands[i] != NULL) {
            OsalMemFree(self->bands[i]);
            self->bands[i] = NULL;
        }
    }
    if (self->supportedRates != NULL) {
        OsalMemFree(self->supportedRates);
        self->supportedRates = NULL;
    }
    OsalMemFree(self);
}

static int32_t WalGet2GCapability(struct ieee80211_supported_band *band, struct WlanHwCapability *hwCapability)
{
    uint8_t loop = 0;

    HDF_LOGE("Get2GCapability ...");

    if (band->n_channels > WIFI_MAX_CHANNEL_NUM)
        band->n_channels = WIFI_MAX_CHANNEL_NUM;

    hwCapability->bands[IEEE80211_BAND_2GHZ] = \
        OsalMemCalloc(sizeof(struct WlanBand) + (sizeof(struct WlanChannel) * band->n_channels));
    if (hwCapability->bands[IEEE80211_BAND_2GHZ] == NULL) {
        HDF_LOGE("{%s::oom!}\r\n", __func__);
        return 0;
    }

    hwCapability->bands[IEEE80211_BAND_2GHZ]->channelCount = band->n_channels;

    for (loop = 0; loop < band->n_channels; loop++) {
        hwCapability->bands[IEEE80211_BAND_2GHZ]->channels[loop].centerFreq = band->channels[loop].center_freq;
        hwCapability->bands[IEEE80211_BAND_2GHZ]->channels[loop].flags = band->channels[loop].flags;
        hwCapability->bands[IEEE80211_BAND_2GHZ]->channels[loop].channelId = band->channels[loop].hw_value;
    }

    return band->n_bitrates;
}

static int32_t WalGet5GCapability(struct ieee80211_supported_band *band5g, struct WlanHwCapability *hwCapability)
{
    uint8_t loop = 0;

    HDF_LOGE("Get5GCapability ...");

    // Fill 5Ghz band
    if (band5g->n_channels > WIFI_MAX_CHANNEL_NUM)
        band5g->n_channels = WIFI_MAX_CHANNEL_NUM;

    hwCapability->bands[IEEE80211_BAND_5GHZ] = \
        OsalMemCalloc(sizeof(struct WlanBand) + (sizeof(struct WlanChannel) * band5g->n_channels));
    if (hwCapability->bands[IEEE80211_BAND_5GHZ] == NULL) {
        HDF_LOGE("%s: oom!\n", __func__);
        return 0;
    }

    hwCapability->bands[IEEE80211_BAND_5GHZ]->channelCount = band5g->n_channels;
    for (loop = 0; loop < band5g->n_channels; loop++) {
        hwCapability->bands[IEEE80211_BAND_5GHZ]->channels[loop].centerFreq = band5g->channels[loop].center_freq;
        hwCapability->bands[IEEE80211_BAND_5GHZ]->channels[loop].flags = band5g->channels[loop].flags;
        hwCapability->bands[IEEE80211_BAND_5GHZ]->channels[loop].channelId = band5g->channels[loop].hw_value;
    }

    return band5g->n_bitrates;
}

int32_t WalGetHwCapability(struct NetDevice *netDev, struct WlanHwCapability **capability)
{
    int32_t ret = 0;
    uint8_t loop, index = 0;
    struct wiphy* wiphy = NULL;
    uint16_t supportedRateCount = 0;
    struct ieee80211_supported_band *band = NULL;
    struct ieee80211_supported_band *band5g = NULL;

    if (capability == NULL) {
        return HDF_FAILURE;
    }

    wiphy = get_linux_wiphy_hdfdev(netDev);
    if (!wiphy) {
        HDF_LOGE("wiphy is NULL!");
        return HDF_FAILURE;
    }

    struct WlanHwCapability *hwCapability = (struct WlanHwCapability *)OsalMemCalloc(sizeof(struct WlanHwCapability));
    if (hwCapability == NULL) {
        HDF_LOGE("{%s::oom!}\r\n", __func__);
        return HDF_FAILURE;
    }

    hwCapability->Release = WalReleaseHwCapability;
    hwCapability->htCapability = (HT_CAP_INFO_SUPP_CHANNEL_WIDTH_SET | HT_CAP_INFO_SHORT_GI20MHZ | \
        HT_CAP_INFO_SHORT_GI40MHZ | HT_CAP_INFO_MAX_AMSDU_SIZE | HT_CAP_INFO_DSSS_CCK40MHZ);

    band = wiphy->bands[IEEE80211_BAND_2GHZ];
    ret = WalGet2GCapability(band, hwCapability);
    supportedRateCount = ret;

    band5g = wiphy->bands[IEEE80211_BAND_5GHZ];
    ret = WalGet5GCapability(band5g, hwCapability);
    supportedRateCount += ret;

    HDF_LOGE("htCapability= %u,%u; supportedRateCount= %u,%u,%u", hwCapability->htCapability, \
        band5g->ht_cap.cap, supportedRateCount, band->n_bitrates, band5g->n_bitrates);

    hwCapability->supportedRateCount = supportedRateCount;
    hwCapability->supportedRates = OsalMemCalloc(sizeof(uint16_t) * supportedRateCount);
    if (hwCapability->supportedRates == NULL) {
        HDF_LOGE("%s: oom!\n", __func__);
        WalReleaseHwCapability(hwCapability);
        return HDF_FAILURE;
    }

    for (loop = 0; loop < band->n_bitrates; loop++) {
        hwCapability->supportedRates[loop] = band->bitrates[loop].bitrate;
    }

    for (loop = band->n_bitrates; loop < supportedRateCount; loop++) {
        hwCapability->supportedRates[loop] = band5g->bitrates[index].bitrate;
        index ++;
    }

    if (hwCapability->supportedRateCount > MAX_SUPPORTED_RATE)
        hwCapability->supportedRateCount = MAX_SUPPORTED_RATE;

    *capability = hwCapability;

    return HDF_SUCCESS;
}

int32_t WalSendAction(struct NetDevice *netDev, WifiActionData *actionData)
{
    (void)netDev;
    (void)actionData;
    HDF_LOGI("%s: start... ", __func__);
    return HDF_ERR_NOT_SUPPORT;
}

int32_t WalGetIftype(struct NetDevice *netDev, uint8_t *iftype)
{
     iftype = (uint8_t *)(&(GET_NET_DEV_CFG80211_WIRELESS(netDev)->iftype));
     return HDF_SUCCESS;
}

static int32_t WalRemainOnChannel(struct NetDevice *netDev, WifiOnChannel *onChannel)
{
    HDF_LOGE("%s: start...", __func__);
    return HDF_SUCCESS;
}

static int32_t WalCancelRemainOnChannel(struct NetDevice *netdev)
{
    HDF_LOGE("%s: start...", __func__);
    return HDF_SUCCESS;
}

static int32_t WalProbeReqReport(struct NetDevice *netdev, int32_t report)
{
    HDF_LOGE("%s: start...", __func__);
    return HDF_SUCCESS;
}

static int32_t WalAddIf(struct NetDevice *netDevice, WifiIfAdd *ifAdd)
{
    if (netDevice == NULL || ifAdd == NULL) {
        HDF_LOGE("%s:NULL ptr!", __func__);
        return HDF_FAILURE;
    }

    HDF_LOGE("%s: ifname = %s, type=%u", __func__, netDevice->name, ifAdd->type);

    return HDF_SUCCESS;
}

static int32_t WalRemoveIf(struct NetDevice *netDevice, WifiIfRemove *ifRemove)
{
    HDF_LOGE("%s: start...", __func__);
    return HDF_SUCCESS;
}

static int32_t WalSetApWpsP2pIe(struct NetDevice *netdev, WifiAppIe *appIe)
{
    HDF_LOGE("%s: start...", __func__);
    return HDF_SUCCESS;
}

static int32_t WalGetDriverFlag(struct NetDevice *netDevice, WifiGetDrvFlags **params)
{
    struct wireless_dev *wdev = NULL;
    WifiGetDrvFlags *getDrvFlag = NULL;
    int iftype = 0;

    if (netDevice == NULL || params == NULL) {
        HDF_LOGE("%s:NULL ptr!", __func__);
        return HDF_FAILURE;
    }

    HDF_LOGE("%s: netdev %s", __func__, netDevice->name);

    wdev = GET_NET_DEV_CFG80211_WIRELESS(netDevice);
    if (wdev != NULL) {
        iftype = wdev->iftype;
    }

    if (strcmp(netDevice->name, "p2p0") == 0) {
        iftype = NL80211_IFTYPE_P2P_DEVICE;
    }

    getDrvFlag = (WifiGetDrvFlags *)OsalMemCalloc(sizeof(WifiGetDrvFlags));

    switch (iftype) {
        case NL80211_IFTYPE_P2P_CLIENT:
             /* fall-through */
        case NL80211_IFTYPE_P2P_GO:
            getDrvFlag->drvFlags = (unsigned int)(RTK_FLAGS_AP);
            break;
        case NL80211_IFTYPE_P2P_DEVICE:
            getDrvFlag->drvFlags = (unsigned int)(RTK_FLAGS_P2P_DEDICATED_INTERFACE | \
                RTK_FLAGS_P2P_CONCURRENT |RTK_FLAGS_P2P_CAPABLE);
            break;
        default:
            getDrvFlag->drvFlags = 0;
            break;
    }

    *params = getDrvFlag;

    HDF_LOGE("%s: %s iftype=%d, drvflag=%lu", __func__, netDevice->name, iftype, getDrvFlag->drvFlags);

    return HDF_SUCCESS;
}

static struct HdfMac80211BaseOps g_baseOps = {
    .SetMode = WalSetMode,
    .AddKey = WalAddKey,
    .DelKey = WalDelKey,
    .SetDefaultKey = WalSetDefaultKey,
    .GetDeviceMacAddr = WalGetDeviceMacAddr,
    .SetMacAddr = WalSetMacAddr,
    .SetTxPower = WalSetTxPower,
    .GetValidFreqsWithBand = WalGetValidFreqsWithBand,
    .GetHwCapability = WalGetHwCapability,
    .SendAction = WalSendAction,
    .GetIftype = WalGetIftype,
};

static struct HdfMac80211STAOps g_staOps = {
    .Connect = WalConnect,
    .Disconnect = WalDisconnect,
    .StartScan = WalStartScan,
    .AbortScan = WalAbortScan,
    .SetScanningMacAddress = WalSetScanningMacAddress,
};

static struct HdfMac80211APOps g_apOps = {
    .ConfigAp = WalConfigAp,
    .StartAp = WalStartAp,
    .StopAp = WalStopAp,
    .ConfigBeacon = WalChangeBeacon,
    .DelStation = WalDelStation,
    .SetCountryCode = WalSetCountryCode,
    .GetAssociatedStasCount = WalGetAssociatedStasCount,
    .GetAssociatedStasInfo = WalGetAssociatedStasInfo
};

static struct HdfMac80211P2POps g_p2pOps = {
    .RemainOnChannel = WalRemainOnChannel,
    .CancelRemainOnChannel = WalCancelRemainOnChannel,
    .ProbeReqReport = WalProbeReqReport,
    .AddIf = WalAddIf,
    .RemoveIf = WalRemoveIf,
    .SetApWpsP2pIe = WalSetApWpsP2pIe,
    .GetDriverFlag = WalGetDriverFlag
};

void HiMac80211Init(struct HdfChipDriver *chipDriver)
{
    if (chipDriver == NULL) {
        HDF_LOGE("%s:input is NULL!", __func__);
        return;
    }
    chipDriver->ops = &g_baseOps;
    chipDriver->staOps = &g_staOps;
    chipDriver->apOps = &g_apOps;
    chipDriver->p2pOps = &g_p2pOps;
}
