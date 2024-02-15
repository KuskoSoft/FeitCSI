/*
 * FeitCSI is the tool for extracting CSI information from supported intel NICs.
 * Copyright (C) 2023-2024 Miroslav Hutar.
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
#include <chrono>
#include "Csi.h"
#include "WiFIController.h"
#include "WiFiCsiController.h"
#include "Netlink.h"
#include "main.h"
#include "PacketInjector.h"
#include "MainController.h"
#include "Logger.h"
#include "Arguments.h"

int main(int argc, char *argv[])
{
    Arguments args;
    args.init();
    args.parse(argc, argv);

    if (Arguments::arguments.outputFile.empty())
    {
        const auto t = std::chrono::system_clock::now();
        int64_t tInt = std::chrono::duration_cast<std::chrono::seconds>(t.time_since_epoch()).count();
        Arguments::arguments.outputFile = "FeitCSI_" + std::to_string(tInt) + ".dat";
    }

    // all arguments ok and sanitized go next

    MainController *mainController = MainController::getInstance();
    if (Arguments::arguments.gui)
    {
        mainController->runGui();
    }
    else if (Arguments::arguments.udpSocket)
    {
        mainController->runUdpSocket();
    }
    else
    {
        mainController->runNoGui();
    }

    if (Arguments::arguments.verbose)
    {
        Logger::log(info) << "Exiting...\n";
    }
    return 0;
}

// lib to install compile libgtkmm-3.0-dev libnl-genl-3-dev libiw-dev libpcap-dev
// fix default output file when GUI running
