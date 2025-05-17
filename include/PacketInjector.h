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

#ifndef PACKET_INJECTOR_H
#define PACKET_INJECTOR_H

#include <stdint.h>
#include "ieee80211_radiotap.h"
#include "rs.h"
#include <pcap.h>

#define BIT(n) (0x1U << (n))

class PacketInjector
{

public:
    void inject();
    void injectNoHT();
    void injectHT();
    void injectVHT();
    void injectHE();

private:
    void send(uint32_t rateNFlags);
    pcap_t *ppcap = nullptr;
};

#endif