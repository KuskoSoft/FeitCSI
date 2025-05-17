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

#include "WiFiCsiController.h"
#include "Csi.h"
#include "MainController.h"
#include "Arguments.h"

#include <errno.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/attr.h>
#include <net/if.h>
#include <thread>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "main.h"
#include "Logger.h"
#include "rs.h"

void WiFiCsiController::init()
{
    Netlink::init();
    this->enableCsi();
}

int WiFiCsiController::listenToCsi()
{
    Cmd cmd{
        .id = NL80211_CMD_VENDOR,
        .idby = CIB_NETDEV,
        .nlFlags = 0,
        .device = if_nametoindex(MONITOR_INTERFACE_NAME),
        .handler = this->listenToCsiHandler,
        .valid_handler = this->processListenToCsiHandler,
    };

    return this->nlExecCommand(cmd);
}

int WiFiCsiController::listenToCsiHandler(struct nl80211_state *state, struct nl_msg *msg, void *arg)
{
    NLA_PUT_U32(msg, NL80211_ATTR_VENDOR_ID, 0x001735);
    NLA_PUT_U32(msg, NL80211_ATTR_VENDOR_SUBCMD, 0x24);
    return 0;
nla_put_failure:
    return -ENOBUFS;
}

int WiFiCsiController::processListenToCsiHandler(struct nl_msg *msg, void *arg)
{
    struct nlattr *attrs[MAX_CMD + 1];
    struct nlmsghdr *nlh = nlmsg_hdr(msg);

    nlmsg_parse(nlh, 32, attrs, MAX_CMD, NULL);
    if (attrs[IWL_MVM_VENDOR_ATTR_CSI_HDR])
    {
        unsigned int dataLength = nla_len(attrs[IWL_MVM_VENDOR_ATTR_CSI_HDR]);
        if (dataLength != CSI_HEADER_LENGTH)
        {
            return NL_SKIP;
        }
        
        uint8_t header[dataLength];
        uint8_t *data = (uint8_t *)nla_data(attrs[IWL_MVM_VENDOR_ATTR_CSI_HDR]);
        memcpy(header, data, dataLength);

        if (attrs[IWL_MVM_VENDOR_ATTR_CSI_DATA])
        {
            dataLength = nla_len(attrs[IWL_MVM_VENDOR_ATTR_CSI_DATA]);
            uint8_t rawCsi[dataLength];
            uint8_t *dataCsi = (uint8_t *)nla_data(attrs[IWL_MVM_VENDOR_ATTR_CSI_DATA]);
            memcpy(rawCsi, dataCsi, dataLength);

            Csi *c = new Csi();
            c->loadFromMemory(header, dataCsi);

            if (
                (c->channelWidth == RATE_MCS_CHAN_WIDTH_20 && Arguments::arguments.channelWidth == 20) ||
                (c->channelWidth == RATE_MCS_CHAN_WIDTH_40 && Arguments::arguments.channelWidth == 40) ||
                (c->channelWidth == RATE_MCS_CHAN_WIDTH_80 && Arguments::arguments.channelWidth == 80) ||
                (c->channelWidth == RATE_MCS_CHAN_WIDTH_160 && Arguments::arguments.channelWidth == 160)
            
            )
            {
                if (
                    (c->format == RATE_MCS_LEGACY_OFDM_MSK && Arguments::arguments.format == "NOHT") ||
                    (c->format == RATE_MCS_HT_MSK && Arguments::arguments.format == "HT") ||
                    (c->format == RATE_MCS_VHT_MSK && Arguments::arguments.format == "VHT") ||
                    (c->format == RATE_MCS_HE_MSK && Arguments::arguments.format == "HESU") ||
                    (c->format == RATE_MCS_EHT_MSK && Arguments::arguments.format == "EHT")
                
                )
                {
                    printf("masm csi\n");
                    if (!Arguments::arguments.strict || (Arguments::arguments.strict && (c->rawHeaderData.rateNflag & RATE_LEGACY_RATE_MSK) == Arguments::arguments.mcs))
                    {
                        if (Arguments::arguments.verbose) {
                            printDetail(c);
                        }
                        if ( MainController::getInstance()->udpSocket ) {
                            c->sendUDP(MainController::getInstance()->udpSocket);
                        } else {
                            c->save();
                        }
                        if (Arguments::arguments.plot)
                        {
                            WiFiCsiController::csiQueueMutex.lock();
                            WiFiCsiController::csiQueue.push(c);
                            WiFiCsiController::csiQueueMutex.unlock();
                        }
                    }
                    
                }
            }
        }
    }

    WiFiCsiController *wcc = (WiFiCsiController*) arg;
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    if (wcc->stopTime != 0 && wcc->stopTime < now)
    {
        return NL_OK;
    }

    return NL_SKIP;
}

