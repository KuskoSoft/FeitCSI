/*
 * FeitCSI is the tool for extracting CSI information from supported intel NICs.
 * Copyright (C) 2024 Miroslav Hutar.
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

#ifndef PLOT_H
#define PLOT_H

#include "Csi.h"
#include <future>
#include <sigc++/sigc++.h>
#include <string>
#include <gtkmm.h>
#include <mutex>

class Plot : public Gtk::DrawingArea
{
public:
    void init();
    void init(Glib::RefPtr<Gtk::Box> box);
    void updateData(Csi *csi, std::vector<double> *data);
    double yTicksMax = 200;
    double yTicksMin = 0;
    std::string yLabel = "";
    std::string xLabel = "Subcarrier index";
    std::string title = "";
    
private:
    const double colors[4][3] = {
        { 1.0, 0.0, 0.0 },
        { 0.0, 0.0, 1.0 },
        { 1.0, 0.7, 0.0 },
        { 0.0, 1.0, 0.0 },
    };
    Csi *csi;
    std::vector<double> *data;
    std::mutex updateDataMutex;
    double yTicks;
    
    bool on_draw(const Cairo::RefPtr<Cairo::Context> &cr);
};

#endif