/*
 * FeitCSI is the tool for extracting CSI information from supported intel NICs.
 * Copyright (C) 2023-2025 Miroslav Hutar.
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

#include "Csi.h"
#include <cstring>
#include <string>
#include <fstream>
#include <iterator>
#include <vector>
#include <iostream>
#include <filesystem>
#include "rs.h"
#include "Logger.h"
#include "Arguments.h"

Csi::Csi()
{
}

Csi::~Csi()
{
    if (this->rawCsiData)
    {
        delete rawCsiData;
    }
}

void Csi::loadFromFile(std::string fileName)
{
    std::ifstream ifs(fileName, std::ios::binary);
    ifs.read((char *)&this->rawHeaderData, CSI_HEADER_LENGTH);
    this->rawCsiData = new uint8_t[this->rawHeaderData.csiDataSize];

    // uint8_t rawCsiData[this->rawHeaderData.csiDataSize];

    ifs.read((char *)this->rawCsiData, this->rawHeaderData.csiDataSize);

    this->processRawCsi();
}

void Csi::loadFromMemory(uint8_t *pHeader, uint8_t *pRawCsiData)
{
    memcpy(&this->rawHeaderData, pHeader, CSI_HEADER_LENGTH);
    this->rawCsiData = new uint8_t[this->rawHeaderData.csiDataSize];
    memcpy(this->rawCsiData, pRawCsiData, this->rawHeaderData.csiDataSize);
    //this->rawHeaderData.timestamp = (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    this->processRawCsi();
}

void Csi::loadFromMemory(uint8_t *rawData)
{
    memcpy(&this->rawHeaderData, rawData, CSI_HEADER_LENGTH);
    this->rawCsiData = new uint8_t[this->rawHeaderData.csiDataSize];
    memcpy(this->rawCsiData, &rawData[CSI_HEADER_LENGTH], this->rawHeaderData.csiDataSize);
    this->processRawCsi();
}

void Csi::save()
{
    std::ofstream outfile;
    outfile.open(Arguments::arguments.outputFile, std::ios_base::app | std::ios::binary);
    if (outfile.fail())
    {
        throw std::ios_base::failure("Open file failed: " + std::string(std::strerror(errno)));
    }
    outfile.write(reinterpret_cast<char *>(&this->rawHeaderData), sizeof(RawHeaderData));
    outfile.write(reinterpret_cast<char *>(this->rawCsiData), this->rawHeaderData.csiDataSize);
    outfile.close();
    std::filesystem::permissions(Arguments::arguments.outputFile, std::filesystem::perms::all & ~(std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec | std::filesystem::perms::others_exec), std::filesystem::perm_options::add);
}

void Csi::sendUDP(UdpSocket *udpSocket)
{
    int size = CSI_HEADER_LENGTH + this->rawHeaderData.csiDataSize;
    char data[size]; 
    memcpy(data, &this->rawHeaderData, CSI_HEADER_LENGTH);
    memcpy(&data[CSI_HEADER_LENGTH], this->rawCsiData, this->rawHeaderData.csiDataSize);
    udpSocket->send(data, size);
}

void Csi::fixCsiBug()
{
    if (this->channelWidth != RATE_MCS_CHAN_WIDTH_160)
    {
        return;
    }

    if (this->format != RATE_MCS_VHT_MSK && this->format != RATE_MCS_HE_MSK)
    {
        return;
    }

    uint16_t newSubcarrierSize = 0;
    if (this->format == RATE_MCS_VHT_MSK && this->rawHeaderData.numSubCarriers == 484)
    {
        return;
    }
    else if (this->format == RATE_MCS_VHT_MSK)
    {
        newSubcarrierSize = 484;
    }

    if (this->format == RATE_MCS_HE_MSK && this->rawHeaderData.numSubCarriers == 1992)
    {
        return;
    }
    else if (this->format == RATE_MCS_HE_MSK)
    {
        newSubcarrierSize = 1992;
    }

    for (uint32_t i = 0; i < this->rawHeaderData.csiDataSize; i = i + 4)
    {
        if (format == RATE_MCS_VHT_MSK)
        {
            if (i > (241 * 4) && i < (256 * 4)) // Fix the firmware bug
                continue;
        }
    }

    uint32_t newTotalSize = newSubcarrierSize * 4 *this->numRx * this->numTx;
    uint8_t fixedCsiData[newTotalSize];

    uint32_t newIndex = 0;
    uint32_t oldIndex = 0;
    for (uint32_t rx = 0; rx < this->numRx; rx++)
    {
        for (uint32_t tx = 0; tx < this->numTx; tx++)
        {
            for (uint32_t n = 0; n < this->numSubCarriers; n++)
            {
                if (this->format == RATE_MCS_VHT_MSK)
                {
                    if (n > 241 && n < 256)
                    {
                        oldIndex += 4;
                        continue;
                    }
                }
                if (this->format == RATE_MCS_HE_MSK)
                {
                    if (n > 995 && n < 1024)
                    {
                        oldIndex += 4;
                        continue;
                    }
                }
                memcpy(&fixedCsiData[newIndex], &this->rawCsiData[oldIndex], 4);
                oldIndex += 4;
                newIndex += 4;
            }
        }
    }

    this->numSubCarriers = newSubcarrierSize;
    this->rawHeaderData.numSubCarriers = this->numSubCarriers;
    this->rawHeaderData.csiDataSize = newTotalSize;
    delete this->rawCsiData;
    this->rawCsiData = new uint8_t[newTotalSize];
    memcpy(this->rawCsiData, fixedCsiData, newTotalSize);
}

void Csi::processRawCsi()
{
    this->numRx = this->rawHeaderData.numRx;
    this->numTx = this->rawHeaderData.numTx;
    this->numSubCarriers = this->rawHeaderData.numSubCarriers;

    this->format = this->rawHeaderData.rateNflag & RATE_MCS_MOD_TYPE_MSK;
    this->channelWidth = this->rawHeaderData.rateNflag & RATE_MCS_CHAN_WIDTH_MSK;
    
    this->fixCsiBug();

    

    for (uint32_t i = 0; i < this->rawHeaderData.csiDataSize; i = i + 4)
    {
        int16_t real = this->rawCsiData[i] | this->rawCsiData[i + 1] << 8;
        int16_t imag = this->rawCsiData[i + 2] | this->rawCsiData[i + 3] << 8;


        /* if (format == RATE_MCS_VHT_MSK)
        {
            if (i > (241 * 4) && i < (256 * 4)) // Fix the firmware bug
                continue;
        } */

        const std::complex<double> c(real, imag);
        this->csi.push_back(c);
        this->magnitude.push_back(std::abs(c));
        this->phase.push_back(std::arg(c));
    }
}

