// Slufter3D - the houses of the EL line: one floor under a black hipped roof, a corner cut away at the entrance
// Author: Jan de Kort
// Created: 2026-11-16
#pragma once

#include "Site.h"

#include <utility>

class Mesh;

// Shared by the models Luxe4EL and LuxeEL
namespace NotchedEL
{
    std::pair<double, double> size4EL();  // the 4EL has its own plan: length and width of its walls [m]
    double ridgeHeight(double width);     // above the floor [m]
    // true->the point (du, dv) lies within margin of the walls: of the 4EL, of an EL of this size
    bool covers4EL(double du, double dv, double margin);
    bool coversEL(double length, double width, double du, double dv, double margin);
    void build4EL(Mesh& mesh, const Site& site);
    void buildEL(Mesh& mesh, const Site& site); // on the rectangle of the site, with the front cut-out only
}
