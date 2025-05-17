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

#ifndef WIFI_CONTROLLER_H
#define WIFI_CONTROLLER_H

/* #define CONFIG_LIBNL30 // fix error compatibility iw
#include <iw/iw.h>
#include <iw/nl80211.h> */
#include "Netlink.h"
#include <linux/netlink.h>
#include <netlink/msg.h>
#include <linux/nl80211.h>

#include <string>
#include <vector>

#define ARRAY_SIZE(ar) (sizeof(ar) / sizeof(ar[0]))

struct InterfaceInfo
{
    std::string ifName;
    int ifIndex;
    uint64_t wdev;
    std::string mac;
    std::string ssid;
    int ifType;
    int wiphy;
    uint32_t freq;
    int channel;
    int channelWidth;
    int centerFreq1;
    int centerFreq2;
    int channelType;
    double txdBm;
};

struct ChanMode
{
    const char *name;
    unsigned int width;
    int freq1_diff;
    int chantype; /* for older kernel */
};

class WiFIController : public Netlink
{

public:
    void killNetworkProcesses();
    void getInterfaces();
    void getInterfaceInfo(const char* ifname);
    void setTxPower();
    void addDevice(const char *name, const nl80211_iftype type);
    int setInterfaceUpDown(const char *ifName, bool up);
    void setFreq(uint16_t freq, const char *bw);
    void removeInterface(const char *name, int ifIndex = 0);
    void createMonitorInteface();
    void createApInteface();
    void getPhys();
    int setApMode();
    static ChanMode getChanMode(const char *width);
    static uint16_t chanModeToWidth(ChanMode &chanMode);
    std::vector<InterfaceInfo> interfaces;
    std::vector<int64_t> phys;
    InterfaceInfo currentInterfaceInfo = {};

private:
    std::string macN2a(const unsigned char *arg);
    int freqToCHannel(int freq);
    int phyLookup(char *name);
    std::string currentDeviceName;

    static int getCf1(const ChanMode *chanmode, unsigned long freq);
    static int processGetInterfacesHandler(nl_msg *msg, void *arg);
    static int processGetInterfaceInfoHandler(nl_msg *msg, void *arg);
    static int processGetPhysHandler(nl_msg *msg, void *arg);
    static int processSetFreq(nl80211_state *state, nl_msg *msg, void *arg);
    static int setTxPowerHandler(nl80211_state *state, nl_msg *msg, void *arg);
    static int processaddDeviceHandler(nl80211_state *state, nl_msg *msg, void *arg);
    static int setApModeHandler(nl80211_state *state, nl_msg *msg, void *arg);
};

#endif