void Csi::backup()
{
    if (this->csiBackup.empty())
    {
        this->csiBackup = this->csi;
    }
}

void Csi::restore()
{
    this->csi = this->csiBackup;
    this->recalcMagnitudePhase();
}

void Csi::magnitudePhaseToComplex()
{
    for (uint32_t i = 0; i < this->csi.size(); i++)
    {
        this->csi[i].real(this->magnitude[i] * cos(this->phase[i]));
        this->csi[i].imag(this->magnitude[i] * sin(this->phase[i]));
    }
}

void Csi::recalcMagnitudePhase()
{
    this->magnitude.clear();
    this->phase.clear();
    for (std::complex<double> c : this->csi)
    {
        this->magnitude.push_back(std::abs(c));
        this->phase.push_back(std::arg(c));
    }
    //this->unwrapPhase();
}

const std::vector<uint32_t> Csi::getPilotIndices()
{
    switch (this->format)
    {
    case RATE_MCS_CCK_MSK: // VERY OLD FORMAT NOT USED NOW
        break;
    case RATE_MCS_LEGACY_OFDM_MSK:
        return this->NO_NHT_20_PILOT_INDICES;
        break;
    case RATE_MCS_HT_MSK:
        switch (channelWidth)
        {
        case RATE_MCS_CHAN_WIDTH_20:
            return HT_VHT_20_PILOT_INDICES;
            break;
        case RATE_MCS_CHAN_WIDTH_40:
            return HT_VHT_40_PILOT_INDICES;
            break;
        }
        break;
    case RATE_MCS_VHT_MSK:
        switch (this->channelWidth)
        {
        case RATE_MCS_CHAN_WIDTH_20:
            return HT_VHT_20_PILOT_INDICES;
            break;
        case RATE_MCS_CHAN_WIDTH_40:
            return HT_VHT_40_PILOT_INDICES;
            break;
        case RATE_MCS_CHAN_WIDTH_80:
            return VHT_80_PILOT_INDICES;
            break;
        case RATE_MCS_CHAN_WIDTH_160:
            return VHT_160_PILOT_INDICES;
            break;
        }
        break;
    case RATE_MCS_HE_MSK:
        switch (channelWidth)
        {
        case RATE_MCS_CHAN_WIDTH_20:
            return HE_20_PILOT_INDICES;
            break;
        case RATE_MCS_CHAN_WIDTH_40:
            return HE_40_PILOT_INDICES;
            break;
        case RATE_MCS_CHAN_WIDTH_80:
            return HE_80_PILOT_INDICES;
            break;
        case RATE_MCS_CHAN_WIDTH_160:
            return HE_160_PILOT_INDICES;
            break;
        }
        break;
    case RATE_MCS_EHT_MSK:
        // TODO WiFi 7
        break;
    }
    std::vector<uint32_t> v;
    return v;
}

double Csi::constrainAngle(double x){
    x = fmod(x + M_PI,M_2_PI);
    if (x < 0)
        x += M_2_PI;
    return x - M_PI;
}

// convert to [-360,360]
double Csi::angleConv(double angle){
    return fmod(constrainAngle(angle),M_2_PI);
}

double Csi::angleDiff(double a,double b){
    double dif = fmod(b - a + M_PI,M_2_PI);
    if (dif < 0)
        dif += M_2_PI;
    return dif - M_PI;
}

double Csi::unwrap(double previousAngle,double newAngle){
    float d = newAngle - previousAngle;
    d = d > M_PI ? d - 2 * M_PI : (d < -M_PI ? d + 2 * M_PI : d);
    return previousAngle + d;

    //return previousAngle - angleDiff(newAngle,angleConv(previousAngle));
}

void Csi::unwrapPhase()
{
    uint32_t offset = 0;
    for (uint32_t rx = 0; rx < this->numRx; rx++)
    {
        for (uint32_t tx = 0; tx < this->numTx; tx++)
        {
            for (uint32_t n = 1; n < this->numSubCarriers; n++)
            {
                uint32_t index = n + offset;
                this->phase[index] = this->unwrap(this->phase[index - 1], this->phase[index]);
            }
            offset += this->numSubCarriers;
        }
    }
}
