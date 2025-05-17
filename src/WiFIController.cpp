/*
 * FeitCSI is the tool for extracting CSI information from supported intel NICs.
 * Copyright (C) 2023-2025 Miroslav Hutar.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "nl80211.h" //fix old libs
#include "WiFIController.h"

#include <errno.h>
#include <iwlib.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/attr.h>
#include <chrono>
#include <thread>
#include <sstream>
#include <iostream>
#include <signal.h>
#include <thread>
#include "main.h"
#include "Logger.h"
#include "Arguments.h"

const char *IF_MODES[NL80211_IFTYPE_MAX + 1] = {
    "unspecified",
    "IBSS",
    "managed",
    "AP",
    "AP/VLAN",
    "WDS",
    "monitor",
    "mesh point",
    "P2P-client",
    "P2P-GO",
    "P2P-device",
    "outside context of a BSS",
    "NAN",
};

void WiFIController::killNetworkProcesses()
{
    std::string procesNames[] = {
        "wpa_action",
        "wpa_supplicant",
        "wpa_cli",
        "dhclient",
        "ifplugd",
        "dhcdbd",
        "dhcpcd",
        "udhcpc",
        "NetworkManager",
        "knetworkmanager",
        "avahi-autoipd",
        "avahi-daemon",
        "wlassistant",
        "wifibox",
        "net_applet",
        "wicd-daemon",
        "wicd-client",
        "iwd",
        "hostapd"};

    for (std::string name : procesNames)
    {
        char buf[512];
        std::string cmnd = "pidof " + name;
        FILE *pipe = popen(cmnd.c_str(), "r");
        fgets(buf, 512, pipe);
        std::stringstream ss(buf);
        std::string word;
        while (ss >> word)
        { // Extract word from the stream.
            pid_t pid = std::atoi(word.c_str());
            if (pid)
            {
                kill(pid, SIGKILL);
            }
        }
        pclose(pipe);
    }
}

void WiFIController::getInterfaces()
{
    Cmd cmd{
        .id = NL80211_CMD_GET_INTERFACE,
        .idby = CIB_NONE,
        .nlFlags = NLM_F_DUMP,
        .handler = NULL,
        .valid_handler = this->processGetInterfacesHandler,
    };

    this->nlExecCommand(cmd);
}

void WiFIController::getInterfaceInfo(const char *ifname)
{
    Cmd cmd{
        .id = NL80211_CMD_GET_INTERFACE,
        .idby = CIB_NETDEV,
        .nlFlags = 0,
        .device = if_nametoindex(ifname),
        .handler = NULL,
        .valid_handler = this->processGetInterfaceInfoHandler,
    };

    this->nlExecCommand(cmd);
}

void WiFIController::setTxPower()
{
    Cmd cmd{
        .id = NL80211_CMD_SET_WIPHY,
        .idby = CIB_NETDEV,
        .nlFlags = 0,
        .device = if_nametoindex(this->currentDeviceName.c_str()),
        .handler = this->setTxPowerHandler,
    };

    this->nlExecCommand(cmd);
}

void WiFIController::addDevice(const char *name, const nl80211_iftype type)
{
    if (this->phys.empty())
    {
        throw std::ios_base::failure("Error no phy devices\n");
        return;
    }

    const void *settings[] = {name, &type};

    Cmd cmd{
        .id = NL80211_CMD_NEW_INTERFACE,
        .idby = CIB_PHY,
        .nlFlags = 0,
        .device = this->phys[0],
        .handler = this->processaddDeviceHandler,
        .valid_handler = NULL,
        .args = settings,
    };
    this->nlExecCommand(cmd);
}

int WiFIController::setInterfaceUpDown(const char *ifName, bool up)
{
    // Create a socket file descriptor
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("Failed to create socket");
        return 1;
    }

    // Get the interface index
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifName, IFNAMSIZ - 1);
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0)
    {
        perror("Failed to get interface index");
        close(sockfd);
        return 1;
    }

    if (up)
    {
        ifr.ifr_flags |= IFF_UP;
        this->currentDeviceName = ifName;
    }
    else
    {
        ifr.ifr_flags &= ~IFF_UP;
    }
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0)
    {
        perror("Failed to bring down interface");
        close(sockfd);
        return 1;
    }

    // Close the socket
    close(sockfd);

    if (Arguments::arguments.verbose)
    {
        Logger::log(info) << "Interface " << ifName << " has been brought " << (up ? "up" : "down") << "\n";
    }

    return 0;
}

void WiFIController::setFreq(uint16_t freq, const char *bw)
{
    const void *settings[] = {&freq, bw};
    Cmd cmd{
        .id = NL80211_CMD_SET_WIPHY,
        .idby = CIB_NETDEV,
        .nlFlags = 0,
        .device = if_nametoindex(this->currentDeviceName.c_str()),
        .handler = processSetFreq,
        .valid_handler = NULL,
        .args = settings,
    };
    this->nlExecCommand(cmd);
}

void WiFIController::removeInterface(const char *name, int ifIndex)
{
    Cmd cmd{
        .id = NL80211_CMD_DEL_INTERFACE,
        .idby = CIB_NETDEV,
        .nlFlags = 0,
        .device = ifIndex ? ifIndex : if_nametoindex(name),
        .handler = NULL,
        .valid_handler = NULL,
    };
    this->nlExecCommand(cmd);
}

void WiFIController::createMonitorInteface()
{
    this->addDevice(MONITOR_INTERFACE_NAME, NL80211_IFTYPE_MONITOR);
    while (this->currentInterfaceInfo.freq != Arguments::arguments.frequency)
    {
        try
        {
            this->setInterfaceUpDown(MONITOR_INTERFACE_NAME, true);
            this->setFreq(Arguments::arguments.frequency, Arguments::arguments.bandwidth.c_str());
        }
        catch (const std::exception &e)
        {
            // Just skip
        }

        this->getInterfaceInfo(MONITOR_INTERFACE_NAME);
    }
    this->setTxPower();
}

void WiFIController::createApInteface()
{
    this->addDevice(AP_INTERFACE_NAME, NL80211_IFTYPE_MONITOR);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    while (this->currentInterfaceInfo.ifType != NL80211_IFTYPE_AP)
    {
        try
        {
            this->setInterfaceUpDown(MONITOR_INTERFACE_NAME, false);
            this->setApMode();
        }
        catch (const std::exception &e)
        {
            // Just skip
        }

        this->getInterfaceInfo(AP_INTERFACE_NAME);
    }
    this->setInterfaceUpDown(AP_INTERFACE_NAME, true);
    this->setTxPower();
}

void WiFIController::getPhys()
{
    Cmd cmd{
        .id = NL80211_CMD_GET_WIPHY,
        .idby = CIB_NONE,
        .nlFlags = NLM_F_DUMP,
        .handler = NULL,
        .valid_handler = this->processGetPhysHandler,
    };
    this->nlExecCommand(cmd);
}

int WiFIController::processGetPhysHandler(struct nl_msg *msg, void *arg)
{
    WiFIController *instance = (WiFIController *)arg;
    struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];
    struct genlmsghdr *gnlh = (genlmsghdr *)nlmsg_data(nlmsg_hdr(msg));
    nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
              genlmsg_attrlen(gnlh, 0), NULL);

    if (tb_msg[NL80211_ATTR_WIPHY])
    {
        int64_t phyId = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY]);
        instance->phys.push_back(phyId);
    }

    return NL_OK;
}

uint16_t WiFIController::chanModeToWidth(struct ChanMode &chanMode)
{
    switch (chanMode.width)
    {
    case NL80211_CHAN_WIDTH_20:
        return 20;
    case NL80211_CHAN_WIDTH_40:
        return 40;
    case NL80211_CHAN_WIDTH_80:
        return 80;
    case NL80211_CHAN_WIDTH_160:
        return 160;
    case NL80211_CHAN_WIDTH_320:
        return 320;

    default:
        break;
    }
    return 0;
}

ChanMode WiFIController::getChanMode(const char *width)
{
    static const struct ChanMode chanMode[] = {
        {.name = "20",
         .width = NL80211_CHAN_WIDTH_20,
         .freq1_diff = 0,
         .chantype = NL80211_CHAN_HT20},
        {.name = "40",
         .width = NL80211_CHAN_WIDTH_40,
         .freq1_diff = 10,
         .chantype = NL80211_CHAN_HT40PLUS},
        {.name = "HT40-",
         .width = NL80211_CHAN_WIDTH_40,
         .freq1_diff = -10,
         .chantype = NL80211_CHAN_HT40MINUS},
        {.name = "NOHT",
         .width = NL80211_CHAN_WIDTH_20_NOHT,
         .freq1_diff = 0,
         .chantype = NL80211_CHAN_NO_HT},
        {.name = "5MHz",
         .width = NL80211_CHAN_WIDTH_5,
         .freq1_diff = 0,
         .chantype = -1},
        {.name = "10MHz",
         .width = NL80211_CHAN_WIDTH_10,
         .freq1_diff = 0,
         .chantype = -1},
        {.name = "80",
         .width = NL80211_CHAN_WIDTH_80,
         .freq1_diff = 0,
         .chantype = -1},
        {.name = "160",
         .width = NL80211_CHAN_WIDTH_160,
         .freq1_diff = 0,
         .chantype = -1},
        {.name = "320MHz",
         .width = NL80211_CHAN_WIDTH_320,
         .freq1_diff = 0,
         .chantype = -1},
        {.name = "1MHz",
         .width = NL80211_CHAN_WIDTH_1,
         .freq1_diff = 0,
         .chantype = -1},
        {.name = "2MHz",
         .width = NL80211_CHAN_WIDTH_2,
         .freq1_diff = 0,
         .chantype = -1},
        {.name = "4MHz",
         .width = NL80211_CHAN_WIDTH_4,
         .freq1_diff = 0,
         .chantype = -1},
        {.name = "8MHz",
         .width = NL80211_CHAN_WIDTH_8,
         .freq1_diff = 0,
         .chantype = -1},
        {.name = "16MHz",
         .width = NL80211_CHAN_WIDTH_16,
         .freq1_diff = 0,
         .chantype = -1},
    };

    struct ChanMode selectedChanMode = {};

    for (unsigned int i = 0; i < ARRAY_SIZE(chanMode); i++)
    {
        if (strcasecmp(chanMode[i].name, width) == 0)
        {
            selectedChanMode = chanMode[i];
            break;
        }
    }

    return selectedChanMode;
}

int WiFIController::processSetFreq(struct nl80211_state *state, struct nl_msg *msg, void *arg)
{
    void **settings = (void **)arg;
    uint16_t settingsFreq = *(uint16_t *)(settings[0]);
    char *settingsWidth = (char *)(settings[1]);

    if (Arguments::arguments.verbose)
    {
        Logger::log(info) << "Setting frequency " << settingsFreq << " channel width " << settingsWidth << "\n";
    }

    uint32_t freq = settingsFreq;
    uint32_t freq_offset = 0;

    unsigned int control_freq = freq;
    unsigned int control_freq_offset = freq_offset;
    unsigned int center_freq1 = freq;
    unsigned int center_freq1_offset = freq_offset;
    enum nl80211_chan_width width = control_freq < 1000 ? NL80211_CHAN_WIDTH_16 : NL80211_CHAN_WIDTH_20_NOHT;

    struct ChanMode selectedChanMode = getChanMode(settingsWidth);

    center_freq1 = getCf1(&selectedChanMode, freq);

    /* For non-S1G frequency */
    if (center_freq1 > 1000)
        center_freq1_offset = 0;

    width = (nl80211_chan_width)selectedChanMode.width;

    NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_FREQ, control_freq);
    NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_FREQ_OFFSET, control_freq_offset);
    NLA_PUT_U32(msg, NL80211_ATTR_CHANNEL_WIDTH, width);

    switch (width)
    {
    case NL80211_CHAN_WIDTH_20_NOHT:
        NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_CHANNEL_TYPE, NL80211_CHAN_NO_HT);
        break;
    case NL80211_CHAN_WIDTH_20:
        NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_CHANNEL_TYPE, NL80211_CHAN_HT20);
        break;
    case NL80211_CHAN_WIDTH_40:
        if (control_freq > center_freq1)
        {
            NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_CHANNEL_TYPE, NL80211_CHAN_HT40MINUS);
        }
        else
        {
            NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_CHANNEL_TYPE, NL80211_CHAN_HT40PLUS);
        }
        break;
    default:
        break;
    }

    if (center_freq1)
    {
        NLA_PUT_U32(msg, NL80211_ATTR_CENTER_FREQ1, center_freq1);
    }

    if (center_freq1_offset)
    {
        NLA_PUT_U32(msg, NL80211_ATTR_CENTER_FREQ1_OFFSET, center_freq1_offset);
    }

    return 0;

