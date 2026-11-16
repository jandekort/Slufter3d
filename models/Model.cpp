// Slufter3D - a vector model of a building
// Author: Jan de Kort
// Created: 2026-11-16
#include "Model.h"

#include <cmath>

// Class Model
bool Model::covers(double length, double width, double du, double dv, double margin) const
{
    return std::fabs(du) < 0.5 * length + margin && std::fabs(dv) < 0.5 * width + margin;
}
