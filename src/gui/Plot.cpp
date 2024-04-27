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

#include "Plot.h"
#include "Arguments.h"
#include "Logger.h"
#include "WiFiCsiController.h"

#include <chrono>
#include <thread>
#include <iostream>

void Plot::init()
{
    signal_draw().connect(sigc::mem_fun(*this, &Plot::on_draw));
}

void Plot::init(Glib::RefPtr<Gtk::Box> box)
{
    this->yTicks = this->yTicksMin >= 0 ? this->yTicksMax : this->yTicksMax + (this->yTicksMin * -1);
    signal_draw().connect(sigc::mem_fun(*this, &Plot::on_draw));
    box->pack_start(*this);
    show();
}

void Plot::updateData(Csi *csi, std::vector<double> *data)
{
    this->csi = csi;
    this->data = data;
    queue_draw();
}

bool Plot::on_draw(const Cairo::RefPtr<Cairo::Context> &cr)
{
    Gtk::Allocation allocation = get_allocation();
    const int width = allocation.get_width();
    const int height = allocation.get_height();
    const double offset = 50;
    const double xTicks = csi ? csi->numSubCarriers : 56;
    bool redraw = false;

    // Set background color
    cr->set_source_rgb(1.0, 1.0, 1.0); // White
    cr->paint();
    
    // Draw x and y axis
    cr->set_source_rgb(0.0, 0.0, 0.0); // Black
    cr->set_line_width(1.0);
    cr->move_to(offset, offset);
    cr->line_to(offset, height - offset);
    cr->line_to(width - offset, height - offset);
    cr->stroke();

    // Calculate the scaling factor for data points to fit in the drawing area
    double xScale = (width - 2 * offset) / (xTicks - 1);
    double yScale = (height - 2 * offset) / yTicks;

    // Draw tick text
    const int perXTick = xTicks / 8;
    for (int i = 0; i < xTicks; i = i + perXTick)
    {
        double x = i * xScale + offset;
        cr->set_source_rgb(0, 0, 0);
        cr->move_to(x, height - 40);
        cr->show_text(std::to_string((i + 1)));
        // Draw grid lines
        cr->set_source_rgb(0.8, 0.7, 0.7); // Light gray
        cr->move_to(x, offset);
        cr->line_to(x, height - offset);
        cr->stroke();
    }
    const int perYTick = yTicks > 8 ? yTicks / 8 : 1;
    for (int i = 0; i <= yTicks; i = i + perYTick)
    {
        double y = height - offset - i * (height - 100) / yTicks;
        cr->set_source_rgb(0, 0, 0);
        cr->move_to(30, y);
        cr->show_text(std::to_string((int)(this->yTicksMin < 0 ? i + this->yTicksMin : i)));
        // Draw grid lines
        cr->set_source_rgb(0.8, 0.8, 0.8); // Light gray
        cr->move_to(offset, y);
        cr->line_to(width - offset, y);
        cr->stroke();
    }

    if (csi)
    {
        // Draw the line chart
        int colorNumber = 0;
        uint32_t index = 0;
        for (uint32_t rx = 0; rx < csi->numRx; rx++)
        {
            for (uint32_t tx = 0; tx < csi->numTx; tx++)
            {
                cr->set_source_rgb(colors[colorNumber][0], colors[colorNumber][1], colors[colorNumber][2]); // Black color
                cr->set_line_width(2.0);
                cr->move_to(offset, (height - offset) - (this->yTicksMin < 0 ? ((this->yTicksMin * -1) + (*this->data)[0]) : (*this->data)[0]) * yScale);
                for (uint32_t n = 0; n < csi->numSubCarriers; n++)
                {
                    double x = n * xScale + offset;
                    double y = (height - offset) - (this->yTicksMin < 0 ? ((this->yTicksMin * -1) + (*this->data)[index]) : (*this->data)[index]) * yScale;
                    if ((*this->data)[index] > this->yTicks)
                    {
                        this->yTicks = (*this->data)[index];
                        redraw = true;
                    }
                    
                    cr->line_to(x, y);
                    index++;
                }
                cr->stroke();
                colorNumber++;
            }
        }

        if (redraw)
        {
            return this->on_draw(cr);
        }
        

        // Draw legend
        colorNumber = 0;
        cr->move_to(offset * 2, offset - 10);
        for (uint32_t rx = 0; rx < csi->numRx; rx++)
        {
            for (uint32_t tx = 0; tx < csi->numTx; tx++)
            {
                cr->set_source_rgb(colors[colorNumber][0], colors[colorNumber][1], colors[colorNumber][2]); // Black color
                cr->show_text(std::string("RX" + std::to_string(rx + 1)) + std::string("TX" + std::to_string(tx + 1)));
                cr->show_text("  ");
                colorNumber++;
            }
        }
    }

    // Draw x label
    cr->set_source_rgb(0.0, 0.0, 0.0); // Black color
    cr->move_to(width / 2, (offset / 2));
    cr->show_text(this->title);

    // Draw x label
    cr->set_source_rgb(0.0, 0.0, 0.0); // Black color
    cr->move_to(width / 2, height - (offset / 2));
    cr->show_text(this->xLabel);

    // Draw y label
    cr->move_to((offset / 2), height / 2);
    cr->rotate(-M_PI / 2); // Rotate text for y label
    cr->show_text(this->yLabel);

    // Draw x label

    cr->set_source_rgb(0.0, 0.0, 0.0); // Black color
    cr->set_font_size(30);
    cr->move_to(width / 2, (offset / 2));
    cr->show_text(this->title);

    return true;
}
