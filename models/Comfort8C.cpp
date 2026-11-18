// Slufter3D - model Comfort8C: detached bungalow with a loft under a steep roof that runs on down over an outshot
// beside the entrance, light brick with darker courses at its foot, green slate gables, a white extension
// Author: Jan de Kort
// Created: 2026-11-18, in place of the first Comfort8C, drawn anew
// Comfort8C (photos Screenshot 2026-11-18 21.58.40, 21.56.33, 21.56.02 and 21.58.17)
// Every 8C in the BGT has the same outline, never mirrored: walls of 8.82 by 6.42 m and, at one end of the entrance
// side, an outshot of 2.80 by 1.13 m. The AHN, as the median over all of them, has both slopes at 45 degrees up to a
// ridge 6.0 m above the ground, and the entrance slope running on in one plane over the outshot, down to about
// 1.5 m. With the soffit at 2.20 m as on the other Comfort bungalows, boards of 0.28 m along the eaves and an
// overhang of 0.35 m, that puts the ridge at 6.04 m: a loft to stand in, with a window in each slate gable.
// In the frame of the model the terrace side is the front, +v, the entrance side the back, -v. The gable with the
// white extension, added later to every house of this type, with its kitchen window and the exhaust of the
// kitchen, is at +u; the gable with the second door at -u, and beside it the outshot under the long slope.
// Terrace side and extension as in 21.58.40 and 21.56.33, the entrance side as in 21.56.02, the gable beside the
// outshot as in 21.58.17. Places along a wall are in metres from its end, worked out from the perspective of the
// photographs and the measures of the BGT. Colours as on Comfort4C, the roof in its dark version.
#include "ComfortKit.h"
#include "ModelKit.h"
#include "Models.h"

#include "Mesh.h"

#include <cmath>
#include <utility>

using namespace ModelKit;
using ComfortKit::Opening;

namespace
{
    const double c8Length = 8.82;       // the walls, as the BGT has them [m]
    const double c8Width = 6.42;
    const double c8OutLength = 2.80;    // the outshot, along the entrance side from the gable at -u (BGT) [m]
    const double c8OutDepth = 1.13;     // and out from that side (BGT) [m]
    const double c8Soffit = 2.20;       // underside of the eaves; frames and doors reach up to it [m]
    const double c8Fascia = 0.28;       // the boards along the eaves [m]
    const double c8Overhang = 0.35;     // of eaves and verges [m]
    const double c8LowOverhang = 0.25;  // of the low eaves of the outshot [m]
    const double c8Pitch = 45.0;        // [deg] (AHN)
    const double c8Plinth = 0.50;       // top of the darker courses [m]
    const double c8Foundation = 2.5;    // walls reach this far into the dune, as those of the kit [m]
    const double c8Reveal = 0.07;       // frames and doors stand this far back in the brick [m]
    const double c8Bar = 0.07;          // width of the white frames [m]
    const double c8DoorTop = 2.00;      // top of the door leaves, under a light up to the soffit [m]
    const double c8Kick = 0.50;         // the living room is glazed down to this height, white panels below [m]
    const double c8Sill = 1.00;         // bottom of the frames of the windows over a sill [m]
    const double c8Rail = 1.55;         // top of the brick beside the front door, under three small windows [m]
    const double c8Corner = 0.07;       // the white post at the glazed corner of the living room, either way [m]
    const double c8Board = 0.22;        // height of the boards along the verges [m]
    // The white extension against the gable at +u, in metres from the terrace side
    const double c8ExtFrom = 2.57;
    const double c8ExtTo = 5.65;
    const double c8ExtDepth = 0.90;
    const double c8ExtTop = 2.08;       // just under the slate
    // The loft windows, in the middle of each gable, their upper corners close under the verges
    const double c8LoftWidth = 1.25;
    const double c8LoftSill = 3.64;
    const double c8LoftHead = 5.07;

