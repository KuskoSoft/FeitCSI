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

#ifndef MAIN_H
#define MAIN_H

#include <string>
#include <cstdint>
#include <map>

#define MONITOR_INTERFACE_NAME "mon0"

enum processor 
{
    interpolateLinear,
    interpolateCubic,
    interpolateCosine,
    phaseCalibrationLinearTransform,
};

struct Arguments
{
    bool verbose;
    uint16_t frequency;
    bool gui = false;
    bool plot = false;
    std::string bandwidth;
    std::string outputFile;
    uint8_t mcs;
    uint16_t channelWidth;
    uint8_t spatialStreams;
    uint8_t txPower;
    uint16_t guardInterval;
    uint32_t injectDelay;
    uint32_t injectRepeat;
    std::string coding;
    std::string format;
    bool inject;
    bool measure;
    std::string mode;
    std::string ltf;
    std::string inputFile;
    std::map<enum processor, bool> processors;
};

/* Get the input argument from argp_parse, which we
   know is a pointer to our arguments structure. */
extern struct Arguments arguments;

#endif