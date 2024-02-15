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

#include "MainController.h"
#include "Arguments.h"
#include "Logger.h"
#include "gui/MainWindow.h"
#include "layout.h"
#include <iostream>
#include <chrono>
#include <thread>

MainController::MainController()
{
    signal(SIGINT, this->intHandler);
}

MainController *MainController::getInstance()
{
    if (INSTANCE == nullptr)
    {
        INSTANCE = new MainController();
    }
    return INSTANCE;
}

void MainController::deleteInstance()
{
    delete MainController::INSTANCE;
}

void MainController::runNoGui(bool detach)
{
    this->initInterface();
    if (Arguments::arguments.measure)
    {
        pthread_create(&this->measureCsiThread, NULL, &MainController::measureCsi, NULL);
    }
    if (Arguments::arguments.inject)
    {
        pthread_create(&this->injectPacketThread, NULL, &MainController::injectPackets, NULL);
    }
    if (Arguments::arguments.measure)
    {
        if (detach) {
            pthread_detach(this->measureCsiThread);
        } else {
            pthread_join(this->measureCsiThread, NULL);
        }
    }
    if (Arguments::arguments.inject)
    {
        if (detach) {
            pthread_detach(this->injectPacketThread);
        } else {
            pthread_join(this->injectPacketThread, NULL);
        }
    }

    if (!detach) {
        this->deleteInstance();
    }
}

void MainController::measureCsi(bool stop)
{
    this->wifiController.setFreq(Arguments::arguments.frequency, Arguments::arguments.bandwidth.c_str());
    if (stop)
    {
        pthread_cancel(this->measureCsiThread);
    }
    else
    {
        pthread_create(&this->measureCsiThread, NULL, &MainController::measureCsi, NULL);
        pthread_detach(this->measureCsiThread);
    }
}

void MainController::injectPackets(bool stop)
{
    this->wifiController.setTxPower();
    this->wifiController.setFreq(Arguments::arguments.frequency, Arguments::arguments.bandwidth.c_str());
    if (stop)
    {
        pthread_cancel(this->injectPacketThread);
    }
    else
    {
        pthread_create(&this->injectPacketThread, NULL, &MainController::injectPackets, NULL);
        pthread_detach(this->injectPacketThread);
    }
}

void MainController::runGui()
{
    Arguments::arguments.plot = true;
    Arguments::arguments.verbose = true;
    Arguments::arguments.measure = false;
    Arguments::arguments.inject = false;
    gtk_init(NULL, NULL);
    Glib::init();
    auto app = Gtk::Application::create("com.kuskosoft.feitcsi");
    std::string layout((char*)MainWindow_glade);
    auto builder = Gtk::Builder::create_from_string(layout);
    builder->get_widget_derived("MainWindow", this->mainWindow);
    app->run(*this->mainWindow);
}

void MainController::runUdpSocket()
{
    this->udpSocket = new UdpSocket();
    udpSocket->init();
}

void MainController::initInterface()
{
    try
    {
        this->wifiController.init();
        this->wifiController.getInterfaces();
        this->wifiController.getPhys();

        // delete actual interfaces
        for (InterfaceInfo interface : this->wifiController.interfaces)
        {
            if (Arguments::arguments.verbose)
            {
                Logger::log(info) << "Remove interface " << interface.ifName << "\n";
            }
            this->bkpInterfaces.push_back(interface);
            this->wifiController.removeInterface(NULL, interface.ifIndex);
        }
        this->wifiController.addMonitorDevice(MONITOR_INTERFACE_NAME, NL80211_IFTYPE_MONITOR);
        this->wifiController.setInterfaceUpDown(MONITOR_INTERFACE_NAME, true);
        this->wifiController.setFreq(Arguments::arguments.frequency, Arguments::arguments.bandwidth.c_str());
        if (!MainController::mainWindow)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); //wait on init then set power and go next
            this->wifiController.setTxPower();
        }
        
    }
    catch(const std::exception& e)
    {
        if (MainController::mainWindow)
        {
            MainController::mainWindow->fatalError(e.what());
        }
        else
        {
            Logger::log(error) << e.what() << '\n';
            delete MainController::INSTANCE;
            exit(1);
        }
    }
}

void *MainController::measureCsi(void *arg)
{
    try
    {
        WiFiCsiController wcs;
        wcs.init();
        wcs.listenToCsi();
    }
    catch (const std::exception &e)
    {
        if (MainController::mainWindow)
        {
            MainController::mainWindow->fatalError(e.what());
        }
        else
        {
            Logger::log(error) << e.what() << '\n';
        }
    }

    return 0;
}

void *MainController::injectPackets(void *arg)
{
    try
    {
        PacketInjector pi;
        if (Arguments::arguments.injectRepeat)
        {
            for (uint32_t i = 0; i < Arguments::arguments.injectRepeat; i++)
            {
                pi.inject();
                std::this_thread::sleep_for(std::chrono::microseconds(Arguments::arguments.injectDelay));
            }
        }
        else
        {
            while (true)
            {
                pi.inject();
                std::this_thread::sleep_for(std::chrono::microseconds(Arguments::arguments.injectDelay));
            }
        }
    }
    catch(const std::exception& e)
    {
        if (MainController::mainWindow)
        {
            MainController::mainWindow->fatalError(e.what());
        }
        else
        {
            Logger::log(error) << e.what() << '\n';
        }
    }
    
    return 0;
}

void MainController::restoreState()
{
    MainController *mainController = MainController::getInstance();

    if (mainController->measureCsiThread)
    {
        pthread_cancel(mainController->measureCsiThread);
    }
    if (mainController->injectPacketThread)
    {
        pthread_cancel(mainController->injectPacketThread);
    }

    mainController->wifiController.removeInterface(MONITOR_INTERFACE_NAME);
    for (InterfaceInfo interface : mainController->bkpInterfaces)
    {
        if (Arguments::arguments.verbose)
        {
            Logger::log(info) << "Recovering interface " << interface.ifName << "\n";
        }
        mainController->wifiController.addMonitorDevice(interface.ifName.c_str(), (nl80211_iftype)interface.ifType);
    }
    mainController->bkpInterfaces.clear();
    mainController->wifiController.interfaces.clear();

    if (Arguments::arguments.verbose)
    {
        Logger::log(info) << "Exiting recovery state...\n";
    }
}

MainController::~MainController()
{
    this->restoreState();
    if (udpSocket) {
        delete udpSocket;
    }
}

void MainController::intHandler(int dummy)
{
    delete MainController::INSTANCE;
    exit(0);
}