// Slufter3D - the houses of the EL line: one floor under a black hipped roof, a corner cut away at the entrance
// Author: Jan de Kort
// Created: 2026-11-16
#pragma once

#include "Site.h"

#include <utility>

class Mesh;

// Drawn for the model Luxe4EL
namespace NotchedEL
{
    std::pair<double, double> size4EL();  // the 4EL has its own plan: length and width of its walls [m]
    double ridgeHeight(double width);     // above the floor [m]
    // true->the point (du, dv) lies within margin of the walls of the 4EL
    bool covers4EL(double du, double dv, double margin);
    void build4EL(Mesh& mesh, const Site& site);
}
