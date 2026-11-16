// Slufter3D - model Luxe6L: thatched hipped roof down to the eaves, red brick, white plinth
// Author: Jan de Kort
// Created: 2026-11-16
// Luxe6L (photo Type_6L)
#include "ModelKit.h"
#include "Models.h"

#include "Check.h"
#include "Mesh.h"

#include <algorithm>
#include <cmath>

using namespace ModelKit;

namespace
{
    const Shape shape6L = { 2.55, 0.65, degrees(47.0), 0.55, 0.55, 0.10, 1.0, 1.0 };
    const Paint paint6L = {
        Color(0.62f, 0.31f, 0.23f),   // red brick
        Color(0.92f, 0.91f, 0.86f),   // white plinth
        Color(0.40f, 0.35f, 0.29f),   // thatch, down to the eaves
        Color(0.40f, 0.35f, 0.29f),
        Color(0.62f, 0.31f, 0.23f),
        Color(0.94f, 0.93f, 0.88f),   // white frames
        Color(0.34f, 0.41f, 0.46f),
        Color(0.33f, 0.35f, 0.37f),   // grey door
        Color(0.24f, 0.24f, 0.25f),   // dark shutters
        Color(0.16f, 0.17f, 0.18f) };

    void buildLuxe6L(Builder& b, const Paint& paint)
    {
        const double hl = b.halfLength();
        const double hw = b.halfWidth();
        const Color frosted(0.80f, 0.86f, 0.84f);
        const Color thatchEdge = paint.m_roof.scaled(0.8f);
        b.walls();
        b.roof();
        b.ridgeCap(Color(0.88f, 0.52f, 0.25f));
        // Entrance side as in the photo: shuttered window, a frosted leaf beside the grey door, a second window
        b.window(Side::Back,  0.42*hl,  0.56*hl, 1.05, 2.05, 1, 1, paint.m_trim, paint.m_glass);
        b.panel(Side::Back,   0.26*hl,  0.41*hl, 1.05, 2.05, paint.m_shutter, 0.06);
        b.door(Side::Back,   -0.02*hl,  0.14*hl, 0.67, 2.50, paint.m_trim, frosted, false, frosted);
        b.door(Side::Back,   -0.19*hl, -0.03*hl, 0.67, 2.50, paint.m_door, paint.m_door, false, frosted);
        b.window(Side::Back, -0.48*hl, -0.38*hl, 1.25, 2.10, 1, 1, paint.m_trim, paint.m_glass);
        b.panel(Side::Back,  -0.62*hl, -0.50*hl, 1.25, 2.10, paint.m_shutter, 0.06);
        // The glazed corner of the living room, at the left for someone facing the entrance
        b.window(Side::Right, -0.85*hw, 0.10*hw, 0.67, 2.45, 3, 1, paint.m_shutter, paint.m_glass);
        b.window(Side::Front,  0.35*hl, 0.92*hl, 0.67, 2.45, 3, 1, paint.m_shutter, paint.m_glass);
        b.window(Side::Front, -0.60*hl, -0.30*hl, 1.05, 2.05, 2, 1, paint.m_trim, paint.m_glass);
        b.shutters(Side::Front, -0.60*hl, -0.30*hl, 1.05, 2.05, 0.40);
        b.window(Side::Left,  -0.25*hw, 0.25*hw, 1.05, 2.05, 2, 1, paint.m_trim, paint.m_glass);
        // Roof windows and the flue, each above a few red tiles
        b.skylight(-0.30*hl, -0.14*hl, 0.62*hw, 0.42*hw, -1.0, thatchEdge, Color(0.60f, 0.70f, 0.76f));
        b.skylight( 0.20*hl,  0.34*hl, 0.62*hw, 0.42*hw,  1.0, thatchEdge, Color(0.60f, 0.70f, 0.76f));
        b.pipe(0.30*hl, -0.55*hw, 0.8, 0.10);
        b.terrace(Side::Front, 0.20*hl, 1.00*hl, 3.0, terraceGrey);
    }

    // Class Luxe6L
    class Luxe6L : public GableModel
    {
    public:
        Luxe6L()
            :
            GableModel(shape6L, paint6L)
        {
        }

        // Accessor
        const char* name() const override { return "Luxe6L"; }
        const char* label() const override { return "6L"; }
        std::pair<double, double> footprint(double outlineLength, double outlineWidth) const override
        {
            return wallsUnderRoof(outlineLength, outlineWidth);
        }

    protected:
        void draw(Builder& builder, const Paint& paint) const override { buildLuxe6L(builder, paint); }
    };
}

const Model& Models::luxe6L()
{
    static const Luxe6L model;
    return model;
}
