// Slufter3D - model Luxe6EL: the 8L with a bay window across its front gable and a brick chimney in its middle
// Author: Jan de Kort
// Created: 2026-11-17
// Luxe6EL (photos B427 and Screenshot 2026-11-17 19.21.41), copied from Luxe8L; drawn for the 6EL and the 8EL
// Measured on B427, taken nearly square to the front gable, with the walls 7.5 m wide as the scale. The roof is
// half-hipped: the verges rise at 54 degrees from the eaves, 2.0 m above the floor, to a level board at 4.8 m and
// 4.5 m long, above which the thatch hips back to the ridge; red tiles roof the slopes up to 3.5 m. Both gables are
// clad in dark boards down to the plinth, within white boarding. Across the front gable stands a flat-roofed bay
// 0.8 m deep, glazed on three sides up to 2.0 m with a transom at 1.5 m, under a white fascia up to 2.3 m. In its
// middle a brick chimney 1.1 m wide, flush with the bay, rises through the hip to about 1 m below the ridge, with a
// steel flue on it. Above the bay a tall window stands either side of the chimney, up to the level board; a brick
// pier closes the front gable at the back corner. The long sides, the back gable and the dormer are those of the 8L.
#include "ModelKit.h"
#include "Models.h"

#include "Mesh.h"

#include <cmath>

using namespace ModelKit;

namespace
{
    const double el6Bay = 0.80;          // depth of the bay in front of the gable [m]
    const double el6BayMargin = 0.42;    // between the bay and either corner of the gable [m]
    const double el6Sill = 0.10;         // bottom of the glazing of the bay [m]
    const double el6Transom = 1.50;      // [m]
    const double el6Glass = 1.98;        // top of the glazing of the bay, under the fascia [m]
    const double el6BayTop = 2.27;       // top of the fascia round the bay [m]
    const double el6Chimney = 0.55;      // half the width of the chimney [m]
    const double el6ChimneyBack = 0.50;  // the chimney reaches this far behind the gable, into the hip [m]
    const double el6BelowRidge = 0.95;   // top of the chimney under the ridge [m]
    const double el6Pier = 0.30;         // brick pier at the back corner of the front gable [m]
    const double el6Inset = 0.16;        // white boarding seen round the dark cladding of the gables [m]
    const double el6UpperSill = 2.42;    // the windows of the upper floor start just above the bay [m]
    const double el6Foundation = 2.5;    // walls reach this far into the dune, as those of the kit [m]

    const Shape shape6EL = { 2.70, 0.60, degrees(54.0), 0.50, 0.35, 0.17, 0.60, 0.40 };
    const Paint paint6EL = {
        Color(0.60f, 0.29f, 0.21f),   // red brick
        Color(0.90f, 0.88f, 0.81f),   // white plinth
        Color(0.66f, 0.21f, 0.15f),   // red tiles at the foot of the roof
        Color(0.37f, 0.32f, 0.27f),   // thatch
        Color(0.19f, 0.21f, 0.25f),   // dark blue-grey cladding
        Color(0.94f, 0.93f, 0.88f),   // white boards and frames
        Color(0.36f, 0.44f, 0.50f),
        Color(0.30f, 0.36f, 0.40f),
        Color(0.20f, 0.20f, 0.21f),   // dark shutters
        Color(0.10f, 0.10f, 0.11f) };
    const Color capGrey(0.80f, 0.80f, 0.78f);
    const Color flueSteel(0.55f, 0.56f, 0.58f);
    const Color bayRoof(0.30f, 0.30f, 0.31f);
    const Color hatch(0.12f, 0.12f, 0.13f);

    // Level of the board under the hip, where the gable ends
    double gableTop(const Builder& b)
    {
        return shape6EL.m_wallHeight + (1.0 - shape6EL.m_hip) * (b.ridge() - shape6EL.m_wallHeight);
    }

