// Slufter3D - model Luxe10LT: black tiled roof with a dormer, brown brick, black cladding
// Author: Jan de Kort
// Created: 2026-11-16
// Luxe10LT (photo B405)
#include "ModelKit.h"
#include "Models.h"

#include "Check.h"
#include "Mesh.h"

#include <algorithm>
#include <cmath>

using namespace ModelKit;

namespace
{
    const Shape shape10LT = { 2.80, 0.60, degrees(47.0), 0.35, 0.25, 0.14, 0.0, 1.0 };
    const Paint paint10LT = {
        Color(0.11f, 0.11f, 0.12f),   // black cladding on the long walls
        Color(0.84f, 0.83f, 0.78f),   // pale plinth
        Color(0.17f, 0.18f, 0.20f),   // black tiles
        Color(0.17f, 0.18f, 0.20f),
        Color(0.43f, 0.23f, 0.19f),   // red-brown brick gables
        Color(0.93f, 0.92f, 0.87f),   // white boards and frames
        Color(0.30f, 0.36f, 0.40f),
        Color(0.17f, 0.20f, 0.22f),   // dark door
        Color(0.16f, 0.17f, 0.18f),   // dark shutters
        Color(0.13f, 0.13f, 0.14f) };

    void buildLuxe10LT(Builder& b, const Paint& paint)
    {
        const double hl = b.halfLength();
        const double hw = b.halfWidth();
        const double floor2 = shape10LT.m_wallHeight + 0.75;
        const Color frosted(0.74f, 0.80f, 0.80f);
        b.walls();
        // Gable ends are brick from the plinth to the ridge, under white verge boards
        for (const Side side : { Side::Left, Side::Right })
        {
            b.panel(side, -hw, hw, shape10LT.m_plinthHeight, shape10LT.m_wallHeight, paint.m_gable, 0.01);
            b.gable(side, paint.m_gable, 0.01, 0.0);
            b.window(side, -0.28*hw, -0.05*hw, floor2, floor2 + 1.25, 1, 1, paint.m_trim, paint.m_glass);
            b.window(side,  0.05*hw,  0.28*hw, floor2, floor2 + 1.25, 1, 1, paint.m_trim, paint.m_glass);
            b.shutters(side, -0.28*hw, 0.28*hw, floor2, floor2 + 1.25, 0.40);
        }
        b.roof();
        // Right-hand gable as in photo B405: dark door, wide window with shutters
        b.window(Side::Right, -0.62*hw, -0.38*hw, 0.62, 2.60, 1, 1, paint.m_gable, paint.m_door);
        b.window(Side::Right,  0.05*hw,  0.50*hw, 1.15, 2.30, 2, 1, paint.m_trim, paint.m_glass);
        b.shutters(Side::Right, 0.05*hw, 0.50*hw, 1.15, 2.30, 0.40);
        b.window(Side::Left,  -0.30*hw,  0.30*hw, 1.15, 2.30, 2, 1, paint.m_trim, paint.m_glass);
        // Front: frosted door leaf beside a dark one, a tall window, the dormer above
        b.window(Side::Front,  0.10*hl,  0.26*hl, 0.62, 2.62, 1, 1, paint.m_trim, frosted);
        b.window(Side::Front,  0.26*hl,  0.42*hl, 0.62, 2.62, 1, 1, paint.m_wall, paint.m_door);
        b.window(Side::Front, -0.62*hl, -0.40*hl, 0.62, 2.40, 2, 1, paint.m_door, paint.m_glass);
        b.dormer(-0.12*hl, 0.22*hl, floor2 + 0.55, floor2 + 1.50, 3, paint.m_roof.scaled(0.9f), paint.m_roof);
        b.window(Side::Back, -0.55*hl, 0.55*hl, 0.62, 2.50, 4, 1, paint.m_trim, paint.m_glass);
        b.terrace(Side::Back, -0.70*hl, 0.70*hl, 3.0, terraceGrey);
        b.pipe( 0.55*hl, 0.45*hw, 0.5, 0.09);
        b.pipe( 0.66*hl, 0.45*hw, 0.6, 0.09);
        b.pipe(-0.70*hl, 0.60*hw, 0.6, 0.08);
    }

    // Class Luxe10LT
    class Luxe10LT : public GableModel
    {
    public:
        Luxe10LT()
            :
            GableModel(shape10LT, paint10LT)
        {
        }

        // Accessor
        const char* name() const override { return "Luxe10LT"; }
        const char* label() const override { return "10LT"; }
        std::pair<double, double> footprint(double outlineLength, double outlineWidth) const override
        {
            return wallsUnderRoof(outlineLength, outlineWidth);
        }

    protected:
        void draw(Builder& builder, const Paint& paint) const override { buildLuxe10LT(builder, paint); }
    };
}

const Model& Models::luxe10LT()
{
    static const Luxe10LT model;
    return model;
}