nla_put_failure:
    return -ENOBUFS;
}

int WiFIController::setTxPowerHandler(nl80211_state *state, nl_msg *msg, void *arg)
{
    enum nl80211_tx_power_setting type = NL80211_TX_POWER_FIXED;
    int mbm = Arguments::arguments.txPower * 100; // dBm to mbm *100

    NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_TX_POWER_SETTING, type);
    NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_TX_POWER_LEVEL, mbm);

    return 0;

nla_put_failure:
    return -ENOBUFS;
}

int WiFIController::processaddDeviceHandler(struct nl80211_state *state, struct nl_msg *msg, void *arg)
{
    void **settings = (void **)arg;
    const char *name = (const char *)settings[0];
    nl80211_iftype *type = (nl80211_iftype *)(settings[1]);

    NLA_PUT_STRING(msg, NL80211_ATTR_IFNAME, name);
    NLA_PUT_U32(msg, NL80211_ATTR_IFTYPE, *type);
    NLA_PUT(msg, NL80211_ATTR_MAC, ETH_ALEN, Arguments::arguments.mac);

    return 0;
nla_put_failure:
    return -ENOBUFS;
}

int WiFIController::getCf1(const struct ChanMode *chanmode, unsigned long freq)
{
    unsigned int cf1 = freq, j;
    unsigned int bw80[] = {5180, 5260, 5500, 5580, 5660, 5745,
                           5955, 6035, 6115, 6195, 6275, 6355,
                           6435, 6515, 6595, 6675, 6755, 6835,
                           6195, 6995};
    unsigned int bw160[] = {5180, 5500, 5955, 6115, 6275, 6435,
                            6595, 6755, 6915};
    /* based on 11be D2 E.1 Country information and operating classes */
    unsigned int bw320[] = {5955, 6115, 6275, 6435, 6595, 6755};

    switch (chanmode->width)
    {
    case NL80211_CHAN_WIDTH_80:
        /* setup center_freq1 */
        for (j = 0; j < ARRAY_SIZE(bw80); j++)
        {
            if (freq >= bw80[j] && freq < bw80[j] + 80)
                break;
        }

        if (j == ARRAY_SIZE(bw80))
            break;

        cf1 = bw80[j] + 30;
        break;
    case NL80211_CHAN_WIDTH_160:
        /* setup center_freq1 */
        for (j = 0; j < ARRAY_SIZE(bw160); j++)
        {
            if (freq >= bw160[j] && freq < bw160[j] + 160)
                break;
        }

        if (j == ARRAY_SIZE(bw160))
            break;

        cf1 = bw160[j] + 70;
        break;
    case NL80211_CHAN_WIDTH_320:
        /* setup center_freq1 */
        for (j = 0; j < ARRAY_SIZE(bw320); j++)
        {
            if (freq >= bw320[j] && freq < bw320[j] + 160)
                break;
        }

        if (j == ARRAY_SIZE(bw320))
            break;

        cf1 = bw320[j] + 150;
        break;
    default:
        cf1 = freq + chanmode->freq1_diff;
        break;
    }

    return cf1;
}