    // Dark boards from the plinth up to the hip, within white boarding along the verges and under the hip;
    // pier->width of a brick pier at the back corner, 0->none
    void cladGable(Builder& b, Side side, const Paint& paint, double pier)
    {
        const double hw = b.halfWidth();
        const double wall = shape6EL.m_wallHeight;
        const double top = gableTop(b) - el6Inset;
        const double halfTop = shape6EL.m_hip * hw - el6Inset / std::cos(shape6EL.m_pitch);
        const double out = 0.04;
        b.gable(side, paint.m_trim, 0.02, 0.0);
        b.panel(side, -hw + pier, hw, shape6EL.m_plinthHeight, wall, paint.m_gable, out);
        b.mesh().addQuad(b.onWall(side, -hw, wall, out), b.onWall(side, hw, wall, out),
            b.onWall(side, halfTop, top, out), b.onWall(side, -halfTop, top, out), paint.m_gable, b.outward(side));
        if (pier > 0.0)
        {
            b.panel(side, -hw, -hw + pier, shape6EL.m_plinthHeight, wall, paint.m_wall, out + 0.01);
        }
    }

    // The bay across the front gable, glazed on three sides under a white fascia, either side of the chimney
    void bay(Builder& b, const Paint& paint)
    {
        const double hl = b.halfLength();
        const double half = b.halfWidth() - el6BayMargin;
        const double fascia = 0.05;     // the fascia stands this far proud of the glazing [m]
        const Vec3 up(0.0, 1.0, 0.0);
        for (const double s : { 1.0, -1.0 })
        {
            const double a0 = s > 0.0 ? el6Chimney : -half;
            const double a1 = s > 0.0 ? half : -el6Chimney;
            const Side side = s > 0.0 ? Side::Front : Side::Back;
            // Front: tall panes under a row of low ones, on a white base
            b.panel(Side::Right, a0, a1, -el6Foundation, el6Sill, paint.m_plinth, el6Bay);
            b.window(Side::Right, a0, a1, el6Sill, el6Transom, 2, 1, paint.m_trim, paint.m_glass, el6Bay);
            b.window(Side::Right, a0, a1, el6Transom - 0.09, el6Glass, 2, 1, paint.m_trim, paint.m_glass, el6Bay);
            // Side, from the wall to the front
            b.panel(side, hl, hl + el6Bay, -el6Foundation, el6Sill, paint.m_plinth, half - b.halfWidth());
            b.window(side, hl, hl + el6Bay + 0.03, el6Sill, el6Transom, 1, 1, paint.m_trim, paint.m_glass,
                half - b.halfWidth());
            b.window(side, hl, hl + el6Bay + 0.03, el6Transom - 0.09, el6Glass, 1, 1, paint.m_trim, paint.m_glass,
                half - b.halfWidth());
            // Fascia and flat roof, from the side of the chimney
            const double inner = el6Chimney - 0.01;
            const double outer = half + fascia;
            const double depth = el6Bay + 0.03 + fascia;
            b.mesh().addBox(b.point(hl + 0.5 * depth, s * 0.5 * (inner + outer), 0.5 * (el6Glass + el6BayTop)),
                b.outward(Side::Right), up, b.outward(Side::Front),
                Vec3(0.5 * depth, 0.5 * (el6BayTop - el6Glass), 0.5 * (outer - inner)), paint.m_trim);
            const double rim = 0.06;
            b.mesh().addQuad(b.point(hl, s * inner, el6BayTop + 0.005), b.point(hl + depth - rim, s * inner, el6BayTop + 0.005),
                b.point(hl + depth - rim, s * (outer - rim), el6BayTop + 0.005), b.point(hl, s * (outer - rim), el6BayTop + 0.005),
                bayRoof, up);
        }
    }

