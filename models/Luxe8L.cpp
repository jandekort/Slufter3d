// Slufter3D - model Luxe8L: thatch over red tiles, red brick, dark gable
// Author: Jan de Kort
// Created: 2026-11-16
// Luxe8L (photos B449a, B449b)
#include "ModelKit.h"
#include "Models.h"

#include "Check.h"
#include "Mesh.h"

#include <algorithm>
#include <cmath>

using namespace ModelKit;

namespace
{
    const Shape shape8L = { 2.70, 0.60, degrees(50.0), 0.45, 0.35, 0.16, 0.36, 0.42 };
    const Paint paint8L = {
        Color(0.60f, 0.29f, 0.21f),   // red brick
        Color(0.90f, 0.88f, 0.81f),   // white plinth
        Color(0.66f, 0.21f, 0.15f),   // red tiles at the foot of the roof
        Color(0.43f, 0.37f, 0.31f),   // thatch
        Color(0.23f, 0.24f, 0.25f),   // dark timber cladding
        Color(0.94f, 0.93f, 0.88f),   // white boards and frames
        Color(0.36f, 0.44f, 0.50f),
        Color(0.30f, 0.36f, 0.40f),
        Color(0.20f, 0.20f, 0.21f),   // dark shutters
        Color(0.10f, 0.10f, 0.11f) };

    void buildLuxe8L(Builder& b, const Paint& paint)
    {
        const double hl = b.halfLength();
        const double hw = b.halfWidth();
        const double floor2 = shape8L.m_wallHeight + 0.55;
        b.walls();
        // Gables: white boarding behind, dark cladding within it
        for (const Side side : { Side::Left, Side::Right })
        {
            b.gable(side, paint.m_trim, 0.02, 0.0);
            b.gable(side, paint.m_gable, 0.04, 0.22);
        }
        b.roof();
        b.ridgeCap(Color(0.86f, 0.47f, 0.27f));
        // Right-hand gable as in photo B449b: glass front to the ground, shuttered windows either side and above
        b.window(Side::Right, -0.42*hw, 0.42*hw, 0.05, 2.62, 4, 3, paint.m_trim, paint.m_glass);
        b.window(Side::Right, -0.82*hw, -0.62*hw, 1.00, 2.10, 1, 1, paint.m_trim, paint.m_glass);
        b.shutters(Side::Right, -0.82*hw, -0.62*hw, 1.00, 2.10, 0.35);
        b.window(Side::Right,  0.62*hw,  0.82*hw, 1.00, 2.10, 1, 1, paint.m_trim, paint.m_glass);
        b.shutters(Side::Right, 0.62*hw, 0.82*hw, 1.00, 2.10, 0.35);
        for (const Side side : { Side::Left, Side::Right })
        {
            b.window(side, -0.26*hw, -0.04*hw, floor2, floor2 + 1.75, 1, 2, paint.m_trim, paint.m_glass);
            b.window(side,  0.04*hw,  0.26*hw, floor2, floor2 + 1.75, 1, 2, paint.m_trim, paint.m_glass);
            b.shutters(side, -0.26*hw, 0.26*hw, floor2 + 0.05, floor2 + 1.70, 0.50);
        }
        b.window(Side::Left, -0.30*hw, 0.30*hw, 1.00, 2.20, 2, 1, paint.m_trim, paint.m_glass);
        b.shutters(Side::Left, -0.30*hw, 0.30*hw, 1.00, 2.20, 0.50);
        // Long sides as in photo B449a: glazed door, shuttered windows, a dormer in the thatch
        for (const Side side : { Side::Front, Side::Back })
        {
            b.window(side, -0.10*hl, 0.14*hl, 0.62, 2.45, 1, 1, paint.m_trim, paint.m_door);
            b.window(side, -0.62*hl, -0.44*hl, 1.10, 2.10, 1, 1, paint.m_trim, paint.m_glass);
            b.shutters(side, -0.62*hl, -0.44*hl, 1.10, 2.10, 0.40);
            b.window(side,  0.50*hl,  0.68*hl, 1.10, 2.10, 1, 1, paint.m_trim, paint.m_glass);
            b.shutters(side, 0.50*hl, 0.68*hl, 1.10, 2.10, 0.40);
        }
        b.dormer(-0.42*hl, -0.20*hl, floor2 + 1.05, floor2 + 2.05, 1, paint.m_roofTop.scaled(0.85f), paint.m_roofTop);
        b.terrace(Side::Front, -0.30*hl, 0.45*hl, 3.0, terraceGrey);
        b.pipe(0.42*hl, 0.62*hw, 0.7, 0.10);
        b.pipe(0.25*hl, 0.62*hw, 0.6, 0.10);
    }

    // Class Luxe8L
    class Luxe8L : public GableModel
    {
    public:
        Luxe8L()
            :
            GableModel(shape8L, paint8L)
        {
        }

        // Accessor
        const char* name() const override { return "Luxe8L"; }
        const char* label() const override { return "8L"; }
        std::pair<double, double> footprint(double outlineLength, double outlineWidth) const override
        {
            return wallsUnderRoof(outlineLength, outlineWidth);
        }

    protected:
        void draw(Builder& builder, const Paint& paint) const override { buildLuxe8L(builder, paint); }
    };
}

const Model& Models::luxe8L()
{
    static const Luxe8L model;
    return model;
}
