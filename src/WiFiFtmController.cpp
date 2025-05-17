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

#include "WiFiFtmController.h"
#include "main.h"
#include "Arguments.h"
#include <net/if.h>
#include "Logger.h"
#include "MainController.h"
#include <netlink/genl/genl.h>
#include <fstream>
#include <filesystem>
#include <cstring>

uint8_t beaconHeader[] = {0x80, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xea, 0xb9, 0xcc, 0xc2, 0x4a, 0xaf, 0xea, 0xb9, 0xcc, 0xc2, 0x4a, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x01, 0x04, 0x00, 0x07, 0x46, 0x65, 0x69, 0x74, 0x43, 0x53, 0x49, 0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24, 0x03, 0x01, 0x01};

void WiFiFtmController::init()
{
    Netlink::init();
}

int WiFiFtmController::startInitiator()
{
    Cmd cmd{
        .id = NL80211_CMD_PEER_MEASUREMENT_START,
        .idby = CIB_NETDEV,
        .nlFlags = 0,
        .device = if_nametoindex(AP_INTERFACE_NAME),
        .handler = this->ftmHandler,
        .valid_handler = this->processFtmHandler,
    };

    return this->nlExecCommand(cmd);
}

int WiFiFtmController::startResponder()
{
    Cmd cmd{
        .id = NL80211_CMD_NEW_BEACON,
        .idby = CIB_NETDEV,
        .nlFlags = 0,
        .device = if_nametoindex(AP_INTERFACE_NAME),
        .handler = this->ftmResponderHandler,
        .valid_handler = NULL,
    };

    return this->nlExecCommand(cmd);
}

int WiFiFtmController::setApMode()
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

int WiFiFtmController::ftmHandler(struct nl80211_state *state, struct nl_msg *msg, void *arg)
{

    struct nlattr *pmsr, *peers, *peer, *req, *reqdata, *ftm, *chan;

    pmsr = nla_nest_start(msg, NL80211_ATTR_PEER_MEASUREMENTS);
    peers = nla_nest_start(msg, NL80211_PMSR_ATTR_PEERS);

    peer = nla_nest_start(msg, 1); // TODO multiple peers
    NLA_PUT(msg, NL80211_PMSR_PEER_ATTR_ADDR, ETH_ALEN, Arguments::arguments.ftmTargetMac);

    req = nla_nest_start(msg, NL80211_PMSR_PEER_ATTR_REQ);
    reqdata = nla_nest_start(msg, NL80211_PMSR_REQ_ATTR_DATA);
    ftm = nla_nest_start(msg, NL80211_PMSR_TYPE_FTM);

    if (Arguments::arguments.ftmBurstExp)
    {
        NLA_PUT_U8(msg, NL80211_PMSR_FTM_REQ_ATTR_NUM_BURSTS_EXP, Arguments::arguments.ftmBurstExp);
    }
    if (Arguments::arguments.ftmBurstPeriod)
    {
        NLA_PUT_U16(msg, NL80211_PMSR_FTM_REQ_ATTR_BURST_PERIOD, Arguments::arguments.ftmBurstPeriod);
    }
    if (Arguments::arguments.ftmBurstDuration)
    {
        NLA_PUT_U8(msg, NL80211_PMSR_FTM_REQ_ATTR_BURST_DURATION, Arguments::arguments.ftmBurstDuration);
    }
    if (Arguments::arguments.ftmPerBurst)
    {
        NLA_PUT_U8(msg, NL80211_PMSR_FTM_REQ_ATTR_FTMS_PER_BURST, Arguments::arguments.ftmPerBurst);
    }
    if (Arguments::arguments.ftmAsap)
    {
        NLA_PUT_FLAG(msg, NL80211_PMSR_FTM_REQ_ATTR_ASAP);
    }

    if (Arguments::arguments.format == "NOHT")
    {
        NLA_PUT_U32(msg, NL80211_PMSR_FTM_REQ_ATTR_PREAMBLE, NL80211_PREAMBLE_LEGACY);
    }
    else if (Arguments::arguments.format == "HT")
    {
        NLA_PUT_U32(msg, NL80211_PMSR_FTM_REQ_ATTR_PREAMBLE, NL80211_PREAMBLE_HT);
    }
    else if (Arguments::arguments.format == "VHT")
    {
        NLA_PUT_U32(msg, NL80211_PMSR_FTM_REQ_ATTR_PREAMBLE, NL80211_PREAMBLE_VHT);
    }
    else if (Arguments::arguments.format == "HESU")
    {
        NLA_PUT_U32(msg, NL80211_PMSR_FTM_REQ_ATTR_PREAMBLE, NL80211_PREAMBLE_HE);
    }

    nla_nest_end(msg, ftm);
    nla_nest_end(msg, reqdata);
    nla_nest_end(msg, req);

    chan = nla_nest_start(msg, NL80211_PMSR_PEER_ATTR_CHAN);
    NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_FREQ, (uint32_t)Arguments::arguments.frequency);

    switch (Arguments::arguments.channelWidth)
    {
    case 20:
        NLA_PUT_U32(msg, NL80211_ATTR_CHANNEL_WIDTH, NL80211_CHAN_WIDTH_20);
        break;
    case 40:
        NLA_PUT_U32(msg, NL80211_ATTR_CHANNEL_WIDTH, NL80211_CHAN_WIDTH_40);
        break;
    case 80:
        NLA_PUT_U32(msg, NL80211_ATTR_CHANNEL_WIDTH, NL80211_CHAN_WIDTH_80);
        break;
    case 160:
        NLA_PUT_U32(msg, NL80211_ATTR_CHANNEL_WIDTH, NL80211_CHAN_WIDTH_160);
        break;
    case 320:
        NLA_PUT_U32(msg, NL80211_ATTR_CHANNEL_WIDTH, NL80211_CHAN_WIDTH_320);
        break;
    }

    nla_nest_end(msg, chan);
    nla_nest_end(msg, peer);
    nla_nest_end(msg, peers);
    nla_nest_end(msg, pmsr);

    return 0;
