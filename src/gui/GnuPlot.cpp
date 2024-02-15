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

#include "gui/GnuPlot.h"
#include "Arguments.h"
#include "GnuPlot.h"

void GnuPlot::init()
{
    if (!Arguments::arguments.plot || gnuPlotPipe)
    {
        return;
    }

    this->gnuPlotPipe = popen("gnuplot > NUL 2>&1", "w");
    fprintf(this->gnuPlotPipe, "set term x11 noraise\n");
    fflush(this->gnuPlotPipe);
}

void GnuPlot::setWindow(uint64_t id)
{
    this->gnuPlotPipe = popen("gnuplot > NUL 2>&1", "w");
    fprintf(this->gnuPlotPipe, "set term x11 noraise\n");
    fprintf(this->gnuPlotPipe, "set terminal x11 window \"%#lx\"\n", id);
    fflush(this->gnuPlotPipe);
}

void GnuPlot::setBlank()
{
    const char *cmd = R"(
        set multiplot layout 2,1
        set ylabel "Magnitude" 
        set xlabel "Subcarrier Index"       
        set title "Magnitude (Abs) " 
        set autoscale x
        set xrange [0:1]
        set yrange [0:1]
        plot 1/0
        set ylabel "Phase (rad) "
        set xlabel "Subcarrier Index"         
        set title "Phase "  
        set autoscale
        set xrange [0:1]
        set yrange [0:1]
        plot 1/0
    )";
    this->lastCmd = cmd;
    fprintf(this->gnuPlotPipe, cmd);
    fflush(this->gnuPlotPipe);
}

void GnuPlot::updateChart(Csi &csi)
{
    if (!Arguments::arguments.plot)
    {
        return;
    }

    std::stringstream ss;
    // ss << "set multiplot layout 2,1";
    ss << R"(
        set multiplot layout 2,1
        set ylabel "Magnitude" 
        set xlabel "Subcarrier Index"       
        set title "Magnitude (Abs) " 
        set autoscale x
    )";

    std::stringstream plotCmd;
    plotCmd << "set xrange [1:" << csi.numSubCarriers << "]\n";
    plotCmd << "plot";
    for (uint32_t tx = 0; tx < csi.numTx; tx++)
    {
        for (uint32_t rx = 0; rx < csi.numRx; rx++)
        {
            plotCmd << " '-' with lines title \"RX" << (rx + 1) << "TX" << (tx + 1) << "\", ";
        }
    }
    plotCmd << "\n";
    ss << plotCmd.str();

    std::stringstream dataMagnitude;
    std::stringstream dataPhase;

    uint32_t index = 0;
    for (uint32_t rx = 0; rx < csi.numRx; rx++)
    {
        for (uint32_t tx = 0; tx < csi.numTx; tx++)
        {
            for (uint32_t n = 0; n < csi.numSubCarriers; n++)
            {
                dataMagnitude << (n + 1) << " " << csi.magnitude[index] << "\n";
                dataPhase << (n + 1) << " " << csi.phase[index] << "\n";
                index++;
            }
            dataMagnitude << "e\n";
            dataPhase << "e\n";
        }
    }

    ss << dataMagnitude.str();

    ss << R"(
        set ylabel "Phase (rad) "
        set xlabel "Subcarrier Index"         
        set title "Phase "  
        set autoscale
    )";

    ss << plotCmd.str();
    ss << dataPhase.str();
    ss << "unset multiplot\n";

    this->lastCmd = ss.str();
    fprintf(this->gnuPlotPipe, ss.str().c_str());
    fflush(this->gnuPlotPipe);
}

void GnuPlot::reload()
{
    fprintf(this->gnuPlotPipe, this->lastCmd.c_str());
    fflush(this->gnuPlotPipe);
}