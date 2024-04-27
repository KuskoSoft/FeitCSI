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

#include "gui/MainWindow.h"
#include <iostream>
#include "MainWindow.h"
#include "MainController.h"
#include "WiFIController.h"
#include "Logger.h"
#include "Arguments.h"

MainWindow::MainWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> const &builder) : Gtk::ApplicationWindow(obj), builder{builder}
{
    set_title("FeitCSI");
    set_default_size(1000, 800);

    this->signal_delete_event().connect(sigc::mem_fun(*this, &MainWindow::onWindowDelete));

    if (!Arguments::arguments.gui && Arguments::arguments.plot)
    {
        Glib::RefPtr<Gtk::ScrolledWindow>::cast_dynamic(this->builder->get_object("consoleScrollbar"))->hide();
        Glib::RefPtr<Gtk::Notebook>::cast_dynamic(this->builder->get_object("leftBar"))->hide();
        return;
    }
    

    this->measureButton = Glib::RefPtr<Gtk::Button>::cast_dynamic(this->builder->get_object("measureButton"));
    this->measureButton->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::measureButtonClicked));

    this->injectButton = Glib::RefPtr<Gtk::Button>::cast_dynamic(this->builder->get_object("injectButton"));
    this->injectButton->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::injectButtonClicked));

    this->frequency = Glib::RefPtr<Gtk::SpinButton>::cast_dynamic(this->builder->get_object("frequency"));
    this->frequency->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::frequencyChange));

    this->channelWidth = Glib::RefPtr<Gtk::ComboBoxText>::cast_dynamic(this->builder->get_object("channelWidth"));
    this->channelWidth->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::channelWidthChange));

    this->outputFile = Glib::RefPtr<Gtk::FileChooserButton>::cast_dynamic(this->builder->get_object("outputFile"));
    this->outputFile->signal_file_set().connect(sigc::mem_fun(*this, &MainWindow::outputFileChange));

    Glib::RefPtr<Gtk::FileChooserDialog> fDialog = Glib::RefPtr<Gtk::FileChooserDialog>::cast_dynamic(this->builder->get_object("filechooserdialog1"));

    fDialog->set_action(Gtk::FILE_CHOOSER_ACTION_SAVE);
    fDialog->add_button("_Cancel", GTK_RESPONSE_CANCEL);
    fDialog->add_button("_Accept", GTK_RESPONSE_ACCEPT);
    this->filePath = Glib::RefPtr<Gtk::Label>::cast_dynamic(this->builder->get_object("filePath"));

    this->format = Glib::RefPtr<Gtk::ComboBoxText>::cast_dynamic(this->builder->get_object("format"));
    this->format->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::formatChange));

    this->mcs = Glib::RefPtr<Gtk::ComboBoxText>::cast_dynamic(this->builder->get_object("mcs"));
    this->mcs->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::mcsChange));

    this->spatialStreams = Glib::RefPtr<Gtk::ComboBoxText>::cast_dynamic(this->builder->get_object("spatialStreams"));
    this->spatialStreams->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::spatialStreamsChange));

    this->guardInterval = Glib::RefPtr<Gtk::ComboBoxText>::cast_dynamic(this->builder->get_object("guardInterval"));
    this->guardInterval->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::guardIntervalChange));

    this->ltf = Glib::RefPtr<Gtk::ComboBoxText>::cast_dynamic(this->builder->get_object("ltf"));
    this->ltf->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::ltfChange));

    this->txPower = Glib::RefPtr<Gtk::ComboBoxText>::cast_dynamic(this->builder->get_object("txPower"));
    this->txPower->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::txPowerChange));

    this->coding = Glib::RefPtr<Gtk::ComboBoxText>::cast_dynamic(this->builder->get_object("coding"));
    this->coding->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::codingChange));

    this->injectDelay = Glib::RefPtr<Gtk::SpinButton>::cast_dynamic(this->builder->get_object("injectDelay"));
    this->injectDelay->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::injectDelayChange));

    this->injectRepeat = Glib::RefPtr<Gtk::SpinButton>::cast_dynamic(this->builder->get_object("injectRepeat"));
    this->injectRepeat->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::injectRepeatChange));

    this->errorMessages = Glib::RefPtr<Gtk::Label>::cast_dynamic(this->builder->get_object("errorMessages"));
    
    this->console = Glib::RefPtr<Gtk::TextView>::cast_dynamic(this->builder->get_object("console"));
    Glib::RefPtr<Gtk::ScrolledWindow> consoleScrollbar = Glib::RefPtr<Gtk::ScrolledWindow>::cast_dynamic(this->builder->get_object("consoleScrollbar"));
    this->consoleScrollbarAdjustment = consoleScrollbar->get_vadjustment();

    this->errorDialog = Glib::RefPtr<Gtk::MessageDialog>::cast_dynamic(this->builder->get_object("errorDialog"));
    Glib::RefPtr<Gtk::Button> errorDialogOkBtn = Glib::RefPtr<Gtk::Button>::cast_dynamic(this->builder->get_object("errorDialogOkBtn"));
    errorDialogOkBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::closeButton));

    Logger::guiOutput = this->console->get_buffer();

    this->signal_check_resize().connect_notify(sigc::mem_fun(*this, &MainWindow::onResize));

    this->signal_show().connect(sigc::mem_fun(*this, &MainWindow::onWindowInit));

    this->csiProcessingWindow.init(this->builder);
}

