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

#ifndef GNU_PLOT_H
#define GNU_PLOT_H

#include <gtkmm.h>
#include <gtkmm/socket.h>
#include <Csi.h>
#include <string>
#include <future>

class GnuPlot
{
public:
    void init();
    void setWindow(uint64_t id);
    void setBlank();
    void reload();
    void updateChartAsync(Csi *csi);

private:
    inline static std::string lastCmd;
    inline static FILE *gnuPlotPipe;
    inline static std::future<void> runningAsyncEvent;
    
    void updateChart(Csi *csi);
};

#endif