int WiFIController::processGetInterfacesHandler(struct nl_msg *msg, void *arg)
{
    WiFIController *instance = (WiFIController *)arg;
    struct genlmsghdr *gnlh = (genlmsghdr *)nlmsg_data(nlmsg_hdr(msg));
    struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];

    struct InterfaceInfo ifInfo;

    nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
              genlmsg_attrlen(gnlh, 0), NULL);

    if (tb_msg[NL80211_ATTR_IFNAME])
    {
        ifInfo.ifName = nla_get_string(tb_msg[NL80211_ATTR_IFNAME]);
    }
    else
    {
        ifInfo.ifName = "Unnamed/non-netdev interface";
    }

    if (tb_msg[NL80211_ATTR_IFINDEX])
    {
        ifInfo.ifIndex = nla_get_u32(tb_msg[NL80211_ATTR_IFINDEX]);
    }
    if (tb_msg[NL80211_ATTR_WDEV])
    {
        ifInfo.wdev = (uint64_t)nla_get_u64(tb_msg[NL80211_ATTR_WDEV]);
    }
    if (tb_msg[NL80211_ATTR_MAC])
    {
        ifInfo.mac = instance->macN2a((const unsigned char *)nla_data(tb_msg[NL80211_ATTR_MAC]));
    }
    if (tb_msg[NL80211_ATTR_SSID])
    {
        ifInfo.ssid = (const char *)nla_data(tb_msg[NL80211_ATTR_SSID]);
    }
    if (tb_msg[NL80211_ATTR_IFTYPE])
    {
        ifInfo.ifType = nla_get_u32(tb_msg[NL80211_ATTR_IFTYPE]);
        /* if (iftype <= NL80211_IFTYPE_MAX && IF_MODES[iftype])
        {
            ifInfo.type = IF_MODES[iftype];
        }
        else
        {
            ifInfo.type = "unknown";
        } */
    }

    if (tb_msg[NL80211_ATTR_WIPHY])
    {
        ifInfo.wiphy = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY]);
    }
    if (tb_msg[NL80211_ATTR_WIPHY_FREQ])
    {
        ifInfo.freq = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY_FREQ]);
        ifInfo.channel = instance->freqToCHannel(ifInfo.freq);
        if (tb_msg[NL80211_ATTR_CHANNEL_WIDTH])
        {
            ifInfo.channelWidth = nla_get_u32(tb_msg[NL80211_ATTR_CHANNEL_WIDTH]);
            if (tb_msg[NL80211_ATTR_CENTER_FREQ1])
            {
                ifInfo.centerFreq1 = nla_get_u32(tb_msg[NL80211_ATTR_CENTER_FREQ1]);
            }
            if (tb_msg[NL80211_ATTR_CENTER_FREQ2])
            {
                ifInfo.centerFreq2 = nla_get_u32(tb_msg[NL80211_ATTR_CENTER_FREQ2]);
            }
        }
        else if (tb_msg[NL80211_ATTR_WIPHY_CHANNEL_TYPE])
        {
            ifInfo.channelType = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY_CHANNEL_TYPE]);
        }
    }

    if (tb_msg[NL80211_ATTR_WIPHY_TX_POWER_LEVEL])
    {
        int32_t txp = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY_TX_POWER_LEVEL]);
        ifInfo.txdBm = txp % 100;
        ifInfo.txdBm += txp / 100;
    }

    if (tb_msg[NL80211_ATTR_IFINDEX])
    {
        instance->interfaces.push_back(ifInfo);
    }

    return NL_OK;
}

