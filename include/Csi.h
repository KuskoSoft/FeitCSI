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

#ifndef CSI_H
#define CSI_H

#include <cstdint>
#include <string>
#include <complex>
#include <vector>
#include "UdpSocket.h"

#define CSI_HEADER_LENGTH 272

struct __attribute__((__packed__)) RawHeaderData
{
    uint32_t csiDataSize;
    uint32_t space4;
    uint32_t ftmClock;
    //uint8_t space12[34];
    uint64_t timestamp;
    uint8_t space20[26];
    uint8_t numRx;
    uint8_t numTx;
    uint8_t space48[4];
    uint32_t numSubCarriers;
    uint8_t space54[4];
    uint32_t rssi1;
    uint32_t rssi2;
    uint8_t srcMac[6];
    uint8_t space75[18];
    uint32_t rateNflag;
    uint32_t space96[44];
};

class Csi
{

public:
    Csi();
    ~Csi();
    // void load(uint8_t *data, uint32_t size);
    void loadFromFile(std::string fileName);
    void loadFromMemory(uint8_t *pHeader, uint8_t *rawCsiData);
    void loadFromMemory(uint8_t *rawData);
    void save();
    void sendUDP(UdpSocket *udpSocket);
    void backup();
    void restore();
    void magnitudePhaseToComplex();
    void recalcMagnitudePhase();
    void unwrapPhase();
    const std::vector<uint32_t> getPilotIndices();

    RawHeaderData rawHeaderData;
    uint32_t numRx;
    uint32_t numTx;
    uint32_t numSubCarriers = 0;
    uint32_t format = 0;
    uint32_t channelWidth = 0;
    std::vector<std::complex<double>> csi;
    std::vector<std::complex<double>> csiBackup;
    std::vector<double> magnitude;
    std::vector<double> phase;

private:
    const std::vector<uint32_t> NO_NHT_20_PILOT_INDICES = {5, 19, 32, 46};                                                                                                                                                              // 52 subcarriers
    const std::vector<uint32_t> HT_VHT_20_PILOT_INDICES = {7, 21, 34, 48};                                                                                                                                                              // 56 subcarriers
    const std::vector<uint32_t> HT_VHT_40_PILOT_INDICES = {5, 33, 47, 66, 80, 108};                                                                                                                                                     // 114 subcarriers
    const std::vector<uint32_t> VHT_80_PILOT_INDICES = {19, 47, 83, 111, 130, 158, 194, 222};                                                                                                                                           // 242 subcarriers
    const std::vector<uint32_t> VHT_160_PILOT_INDICES = {19, 47, 83, 111, 130, 158, 194, 222, 261, 289, 325, 353, 372, 400, 436, 464};                                                                                                  // 484 subcarriers
    const std::vector<uint32_t> HE_20_PILOT_INDICES = {6, 32, 74, 100, 141, 167, 209, 235};                                                                                                                                             // 242 subcarriers
    const std::vector<uint32_t> HE_40_PILOT_INDICES = {6, 32, 74, 100, 140, 166, 208, 234, 249, 275, 317, 343, 383, 409, 451, 477};                                                                                                     // 484 subcarriers
    const std::vector<uint32_t> HE_80_PILOT_INDICES = {32, 100, 166, 234, 274, 342, 408, 476, 519, 587, 653, 721, 761, 829, 895, 963};                                                                                                  // 996 subcarriers
    const std::vector<uint32_t> HE_160_PILOT_INDICES = {32, 100, 166, 234, 274, 342, 408, 476, 519, 587, 653, 721, 761, 829, 895, 963, 1028, 1096, 1162, 1230, 1270, 1338, 1404, 1472, 1515, 1583, 1649, 1717, 1757, 1825, 1891, 1959}; // 1992 subcarriers

    std::string saveFilePath;
    uint8_t *rawCsiData = nullptr;

    void fixCsiBug();
    void processRawCsi();

    double constrainAngle(double x);
    double angleConv(double angle);
    double angleDiff(double a, double b);
    double unwrap(double previousAngle, double newAngle);
};

#endif