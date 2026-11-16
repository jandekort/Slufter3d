// Slufter3D - model Luxe10L: as 10LT with timber boarding, dark grey tiles, a pergola at the gable
// Author: Jan de Kort
// Created: 2026-11-16
// Luxe10L (photo Type_10L)
// The same house as 10LT with timber boarding on the long walls, a dark grey roof and a pergola at the gable
#include "ModelKit.h"
#include "Models.h"

#include "Check.h"
#include "Mesh.h"

#include <algorithm>
#include <cmath>

using namespace ModelKit;

namespace
{
    const Shape shape10L = { 2.80, 0.60, degrees(49.0), 0.35, 0.25, 0.14, 0.0, 1.0 };
    const Paint paint10L = {
        Color(0.45f, 0.40f, 0.29f),   // weathered timber boarding on the long walls
        Color(0.90f, 0.89f, 0.85f),   // white plinth
        Color(0.24f, 0.25f, 0.27f),   // dark grey tiles
        Color(0.24f, 0.25f, 0.27f),
        Color(0.52f, 0.27f, 0.21f),   // red-brown brick gables
        Color(0.95f, 0.94f, 0.90f),   // white verge boards and frames
        Color(0.33f, 0.40f, 0.45f),
        Color(0.90f, 0.90f, 0.87f),   // white door
        Color(0.22f, 0.23f, 0.25f),   // dark grey shutters
        Color(0.13f, 0.13f, 0.14f) };

    void buildLuxe10L(Builder& b, const Paint& paint)
    {
        const double hl = b.halfLength();
        const double hw = b.halfWidth();
        const double floor2 = shape10L.m_wallHeight + 0.75;
        const Color wood(0.55f, 0.52f, 0.47f);
        b.walls();
        for (const Side side : { Side::Left, Side::Right })
        {
            b.panel(side, -hw, hw, shape10L.m_plinthHeight, shape10L.m_wallHeight, paint.m_gable, 0.01);
            b.gable(side, paint.m_trim, 0.01, 0.0);
            b.gable(side, paint.m_gable, 0.03, 0.18);
            b.window(side, -0.26*hw, -0.04*hw, floor2, floor2 + 1.30, 1, 1, paint.m_trim, paint.m_glass);
            b.window(side,  0.04*hw,  0.26*hw, floor2, floor2 + 1.30, 1, 1, paint.m_trim, paint.m_glass);
            b.shutters(side, -0.26*hw, 0.26*hw, floor2, floor2 + 1.30, 0.40);
        }
        b.roof();
        // Right-hand gable as in the photo: garden doors, a shuttered window, the pergola over the terrace
        b.window(Side::Right, -0.45*hw, -0.12*hw, 0.62, 2.55, 2, 1, paint.m_trim, paint.m_glass);
        b.window(Side::Right,  0.30*hw,  0.55*hw, 1.10, 2.40, 1, 1, paint.m_trim, paint.m_glass);
        b.shutters(Side::Right, 0.30*hw, 0.55*hw, 1.10, 2.40, 0.45);
        b.terrace(Side::Right, -1.2*hw, 0.75*hw, 4.2, terraceGrey);
        b.pergola(Side::Right, -1.1*hw, 0.65*hw, 3.8, 2.55, wood);
        b.window(Side::Left, -0.30*hw, 0.30*hw, 1.15, 2.30, 2, 1, paint.m_trim, paint.m_glass);
        // Long side: tall window, boarded bays, the door at the far end, a dormer above
        b.window(Side::Front, 0.55*hl, 0.75*hl, 0.62, 2.55, 1, 1, paint.m_trim, paint.m_glass);
        b.door(Side::Front, -0.55*hl, -0.38*hl, 0.62, 2.60, paint.m_trim, paint.m_door, true, paint.m_glass);
        b.window(Side::Front, -0.20*hl, 0.20*hl, 1.20, 2.40, 2, 1, paint.m_trim, paint.m_glass);
        b.dormer(-0.10*hl, 0.18*hl, floor2 + 0.55, floor2 + 1.50, 2, paint.m_roof.scaled(0.9f), paint.m_roof);
        b.window(Side::Back, -0.55*hl, 0.55*hl, 0.62, 2.50, 4, 1, paint.m_trim, paint.m_glass);
        b.pipe( 0.35*hl, 0.50*hw, 0.6, 0.09);
    }

    // Class Luxe10L
    class Luxe10L : public GableModel
    {
    public:
        Luxe10L()
            :
            GableModel(shape10L, paint10L)
        {
        }

        // Accessor
        const char* name() const override { return "Luxe10L"; }
        const char* label() const override { return "10L"; }
        std::pair<double, double> footprint(double outlineLength, double outlineWidth) const override
        {
            return wallsUnderRoof(outlineLength, outlineWidth);
        }

    protected:
        void draw(Builder& builder, const Paint& paint) const override { buildLuxe10L(builder, paint); }
    };
}

const Model& Models::luxe10L()
{
    static const Luxe10L model;
    return model;
}
