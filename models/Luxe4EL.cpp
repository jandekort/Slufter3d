// Slufter3D - model Luxe4EL: a square with two corners cut away under one black hipped roof
// Author: Jan de Kort
// Created: 2026-11-16
// Photos B453_Type_4EL, 4EL-2, images, images-1. Black boarding and the double doors in the front cut-out.
// Drawn by NotchedEL.
#include "Model.h"
#include "Models.h"
#include "ModelKit.h"
#include "NotchedEL.h"

namespace
{
    // Class Luxe4EL
    class Luxe4EL : public Model
    {
    public:
        // Accessor
        const char* name() const override { return "Luxe4EL"; }
        const char* label() const override { return "4EL"; }
        std::pair<double, double> footprint(double, double) const override { return NotchedEL::size4EL(); }
        double ridgeHeight(double width) const override { return NotchedEL::ridgeHeight(width); }
        bool covers(double, double, double du, double dv, double margin) const override
        {
            return NotchedEL::covers4EL(du, dv, margin);
        }

        void build(Mesh& mesh, const Site& site) const override { NotchedEL::build4EL(mesh, site); }
    };
}

const Model& Models::luxe4EL()
{
    static const Luxe4EL model;
    return model;
}
