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

#include "PacketInjector.h"
#include "main.h"
#include "Logger.h"
#include "Arguments.h"
#include <string.h>
#include <iostream>

#define SPATIAL_STREAM 16

uint8_t ieee80211Header[] = {
    0xe0, 0x80, 0x00, 0x00, 0x00, 0x16, 0xea, 0x12, 0x34, 0x56, 0x00, 0x16, 0xea, 0x12, 0x34, 0x56, 0x98, 0x59, 0x7a, 0x8b, 0x34, 0x3b, 0x00, 0x00, 0x15, 0x03, 0x15, 0x20};

uint8_t ieee80211Body[] = {};

void PacketInjector::inject()
{
    if (Arguments::arguments.verbose)
    {
        Logger::log(info) << "Injecting " << Arguments::arguments.format << "\n";
    }

    
    if (Arguments::arguments.format == "NOHT")
    {
        this->injectNoHT();
    }
    else if (Arguments::arguments.format == "HT")
    {
        this->injectHT();
    }
    else if (Arguments::arguments.format == "VHT")
    {
        this->injectVHT();
    }
    else if (Arguments::arguments.format == "HESU")
    {
        this->injectHE();
    }
}

void PacketInjector::injectNoHT()
{
    uint8_t mcs = 0;
    if (RATE_LEGACY_RATE_MSK >= Arguments::arguments.mcs)
    {
        mcs = RATE_LEGACY_RATE_MSK & Arguments::arguments.mcs;
    }
    uint32_t rateNFlags = RATE_MCS_LEGACY_OFDM_MSK | mcs | Arguments::arguments.antenna;

    this->send(rateNFlags);
}

void PacketInjector::injectHT()
{
    uint8_t mcs = 0;
    if (RATE_HT_MCS_CODE_MSK >= Arguments::arguments.mcs)
    {
        mcs = RATE_HT_MCS_CODE_MSK & Arguments::arguments.mcs;
    }
    uint32_t rateNFlags =
        RATE_MCS_HT_MSK |
        mcs |
        Arguments::arguments.antenna |
        (Arguments::arguments.channelWidth == 40 ? RATE_MCS_CHAN_WIDTH_40 : 0) |
        (Arguments::arguments.spatialStreams == 2 ? SPATIAL_STREAM : 0) |
        (Arguments::arguments.spatialStreams == 2 ? RATE_MCS_ANT_AB_MSK : 0) |
        (Arguments::arguments.guardInterval == 400 ? RATE_MCS_SGI_MSK : 0) |
        (Arguments::arguments.coding == "LDPC" ? RATE_MCS_LDPC_MSK : 0);
    this->send(rateNFlags);
}

void PacketInjector::injectVHT()
{
    uint8_t mcs = 0;
    if (RATE_MCS_CODE_MSK >= Arguments::arguments.mcs)
    {
        mcs = RATE_MCS_CODE_MSK & Arguments::arguments.mcs;
    }
    uint32_t rateNFlags =
        RATE_MCS_VHT_MSK |
        mcs |
        Arguments::arguments.antenna |
        (Arguments::arguments.channelWidth == 40 ? RATE_MCS_CHAN_WIDTH_40 : 0) |
        (Arguments::arguments.channelWidth == 80 ? RATE_MCS_CHAN_WIDTH_80 : 0) |
        (Arguments::arguments.channelWidth == 160 ? RATE_MCS_CHAN_WIDTH_160 : 0) |
        (Arguments::arguments.spatialStreams == 2 ? SPATIAL_STREAM : 0) |
        (Arguments::arguments.spatialStreams == 2 ? RATE_MCS_ANT_AB_MSK : 0) |
        (Arguments::arguments.guardInterval == 400 ? RATE_MCS_SGI_MSK : 0) |
        (Arguments::arguments.coding == "LDPC" ? RATE_MCS_LDPC_MSK : 0);
    this->send(rateNFlags);
}

void PacketInjector::injectHE()
{
    uint8_t mcs = 0;
    if (RATE_MCS_CODE_MSK >= Arguments::arguments.mcs)
    {
        mcs = RATE_MCS_CODE_MSK & Arguments::arguments.mcs;
    }

    uint32_t ltf = 1;
    if (Arguments::arguments.ltf == "2xLTF+0.8")
    {
        ltf = 1;
    }
    else if (Arguments::arguments.ltf == "2xLTF+1.6")
    {
        ltf = 2;
    }
    else if (Arguments::arguments.ltf == "4xLTF+3.2")
    {
        ltf = 3;
    }
    else if (Arguments::arguments.ltf == "4xLTF+0.8")
    {
        ltf = 4;
    }
    ltf = (ltf << RATE_MCS_HE_GI_LTF_POS) & RATE_MCS_HE_GI_LTF_MSK;

    uint32_t rateNFlags =
        RATE_MCS_HE_MSK |
        RATE_MCS_LDPC_MSK |
        mcs |
        Arguments::arguments.antenna |
        ltf |
        (Arguments::arguments.channelWidth == 40 ? RATE_MCS_CHAN_WIDTH_40 : 0) |
        (Arguments::arguments.channelWidth == 80 ? RATE_MCS_CHAN_WIDTH_80 : 0) |
        (Arguments::arguments.channelWidth == 160 ? RATE_MCS_CHAN_WIDTH_160 : 0) |
        (Arguments::arguments.spatialStreams == 2 ? SPATIAL_STREAM : 0) |
        (Arguments::arguments.spatialStreams == 2 ? RATE_MCS_ANT_AB_MSK : 0);
    this->send(rateNFlags);
}

void PacketInjector::send(uint32_t rateNFlags)
{
    uint32_t pos = 0;
    struct ieee80211_radiotap_header rthdr;
    rthdr.it_version = 0;
    rthdr.it_pad = 0;
    rthdr.it_present = 0;

    rthdr.it_present |= BIT(IEEE80211_RADIOTAP_FLAGS);
    rthdr.it_optional[pos++] = 0x80;

    memcpy(&rthdr.it_optional[pos++], &rateNFlags, 4);
    pos += 3;

    rthdr.it_len = pos + 8;

    uint8_t sendBuffer[1000];
    memcpy(sendBuffer, (uint8_t *)&rthdr, rthdr.it_len);
    memcpy(&sendBuffer[rthdr.it_len], ieee80211Header, sizeof(ieee80211Header));
    memcpy(&sendBuffer[rthdr.it_len + sizeof(ieee80211Header)], ieee80211Body, sizeof(ieee80211Body));

    int totalSize = rthdr.it_len + sizeof(ieee80211Header) + sizeof(ieee80211Body);

    char szErrbuf[500];

    if (ppcap == nullptr) {
        ppcap = pcap_open_live(MONITOR_INTERFACE_NAME, 800, 1, 20, szErrbuf);
    }

    int r = pcap_inject(ppcap, sendBuffer, totalSize);
    if (r != totalSize)
    {
        //pcap_perror(ppcap, "Failed to inject packet");
        pcap_close(ppcap);
        ppcap = nullptr;
        //throw std::ios_base::failure("Failed to inject packet\n");
    }

    //pcap_close(ppcap);
}