void WiFiCsiController::printDetail(Csi *c)
{
    Logger::log(info) << "Subcarrier count: " << c->rawHeaderData.numSubCarriers << ", ";
    Logger::log(info, true) << "RX: " << +c->rawHeaderData.numRx << ", ";
    Logger::log(info, true) << "TX: " << +c->rawHeaderData.numTx << ", ";

    switch (c->channelWidth)
    {
    case RATE_MCS_CHAN_WIDTH_20:
        Logger::log(info, true) << "Channel width: 20, ";
        break;
    case RATE_MCS_CHAN_WIDTH_40:
        Logger::log(info, true) << "Channel width: 40, ";
        break;
    case RATE_MCS_CHAN_WIDTH_80:
        Logger::log(info, true) << "Channel width: 80, ";
        break;
    case RATE_MCS_CHAN_WIDTH_160:
        Logger::log(info, true) << "Channel width: 160, ";
        break;
    }
    switch (c->format)
    {
    case RATE_MCS_CCK_MSK: // VERY OLD FORMAT
        Logger::log(info, true) << "Format: CCK\n";
        break;
    case RATE_MCS_LEGACY_OFDM_MSK:
        Logger::log(info, true) << "Format: LEGACY_OFDM\n";
        break;
    case RATE_MCS_HT_MSK:
        Logger::log(info, true) << "Format: HT\n";
        break;
    case RATE_MCS_VHT_MSK:
        Logger::log(info, true) << "Format: VHT\n";
        break;
    case RATE_MCS_HE_MSK:
        Logger::log(info, true) << "Format: HE\n";
        break;
    case RATE_MCS_EHT_MSK:
        Logger::log(info, true) << "Format: EHT\n";
        break;
    }
}

void WiFiCsiController::enableCsi(bool enable)
{
    if (Arguments::arguments.verbose)
    {
        if (enable)
        {
            Logger::log(info) << "Enabling CSI measurement\n";
        } 
        else
        {
            Logger::log(info) << "Disabling CSI measurement\n";
        }
    }

    const char *baseDir = "/sys/kernel/debug/iwlwifi";

    if (!std::filesystem::is_directory(baseDir))
    {
        throw std::ios_base::failure("/sys/kernel/debug/iwlwifi not found. Did you compile iwlwifi with debugfs enabled?\n");
        return;
    }

    bool csiEnabled = false;
    for (const auto &dirEntry : std::filesystem::directory_iterator(baseDir))
    {
        std::filesystem::path path = dirEntry.path().string() + "/iwlmvm/csi_enabled";
        if (std::filesystem::exists(path))
        {
            std::ofstream ofs(path);
            ofs << (enable ? "1" : "0") << std::endl;
            ofs.flush();
            ofs.close();
            csiEnabled = true;
        }
    }

    if (!csiEnabled)
    {
        throw std::ios_base::failure("Failed to enable csi measurement. Maybe device not support it.\n");
    }
}

WiFiCsiController::~WiFiCsiController()
{
    this->enableCsi(false);
}