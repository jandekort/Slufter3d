// Slufter3D - model Comfort8C: bungalow with a loft, navy gable, white conservatory front
// Author: Jan de Kort
// Created: 2026-11-16
// Comfort8C (photo B3)
#include "ModelKit.h"
#include "Models.h"

#include "Check.h"
#include "Mesh.h"

#include <algorithm>
#include <cmath>

using namespace ModelKit;

namespace
{
    const Shape shape8C = { 2.50, 0.0, degrees(46.0), 0.45, 0.20, 0.34, 0.0, 1.0 };
    const Paint paint8C = {
        Color(0.72f, 0.70f, 0.65f),   // light sand-lime brick
        Color(0.64f, 0.62f, 0.58f),
        Color(0.42f, 0.41f, 0.41f),   // grey tiles with lichen
        Color(0.42f, 0.41f, 0.41f),
        Color(0.19f, 0.27f, 0.39f),   // navy shingles
        Color(0.17f, 0.25f, 0.37f),   // navy fascia
        Color(0.33f, 0.39f, 0.44f),
        Color(0.92f, 0.92f, 0.89f),   // white door
        Color(0.19f, 0.27f, 0.39f),
        Color(0.22f, 0.22f, 0.23f) };

    void buildComfort8C(Builder& b, const Paint& paint)
    {
        const double hl = b.halfLength();
        const double hw = b.halfWidth();
        const Color white(0.93f, 0.93f, 0.89f);
        const double loft = shape8C.m_wallHeight + 0.75;
        b.walls();
        b.gable(Side::Left, paint.m_gable, 0.02, 0.0);
        b.gable(Side::Right, paint.m_gable, 0.02, 0.0);
        b.roof();
        // Front: door, then the white conservatory glazing along the living room
        b.window(Side::Front, -0.62*hl, -0.42*hl, 0.02, 2.20, 1, 1, white, paint.m_glass);
        b.window(Side::Front, -0.40*hl,  0.96*hl, 0.45, 2.30, 4, 1, white, paint.m_glass);
        b.panel(Side::Front,  -0.40*hl,  0.96*hl, 0.02, 0.45, white, 0.03);
        b.window(Side::Front, -0.90*hl, -0.70*hl, 1.00, 2.05, 1, 1, white, paint.m_glass);
        // Gables: a loft window in the shingles, the white shed against the right-hand one
        b.window(Side::Right, -0.28*hw, 0.28*hw, loft, loft + 1.45, 2, 1, white, paint.m_glass);
        b.window(Side::Left,  -0.28*hw, 0.28*hw, loft, loft + 1.45, 2, 1, white, paint.m_glass);
        b.window(Side::Right,  0.35*hw, 0.90*hw, 0.30, 2.25, 1, 1, white, paint.m_glass);
        b.annex(Side::Right, -0.75*hw, 0.20*hw, 1.5, 2.35, white);
        b.window(Side::Left,  -0.40*hw, 0.20*hw, 0.95, 2.05, 2, 1, white, paint.m_glass);
        b.window(Side::Back,  -0.55*hl, -0.20*hl, 0.95, 2.05, 2, 1, white, paint.m_glass);
        b.window(Side::Back,   0.20*hl,  0.55*hl, 0.95, 2.05, 2, 1, white, paint.m_glass);
        b.terrace(Side::Front, -0.70*hl, 1.00*hl, 3.0, terraceGrey);
        b.pipe(0.30*hl, 0.45*hw, 1.9, 0.11);
    }

    // Class Comfort8C
    class Comfort8C : public GableModel
    {
    public:
        Comfort8C()
            :
            GableModel(shape8C, paint8C)
        {
        }

        // Accessor
        const char* name() const override { return "Comfort8C"; }
        const char* label() const override { return "8C"; }
        std::pair<double, double> footprint(double outlineLength, double outlineWidth) const override
        {
            (void) outlineLength;
            (void) outlineWidth;
            return comfortFootprint();
        }

    protected:
        void draw(Builder& builder, const Paint& paint) const override { buildComfort8C(builder, paint); }
    };
}

const Model& Models::comfort8C()
{
    static const Comfort8C model;
    return model;
}