nla_put_failure:
    return -ENOBUFS;
}

int WiFiFtmController::ftmResponderHandler(struct nl80211_state *state, struct nl_msg *msg, void *arg)
{
    NLA_PUT(msg, NL80211_ATTR_BEACON_HEAD, 58, beaconHeader);

    NLA_PUT_U32(msg, NL80211_ATTR_BEACON_INTERVAL, 100);
    NLA_PUT_U32(msg, NL80211_ATTR_DTIM_PERIOD, 2);
    NLA_PUT(msg, NL80211_ATTR_SSID, 7, "FeitCSI");
    NLA_PUT_U32(msg, NL80211_ATTR_HIDDEN_SSID, NL80211_HIDDEN_SSID_NOT_IN_USE);
    NLA_PUT_U32(msg, NL80211_ATTR_AUTH_TYPE, NL80211_AUTHTYPE_OPEN_SYSTEM);

    struct nlattr *ftm;
    ftm = nla_nest_start(msg, NL80211_ATTR_FTM_RESPONDER);
    NLA_PUT_FLAG(msg, NL80211_FTM_RESP_ATTR_ENABLED);
    nla_nest_end(msg, ftm);

    NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_FREQ, (uint32_t)Arguments::arguments.frequency);
    switch (Arguments::arguments.channelWidth)
    {
    case 20:
        NLA_PUT_U32(msg, NL80211_ATTR_CHANNEL_WIDTH, NL80211_CHAN_WIDTH_20);
        break;
    case 40:
        NLA_PUT_U32(msg, NL80211_ATTR_CHANNEL_WIDTH, NL80211_CHAN_WIDTH_40);
        break;
    case 80:
        NLA_PUT_U32(msg, NL80211_ATTR_CHANNEL_WIDTH, NL80211_CHAN_WIDTH_80);
        break;
    case 160:
        NLA_PUT_U32(msg, NL80211_ATTR_CHANNEL_WIDTH, NL80211_CHAN_WIDTH_160);
        break;
    case 320:
        NLA_PUT_U32(msg, NL80211_ATTR_CHANNEL_WIDTH, NL80211_CHAN_WIDTH_320);
        break;
    }

    NLA_PUT_FLAG(msg, NL80211_ATTR_SOCKET_OWNER);

    // nl_msg_dump(msg, stdout);

    return 0;
nla_put_failure:
    return -ENOBUFS;
}

