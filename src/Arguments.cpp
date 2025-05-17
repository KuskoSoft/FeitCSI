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

#include "Arguments.h"
#include "WiFIController.h"
#include "rs.h"

const std::string VERSION = (std::string("FeitCSI ") + FEITCSI_VERSION);
const char *argp_program_version = VERSION.c_str();
const char *argp_program_bug_address = "https://github.com/KuskoSoft/FeitCSI/issues";

void Arguments::init()
{
    Arguments::arguments = {
        .strict = false,
        .verbose = false,
        .frequency = 2412,
        .gui = false,
        .udpSocket = false,
        .plot = false,
        .bandwidth = "20",
        .mcs = 0,
        .channelWidth = 20,
        .spatialStreams = 1,
        .txPower = 10,
        .antenna = RATE_MCS_ANT_A_MSK,
        .guardInterval = 400,
        .injectDelay = 100000,
        .injectRepeat = 0,
        .coding = "LDPC",
        .format = "HT",
        .inject = false,
        .measure = true,
        .mode = "measure",
        .ltf = "1xLTF+0.8",
        .modeDelay = 3000,
        .ftm = false,
        .ftmResponder = false,
        .ftmAsap = false,
        .ftmBurstExp = 0,
        .ftmPerBurst = 0,
        .ftmBurstPeriod = 0,
        .ftmBurstDuration = 0,
        .mac = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}
    };
}

void Arguments::parse(int argc, char *argv[])
{
    static struct argp argp = {options, parse_opt, args_doc, doc};

    argp_parse(&argp, argc, argv, 0, 0, &arguments);
}