int WiFIController::processGetInterfaceInfoHandler(struct nl_msg *msg, void *arg)
{
    struct genlmsghdr *gnlh = (genlmsghdr *)nlmsg_data(nlmsg_hdr(msg));
    struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];

    nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
              genlmsg_attrlen(gnlh, 0), NULL);
    WiFIController *wc = (WiFIController *)arg;

    if (tb_msg[NL80211_ATTR_IFNAME])
    {
        wc->currentInterfaceInfo.ifName = nla_get_string(tb_msg[NL80211_ATTR_IFNAME]);
    }
    else
    {
        wc->currentInterfaceInfo.ifName = "Unnamed/non-netdev interface";
        return NL_OK;
    }

    if (tb_msg[NL80211_ATTR_IFINDEX])
    {
        wc->currentInterfaceInfo.ifIndex = nla_get_u32(tb_msg[NL80211_ATTR_IFINDEX]);
    }
    else
    {
        wc->currentInterfaceInfo.ifIndex = 0;
    }

    if (tb_msg[NL80211_ATTR_WDEV])
    {
        wc->currentInterfaceInfo.wdev = (uint64_t)nla_get_u64(tb_msg[NL80211_ATTR_WDEV]);
    }
    else
    {
        wc->currentInterfaceInfo.wdev = 0;
    }

    if (tb_msg[NL80211_ATTR_MAC])
    {
        wc->currentInterfaceInfo.mac = wc->macN2a((const unsigned char *)nla_data(tb_msg[NL80211_ATTR_MAC]));
    }
    else
    {
        wc->currentInterfaceInfo.mac = "";
    }

    if (tb_msg[NL80211_ATTR_SSID])
    {
        wc->currentInterfaceInfo.ssid = (const char *)nla_data(tb_msg[NL80211_ATTR_SSID]);
    }
    else
    {
        wc->currentInterfaceInfo.ssid = "";
    }

    if (tb_msg[NL80211_ATTR_IFTYPE])
    {
        wc->currentInterfaceInfo.ifType = nla_get_u32(tb_msg[NL80211_ATTR_IFTYPE]);
    }
    else
    {
        wc->currentInterfaceInfo.ifType = 0;
    }

    if (tb_msg[NL80211_ATTR_WIPHY])
    {
        wc->currentInterfaceInfo.wiphy = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY]);
    }
    else
    {
        wc->currentInterfaceInfo.wiphy = 0;
    }

    if (tb_msg[NL80211_ATTR_WIPHY_FREQ])
    {
        wc->currentInterfaceInfo.freq = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY_FREQ]);
        wc->currentInterfaceInfo.channel = wc->freqToCHannel(wc->currentInterfaceInfo.freq);
    }
    else
    {
        wc->currentInterfaceInfo.freq = 0;
        wc->currentInterfaceInfo.channel = 0;
    }

    if (tb_msg[NL80211_ATTR_WIPHY_TX_POWER_LEVEL])
    {
        int32_t txp = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY_TX_POWER_LEVEL]);
        wc->currentInterfaceInfo.txdBm = txp % 100;
        wc->currentInterfaceInfo.txdBm += txp / 100;
    }
    else
    {
        wc->currentInterfaceInfo.txdBm = 0;
    }

    return NL_OK;
}

