/*
 * FeitCSI is the tool for extracting CSI information from supported intel NICs.
 * Copyright (C) 2025 Miroslav Hutar.
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

#ifndef WIFI_FTM_CONTROLLER_H
#define WIFI_FTM_CONTROLLER_H

#include "Netlink.h"

struct __attribute__((__packed__)) Ftm
{
    uint32_t burstIndex;
    uint32_t numFtmrAttempts;
    uint32_t numFtmrSuccesses;
    uint32_t numBurstsExp;
    uint8_t burstDuration;
    uint8_t ftmsPerBurst;
    uint8_t rssiAvg;
    uint32_t rssiSpread;
    uint64_t rttAvg;
    uint64_t rttVariance;
    uint64_t rttSpread;
    uint64_t distAvg;
    uint64_t distVariance;
    uint64_t distSpread;
    uint64_t timestamp;
}; // size 79 bytes

#define FTM_SIZE sizeof(Ftm)

class WiFiFtmController : public Netlink
{

public:
    void init();
    int startInitiator();
    int startResponder();
    int setApMode();
    uint64_t lastRttIsSuccess = false;

private:

    static int ftmHandler(nl80211_state *state, nl_msg *msg, void *arg);
    static int ftmResponderHandler(nl80211_state *state, nl_msg *msg, void *arg);
    static int setApModeHandler(nl80211_state *state, nl_msg *msg, void *arg);
    static int processFtmHandler(nl_msg *msg, void *arg);
};

#endif