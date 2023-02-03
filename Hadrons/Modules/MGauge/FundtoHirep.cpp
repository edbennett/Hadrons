/*
 * FundtoHirep.cpp, part of Hadrons (https://github.com/aportelli/Hadrons)
 *
 * Copyright (C) 2015 - 2020
 *
 * Author: Antonin Portelli <antonin.portelli@me.com>
 *
 * Hadrons is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hadrons is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hadrons.  If not, see <http://www.gnu.org/licenses/>.
 *
 * See the full license in the file "LICENSE" in the top level distribution 
 * directory.
 */

/*  END LEGAL */

#include <Hadrons/Modules/MGauge/FundtoHirep.hpp>

// Template instantiation
//Hadrons::MGauge::TFundtoHirep<AdjointRepresentation> _test("testme");
//template class TFundtoHirep<AdjointRepresentation>;
//template TFundtoHirep<AdjointRepresentation>::TFundtoHirep(const std::string);
//template std::vector<std::string> TFundtoHirep<AdjointRepresentation>::getInput(void);
//template std::vector<std::string> TFundtoHirep<AdjointRepresentation>::getOutput(void);
//template void TFundtoHirep<AdjointRepresentation>::setup(void);
//template void TFundtoHirep<AdjointRepresentation>::execute(void);

using namespace Grid;
using namespace Hadrons;
using namespace MGauge;

template class Grid::Hadrons::MGauge::TFundtoHirep<Grid::AdjointRepresentation>;
