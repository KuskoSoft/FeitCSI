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

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <gtkmm.h>
#include "gui/Plot.h"
#include "gui/CsiProcessingWindow.h"

class MainWindow : public Gtk::ApplicationWindow
{
    Glib::RefPtr<Gtk::Builder> ui;

public:
    MainWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> const &builder);
    void fatalError(std::string msg);

    Glib::RefPtr<Gtk::Builder> builder;

private:
    int chartWidth = 0;
    int chartHeight = 0;
    bool measuring = false;
    bool injecting = false;

    CsiProcessingWindow csiProcessingWindow;

    std::stringstream coutBuffer;

    std::map<std::string, std::string> errors;
    Glib::RefPtr<Gtk::Adjustment> consoleScrollbarAdjustment;
    Glib::RefPtr<Gtk::MessageDialog> errorDialog;

    Glib::RefPtr<Gtk::Button> measureButton;
    Glib::RefPtr<Gtk::Button> injectButton;

    Glib::RefPtr<Gtk::SpinButton> frequency;
    Glib::RefPtr<Gtk::ComboBoxText> channelWidth;
    Glib::RefPtr<Gtk::FileChooserButton> outputFile;
    Glib::RefPtr<Gtk::Label> filePath;
    Glib::RefPtr<Gtk::ComboBoxText> format;
    Glib::RefPtr<Gtk::ComboBoxText> mcs;
    Glib::RefPtr<Gtk::ComboBoxText> spatialStreams;
    Glib::RefPtr<Gtk::ComboBoxText> guardInterval;
    Glib::RefPtr<Gtk::ComboBoxText> ltf;
    Glib::RefPtr<Gtk::ComboBoxText> txPower;
    Glib::RefPtr<Gtk::ComboBoxText> coding;
    Glib::RefPtr<Gtk::SpinButton> injectDelay;
    Glib::RefPtr<Gtk::SpinButton> injectRepeat;

    Glib::RefPtr<Gtk::TextView> console;
    Glib::RefPtr<Gtk::Label> errorMessages;

    Plot plot;
    
    void closeButton();

    bool onWindowDelete(GdkEventAny *event);

    void onWindowInit();

    void onResize();

    void frequencyChange();

    void outputFileChange();

    void channelWidthChange();

    void formatChange();

    void mcsChange();

    void spatialStreamsChange();

    void ltfChange();

    void guardIntervalChange();

    void txPowerChange();

    void codingChange();

    void injectDelayChange();

    void injectRepeatChange();

    void updateErrorMessages();

    void measureButtonClicked();

    void injectButtonClicked();
    
    void updateSensitivity();
};

#endif