    // The chimney in the middle of the bay, flush with its glazing, up through the hip; a hatch low in its face,
    // a concrete cap, a steel flue with a hood and a thin pipe beside it
    void chimney(Builder& b, const Paint& paint)
    {
        const double hl = b.halfLength();
        const double front = el6Bay + 0.06;
        const double top = b.ridge() - el6BelowRidge;
        const Vec3 up(0.0, 1.0, 0.0);
        const auto block = [&](double back, double face, double halfWidth, double y0, double y1, const Color& color)
            {
                b.mesh().addBox(b.point(hl + 0.5 * (back + face), 0.0, 0.5 * (y0 + y1)), b.outward(Side::Right), up,
                    b.outward(Side::Front), Vec3(0.5 * (face - back), 0.5 * (y1 - y0), halfWidth), color);
            };
        block(-el6ChimneyBack, front, el6Chimney, -el6Foundation, top, paint.m_wall);
        block(-el6ChimneyBack - 0.05, front + 0.05, el6Chimney + 0.05, top, top + 0.10, capGrey);
        b.panel(Side::Right, -0.23, 0.23, 0.30, 1.08, capGrey, front + 0.01);
        b.panel(Side::Right, -0.17, 0.17, 0.36, 1.02, hatch, front + 0.02);
        const Vec3 flue = b.point(hl + 0.10, -0.15, top + 0.10);
        const double flueHeight = 0.75;
        addDrum(b.mesh(), flue, 0.10, flueHeight, flueSteel);
        addDrum(b.mesh(), flue + up * flueHeight, 0.18, 0.06, flueSteel.scaled(0.75f));
        b.mesh().addBox(b.point(hl - 0.10, 0.30, top + 0.25), b.outward(Side::Right), up, b.outward(Side::Front),
            Vec3(0.05, 0.15, 0.05), flueSteel);
    }

    void buildLuxe6EL(Builder& b, const Paint& paint)
    {
        const double hl = b.halfLength();
        const double hw = b.halfWidth();
        const double floor2 = shape6EL.m_wallHeight + 0.55;
        const double head = gableTop(b) - el6Inset - 0.10;   // windows of the upper floor reach up to the board
        b.walls();
        cladGable(b, Side::Right, paint, el6Pier);
        cladGable(b, Side::Left, paint, 0.0);
        b.roof();
        b.ridgeCap(Color(0.86f, 0.47f, 0.27f));
        // Front gable as in photo B427: the bay with the chimney, and a tall window either side of the chimney
        bay(b, paint);
        chimney(b, paint);
        const double inner = el6Chimney + 0.07;
        const double outer = 0.49 * hw;
        b.window(Side::Right, inner, outer, el6UpperSill, head, 1, 2, paint.m_trim, paint.m_glass, 0.02);
        b.window(Side::Right, -outer, -inner, el6UpperSill, head, 1, 2, paint.m_trim, paint.m_glass, 0.02);
        // Back gable, not photographed: the windows of the 8L, the upper ones under the board
        b.window(Side::Left, -0.26*hw, -0.04*hw, el6UpperSill, head, 1, 2, paint.m_trim, paint.m_glass, 0.02);
        b.window(Side::Left,  0.04*hw,  0.26*hw, el6UpperSill, head, 1, 2, paint.m_trim, paint.m_glass, 0.02);
        b.shutters(Side::Left, -0.26*hw, 0.26*hw, el6UpperSill + 0.05, head - 0.05, 0.50);
        b.window(Side::Left, -0.30*hw, 0.30*hw, 1.00, 2.20, 2, 1, paint.m_trim, paint.m_glass, 0.02);
        b.shutters(Side::Left, -0.30*hw, 0.30*hw, 1.00, 2.20, 0.50);
        // Long sides as the 8L: glazed door, shuttered windows, a dormer in the thatch
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
        b.pipe(0.30*hl, 0.78*hw, 0.5, 0.08);
    }

    // Class Luxe6EL
    class Luxe6EL : public GableModel
    {
    public:
        Luxe6EL()
            :
            GableModel(shape6EL, paint6EL)
        {
        }

        // Accessor
        const char* name() const override { return "Luxe6EL"; }
        const char* label() const override { return "6EL"; }
        std::pair<double, double> footprint(double outlineLength, double outlineWidth) const override
        {
            return wallsUnderRoof(outlineLength, outlineWidth);
        }
        // The walls and the bay in front of the right-hand gable
        bool covers(double length, double width, double du, double dv, double margin) const override
        {
            const bool inBay = du > 0.0 && du < 0.5 * length + el6Bay + margin
                && std::fabs(dv) < 0.5 * width - el6BayMargin + margin;
            return Model::covers(length, width, du, dv, margin) || inBay;
        }

    protected:
        void draw(Builder& builder, const Paint& paint) const override { buildLuxe6EL(builder, paint); }
    };
}

const Model& Models::luxe6EL()
{
    static const Luxe6EL model;
    return model;
}