    const Shape shape8C = { c8Soffit + c8Fascia + c8Overhang * std::tan(degrees(c8Pitch)), c8Plinth, degrees(c8Pitch),
        c8Overhang, c8Overhang, c8Fascia, 0.0, 1.0 };
    const Paint paint8C = {
        Color(0.70f, 0.68f, 0.62f),   // light beige-grey brick
        Color(0.50f, 0.48f, 0.43f),   // the darker courses at the foot of the wall
        Color(0.30f, 0.28f, 0.28f),   // dark tiles
        Color(0.30f, 0.28f, 0.28f),
        Color(0.24f, 0.35f, 0.37f),   // green slate of the gables
        Color(0.15f, 0.25f, 0.31f),   // dark petrol boards along eaves and verges
        Color(0.30f, 0.36f, 0.42f),
        Color(0.17f, 0.30f, 0.27f),   // green doors
        Color(0.29f, 0.35f, 0.41f),
        Color(0.16f, 0.16f, 0.17f) }; // flues
    const Color c8White(0.92f, 0.92f, 0.90f);       // frames, panels, downpipes
    const Color c8Underside(0.86f, 0.86f, 0.83f);   // the soffits, light boards
    const Color c8Stone(0.80f, 0.80f, 0.78f);       // sills and thresholds
    const Color c8Frosted(0.62f, 0.68f, 0.70f);     // pane of the front door
    const Color c8Paving(0.40f, 0.41f, 0.43f);
    const Color c8Boards(0.90f, 0.90f, 0.86f);      // the white boards of the extension
    const Color c8Zinc(0.62f, 0.63f, 0.64f);        // its cap, the sill of its window
    const Color c8Steel(0.74f, 0.75f, 0.76f);       // the exhaust of the kitchen
    const Color c8Vent(0.20f, 0.20f, 0.21f);
    const ComfortKit::Style style8C = { c8Soffit, c8Fascia, c8Overhang, c8Plinth, c8Foundation, c8Reveal, c8Bar, c8DoorTop,
        c8White, c8Underside, c8Stone };

    // Struct Roof: the measures of the roof on a builder
    struct Roof
    {
        explicit Roof(const Builder& b)
            :
            m_u0(-b.halfLength() - c8Overhang)
            , m_u1(b.halfLength() + c8Overhang)
            , m_uOut(-b.halfLength() + c8OutLength)
            , m_eaveV(b.halfWidth() + c8Overhang)
            , m_eaveY(c8Soffit + c8Fascia)
            , m_outV(b.halfWidth() + c8OutDepth)
            , m_lowV(b.halfWidth() + c8OutDepth + c8LowOverhang)
            , m_lowY(b.roofHeight(b.halfWidth() + c8OutDepth + c8LowOverhang))
            , m_lowSoffit(b.roofHeight(b.halfWidth() + c8OutDepth + c8LowOverhang) - c8Fascia)
        {
        }

        double m_u0;        // over the verge at -u
        double m_u1;        // and at +u
        double m_uOut;      // the outshot reaches from the gable at -u to here
        double m_eaveV;     // the eaves, from the ridge
        double m_eaveY;     // their upper edge
        double m_outV;      // the wall of the outshot, from the ridge
        double m_lowV;      // the low eaves over it
        double m_lowY;      // their upper edge
        double m_lowSoffit; // and their underside; the walls of the outshot reach up to it
    };