std::string WiFIController::macN2a(const unsigned char *arg)
{
    int i, l;
    char mac_addr[20];

    l = 0;
    for (i = 0; i < ETH_ALEN; i++)
    {
        if (i == 0)
        {
            sprintf(mac_addr + l, "%02x", arg[i]);
            l += 2;
        }
        else
        {
            sprintf(mac_addr + l, ":%02x", arg[i]);
            l += 3;
        }
    }
    std::string ret = mac_addr;
    return ret;
}

int WiFIController::freqToCHannel(int freq)
{
    if (freq < 1000)
        return 0;
    /* see 802.11-2007 17.3.8.3.2 and Annex J */
    if (freq == 2484)
        return 14;
    /* see 802.11ax D6.1 27.3.23.2 and Annex E */
    else if (freq == 5935)
        return 2;
    else if (freq < 2484)
        return (freq - 2407) / 5;
    else if (freq >= 4910 && freq <= 4980)
        return (freq - 4000) / 5;
    else if (freq < 5950)
        return (freq - 5000) / 5;
    else if (freq <= 45000) /* DMG band lower limit */
        /* see 802.11ax D6.1 27.3.23.2 */
        return (freq - 5950) / 5;
    else if (freq >= 58320 && freq <= 70200)
        return (freq - 56160) / 2160;
    else
        return 0;
}

int WiFIController::phyLookup(char *name)
{
    char buf[200];
    int fd, pos;

    snprintf(buf, sizeof(buf), "/sys/class/ieee80211/%s/index", name);

    fd = open(buf, O_RDONLY);
    if (fd < 0)
        return -1;
    pos = read(fd, buf, sizeof(buf) - 1);
    if (pos < 0)
    {
        close(fd);
        return -1;
    }
    buf[pos] = '\0';
    close(fd);
    return atoi(buf);
}

int WiFIController::setApMode()
{
    Cmd cmd{
        .id = NL80211_CMD_SET_INTERFACE,
        .idby = CIB_NETDEV,
        .nlFlags = 0,
        .device = if_nametoindex(AP_INTERFACE_NAME),
        .handler = this->setApModeHandler,
        .valid_handler = NULL,
    };

    return this->nlExecCommand(cmd);
}

int WiFIController::setApModeHandler(struct nl80211_state *state, struct nl_msg *msg, void *arg)
{
    NLA_PUT_U32(msg, NL80211_ATTR_IFTYPE, NL80211_IFTYPE_AP);
    return 0;
nla_put_failure:
    return -ENOBUFS;
}