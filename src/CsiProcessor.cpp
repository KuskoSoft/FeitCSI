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

#include "CsiProcessor.h"
#include "main.h"
#include "Logger.h"
#include "interpolation.h"
#include "Arguments.h"

#include <fstream>
#include <numeric>
#include <filesystem>
#include <cstring>

bool CsiProcessor::loadCsi()
{
    this->clearState();
    std::ifstream ifs(Arguments::arguments.inputFile, std::ios::binary);

    ifs.seekg (0, ifs.end);
    int length = ifs.tellg();
    ifs.seekg (0, ifs.beg);
    int currentPosition = 0;

    while (currentPosition < length)
    {
        uint32_t csiDataSize;

        ifs.read((char *)&csiDataSize, 4);
        ifs.seekg(currentPosition, ifs.beg);

        uint8_t rawData[CSI_HEADER_LENGTH + csiDataSize];
        ifs.read((char *)rawData, CSI_HEADER_LENGTH + csiDataSize);
        Csi *c = new Csi();
        c->loadFromMemory(rawData);
        this->csiData.push_back(c);
        currentPosition = ifs.tellg();
        //break;
    }

    Logger::log(info) << "Csi loaded \n";
    return true;
}

void CsiProcessor::saveCsi()
{
    std::ofstream outfile;
    outfile.open(Arguments::arguments.outputFile, std::ios_base::app | std::ios::binary);
    if (outfile.fail())
    {
        throw std::ios_base::failure("Open file failed: " + std::string(std::strerror(errno)));
    }
    this->process(*(this->csiData[0]));
    
    for (Csi *c : this->csiData) {
        this->process(*c);
        c->rawHeaderData.csiDataSize = sizeof(std::complex<double>) * c->csi.size();
        outfile.write(reinterpret_cast<char *>(&c->rawHeaderData), sizeof(RawHeaderData));
        outfile.write(reinterpret_cast<char *>(c->csi.data()), c->rawHeaderData.csiDataSize);
    }
    outfile.close();
    std::filesystem::permissions(Arguments::arguments.outputFile, std::filesystem::perms::all & ~(std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec | std::filesystem::perms::others_exec), std::filesystem::perm_options::add);
}

CsiProcessor::~CsiProcessor()
{
    this->clearState();
}

void CsiProcessor::clearState()
{
    if (this->csiData.empty())
    {
        return;
    }
    
    for (Csi *c : this->csiData) {
        delete c;
    }
}

void CsiProcessor::interpolate(Csi &csi, processor type)
{
    const std::vector<uint32_t> pilotIndices = csi.getPilotIndices();
    uint32_t offset = 0;
    for (uint32_t rx = 0; rx < csi.numRx; rx++)
    {
        for (uint32_t tx = 0; tx < csi.numTx; tx++)
        {
            for (uint32_t pilotIndice : pilotIndices) {
                uint32_t index = pilotIndice + offset;
                if (type == processor::interpolateLinear)
                {
                    csi.magnitude[index] = interpolation::linearInterpolate(csi.magnitude[index - 1], csi.magnitude[index + 1], 0.5);
                    csi.phase[index] = interpolation::linearInterpolate(csi.phase[index - 1], csi.phase[index + 1], 0.5);
                }
                else if(type == processor::interpolateCubic)
                {
                    csi.magnitude[index] = interpolation::cubicInterpolate(csi.magnitude[index - 2], csi.magnitude[index - 1], csi.magnitude[index + 1], csi.magnitude[index + 2], 0.5);
                    csi.phase[index] = interpolation::cubicInterpolate(csi.phase[index - 2], csi.phase[index - 1], csi.phase[index + 1], csi.phase[index + 2], 0.5);
                }
                else if(type == processor::interpolateCosine)
                {
                    csi.magnitude[index] = interpolation::cosineInterpolate(csi.magnitude[index - 1], csi.magnitude[index + 1], 0.5);
                    csi.phase[index] = interpolation::cosineInterpolate(csi.phase[index - 1], csi.phase[index + 1], 0.5);
                }
            }
            offset += csi.numSubCarriers;
        }
    }

    csi.magnitudePhaseToComplex();
}

void CsiProcessor::process(Csi &csi)
{
    csi.backup();
    csi.restore();

    if (Arguments::arguments.processors[processor::interpolateLinear])
    {
        this->interpolate(csi, processor::interpolateLinear);
    } 
    else if (Arguments::arguments.processors[processor::interpolateCubic])
    {
        this->interpolate(csi, processor::interpolateCubic);
    }
    else if (Arguments::arguments.processors[processor::interpolateCosine])
    {
        this->interpolate(csi, processor::interpolateCosine);
    }

    if (Arguments::arguments.processors[processor::phaseCalibrationLinearTransform])
    {
        this->phaseCalibLinearTransform(csi);
    } 
}

//WiFi-Based Real-Time Calibration-Free Passive Human Motion Detection
void CsiProcessor::phaseCalibLinearTransform(Csi &csi)
{
    /* double a = (csi.phase.back() - csi.phase[0]) / (csi.phase.size());
    double b = std::reduce(csi.phase.begin(), csi.phase.end()) / csi.phase.size();
    
    for (uint32_t i = 0; i < csi.phase.size(); i++) {
        csi.phase[i] = csi.phase[i] - (a*(i + 1) + b);
    } */

    uint32_t offset = 0;
    //double sk[] = {-26, -25, -24, -23, -22, -21, -20, -19, -18, -17, -16, -15, -14, -13, -12, -11, -10, -9, -8, -7, -6, -5, -4, -3, -2, -1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26};

    std::vector<int> sk;
    for (int i = (csi.numSubCarriers / 2); i >= 0 ; i--)
    {
        sk.push_back(-i);
    }
    for (uint32_t i = 1; i <= (csi.numSubCarriers / 2) ; i++)
    {
        sk.push_back(i);
    }
    
    
    csi.unwrapPhase();

    for (uint32_t rx = 0; rx < csi.numRx; rx++)
    {
        for (uint32_t tx = 0; tx < csi.numTx; tx++)
        {
            uint32_t firstIndex = offset;
            uint32_t lastIndex = offset + (csi.numSubCarriers - 1);
            
            double sum = 0;
            for (uint32_t i = firstIndex; i <= lastIndex; i++) {
                sum += csi.phase[i];
            }

            double a = (csi.phase[lastIndex] - csi.phase[firstIndex]) / (sk.back() - sk[0]);
            double b = sum / csi.numSubCarriers;

            uint32_t k = 0;
            for (uint32_t i = firstIndex; i <= lastIndex; i++) {
                csi.phase[i] = csi.phase[i] - a*sk[k]  - b;
                k++;
            }

            offset += csi.numSubCarriers;
        }
    }
    csi.magnitudePhaseToComplex();
}