    // The roof: the terrace slope, the entrance slope that runs on down over the outshot, the boards along eaves
    // and verges, the soffits
    void roof(Builder& b, const Paint& paint)
    {
        Mesh& mesh = b.mesh();
        const Roof r(b);
        const Vec3 up(0.0, 1.0, 0.0);
        const Vec3 alongU = normalize(b.point(1.0, 0.0, 0.0) - b.point(0.0, 0.0, 0.0));
        const Vec3 front = b.outward(Side::Front);
        const double hw = b.halfWidth();
        const double ridge = b.ridge();
        const Vec3 down(0.0, -c8Board, 0.0);
        const double lip = 0.02;    // the boards stand this far above the tiles
        { // Terrace slope
            mesh.addQuad(b.point(r.m_u0, r.m_eaveV, r.m_eaveY), b.point(r.m_u1, r.m_eaveV, r.m_eaveY), b.point(r.m_u1, 0.0, ridge),
                b.point(r.m_u0, 0.0, ridge), paint.m_roof, front + up);
            mesh.addQuad(b.point(r.m_u0, r.m_eaveV, c8Soffit), b.point(r.m_u1, r.m_eaveV, c8Soffit),
                b.point(r.m_u1, r.m_eaveV, r.m_eaveY + lip), b.point(r.m_u0, r.m_eaveV, r.m_eaveY + lip), paint.m_trim, front);
            mesh.addQuad(b.point(r.m_u0, hw, c8Soffit), b.point(r.m_u1, hw, c8Soffit), b.point(r.m_u1, r.m_eaveV, c8Soffit),
                b.point(r.m_u0, r.m_eaveV, c8Soffit), c8Underside, -up);
        }
        { // Entrance slope: down to the eaves, in two parts that meet where the outshot ends, and on over the outshot
            for (const auto& [from, to] : { std::pair<double, double>(r.m_u0, r.m_uOut), std::pair<double, double>(r.m_uOut, r.m_u1) })
            {
                mesh.addQuad(b.point(from, -r.m_eaveV, r.m_eaveY), b.point(to, -r.m_eaveV, r.m_eaveY), b.point(to, 0.0, ridge),
                    b.point(from, 0.0, ridge), paint.m_roof, up - front);
            }
            mesh.addQuad(b.point(r.m_u0, -r.m_lowV, r.m_lowY), b.point(r.m_uOut, -r.m_lowV, r.m_lowY),
                b.point(r.m_uOut, -r.m_eaveV, r.m_eaveY), b.point(r.m_u0, -r.m_eaveV, r.m_eaveY), paint.m_roof, up - front);
            // Boards and soffit of the eaves beside the outshot
            mesh.addQuad(b.point(r.m_uOut, -r.m_eaveV, c8Soffit), b.point(r.m_u1, -r.m_eaveV, c8Soffit),
                b.point(r.m_u1, -r.m_eaveV, r.m_eaveY + lip), b.point(r.m_uOut, -r.m_eaveV, r.m_eaveY + lip), paint.m_trim, -front);
            mesh.addQuad(b.point(r.m_uOut, -hw, c8Soffit), b.point(r.m_u1, -hw, c8Soffit), b.point(r.m_u1, -r.m_eaveV, c8Soffit),
                b.point(r.m_uOut, -r.m_eaveV, c8Soffit), c8Underside, -up);
            // And of the low eaves of the outshot
            mesh.addQuad(b.point(r.m_u0, -r.m_lowV, r.m_lowSoffit), b.point(r.m_uOut, -r.m_lowV, r.m_lowSoffit),
                b.point(r.m_uOut, -r.m_lowV, r.m_lowY + lip), b.point(r.m_u0, -r.m_lowV, r.m_lowY + lip), paint.m_trim, -front);
            mesh.addQuad(b.point(r.m_u0, -r.m_outV, r.m_lowSoffit), b.point(r.m_uOut, -r.m_outV, r.m_lowSoffit),
                b.point(r.m_uOut, -r.m_lowV, r.m_lowSoffit), b.point(r.m_u0, -r.m_lowV, r.m_lowSoffit), c8Underside, -up);
            // The board along the edge of the long slope, from the eaves down to the low eaves
            const Vec3 high = b.point(r.m_uOut + 0.01, -r.m_eaveV, r.m_eaveY + lip);
            const Vec3 low = b.point(r.m_uOut + 0.01, -r.m_lowV, r.m_lowY + lip);
            mesh.addQuad(low + down, high + down, high, low, paint.m_trim, alongU);
        }
        // Verge boards: both slopes over the gable at +u; the terrace slope and the long slope over that at -u
        const Vec3 topA = b.point(r.m_u1, 0.0, ridge);
        const Vec3 topB = b.point(r.m_u0, 0.0, ridge);
        for (const double s : { 1.0, -1.0 })
        {
            const Vec3 low = b.point(r.m_u1, s * r.m_eaveV, r.m_eaveY);
            mesh.addQuad(low + down, topA + down, topA, low, paint.m_trim, alongU);
        }
        const Vec3 lowFront = b.point(r.m_u0, r.m_eaveV, r.m_eaveY);
        const Vec3 lowBack = b.point(r.m_u0, -r.m_lowV, r.m_lowY);
        mesh.addQuad(lowFront + down, topB + down, topB, lowFront, paint.m_trim, -alongU);
        mesh.addQuad(lowBack + down, topB + down, topB, lowBack, paint.m_trim, -alongU);
    }

