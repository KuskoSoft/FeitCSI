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

#include "interpolation.h"
#include <cmath>

namespace interpolation
{
    double linearInterpolate(double y1, double y2, double mu)
    {
        return (y1 * (1 - mu) + y2 * mu);
    }

    double cosineInterpolate(double y1, double y2, double mu)
    {
        double mu2;
        mu2 = (1 - cos(mu * M_PI)) / 2;
        return (y1 * (1 - mu2) + y2 * mu2);
    }

    double cubicInterpolate(double y0, double y1, double y2, double y3, double mu)
    {
        double a0, a1, a2, a3, mu2;

        mu2 = mu * mu;
        a0 = y3 - y2 - y0 + y1;
        a1 = y0 - y1 - a0;
        a2 = y2 - y0;
        a3 = y1;

        return (a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3);
    }
}