/*
 * FeitCSI is the tool for extracting CSI information from supported intel NICs.
 * Copyright (C) 2023 Miroslav Hutar.
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

#include <iostream>
#include <thread>
#include <argp.h>
#include <chrono>
#include "Csi.h"
#include "WiFIController.h"
#include "WiFiCsiController.h"
#include "Netlink.h"
#include "main.h"
#include "PacketInjector.h"
#include "MainController.h"
#include "Logger.h"

struct Arguments arguments;

const char *argp_program_version =
    "argp-ex3 1.0";
const char *argp_program_bug_address =
    "<bug-gnu-utils@gnu.org>";

/* Program documentation. */
static char doc[] =
    "Argp example #3 -- a program with options and arguments using argp";

/* A description of the arguments we accept. */
static char args_doc[] = "ARG1 ARG2";

/* The options we understand. */
static struct argp_option options[] = {
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
    {"mode", 'i', "MODE", 0, "Mode of program[measure|inject|measureinject]"},
    {"inject-delay", 'd', "INJECTDELAY", 0, "Delay between frame injections is us"},
    {"inject-repeat", 'j', "INJECTREPEAT", 0, "How many times inject frame"},
    {"verbose", 'v', 0, OPTION_ARG_OPTIONAL, "Produce verbose output"},
    {"plot", 'p', 0, OPTION_ARG_OPTIONAL, "Plot CSI data"},
    {"gui", 'x', 0, OPTION_ARG_OPTIONAL, "Run application in GUI"},
    {0}};

/* Used by main to communicate with parse_opt. */

/* Parse a single option. */
static error_t
parse_opt(int key, char *arg, struct argp_state *state)
{
    /* Get the input argument from argp_parse, which we
       know is a pointer to our arguments structure. */
    struct Arguments *args = (struct Arguments *)state->input;

    switch (key)
    {
    case 'v':
        args->verbose = true;
        break;
    case 'x':
        args->gui = true;
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
        else
        {
            argp_failure(state, 1, 0, "Bad mode. Possible values [measure|inject|measureinject]");
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
        } else
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
        } else
        {
            argp_failure(state, 1, 0, "Bad LTF. Possible values [2xLTF+0.8|2xLTF+1.6|4xLTF+3.2|4xLTF+0.8]");
            exit(ARGP_ERR_UNKNOWN);
        }
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

/* Our argp parser. */
static struct argp argp = {options, parse_opt, args_doc, doc};

int main(int argc, char *argv[])
{
    struct Arguments defValues = {
        .verbose = false,
        .frequency = 2412,
        .gui = false,
        .plot = false,
        .bandwidth = "20",
        .mcs = 0,
        .channelWidth = 20,
        .spatialStreams = 1,
        .txPower = 10,
        .guardInterval = 400,
        .injectDelay = 100000,
        .injectRepeat = 0,
        .coding = "LDPC",
        .format = "HT",
        .inject = false,
        .measure = true,
        .mode = "measure",
        .ltf = "1xLTF+0.8",
    };

    arguments = defValues;

    if (arguments.outputFile.empty())
    {
        const auto t = std::chrono::system_clock::now();
        int64_t tInt = std::chrono::duration_cast<std::chrono::seconds>(t.time_since_epoch()).count();
        arguments.outputFile = "FeitCSI_" + std::to_string(tInt) + ".dat";
    }

    /* Default values. */

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    // all arguments ok and sanitized go next

    MainController *mainController = MainController::getInstance();
    if (!arguments.gui)
    {
        mainController->runNoGui();
    } else
    {
        arguments = defValues;
        mainController->runGui();
    }

    if (arguments.verbose)
    {
        Logger::log(info) << "Exiting...\n";
    }
    return 0;
}

// lib to install compile libgtkmm-3.0-dev libnl-genl-3-dev libiw-dev libpcap-dev
// fix default output file when GUI running
