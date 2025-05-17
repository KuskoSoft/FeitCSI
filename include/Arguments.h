/*
 * FeitCSI is the tool for extracting CSI information from supported intel NICs.
 * Copyright (C) 2024-2025 Miroslav Hutar.
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

#ifndef ARGUMENTS_PARSER_H
#define ARGUMENTS_PARSER_H

#include <string>
#include <cstdint>
#include <map>
#include <argp.h>
#include "main.h"

#define ETH_ALEN 6

struct Args
{
    bool strict;
    bool verbose;
    uint16_t frequency;
    bool gui = false;
    bool udpSocket = false;
    bool plot = false;
    std::string bandwidth;
    std::string outputFile;
    uint8_t mcs;
    uint16_t channelWidth;
    uint8_t spatialStreams;
    uint8_t txPower;
    uint32_t antenna;
    uint16_t guardInterval;
    uint32_t injectDelay;
    uint32_t injectRepeat;
    std::string coding;
    std::string format;
    bool inject;
    bool measure;
    std::string mode;
    std::string ltf;
    uint32_t modeDelay;
    bool ftm = false;
    bool ftmResponder = false;
    bool ftmAsap = false;
    uint8_t ftmBurstExp;
    uint8_t ftmPerBurst;
    uint16_t ftmBurstPeriod;
    uint8_t ftmBurstDuration;
    uint8_t mac[ETH_ALEN];
    uint8_t ftmTargetMac[ETH_ALEN];
    std::string inputFile;
    std::map<enum processor, bool> processors;
};

class Arguments
{

public:
    inline static struct Args arguments;

    static void init();
    static void parse(int argc, char *argv[]);

    static error_t parse_opt(int key, char *arg, argp_state *state);

private:

    /* Program documentation. */
    inline static char doc[] =
        "FeitCSI - FeitCSI, the tool that enables CSI extraction and injection IEEE 802.11 frames";

    /* A description of the arguments we accept. */
    inline static char args_doc[] = "ARG1 ARG2";

    /* The options we understand. */
    inline static struct argp_option options[] = {
        {"frequency", 'f', "FREQUENCY", 0, "Frequency to measure/inject CSI"},
        {"channel-width", 'w', "CHANNELWIDTH", 0, "Channel width to measure/inject CSI. Possible values [20|40|HT40-|80|160]"},
        {"output-file", 'o', "FILE", 0, "Output file where measurements should be stored."},
        {"mcs", 'm', "MCS", 0, "Mcs index [0-11]"},
        {"format", 'r', "FORMAT", 0, "Data frame format [NOHT|HT|VHT|HESU]"},
        {"spatial-streams", 's', "SPATIALSTREAMS", 0, "Number of spatial streams [1|2]"},
        {"guard-interval", 'g', "GUARDINTERVAL", 0, "Guard interval in ns [400|800]"},
        {"ltf", 'l', "LTF", 0, "HE LTF [2xLTF+0.8|2xLTF+1.6|4xLTF+3.2|4xLTF+0.8]"},
        {"coding", 'c', "CODING", 0, "Coding scheme [LDPC|BCC]"},
        {"tx-power", 't', "TXPOWER", 0, "TX power of antenna in dBm [1-22]"},
        {"antenna", 'a', "ANTENNA", 0, "Transmitting antenna 1, 2 or 12 for both"},
        {"mode", 'i', "MODE", 0, "Mode of program[measure|inject|measureinject|ftm|ftmres|injectftmres|measureftm]"},
        {"inject-delay", 'd', "INJECTDELAY", 0, "Delay between frame injections is us"},
        {"inject-repeat", 'j', "INJECTREPEAT", 0, "How many times inject frame"},
        {"verbose", 'v', 0, OPTION_ARG_OPTIONAL, "Produce verbose output"},
        {"plot", 'p', 0, OPTION_ARG_OPTIONAL, "Plot CSI data"},
        {"gui", 'x', 0, OPTION_ARG_OPTIONAL, "Run application in GUI"},
        {"udp-socket", 'u', 0, OPTION_ARG_OPTIONAL, "Run application and listen to UDP"},
        {"ftm-asap", 'b', 0, OPTION_ARG_OPTIONAL, "FTM asap mode"},
        {"ftm-burst-exp", 'q', "FTMBURSTEXP", 0, "FTM burst exponent"},
        {"ftm-per-burst", 'e', "FTMPERBURST", 0, "FTM samples per burst"},
        {"ftm-burst-period", 'h', "FTMBURSTPERIOD", 0, "FTM burst period"},
        {"ftm-burst-duration", 'k', "FTMBURTSDURATION", 0, "FTM burst duration"},
        {"ftm-mac", 'n', "FTMMAC", 0, "FTM target MAC address xx:xx:xx:xx:xx:xx"},
        {"mode-delay", 'y', "SWAPTIME", 0, "Delay in ms between inject and ftm responder or measure and ftm initiator when modes are injectftmres|measureftm"},
        {"strict", 'z', 0, OPTION_ARG_OPTIONAL, "Strict mode: filter out values that do not contain a specific MCS"},
        {"mac", '#', "MAC", 0, "Default NICs MAC will be change to providing MAC xx:xx:xx:xx:xx:xx"},
        {0}};
};

#endif