    // The outshot under the long slope: its wall along the entrance side up to the low eaves, its end beside the
    // front door up to the roof, its other end flush with the gable, and its downpipe
    void outshot(Builder& b, ComfortKit::Mason& m, const Paint& paint)
    {
        Mesh& mesh = b.mesh();
        const Roof r(b);
        const Vec3 alongU = normalize(b.point(1.0, 0.0, 0.0) - b.point(0.0, 0.0, 0.0));
        const double hl = b.halfLength();
        const double hw = b.halfWidth();
        m.brick(Side::Back, -hl, r.m_uOut, -c8Foundation, r.m_lowSoffit, c8OutDepth);
        m.brick(Side::Left, -r.m_outV, -hw, -c8Foundation, r.m_lowSoffit, 0.0);
        mesh.addQuad(b.point(r.m_uOut, -hw, -c8Foundation), b.point(r.m_uOut, -r.m_outV, -c8Foundation),
            b.point(r.m_uOut, -r.m_outV, c8Plinth), b.point(r.m_uOut, -hw, c8Plinth), paint.m_plinth, alongU);
        mesh.addQuad(b.point(r.m_uOut, -hw, c8Plinth), b.point(r.m_uOut, -r.m_outV, c8Plinth),
            b.point(r.m_uOut, -r.m_outV, b.roofHeight(r.m_outV) - 0.02), b.point(r.m_uOut, -hw, b.roofHeight(hw) - 0.02),
            paint.m_wall, alongU);
        const Vec3 foot = b.point(-hl + 0.12, -r.m_outV - 0.12, 0.0);
        const Vec3 rise(0.0, 1.0, 0.0);
        mesh.addBeam(b.point(-hl + 0.12, -r.m_lowV + 0.05, r.m_lowSoffit), foot + rise * (r.m_lowSoffit - 0.25), 0.04, c8White);
        mesh.addBeam(foot + rise * (r.m_lowSoffit - 0.25), foot + rise * (b.groundUnder(foot) - 0.2), 0.04, c8White);
    }

    // The slate of the gable at -u: from the soffit up under the roof and, beside it, down the long slope to the
    // low eaves over the end of the outshot; flush with the boards of the verges, with the soffits under it
    void slateBesideOutshot(Builder& b, const Paint& paint)
    {
        Mesh& mesh = b.mesh();
        const Roof r(b);
        const Vec3 up(0.0, 1.0, 0.0);
        const Vec3 out = b.outward(Side::Left);
        const double hw = b.halfWidth();
        const double proud = c8Overhang - 0.02;
        const auto at = [&](double v, double y) { return b.onWall(Side::Left, v, y, proud); };
        const Vec3 eaveFoot = at(r.m_eaveV, c8Soffit);
        const Vec3 eaveTop = at(r.m_eaveV, r.m_eaveY);
        const Vec3 top = at(0.0, b.ridge());
        const Vec3 above = at(-hw, b.roofHeight(hw));      // under the roof, over the corner of the walls
        const Vec3 corner = at(-hw, c8Soffit);
        const Vec3 step = at(-hw, r.m_lowSoffit);
        const Vec3 lowTop = at(-r.m_lowV, r.m_lowY);
        const Vec3 lowFoot = at(-r.m_lowV, r.m_lowSoffit);
        mesh.addTriangle(eaveFoot, eaveTop, top, paint.m_gable, out);
        mesh.addTriangle(eaveFoot, top, above, paint.m_gable, out);
        mesh.addTriangle(eaveFoot, above, corner, paint.m_gable, out);
        mesh.addTriangle(lowTop, lowFoot, step, paint.m_gable, out);
        mesh.addTriangle(lowTop, step, corner, paint.m_gable, out);
        mesh.addTriangle(lowTop, corner, above, paint.m_gable, out);
        // Soffits: under the slate over the gable wall, and lower over the end of the outshot; between them the
        // side of the slate
        mesh.addQuad(b.onWall(Side::Left, -hw, c8Soffit, 0.0), b.onWall(Side::Left, hw, c8Soffit, 0.0),
            b.onWall(Side::Left, hw, c8Soffit, proud), corner, c8Underside, -up);
        mesh.addQuad(b.onWall(Side::Left, -r.m_outV, r.m_lowSoffit, 0.0), b.onWall(Side::Left, -hw, r.m_lowSoffit, 0.0), step,
            at(-r.m_outV, r.m_lowSoffit), c8Underside, -up);
        mesh.addQuad(b.onWall(Side::Left, -hw, r.m_lowSoffit, 0.0), step, corner, b.onWall(Side::Left, -hw, c8Soffit, 0.0),
            paint.m_gable.scaled(0.85f), b.outward(Side::Front));
    }