void MainWindow::fatalError(std::string msg)
{
    errorDialog->set_message(msg + "\n Program will be closed now");
    errorDialog->show();
}

void MainWindow::closeButton()
{
    this->errorDialog->close();
    this->close();
}

bool MainWindow::onWindowDelete(GdkEventAny *event)
{
    MainController *mainController = MainController::getInstance();
    mainController->deleteInstance();
    return true;
}


void MainWindow::onWindowInit()
{
    Glib::RefPtr<Gtk::Paned> socketConsolePaned = Glib::RefPtr<Gtk::Paned>::cast_dynamic(this->builder->get_object("socketConsolePaned"));
    socketConsolePaned->set_position(socketConsolePaned->get_allocated_height() * 0.8);
}

void MainWindow::onResize()
{
    if (this->measuring)
    {
        this->consoleScrollbarAdjustment->set_value(this->consoleScrollbarAdjustment->get_upper());
        return;
    }
}

void MainWindow::frequencyChange()
{
    int v = std::atoi(this->frequency->get_text().raw().c_str());
    Arguments::arguments.frequency = (uint16_t)v;
}

void MainWindow::channelWidthChange()
{
    Arguments::arguments.bandwidth = this->channelWidth->get_active_text().raw();
    struct ChanMode chMode = WiFIController::getChanMode(Arguments::arguments.bandwidth.c_str());
    Arguments::arguments.channelWidth = WiFIController::chanModeToWidth(chMode);
}

void MainWindow::outputFileChange()
{
    Arguments::arguments.outputFile = this->outputFile->get_filename();
    this->filePath->set_text(Arguments::arguments.outputFile);
}

void MainWindow::formatChange()
{
    Arguments::arguments.format = this->format->get_active_text().raw();
}

void MainWindow::mcsChange()
{
    int v = std::atoi(this->mcs->get_active_text().raw().c_str());
    Arguments::arguments.mcs = (uint8_t)v;
}

void MainWindow::spatialStreamsChange()
{
    int v = std::atoi(this->spatialStreams->get_active_text().raw().c_str());
    Arguments::arguments.spatialStreams = (uint8_t)v;
}

void MainWindow::ltfChange()
{
    Arguments::arguments.ltf = this->ltf->get_active_text().raw();
}

void MainWindow::guardIntervalChange()
{
    int v = std::atoi(this->guardInterval->get_active_text().raw().c_str());
    Arguments::arguments.guardInterval = (uint16_t)v;
}

void MainWindow::txPowerChange()
{
    int v = std::atoi(this->txPower->get_active_text().raw().c_str());
    Arguments::arguments.txPower = (uint8_t)v;
}

void MainWindow::codingChange()
{
    Arguments::arguments.coding = this->coding->get_active_text().raw();
}

void MainWindow::injectDelayChange()
{
    int v = std::atoi(this->injectDelay->get_text().raw().c_str());
    Arguments::arguments.injectDelay = (uint32_t)v;
}

void MainWindow::injectRepeatChange()
{
    int v = std::atoi(this->injectRepeat->get_text().raw().c_str());
    Arguments::arguments.injectRepeat = (uint32_t)v;
}

void MainWindow::updateErrorMessages()
{
    std::string concatErrors;
    for (const auto &kv : this->errors)
    {
        concatErrors += kv.second + "\n";
    }
    this->errorMessages->set_text(concatErrors);
}

void MainWindow::measureButtonClicked()
{
    MainController *mainController = MainController::getInstance();
    if (!this->measuring)
    {
        mainController->measureCsi();
        this->measureButton->set_label("Stop measure");
        this->measuring = true;
    } else {
        mainController->measureCsi(true);
        this->measureButton->set_label("Measure");
        this->measuring = false;
    }
    this->updateSensitivity();
}

void MainWindow::injectButtonClicked()
{
    MainController *mainController = MainController::getInstance();
    if (!this->injecting)
    {
        mainController->injectPackets();
        this->injectButton->set_label("Stop inject");
        this->injecting = true;
    }
    else
    {
        mainController->injectPackets(true);
        this->injectButton->set_label("Inject");
        this->injecting = false;
    }
    this->updateSensitivity();
}

void MainWindow::updateSensitivity()
{
    this->frequency->set_sensitive(!this->measuring);
    this->channelWidth->set_sensitive(!this->measuring);
    this->outputFile->set_sensitive(!this->measuring);

    this->format->set_sensitive(!this->injecting);
    this->mcs->set_sensitive(!this->injecting);
    this->spatialStreams->set_sensitive(!this->injecting);
    this->guardInterval->set_sensitive(!this->injecting);
    this->txPower->set_sensitive(!this->injecting);
    this->coding->set_sensitive(!this->injecting);
    this->injectDelay->set_sensitive(!this->injecting);
    this->injectRepeat->set_sensitive(!this->injecting);
}