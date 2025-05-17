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

#ifndef WIFI_CSI_CONTROLLER_H
#define WIFI_CSI_CONTROLLER_H

#include "Netlink.h"
#include "Csi.h"
#include <mutex>
#include <queue>

#define IWL_MVM_VENDOR_ATTR_CSI_HDR 0x4d
#define IWL_MVM_VENDOR_ATTR_CSI_DATA 0x4e
#define MAX_CMD 0x4f

class WiFiCsiController : public Netlink
{

public:
    void init();
    int listenToCsi();
    static void enableCsi(bool enable = true);
    inline static std::mutex csiQueueMutex;
    inline static std::queue<Csi*> csiQueue;
    int64_t stopTime = 0;

    ~WiFiCsiController();

private:
    static int listenToCsiHandler(nl80211_state *state, nl_msg *msg, void *arg);
    static int processListenToCsiHandler(nl_msg *msg, void *arg);
    static void printDetail(Csi *c);
};

#endif