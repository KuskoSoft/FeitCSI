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

#ifndef CSI_PROCESSOR_H
#define CSI_PROCESSOR_H

#include <string>
#include <vector>
#include "Csi.h"
#include "main.h"

class CsiProcessor
{

public:
    std::vector<Csi*> csiData;

    bool loadCsi();
    void saveCsi();
    void process(Csi &csi);


    ~CsiProcessor();
private:
    void clearState();
    void interpolate(Csi &csi, enum processor type);
    void phaseCalibLinearTransform(Csi &csi);
};

#endif