int WiFiFtmController::setApModeHandler(struct nl80211_state *state, struct nl_msg *msg, void *arg)
{
    NLA_PUT_U32(msg, NL80211_ATTR_IFTYPE, NL80211_IFTYPE_AP);
    return 0;
nla_put_failure:
    return -ENOBUFS;
}

int WiFiFtmController::processFtmHandler(struct nl_msg *msg, void *arg)
{
    struct genlmsghdr *gnlh = (genlmsghdr *)nlmsg_data(nlmsg_hdr(msg));
    struct nlattr *tb[NL80211_ATTR_MAX + 1];

    nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

    if (gnlh->cmd == NL80211_CMD_PEER_MEASUREMENT_COMPLETE)
    {
        return NL_OK;
    }

    if (gnlh->cmd == NL80211_CMD_PEER_MEASUREMENT_RESULT)
    {
        if (!tb[NL80211_ATTR_PEER_MEASUREMENTS])
        {
            return NL_OK;
        }
        struct nlattr *pmsr[NL80211_PMSR_ATTR_MAX + 1];
        nla_parse_nested(pmsr, NL80211_PMSR_ATTR_MAX, tb[NL80211_ATTR_PEER_MEASUREMENTS], NULL);

        if (!pmsr[NL80211_PMSR_ATTR_PEERS])
        {
            return NL_OK;
        }

        struct nlattr *peer = (nlattr *)nla_data(pmsr[NL80211_PMSR_ATTR_PEERS]);

        struct nlattr *tbpeer[NL80211_PMSR_PEER_ATTR_MAX + 1];
        struct nlattr *resp[NL80211_PMSR_RESP_ATTR_MAX + 1];
        struct nlattr *data[NL80211_PMSR_TYPE_MAX + 1];

        nla_parse_nested(tbpeer, NL80211_PMSR_PEER_ATTR_MAX, peer, NULL);

        if (!tbpeer[NL80211_PMSR_PEER_ATTR_ADDR])
        {
            return NL_OK;
        }

        uint8_t mac[ETH_ALEN];
        memcpy(mac, nla_data(tbpeer[NL80211_PMSR_PEER_ATTR_ADDR]), ETH_ALEN);

        if (!tbpeer[NL80211_PMSR_PEER_ATTR_ADDR])
        {
            return NL_OK;
        }

        nla_parse_nested(resp, NL80211_PMSR_RESP_ATTR_MAX, tbpeer[NL80211_PMSR_PEER_ATTR_RESP], NULL);

        if (!resp[NL80211_PMSR_RESP_ATTR_DATA])
        {
            return NL_OK;
        }

        nla_parse_nested(data, NL80211_PMSR_TYPE_MAX, resp[NL80211_PMSR_RESP_ATTR_DATA], NULL);

        if (!data[NL80211_PMSR_TYPE_FTM])
        {
            return NL_OK;
        }

        struct nlattr *ftm[NL80211_PMSR_FTM_RESP_ATTR_MAX + 1];
        nla_parse_nested(ftm, NL80211_PMSR_FTM_RESP_ATTR_MAX, data[NL80211_PMSR_TYPE_FTM], NULL);
        WiFiFtmController *wfc = (WiFiFtmController *)arg;
        if (ftm[NL80211_PMSR_FTM_RESP_ATTR_FAIL_REASON])
        {
            uint32_t reason = nla_get_u32(ftm[NL80211_PMSR_FTM_RESP_ATTR_FAIL_REASON]);
            if (Arguments::arguments.verbose)
            {
                Logger::log(info) << "FTM failed to measure " << reason << "\n";
            }
            wfc->lastRttIsSuccess = false;
            return NL_OK;
        }

        Ftm ftmData{
            .burstIndex = (uint32_t)(ftm[NL80211_PMSR_FTM_RESP_ATTR_BURST_INDEX] ? nla_get_u32(ftm[NL80211_PMSR_FTM_RESP_ATTR_BURST_INDEX]) : 0),
            .numFtmrAttempts = (uint32_t)(ftm[NL80211_PMSR_FTM_RESP_ATTR_NUM_FTMR_ATTEMPTS] ? nla_get_u32(ftm[NL80211_PMSR_FTM_RESP_ATTR_NUM_FTMR_ATTEMPTS]) : 0),
            .numFtmrSuccesses = (uint32_t)(ftm[NL80211_PMSR_FTM_RESP_ATTR_NUM_FTMR_SUCCESSES] ? nla_get_u32(ftm[NL80211_PMSR_FTM_RESP_ATTR_NUM_FTMR_SUCCESSES]) : 0),
            .numBurstsExp = (uint32_t)(ftm[NL80211_PMSR_FTM_RESP_ATTR_NUM_BURSTS_EXP] ? nla_get_u32(ftm[NL80211_PMSR_FTM_RESP_ATTR_NUM_BURSTS_EXP]) : 0),
            .burstDuration = (uint8_t)(ftm[NL80211_PMSR_FTM_RESP_ATTR_BURST_DURATION] ? nla_get_u8(ftm[NL80211_PMSR_FTM_RESP_ATTR_BURST_DURATION]) : 0),
            .ftmsPerBurst = (uint8_t)(ftm[NL80211_PMSR_FTM_RESP_ATTR_FTMS_PER_BURST] ? nla_get_u8(ftm[NL80211_PMSR_FTM_RESP_ATTR_FTMS_PER_BURST]) : 0),
            .rssiAvg = (uint8_t)(ftm[NL80211_PMSR_FTM_RESP_ATTR_RSSI_AVG] ? nla_get_u8(ftm[NL80211_PMSR_FTM_RESP_ATTR_RSSI_AVG]) : 0),
            .rssiSpread = (uint32_t)(ftm[NL80211_PMSR_FTM_RESP_ATTR_RSSI_SPREAD] ? nla_get_u32(ftm[NL80211_PMSR_FTM_RESP_ATTR_RSSI_SPREAD]) : 0),
            .rttAvg = (uint64_t)(ftm[NL80211_PMSR_FTM_RESP_ATTR_RTT_AVG] ? nla_get_u64(ftm[NL80211_PMSR_FTM_RESP_ATTR_RTT_AVG]) : 0),
            .rttVariance = (uint64_t)(ftm[NL80211_PMSR_FTM_RESP_ATTR_RTT_VARIANCE] ? nla_get_u64(ftm[NL80211_PMSR_FTM_RESP_ATTR_RTT_VARIANCE]) : 0),
            .rttSpread = (uint64_t)(ftm[NL80211_PMSR_FTM_RESP_ATTR_RTT_SPREAD] ? nla_get_u64(ftm[NL80211_PMSR_FTM_RESP_ATTR_RTT_SPREAD]) : 0),
            .distAvg = (uint64_t)(ftm[NL80211_PMSR_FTM_RESP_ATTR_DIST_AVG] ? nla_get_u64(ftm[NL80211_PMSR_FTM_RESP_ATTR_RTT_AVG]) : 0),
            .distVariance = (uint64_t)(ftm[NL80211_PMSR_FTM_RESP_ATTR_DIST_VARIANCE] ? nla_get_u64(ftm[NL80211_PMSR_FTM_RESP_ATTR_DIST_VARIANCE]) : 0),
            .distSpread = (uint64_t)(ftm[NL80211_PMSR_FTM_RESP_ATTR_DIST_SPREAD] ? nla_get_u64(ftm[NL80211_PMSR_FTM_RESP_ATTR_DIST_SPREAD]) : 0),
            .timestamp = (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count(),
        };

        if (Arguments::arguments.verbose)
        {
            Logger::log(info) << "FTM average RTT: " << ftmData.rttAvg << "ps\n";
        }

        if (MainController::getInstance()->udpSocket)
        {
            MainController::getInstance()->udpSocket->send(reinterpret_cast<char *>(&ftmData), FTM_SIZE);
        } else {
            std::ofstream outfile;
            std::string filename = std::string("FTM_") + Arguments::arguments.outputFile;
            outfile.open(filename, std::ios_base::app | std::ios::binary);
            if (outfile.fail())
            {
                throw std::ios_base::failure("Open file failed: " + std::string(std::strerror(errno)));
            }
            outfile.write(reinterpret_cast<char *>(&ftmData), FTM_SIZE);
            outfile.close();
            std::filesystem::permissions(filename, std::filesystem::perms::all & ~(std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec | std::filesystem::perms::others_exec), std::filesystem::perm_options::add);
        }

        wfc->lastRttIsSuccess = true;
        return NL_OK;
    }
    
    return NL_OK;
}