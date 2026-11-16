// Slufter3D - model ComfortSemi4C: half of a low semi-detached block
// Author: Jan de Kort
// Created: 2026-11-16
// ComfortSemi4C (photo B14_15a)
#include "ModelKit.h"
#include "Models.h"

#include "Check.h"
#include "Mesh.h"

#include <algorithm>
#include <cmath>

using namespace ModelKit;

namespace
{
    const Shape shapeSemi4C = { 2.35, 0.0, degrees(21.0), 0.40, 0.0, 0.28, 0.0, 1.0 };
    const Paint paintSemi4C = {
        Color(0.60f, 0.56f, 0.53f),   // grey-brown brick
        Color(0.54f, 0.50f, 0.47f),
        Color(0.37f, 0.33f, 0.30f),   // weathered brown tiles
        Color(0.37f, 0.33f, 0.30f),
        Color(0.28f, 0.42f, 0.44f),   // blue-green shingles
        Color(0.72f, 0.87f, 0.78f),   // mint boards
        Color(0.19f, 0.23f, 0.27f),
        Color(0.13f, 0.22f, 0.18f),   // dark green door
        Color(0.28f, 0.42f, 0.44f),
        Color(0.30f, 0.31f, 0.34f) };

    void buildComfortSemi4C(Builder& b, const Paint& paint)
    {
        const double hl = b.halfLength();
        const Color white(0.90f, 0.90f, 0.86f);
        b.walls();
        b.gable(Side::Left, paint.m_gable, 0.02, 0.0);
        b.gable(Side::Right, paint.m_gable, 0.02, 0.0);
        b.roof();
        // Front: window, door in a white frame, two small high windows
        b.window(Side::Front, -0.80*hl, -0.45*hl, 0.95, 2.05, 1, 1, white, paint.m_glass);
        b.window(Side::Front, -0.22*hl,  0.00*hl, 0.02, 2.08, 1, 1, white, paint.m_door);
        b.window(Side::Front,  0.12*hl,  0.50*hl, 1.45, 2.05, 2, 1, white, paint.m_glass);
        b.window(Side::Front,  0.62*hl,  0.88*hl, 0.95, 2.05, 1, 1, white, paint.m_glass);
        b.window(Side::Back,  -0.75*hl,  0.10*hl, 0.20, 2.10, 3, 1, white, paint.m_glass);
        b.window(Side::Back,   0.35*hl,  0.75*hl, 0.95, 2.05, 2, 1, white, paint.m_glass);
        b.pipe(-0.40*hl, -0.6, 0.45, 0.07);
        b.pipe( 0.35*hl, -0.6, 0.45, 0.07);
        b.pipe( 0.05*hl,  1.2, 0.25, 0.10);
    }

    // Class ComfortSemi4C
    class ComfortSemi4C : public GableModel
    {
    public:
        ComfortSemi4C()
            :
            GableModel(shapeSemi4C, paintSemi4C)
        {
        }

        // Accessor
        const char* name() const override { return "ComfortSemi4C"; }
        const char* label() const override { return "4C semi"; }
        std::pair<double, double> footprint(double outlineLength, double outlineWidth) const override
        {
            (void) outlineLength;
            (void) outlineWidth;
            return comfortFootprint();
        }

    protected:
        void draw(Builder& builder, const Paint& paint) const override { buildComfortSemi4C(builder, paint); }
    };
}

const Model& Models::comfortSemi4C()
{
    static const ComfortSemi4C model;
    return model;
}
