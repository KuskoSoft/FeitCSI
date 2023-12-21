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

#ifndef MAIN_CONTROLLER_H
#define MAIN_CONTROLLER_H

#include "WiFiCsiController.h"
#include "WiFIController.h"
#include "PacketInjector.h"
#include "gui/MainWindow.h"
#include <thread>

class MainController
{

public:
    static MainController *getInstance();

    
    static void deleteInstance();

    void runNoGui();

    void measureCsi(bool stop = false);

    void injectPackets(bool stop = false);

    void runGui();

    void initInterface();
    ~MainController();
    
private:
    MainController();

    inline static MainController *INSTANCE = nullptr;

    inline static MainWindow *mainWindow = nullptr;

    pthread_t measureCsiThread = 0;

    pthread_t injectPacketThread = 0;

    PacketInjector packetInjector;

    WiFIController wifiController;

    std::vector<InterfaceInfo> bkpInterfaces;


    static void *measureCsi(void *arg);

    static void intHandler(int dummy);

    static void *injectPackets(void *arg);
    
    void restoreState();
};

#endif