    // The window of the loft in the slate of a gable: a wide light and a narrower one in a white frame
    void loftWindow(Builder& b, Side side, const Paint& paint)
    {
        const double proud = c8Overhang - 0.02;
        const double half = 0.5 * c8LoftWidth;
        const double post = half - 0.56 * c8LoftWidth;
        b.panel(side, -half, half, c8LoftSill, c8LoftHead, c8White, proud + 0.012);
        b.panel(side, -half + c8Bar, post - 0.5 * c8Bar, c8LoftSill + c8Bar, c8LoftHead - c8Bar, paint.m_glass, proud + 0.024);
        b.panel(side, post + 0.5 * c8Bar, half - c8Bar, c8LoftSill + c8Bar, c8LoftHead - c8Bar, paint.m_glass, proud + 0.024);
        b.mesh().addBox(b.onWall(side, 0.0, c8LoftSill - 0.03, proud + 0.03), normalize(b.onWall(side, 1.0, 0.0, 0.0) - b.onWall(side, 0.0, 0.0, 0.0)),
            Vec3(0.0, 1.0, 0.0), b.outward(side), Vec3(half + 0.04, 0.03, 0.03), c8Stone);
    }

    // The white extension against the gable at +u: boards under a zinc cap, the kitchen window high in its face
    // and a small one in its side, and the round exhaust of the kitchen
    void extension(Builder& b, const Paint& paint)
    {
        Mesh& mesh = b.mesh();
        const Vec3 up(0.0, 1.0, 0.0);
        const Vec3 alongU = normalize(b.point(1.0, 0.0, 0.0) - b.point(0.0, 0.0, 0.0));
        const Vec3 alongV = b.outward(Side::Front);
        const double hl = b.halfLength();
        const double v1 = b.halfWidth() - c8ExtFrom;
        const double v0 = b.halfWidth() - c8ExtTo;
        const double middle = 0.5 * (v0 + v1);
        const double bottom = -c8Foundation;    // of its footing, in the dune
        mesh.addBox(b.point(hl + 0.5 * c8ExtDepth, middle, 0.5 * c8ExtTop), alongU, up, alongV,
            Vec3(0.5 * c8ExtDepth, 0.5 * c8ExtTop, 0.5 * (v1 - v0)), c8Boards);
        mesh.addBox(b.point(hl + 0.5 * c8ExtDepth, middle, 0.5 * bottom), alongU, up, alongV,
            Vec3(0.5 * c8ExtDepth - 0.01, -0.5 * bottom, 0.5 * (v1 - v0) - 0.01), paint.m_plinth);
        mesh.addBox(b.point(hl + 0.5 * c8ExtDepth, middle, c8ExtTop + 0.025), alongU, up, alongV,
            Vec3(0.5 * c8ExtDepth + 0.03, 0.025, 0.5 * (v1 - v0) + 0.03), c8Zinc);
        { // The kitchen window: from 0.41 to 0.94 of the width, counted from the terrace side
            const double a0 = v1 - 0.94 * (v1 - v0);
            const double a1 = v1 - 0.41 * (v1 - v0);
            b.panel(Side::Right, a0, a1, 1.50, 2.00, c8White, c8ExtDepth + 0.012);
            b.panel(Side::Right, a0 + c8Bar, a1 - c8Bar, 1.50 + c8Bar, 2.00 - c8Bar, paint.m_glass, c8ExtDepth + 0.024);
            mesh.addBox(b.onWall(Side::Right, 0.5 * (a0 + a1), 1.47, c8ExtDepth + 0.03), alongV, up, alongU,
                Vec3(0.5 * (a1 - a0) + 0.03, 0.03, 0.03), c8Zinc);
        }
        { // The small window in the side towards the entrance
            const double v = v0 - 0.012;
            mesh.addQuad(b.point(hl + 0.25, v, 1.50), b.point(hl + 0.65, v, 1.50), b.point(hl + 0.65, v, 2.00),
                b.point(hl + 0.25, v, 2.00), c8White, -alongV);
            mesh.addQuad(b.point(hl + 0.25 + c8Bar, v - 0.012, 1.50 + c8Bar), b.point(hl + 0.65 - c8Bar, v - 0.012, 1.50 + c8Bar),
                b.point(hl + 0.65 - c8Bar, v - 0.012, 2.00 - c8Bar), b.point(hl + 0.25 + c8Bar, v - 0.012, 2.00 - c8Bar),
                paint.m_glass, -alongV);
        }
        { // The exhaust: a round steel cowl beside the window
            const double radius = 0.11;
            const double proud = 0.07;
            const int sides = 8;
            const double v = v1 - 0.24 * (v1 - v0);
            const double y = 1.74;
            const auto rim = [&](int k, double out)
            {
                const double angle = 2.0 * pi * (double) k / (double) sides;
                return b.onWall(Side::Right, v + radius * std::cos(angle), y + radius * std::sin(angle), c8ExtDepth + out);
            };
            const Vec3 centre = b.onWall(Side::Right, v, y, c8ExtDepth + proud);
            for (int k = 0; k < sides; ++k)
            {
                const double angle = 2.0 * pi * ((double) k + 0.5) / (double) sides;
                mesh.addTriangle(centre, rim(k, proud), rim(k + 1, proud), c8Steel, alongU);
                mesh.addQuad(rim(k, 0.0), rim(k + 1, 0.0), rim(k + 1, proud), rim(k, proud), c8Steel.scaled(0.85f),
                    alongV * std::cos(angle) + up * std::sin(angle));
            }
        }
    }

