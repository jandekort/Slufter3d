// Slufter3D - model Comfort6C: low detached bungalow, grey roof, navy gutters
// Author: Jan de Kort
// Created: 2026-11-16
// Comfort6C (photos Type_6C, Type_6C_front, B17a)
// Seen from the terrace in photo Type_6C: wall 2.45 m to the gutter, the darker courses the lowest 0.55 m of it
#include "ModelKit.h"
#include "Models.h"

#include "Check.h"
#include "Mesh.h"

#include <algorithm>
#include <cmath>

using namespace ModelKit;

namespace
{
    const Shape shape6C = { 2.45, 0.55, degrees(24.0), 0.35, 0.25, 0.28, 0.0, 1.0 };
    const Paint paint6C = {
        Color(0.66f, 0.63f, 0.59f),   // light grey-beige brick
        Color(0.55f, 0.53f, 0.50f),   // darker courses at the foot of the wall
        Color(0.45f, 0.43f, 0.40f),   // grey-brown concrete tiles with lichen
        Color(0.45f, 0.43f, 0.40f),
        Color(0.25f, 0.30f, 0.38f),   // blue-grey shingles
        Color(0.17f, 0.25f, 0.37f),   // navy gutter boards and verges, as on the 8C
        Color(0.33f, 0.39f, 0.44f),
        Color(0.17f, 0.25f, 0.37f),   // navy door
        Color(0.25f, 0.30f, 0.38f),
        Color(0.16f, 0.16f, 0.17f) };

    void buildComfort6C(Builder& b, const Paint& paint)
    {
        const double hl = b.halfLength();
        const double hw = b.halfWidth();
        const Color white(0.93f, 0.93f, 0.90f);
        const Color navy = paint.m_door;
        const Color frosted(0.76f, 0.81f, 0.83f);
        const double sill = 0.50;    // top of the white boards under the living-room windows [m]
        const double head = 2.32;    // the frames end just under the gutter board [m]
        b.walls();
        b.gable(Side::Left, paint.m_gable, 0.02, 0.0);
        b.gable(Side::Right, paint.m_gable, 0.02, 0.0);
        b.roof();
        // Terrace side as in photo Type_6C, from left to right for someone facing it: three living-room windows
        // in white frames over white boards, the navy door with its pane and a light over it, brick, a window of four
        b.window(Side::Front, -0.98*hl, -0.25*hl, sill, head, 3, 1, white, paint.m_glass);
        b.panel(Side::Front,  -0.98*hl, -0.25*hl, 0.02, sill, white, 0.03);
        b.panel(Side::Front,  -0.245*hl, -0.045*hl, 0.02, 2.06, navy, 0.03);
        b.panel(Side::Front,  -0.245*hl + 0.17, -0.045*hl - 0.17, 0.30, 1.90, paint.m_glass, 0.06);
        b.window(Side::Front, -0.245*hl, -0.045*hl, 2.06, head, 1, 1, white, paint.m_glass);
        b.window(Side::Front,  0.25*hl,  0.72*hl, 1.05, 2.28, 4, 1, white, paint.m_glass);
        b.panel(Side::Front,   0.25*hl - 0.05, 0.72*hl + 0.05, 0.99, 1.05, white, 0.08);
        // The downpipe at the right-hand corner, into its drum
        b.downpipe(Side::Front, hl - 0.10, white);
        // Right-hand gable: a narrow window towards the back, the meter box
        b.window(Side::Right, -0.50*hw, -0.26*hw, 1.00, 2.05, 1, 1, white, paint.m_glass);
        b.panel(Side::Right,   0.05*hw,  0.05*hw + 0.30, 1.45, 1.80, Color(0.80f, 0.80f, 0.78f), 0.05);
        // Entrance side as in photo Type_6C_front, from left to right for someone facing it: bedroom window,
        // front door with a frosted pane, two small top-hung windows, the solid door of the store, a narrow window
        b.window(Side::Back,  0.50*hl,  0.86*hl, 0.95, 2.10, 2, 1, white, paint.m_glass);
        b.door(Side::Back,    0.10*hl,  0.31*hl, 0.02, 2.10, white, navy, true, frosted);
        b.window(Side::Back,  0.10*hl,  0.31*hl, 2.10, 2.38, 1, 1, white, paint.m_glass);
        b.window(Side::Back, -0.30*hl,  0.06*hl, 1.55, 2.38, 2, 1, white, paint.m_glass);
        b.door(Side::Back,   -0.56*hl, -0.35*hl, 0.02, 2.10, white, navy, false, frosted);
        b.window(Side::Back, -0.56*hl, -0.35*hl, 2.10, 2.38, 1, 1, white, paint.m_glass);
        b.window(Side::Back, -0.88*hl, -0.76*hl, 1.00, 2.10, 1, 1, white, paint.m_glass);
        b.terrace(Side::Back, -0.62*hl, 0.38*hl, 1.3, terraceGrey);
        b.terrace(Side::Front, -1.25*hl, 0.00*hl, 2.6, Color(0.62f, 0.60f, 0.56f));
        b.pipe(-0.45*hl, -0.15*hw, 0.7, 0.12);
    }

    // Class Comfort6C
    class Comfort6C : public GableModel
    {
    public:
        Comfort6C()
            :
            GableModel(shape6C, paint6C)
        {
        }

        // Accessor
        const char* name() const override { return "Comfort6C"; }
        const char* label() const override { return "6C"; }
        std::pair<double, double> footprint(double outlineLength, double outlineWidth) const override
        {
            (void) outlineLength;
            (void) outlineWidth;
            return comfortFootprint();
        }

    protected:
        void draw(Builder& builder, const Paint& paint) const override { buildComfort6C(builder, paint); }
    };
}

const Model& Models::comfort6C()
{
    static const Comfort6C model;
    return model;
}