error_t Arguments::parse_opt(int key, char *arg, struct argp_state *state)
{
    /* Get the input argument from argp_parse, which we
       know is a pointer to our arguments structure. */
    struct Args *args = (struct Args *)state->input;

    switch (key)
    {
    case 'v':
        args->verbose = true;
        break;
    case 'z':
        args->strict = true;
        break;
    case 'x':
        args->gui = true;
        break;
    case 'u':
        args->udpSocket = true;
        break;
    case 'p':
        args->plot = true;
        break;
    case 'i':
    {
        args->mode.assign(arg);
        if (args->mode == "measure")
        {
            args->measure = true;
        }
        else if (args->mode == "inject")
        {
            args->inject = true;
            args->measure = false;
        }
        else if (args->mode == "measureinject")
        {
            args->measure = true;
            args->inject = true;
        }
        else if (args->mode == "measureftm")
        {
            args->measure = true;
            args->ftm = true;
        }
        else if (args->mode == "ftm")
        {
            args->measure = false;
            args->ftm = true;
        }
        else if (args->mode == "ftmres")
        {
            args->measure = false;
            args->ftmResponder = true;
        }
        else if (args->mode == "injectftmres")
        {
            args->measure = false;
            args->ftmResponder = true;
            args->inject = true;
        }
        else
        {
            argp_failure(state, 1, 0, "Bad mode. Possible values [measure|inject|measureinject|ftm]");
            exit(ARGP_ERR_UNKNOWN);
        }
        break;
    }
    case 'm':
    {
        int mcs = std::atoi(arg);
        if (mcs < 0 || mcs > 11)
        {
            argp_failure(state, 1, 0, "Bad MCS index. Possible values [0-11]");
            exit(ARGP_ERR_UNKNOWN);
        }
        args->mcs = (uint8_t)mcs;
        break;
    }
    case 'r':
    {
        args->format.assign(arg);
        if (args->format == "NOHT" || args->format == "HT" || args->format == "VHT" || args->format == "HESU")
        {
        }
        else
        {
            argp_failure(state, 1, 0, "Bad format. Possible values [NOHT|HT|VHT|HESU]");
            exit(ARGP_ERR_UNKNOWN);
        }
        break;
    }
    case 'c':
    {
        args->coding.assign(arg);
        if (args->coding == "LDPC" || args->coding == "BCC")
        {
        }
        else
        {
            argp_failure(state, 1, 0, "Bad coding. Possible values [LDPC|BCC]");
            exit(ARGP_ERR_UNKNOWN);
        }
        break;
    }
    case 'l':
    {
        args->ltf.assign(arg);
        if (args->ltf == "1xLTF+0.8" || args->ltf == "2xLTF+0.8" || args->ltf == "2xLTF+1.6" || args->ltf == "4xLTF+3.2" || args->ltf == "4xLTF+0.8")
        {
        }
        else
        {
            argp_failure(state, 1, 0, "Bad LTF. Possible values [2xLTF+0.8|2xLTF+1.6|4xLTF+3.2|4xLTF+0.8]");
            exit(ARGP_ERR_UNKNOWN);
        }
        break;
    }
    case 'y':
    {
        int modeDelay = std::atoi(arg);
        if (modeDelay <= 0)
        {
            argp_failure(state, 1, 0, "Mode delay is not correct number");
            exit(ARGP_ERR_UNKNOWN);
        }
        args->modeDelay = (uint32_t)modeDelay;
        break;
    }
    case 'g':
    {
        int gi = std::atoi(arg);
        if (gi == 400 || gi == 800)
        {
            args->guardInterval = (uint16_t)gi;
        }
        else
        {
            argp_failure(state, 1, 0, "Bad guard interval. Possible values [400|800]");
            exit(ARGP_ERR_UNKNOWN);
        }
        break;
    }
    case 'd':
    {
        int injd = std::atoi(arg);
        if (injd <= 0)
        {
            argp_failure(state, 1, 0, "Inject delay is not correct number");
            exit(ARGP_ERR_UNKNOWN);
        }
        args->injectDelay = (uint32_t)injd;
        break;
    }
    case 'j':
    {
        int injr = std::atoi(arg);
        if (injr <= 0)
        {
            argp_failure(state, 1, 0, "Inject repeat is not correct number");
            exit(ARGP_ERR_UNKNOWN);
        }
        args->injectRepeat = (uint32_t)injr;
        break;
    }
    case 's':
    {
        int ss = std::atoi(arg);
        if (ss < 1 || ss > 2)
        {
            argp_failure(state, 1, 0, "Bad spatial stream. Possible values [1|2]");
            exit(ARGP_ERR_UNKNOWN);
        }
        args->spatialStreams = (uint8_t)ss;
        break;
    }
    case 't':
    {
        int tx = std::atoi(arg);
        if (tx < 1 || tx > 22)
        {
            argp_failure(state, 1, 0, "Bad tx power. Possible values [1-22]");
            exit(ARGP_ERR_UNKNOWN);
        }
        args->txPower = (uint8_t)tx;
        break;
    }
    case 'a':
    {
        int a = std::atoi(arg);
        if (a == 1)
        {
            args->antenna = RATE_MCS_ANT_A_MSK;
        }
        else if (a == 2)
        {
            args->antenna = RATE_MCS_ANT_B_MSK;
        }
        else if (a == 12)
        {
            args->antenna = RATE_MCS_ANT_AB_MSK;
        }
        else
        {
            argp_failure(state, 1, 0, "Bad transmitting antenna value. Possible values 1, 2 or 12 for both");
            exit(ARGP_ERR_UNKNOWN);
        }
        break;
    }
    case 'f':
    {
        int f = std::atoi(arg);
        if (f <= 0)
        {
            argp_failure(state, 1, 0, "Frequency is not correct");
            exit(ARGP_ERR_UNKNOWN);
        }
        args->frequency = (uint16_t)f;
        break;
    }
    case 'w':
    {
        struct ChanMode chMode = WiFIController::getChanMode(arg);
        if (chMode.width == 0)
        {
            argp_failure(state, 1, 0, "Bad bandwidth. Possible values of bandwidth are [20|40|HT40-|80|160]");
            exit(ARGP_ERR_UNKNOWN);
        }
        args->bandwidth = arg;
        args->channelWidth = WiFIController::chanModeToWidth(chMode);
        break;
    }
    case 'o':
        args->outputFile = arg;
        break;
    case 'b':
        args->ftmAsap = true;
        break;
    case 'q':
    {
        int f = std::atoi(arg);
        if (f <= 0)
        {
            argp_failure(state, 1, 0, "FTM burst exponent is not correct");
            exit(ARGP_ERR_UNKNOWN);
        }
        args->ftmBurstExp = (uint8_t)f;
        break;
    }
    case 'e':
    {
        int f = std::atoi(arg);
        if (f <= 0)
        {
            argp_failure(state, 1, 0, "FTM per burst is not correct");
            exit(ARGP_ERR_UNKNOWN);
        }
        args->ftmPerBurst = (uint8_t)f;
        break;
    }
    case 'h':
    {
        int f = std::atoi(arg);
        if (f <= 0)
        {
            argp_failure(state, 1, 0, "FTM burst period is not correct");
            exit(ARGP_ERR_UNKNOWN);
        }
        args->ftmBurstPeriod = (uint16_t)f;
        break;
    }
    case 'k':
    {
        int f = std::atoi(arg);
        if (f <= 0)
        {
            argp_failure(state, 1, 0, "FTM burst duration is not correct");
            exit(ARGP_ERR_UNKNOWN);
        }
        args->ftmBurstDuration = (uint8_t)f;
        break;
    }
    case '#':
    {
        int res = sscanf(arg, "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx", &args->mac[0], &args->mac[1], &args->mac[2], &args->mac[3], &args->mac[4], &args->mac[5]);
        if (res != ETH_ALEN)
        {
            argp_failure(state, 1, 0, "Bad mac address");
            exit(ARGP_ERR_UNKNOWN);
        }
        break;
    }
    case 'n':
    {
        int res = sscanf(arg, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                         &args->ftmTargetMac[0], &args->ftmTargetMac[1], &args->ftmTargetMac[2], &args->ftmTargetMac[3], &args->ftmTargetMac[4], &args->ftmTargetMac[5]);
        if (res != ETH_ALEN)
        {
            argp_failure(state, 1, 0, "FTM target mac address is not correct");
            exit(ARGP_ERR_UNKNOWN);
        }
        break;
    }
    case ARGP_KEY_ARG:
    case ARGP_KEY_END:
        if (args->frequency == 0 ||
            args->bandwidth.empty())
        {
            argp_failure(state, 1, 0, "Fill required arguments -f -b . See --help for more information");
            exit(ARGP_ERR_UNKNOWN);
        }
        return 0;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}