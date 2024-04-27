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

#include "gui/CsiProcessingWindow.h"
#include "main.h"
#include "Logger.h"
#include "Arguments.h"
#include "CsiProcessingWindow.h"

void CsiProcessingWindow::init(Glib::RefPtr<Gtk::Builder> &builder)
{
    this->inputFile = Glib::RefPtr<Gtk::FileChooserButton>::cast_dynamic(builder->get_object("inputFile"));
    this->inputFile->signal_file_set().connect(sigc::mem_fun(*this, &CsiProcessingWindow::inputFileChange));

    this->previousCsiButton = Glib::RefPtr<Gtk::Button>::cast_dynamic(builder->get_object("previousCsiButton"));
    this->previousCsiButton->signal_clicked().connect(sigc::mem_fun(*this, &CsiProcessingWindow::previousCsiButtonClicked));

    this->nextCsiButton = Glib::RefPtr<Gtk::Button>::cast_dynamic(builder->get_object("nextCsiButton"));
    this->nextCsiButton->signal_clicked().connect(sigc::mem_fun(*this, &CsiProcessingWindow::nextCsiButtonClicked));

    this->interpolationLinearRadioButton = Glib::RefPtr<Gtk::RadioButton>::cast_dynamic(builder->get_object("interpolationLinearRadioButton"));
    this->interpolationLinearRadioButton->signal_clicked().connect(sigc::mem_fun(*this, &CsiProcessingWindow::interpolationLinearRadioButtonClicked));

    this->interpolationCubicRadioButton = Glib::RefPtr<Gtk::RadioButton>::cast_dynamic(builder->get_object("interpolationCubicRadioButton"));
    this->interpolationCubicRadioButton->signal_clicked().connect(sigc::mem_fun(*this, &CsiProcessingWindow::interpolationCubicRadioButtonClicked));

    this->interpolationCosineButton = Glib::RefPtr<Gtk::RadioButton>::cast_dynamic(builder->get_object("interpolationCosineButton"));
    this->interpolationCosineButton->signal_clicked().connect(sigc::mem_fun(*this, &CsiProcessingWindow::interpolationCosineButtonClicked));

    this->phaseLinearTransformCheckButton = Glib::RefPtr<Gtk::CheckButton>::cast_dynamic(builder->get_object("phaseLinearTransformCheckButton"));
    this->phaseLinearTransformCheckButton->signal_clicked().connect(sigc::mem_fun(*this, &CsiProcessingWindow::phaseLinearTransformCheckButtonClicked));

    this->processingSaveGtkButton = Glib::RefPtr<Gtk::Button>::cast_dynamic(builder->get_object("processingSaveGtkButton"));
    this->processingSaveGtkButton->signal_clicked().connect(sigc::mem_fun(*this, &CsiProcessingWindow::processingSaveGtkButtonClicked));

    this->currentDataCount = Glib::RefPtr<Gtk::Label>::cast_dynamic(builder->get_object("currentDataCount"));
}

void CsiProcessingWindow::inputFileChange()
{
    Arguments::arguments.inputFile = this->inputFile->get_filename();
    csiProcessor.loadCsi();
    this->refresh();
}

void CsiProcessingWindow::refresh()
{
    std::string indexLabelText = std::to_string(this->currentIndex) + " " + std::to_string(this->csiProcessor.csiData.size());
    
    this->currentDataCount->set_text(indexLabelText);
    
/*     if (!this->csiProcessor.csiData.empty())
    {
        gnuPlot.updateChartAsync(this->csiProcessor.csiData[this->currentIndex]);
    } */
    
}

void CsiProcessingWindow::previousCsiButtonClicked()
{
    if (this->currentIndex > 0)
    {
        this->currentIndex--;
        this->csiProcessor.process(*this->csiProcessor.csiData[this->currentIndex]);
        this->refresh();
    }
}

void CsiProcessingWindow::nextCsiButtonClicked()
{
    if (this->currentIndex < (this->csiProcessor.csiData.size() - 1))
    {
        this->currentIndex++;
        this->csiProcessor.process(*this->csiProcessor.csiData[this->currentIndex]);
        this->refresh();
    }
}

void CsiProcessingWindow::interpolationLinearRadioButtonClicked()
{
    if (!this->csiProcessor.csiData.empty())
    {
        Arguments::arguments.processors[processor::interpolateLinear] = this->interpolationLinearRadioButton->get_active();
        this->csiProcessor.process(*this->csiProcessor.csiData[this->currentIndex]);
        this->refresh();
    }
}

void CsiProcessingWindow::interpolationCubicRadioButtonClicked()
{
    if (!this->csiProcessor.csiData.empty())
    {
        Arguments::arguments.processors[processor::interpolateCubic] = this->interpolationCubicRadioButton->get_active();
        this->csiProcessor.process(*this->csiProcessor.csiData[this->currentIndex]);
        this->refresh();
    }
}

void CsiProcessingWindow::interpolationCosineButtonClicked()
{
    if (!this->csiProcessor.csiData.empty())
    {
        Arguments::arguments.processors[processor::interpolateCosine] = this->interpolationCosineButton->get_active();
        this->csiProcessor.process(*this->csiProcessor.csiData[this->currentIndex]);
        this->refresh();
    }
}

void CsiProcessingWindow::phaseLinearTransformCheckButtonClicked()
{
    if (!this->csiProcessor.csiData.empty())
    {
        Arguments::arguments.processors[processor::phaseCalibrationLinearTransform] = this->phaseLinearTransformCheckButton->get_active();
        this->csiProcessor.process(*this->csiProcessor.csiData[this->currentIndex]);
        this->refresh();
    }
}

void CsiProcessingWindow::processingSaveGtkButtonClicked()
{
    if (!this->csiProcessor.csiData.empty())
    {
        Arguments::arguments.outputFile = "processedCsi.bin";
        this->csiProcessor.saveCsi();
    }
}
