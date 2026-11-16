// Slufter3D - model LuxeEL: the 4EL on the rectangle of the house, with only the front cut-out
// Author: Jan de Kort
// Created: 2026-11-16
// For the EL types without a photograph (6EL, 8EL). Drawn by NotchedEL.
#include "Model.h"
#include "Models.h"
#include "ModelKit.h"
#include "NotchedEL.h"

namespace
{
    // Class LuxeEL
    class LuxeEL : public Model
    {
    public:
        // Accessor
        const char* name() const override { return "LuxeEL"; }
        const char* label() const override { return "EL"; }
        std::pair<double, double> footprint(double outlineLength, double outlineWidth) const override
        {
            return ModelKit::wallsUnderRoof(outlineLength, outlineWidth);
        }
        double ridgeHeight(double width) const override { return NotchedEL::ridgeHeight(width); }
        bool covers(double length, double width, double du, double dv, double margin) const override
        {
            return NotchedEL::coversEL(length, width, du, dv, margin);
        }

        void build(Mesh& mesh, const Site& site) const override { NotchedEL::buildEL(mesh, site); }
    };
}

const Model& Models::luxeEL()
{
    static const LuxeEL model;
    return model;
}
