// Slufter3D - model ReceptionWing: a wing of the central building, red tiles, white brick, black gables
// Author: Jan de Kort
// Created: 2026-11-16
// ReceptionWing (photo Receptie)
#include "ModelKit.h"
#include "Models.h"

#include "Check.h"
#include "Mesh.h"

#include <algorithm>
#include <cmath>

using namespace ModelKit;

namespace
{
    const Shape shapeReception = { 3.00, 0.55, degrees(42.0), 0.25, 0.25, 0.18, 0.0, 1.0 };
    const double receptionMaxRise = 4.2;   // wide wings get a flatter roof, so that no ridge towers over the rest [m]
    const Paint paintReception = {
        Color(0.91f, 0.90f, 0.86f),   // white painted brick
        Color(0.13f, 0.13f, 0.14f),   // black plinth
        Color(0.70f, 0.29f, 0.17f),   // orange-red tiles
        Color(0.70f, 0.29f, 0.17f),
        Color(0.12f, 0.12f, 0.13f),   // black boarded gables
        Color(0.95f, 0.94f, 0.90f),   // white verge boards and frames
        Color(0.21f, 0.25f, 0.29f),
        Color(0.93f, 0.93f, 0.90f),
        Color(0.12f, 0.12f, 0.13f),
        Color(0.20f, 0.20f, 0.21f) };

    void buildReceptionWing(Builder& b, const Paint& paint)
    {
        const double hl = b.halfLength();
        const double hw = b.halfWidth();
        const double bay = 4.5;              // one window per bay along the wall [m]
        const Color darkFrame(0.10f, 0.10f, 0.11f);
        b.walls();
        for (const Side side : { Side::Left, Side::Right })
        {
            b.gable(side, paint.m_trim, 0.02, 0.0);
            b.gable(side, paint.m_gable, 0.04, 0.20);
            if (b.ridge() - shapeReception.m_wallHeight > 2.4)
            {
                b.window(side, -0.50, 0.50, shapeReception.m_wallHeight + 0.55, shapeReception.m_wallHeight + 1.90, 2, 1,
                    paint.m_trim, paint.m_glass);
            }
            b.window(side, -0.45*hw, 0.45*hw, 0.80, 2.45, 2, 1, darkFrame, paint.m_glass);
        }
        b.roof();
        const int bays = std::max(1, (int) (2.0 * hl / bay));
        for (int k = 0; k < bays; ++k)
        {
            const double mid = -hl + (2.0 * hl) * ((double) k + 0.5) / (double) bays;
            for (const Side side : { Side::Front, Side::Back })
            {
                b.window(side, mid - 1.2, mid + 1.2, 0.80, 2.45, 2, 1, darkFrame, paint.m_glass);
            }
            if (k % 2 == 0)
            {
                b.skylight(mid - 0.45, mid + 0.45, 0.62*hw, 0.30*hw,  1.0, darkFrame, Color(0.55f, 0.66f, 0.74f));
                b.skylight(mid - 0.45, mid + 0.45, 0.62*hw, 0.30*hw, -1.0, darkFrame, Color(0.55f, 0.66f, 0.74f));
            }
        }
    }

    // Class ReceptionWing
    class ReceptionWing : public GableModel
    {
    public:
        ReceptionWing()
            :
            GableModel(shapeReception, paintReception)
        {
        }

        // Accessor
        const char* name() const override { return "ReceptionWing"; }
        const char* label() const override { return "reception"; }
        std::pair<double, double> footprint(double outlineLength, double outlineWidth) const override
        {
            return { outlineLength, outlineWidth }; // a wing is given as its walls
        }

    protected:
        Shape shapeFor(const Site& site) const override
        {
            Shape shape = shapeReception;
            shape.m_pitch = std::atan(std::min(std::tan(shape.m_pitch), receptionMaxRise / (0.5 * site.m_width)));
            return shape;
        }

        void draw(Builder& builder, const Paint& paint) const override { buildReceptionWing(builder, paint); }
    };
}

const Model& Models::receptionWing()
{
    static const ReceptionWing model;
    return model;
}