    void buildComfort8C(Builder& b, const Paint& paint)
    {
        const double hl = b.halfLength();
        const double hw = b.halfWidth();
        // Places along a side, in metres from its end: along the terrace and the entrance side from the gable
        // with the extension, along that gable from the terrace side, along the other from the entrance side
        const auto fromExtension = [&](double d) { return hl - d; };
        const auto fromTerrace = [&](double d) { return hw - d; };
        const auto fromEntrance = [&](double d) { return -hw + d; };
        ComfortKit::Mason m(b, style8C, paint);
        roof(b, paint);
        m.ridgeTiles(-(hl + c8Overhang), hl + c8Overhang, paint.m_roof.scaled(0.9f));
        m.slateGable(Side::Right, c8Overhang - 0.02, false);
        slateBesideOutshot(b, paint);
        loftWindow(b, Side::Right, paint);
        loftWindow(b, Side::Left, paint);
        { // Terrace side, from the gable with the extension: the living room glazed from its corner, three lights
          // over white panels and its door, all in one frame; brick; a narrow window over a sill
            const Opening living = { fromExtension(4.20), hl - c8Corner, 0.0, c8Soffit, false };
            const Opening narrow = { fromExtension(6.75), fromExtension(5.90), c8Sill, c8Soffit, true };
            m.wall(Side::Front, { living, narrow });
            m.frame(Side::Front, living);
            const double door = fromExtension(3.25);    // the door from living.m_a0 to here
            const double light = (living.m_a1 - door) / 3.0;
            m.lights(Side::Front, { door, door + light, door + 2.0 * light, living.m_a1 }, c8Kick, c8Soffit);
            m.door(Side::Front, living.m_a0, door, c8White);
            m.fill(Side::Front, living.m_a0 + c8Bar + 0.09, door - c8Bar - 0.09, 0.45, 1.90, paint.m_glass, true);
            m.handle(Side::Front, door - c8Bar - 0.10);
            m.frame(Side::Front, narrow);
            m.lights(Side::Front, { narrow.m_a0, narrow.m_a1 }, narrow.m_y0, narrow.m_y1);
            b.downpipe(Side::Front, -hl + 0.10, c8White, false);
            m.paving(Side::Front, fromExtension(4.80), hl + 1.40, 3.00, 0.60, c8Paving);
        }
        { // Entrance side, from the gable with the extension: a downpipe, a window of two over a sill, three small
          // windows over brick beside the front door, all in one frame; then the outshot
            const Opening window = { fromExtension(3.00), fromExtension(1.98), c8Sill, c8Soffit, true };
            const Opening group = { fromExtension(5.87), fromExtension(3.40), 0.0, c8Soffit, false };
            m.wall(Side::Back, { window, group });
            m.frame(Side::Back, window);
            m.lights(Side::Back, { window.m_a0, fromExtension(2.49), window.m_a1 }, window.m_y0, window.m_y1);
            m.frame(Side::Back, group);
            const double door = fromExtension(4.97);    // the door from group.m_a0 to here
            const double light = (group.m_a1 - door) / 3.0;
            m.door(Side::Back, group.m_a0, door, paint.m_door);
            m.fill(Side::Back, group.m_a0 + c8Bar + 0.12, door - c8Bar - 0.12, 0.30, 1.85, c8Frosted, true);
            m.handle(Side::Back, door - c8Bar - 0.10);
            m.brick(Side::Back, door + 0.5 * c8Bar, group.m_a1 - 0.5 * c8Bar, 0.0, c8Rail - c8Bar, -c8Reveal + 0.012);
            m.lights(Side::Back, { door, door + light, door + 2.0 * light, group.m_a1 }, c8Rail, c8Soffit);
            b.downpipe(Side::Back, fromExtension(0.55), c8White);
            m.paving(Side::Back, group.m_a0 - 0.10, door + 0.30, 1.00, 0.10, c8Paving);
            outshot(b, m, paint);
        }
        { // Gable with the extension, from the terrace side: the glazed corner of the living room over a white
          // panel, brick, the extension, brick
            const Opening corner = { fromTerrace(1.25), hw - c8Corner, 0.0, c8Soffit, false };
            m.wall(Side::Right, { corner });
            m.frame(Side::Right, corner);
            m.lights(Side::Right, { corner.m_a0, corner.m_a1 }, c8Kick, c8Soffit);
            b.mesh().addBox(b.point(hl, hw, 0.5 * c8Soffit), normalize(b.point(1.0, 0.0, 0.0) - b.point(0.0, 0.0, 0.0)),
                Vec3(0.0, 1.0, 0.0), b.outward(Side::Front), Vec3(c8Corner + 0.01, 0.5 * c8Soffit, c8Corner + 0.01), c8White);
            extension(b, paint);
            m.paving(Side::Right, fromTerrace(c8ExtTo + 0.40), hw, 2.00, 0.0, c8Paving);
        }
        { // Gable beside the outshot, from the entrance side: a narrow window over a sill, the second door, brick
            const Opening window = { fromEntrance(0.30), fromEntrance(0.90), c8Sill, c8Soffit, true };
            const Opening door = { fromEntrance(1.10), fromEntrance(2.00), 0.0, c8Soffit, false };
            m.wall(Side::Left, { window, door });
            m.frame(Side::Left, window);
            m.lights(Side::Left, { window.m_a0, window.m_a1 }, window.m_y0, window.m_y1);
            m.frame(Side::Left, door);
            m.door(Side::Left, door.m_a0, door.m_a1, paint.m_door);
            m.handle(Side::Left, door.m_a0 + c8Bar + 0.10);
            m.paving(Side::Left, door.m_a0 - 0.30, door.m_a1 + 0.30, 1.00, 0.30, c8Paving);
        }
        // On the roof: the tall flue of the stove on the terrace slope, near the gable with the extension; two
        // flues near the ridge and two vents lower on the entrance slope
        m.pipe(fromExtension(1.90), 0.42 * hw, 1.90, 0.10, paint.m_pipe);
        m.pipe(fromExtension(5.45), -0.22 * hw, 0.55, 0.07, paint.m_pipe);
        m.pipe(fromExtension(5.90), -0.22 * hw, 0.55, 0.07, paint.m_pipe);
        m.pipe(fromExtension(4.25), -0.55 * hw, 0.30, 0.07, c8Vent);
        m.pipe(fromExtension(6.20), -0.55 * hw, 0.30, 0.07, c8Vent);
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
        // The plan of the BGT, the same for every 8C
        std::pair<double, double> footprint(double, double) const override { return { c8Length, c8Width }; }
        // The walls, the outshot beside the entrance and the extension against the gable
        bool covers(double length, double width, double du, double dv, double margin) const override
        {
            const bool inOutshot = du > -0.5 * length - margin && du < -0.5 * length + c8OutLength + margin
                && dv < -0.5 * width + margin && dv > -0.5 * width - c8OutDepth - margin;
            const bool inExtension = du > 0.5 * length - margin && du < 0.5 * length + c8ExtDepth + margin
                && dv > 0.5 * width - c8ExtTo - margin && dv < 0.5 * width - c8ExtFrom + margin;
            return Model::covers(length, width, du, dv, margin) || inOutshot || inExtension;
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
