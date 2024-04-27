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

#ifndef CSI_PROCESSING_WINDOW_H
#define CSI_PROCESSING_WINDOW_H

#include <gtkmm.h>
#include "CsiProcessor.h"

class CsiProcessingWindow
{
public:
    void init(Glib::RefPtr<Gtk::Builder> &builder);

private:
    Glib::RefPtr<Gtk::FileChooserButton> inputFile;
    Glib::RefPtr<Gtk::Label> currentDataCount;
    Glib::RefPtr<Gtk::Button> previousCsiButton;
    Glib::RefPtr<Gtk::Button> nextCsiButton;
    Glib::RefPtr<Gtk::RadioButton> interpolationLinearRadioButton;
    Glib::RefPtr<Gtk::RadioButton> interpolationCubicRadioButton;
    Glib::RefPtr<Gtk::RadioButton> interpolationCosineButton;
    Glib::RefPtr<Gtk::CheckButton> phaseLinearTransformCheckButton;
    Glib::RefPtr<Gtk::Button> processingSaveGtkButton;

    void inputFileChange();
    void refresh();
    void previousCsiButtonClicked();
    void nextCsiButtonClicked();

    void interpolationLinearRadioButtonClicked();

    void interpolationCubicRadioButtonClicked();

    void interpolationCosineButtonClicked();

    void phaseLinearTransformCheckButtonClicked();

    void processingSaveGtkButtonClicked();

    CsiProcessor csiProcessor;

    uint32_t currentIndex = 0;
};

#endif