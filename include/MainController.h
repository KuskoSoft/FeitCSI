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

#ifndef MAIN_CONTROLLER_H
#define MAIN_CONTROLLER_H

#include "WiFiCsiController.h"
#include "WiFIController.h"
#include "PacketInjector.h"
#include "gui/MainWindow.h"
#include "gui/Plot.h"
#include "Csi.h"
#include "UdpSocket.h"
#include <thread>

class MainController
{

public:
    inline static UdpSocket *udpSocket = nullptr;

    Plot *plotAmplitude;

    Plot *plotPhase;

    static MainController *getInstance();

    static gint updatePlots();

    void initPlots();

    static void deleteInstance();

    void runNoGui(bool detach = false);

    void measureCsi(bool stop = false);

    void injectPackets(bool stop = false);

    void runGui();

    void runUdpSocket();

    void initInterface();
    
    void restoreState();
    
    ~MainController();

private:
    MainController();

    inline static MainController *INSTANCE = nullptr;

    inline static MainWindow *mainWindow = nullptr;

    Glib::RefPtr<Gtk::Application> app;

    inline static Csi *csiToPlot = nullptr;

    guint updatePlotsSourceId = 0;

    pthread_t measureCsiThread = 0;

    pthread_t injectPacketThread = 0;

    pthread_t ftmThread = 0;

    pthread_t ftmResponderThread = 0;

    PacketInjector packetInjector;

    WiFIController wifiController;

    std::vector<InterfaceInfo> bkpInterfaces;

    static void *measureCsi(void *arg);

    static void intHandler(int dummy);

    static void *injectPackets(void *arg);

    static void *ftm(void *arg);

    static void *ftmResponder(void *arg);

    bool measuring = false;

    bool injecting = false;

    bool ftmEnabled = false;

    bool ftmResponderEnabled = false;
};

#endif