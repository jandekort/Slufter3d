// Slufter3D - model Comfort6C: detached bungalow of light grey brick under brown tiles, slate gables, navy boards
// Author: Jan de Kort
// Created: 2026-11-16
// Comfort6C (photos Screenshot 2026-11-17 16.36.13, 16.36.46, 16.37.45, 16.38.08, 16.38.50 and 17.11.53)
// Measured on 16.37.45, house 87, by the vanishing points of its entrance side and its gable, checked against its
// brick courses of 60 mm and its doors of 2.0 m: every frame and door reaches up to the soffit at 2.2 m above the
// floor, and the ridge lies about 4.5 m above it, a pitch of 31 degrees. Eaves and verges reach 0.35 m beyond the
// walls, with a light soffit under them all round; the slate gables stand flush with the navy boards. The darker
// courses end 0.55 m above the floor, level all round: 0.50 m in 16.37.45, 0.60 m in 16.38.08 and 16.38.50.
// Places along a wall are parts of its length, so that the picture keeps on every outline: the entrance side as
// in 16.37.45, the terrace side as in 16.36.46, the gables as in 16.38.50 and 16.36.13. Frames and doors stand
// back in the brick, windows over sills. Terrace and front step are paved at floor level against the wall, from
// where they slope down to lie on the ground at their edges, so that they show no thickness.
#include "ModelKit.h"
#include "Models.h"

#include "Mesh.h"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace ModelKit;

namespace
{
    const double c6Soffit = 2.20;       // underside of the eaves all round; frames and doors reach up to it [m]
    const double c6Fascia = 0.28;       // navy board along the eaves [m]
    const double c6Overhang = 0.35;     // of eaves and verges alike [m]
    const double c6Pitch = 31.0;        // [deg]
    const double c6Plinth = 0.55;       // top of the darker courses [m]
    const double c6Foundation = 2.5;    // walls reach this far into the dune, as those of the kit [m]
    const double c6Reveal = 0.07;       // frames and doors stand this far back in the brick [m]
    const double c6Bar = 0.07;          // width of the white frames [m]
    const double c6DoorTop = 2.00;      // top of the door leaves, under a light up to the soffit [m]
    const double c6Kick = 0.50;         // the living room is glazed down to this height, white panels below [m]
    const double c6WindowSill = 1.02;   // bottom of the frames of the windows over a sill [m]
    const double c6Rail = 1.58;         // top of the brick between the two front doors, under two small windows [m]
    const double c6Corner = 0.07;       // the white post at the glazed corner of the living room, either way [m]

    const Shape shape6C = { c6Soffit + c6Fascia + c6Overhang * std::tan(degrees(c6Pitch)), c6Plinth, degrees(c6Pitch),
        c6Overhang, c6Overhang, c6Fascia, 0.0, 1.0 };
    // Off the overcast 16.38.08 and 16.38.50, brightened until their white frames are white; the tiles off 17.11.53,
    // the most common colour, as dark against the brick as there
    const Paint paint6C = {
        Color(0.64f, 0.64f, 0.62f),   // light grey brick
        Color(0.47f, 0.47f, 0.46f),   // the darker courses at the foot of the wall
        Color(0.38f, 0.32f, 0.28f),   // brown tiles
        Color(0.38f, 0.32f, 0.28f),
        Color(0.33f, 0.40f, 0.47f),   // slate of the gables
        Color(0.12f, 0.20f, 0.32f),   // navy boards along eaves and verges
        Color(0.30f, 0.36f, 0.42f),
        Color(0.10f, 0.16f, 0.27f),   // navy front doors
        Color(0.29f, 0.35f, 0.41f),
        Color(0.16f, 0.16f, 0.17f) }; // flues
    const Color c6White(0.92f, 0.92f, 0.90f);       // frames, panels, downpipes
    const Color c6Underside(0.82f, 0.78f, 0.66f);   // the soffits
    const Color c6Frosted(0.55f, 0.62f, 0.67f);     // pane of the front door
    const Color c6Blue(0.30f, 0.42f, 0.55f);        // the door of the living room
    const Color c6Stone(0.80f, 0.80f, 0.78f);       // sills and thresholds
    const Color c6Paving(0.50f, 0.50f, 0.49f);
    const Color c6Vent(0.72f, 0.42f, 0.30f);        // terracotta vents on the entrance slope

    // Struct Opening: a hole in a wall from a0 to a1 along it, y0 to y1 high, with a1 > a0
    struct Opening
    {
        double m_a0;
        double m_a1;
        double m_y0;
        double m_y1;
        bool m_sill;    // true->a sill under it, false->a threshold
    };

    Vec3 along(Builder& b, Side side)
    {
        return normalize(b.onWall(side, 1.0, 0.0, 0.0) - b.onWall(side, 0.0, 0.0, 0.0));
    }

    // Brick from y0 to y1 between a0 and a1, out in front of the wall, darker below the plinth line
    void brick(Builder& b, Side side, double a0, double a1, double y0, double y1, double out, const Paint& paint)
    {
        if (a1 - a0 < 1.0e-6 || y1 - y0 < 1.0e-6)
        {
            return;
        }
        if (y0 < c6Plinth)
        {
            b.panel(side, a0, a1, y0, std::min(y1, c6Plinth), paint.m_plinth, out);
        }
        if (y1 > c6Plinth)
        {
            b.panel(side, a0, a1, std::max(y0, c6Plinth), y1, paint.m_wall, out);
        }
    }

    // The sides of an opening, back to its frame: brick at either side, the soffit over it when it reaches up to
    // that, else brick, and stone at its foot unless a sill covers it
    void reveal(Builder& b, Side side, const Opening& o, const Paint& paint)
    {
        Mesh& mesh = b.mesh();
        const Vec3 a = along(b, side);
        const Vec3 up(0.0, 1.0, 0.0);
        auto jamb = [&](double at, double y0, double y1, const Color& color, const Vec3& facing)
        {
            if (y1 - y0 > 1.0e-6)
            {
                mesh.addQuad(b.onWall(side, at, y0, 0.0), b.onWall(side, at, y0, -c6Reveal), b.onWall(side, at, y1, -c6Reveal),
                    b.onWall(side, at, y1, 0.0), color, facing);
            }
        };
        for (const double s : { 1.0, -1.0 })
        {
            const double at = s > 0.0 ? o.m_a0 : o.m_a1;
            jamb(at, o.m_y0, std::min(o.m_y1, c6Plinth), paint.m_plinth, a * s);
            jamb(at, std::max(o.m_y0, c6Plinth), o.m_y1, paint.m_wall, a * s);
        }
        auto flat = [&](double y, const Color& color, const Vec3& facing)
        {
            mesh.addQuad(b.onWall(side, o.m_a0, y, 0.0), b.onWall(side, o.m_a1, y, 0.0), b.onWall(side, o.m_a1, y, -c6Reveal),
                b.onWall(side, o.m_a0, y, -c6Reveal), color, facing);
        };
        flat(o.m_y1, o.m_y1 >= c6Soffit ? c6Underside : paint.m_wall, -up);
        if (!o.m_sill)
        {
            flat(o.m_y0, c6Stone, up);
        }
    }

    // A wall from deep in the dune up to the soffit, with its openings in it
    void wall(Builder& b, Side side, std::vector<Opening> openings, const Paint& paint)
    {
        std::sort(openings.begin(), openings.end(), [](const Opening& p, const Opening& q) { return p.m_a0 < q.m_a0; });
        const double half = (side == Side::Front || side == Side::Back) ? b.halfLength() : b.halfWidth();
        double from = -half;
        for (const Opening& o : openings)
        {
            brick(b, side, from, o.m_a0, -c6Foundation, c6Soffit, 0.0, paint);
            brick(b, side, o.m_a0, o.m_a1, -c6Foundation, o.m_y0, 0.0, paint);
            brick(b, side, o.m_a0, o.m_a1, o.m_y1, c6Soffit, 0.0, paint);
            reveal(b, side, o, paint);
            if (o.m_sill)
            {
                const double out = 0.06;
                b.mesh().addBox(b.onWall(side, 0.5 * (o.m_a0 + o.m_a1), o.m_y0 - 0.03, 0.5 * (out - c6Reveal)), along(b, side),
                    Vec3(0.0, 1.0, 0.0), b.outward(side), Vec3(0.5 * (o.m_a1 - o.m_a0) + 0.04, 0.03, 0.5 * (out + c6Reveal)), c6Stone);
            }
            from = o.m_a1;
        }
        brick(b, side, from, half, -c6Foundation, c6Soffit, 0.0, paint);
    }

    // The white frame that fills an opening at the back of its reveal
    void frame(Builder& b, Side side, const Opening& o)
    {
        b.panel(side, o.m_a0, o.m_a1, o.m_y0, o.m_y1, c6White, -c6Reveal);
    }

    // Glass, a leaf or a panel in the frame, just before it; inLeaf->a pane in a leaf, just before that
    void fill(Builder& b, Side side, double a0, double a1, double y0, double y1, const Color& color, bool inLeaf = false)
    {
        b.panel(side, a0, a1, y0, y1, color, -c6Reveal + (inLeaf ? 0.024 : 0.012));
    }

    // Lights of glass between the bars of a frame at the given places along the wall, from y0 to y1
    void lights(Builder& b, Side side, const std::vector<double>& bars, double y0, double y1, const Paint& paint)
    {
        for (size_t k = 0; k + 1 < bars.size(); ++k)
        {
            fill(b, side, bars[k] + 0.5 * c6Bar, bars[k + 1] - 0.5 * c6Bar, y0 + 0.5 * c6Bar, y1 - c6Bar, paint.m_glass);
        }
    }

    // A door from a0 to a1 in its frame: its leaf up to c6DoorTop, a light over it up to the soffit
    void door(Builder& b, Side side, double a0, double a1, const Color& leaf, const Paint& paint)
    {
        fill(b, side, a0 + c6Bar, a1 - c6Bar, 0.0, c6DoorTop, leaf);
        fill(b, side, a0 + c6Bar, a1 - c6Bar, c6DoorTop + c6Bar, c6Soffit - c6Bar, paint.m_glass);
    }

    // A lever on a front door, at a along the wall
    void handle(Builder& b, Side side, double a)
    {
        b.mesh().addBox(b.onWall(side, a, 1.05, -c6Reveal + 0.05), along(b, side), Vec3(0.0, 1.0, 0.0), b.outward(side),
            Vec3(0.07, 0.015, 0.025), c6Stone);
    }

    // The slate of a gable, flush with the boards of the eaves, from the soffit up under the roof, and the soffit
    // between it and the wall
    void slateGable(Builder& b, Side side, const Color& slate)
    {
        const double eaveV = b.halfWidth() + c6Overhang;
        const double eaveY = c6Soffit + c6Fascia;
        const double out = c6Overhang - 0.02;     // just behind the boards along the verges
        const Vec3 o = b.outward(side);
        b.mesh().addQuad(b.onWall(side, -eaveV, c6Soffit, out), b.onWall(side, eaveV, c6Soffit, out),
            b.onWall(side, eaveV, eaveY, out), b.onWall(side, 0.0, b.ridge(), out), slate, o);
        b.mesh().addTriangle(b.onWall(side, -eaveV, c6Soffit, out), b.onWall(side, 0.0, b.ridge(), out),
            b.onWall(side, -eaveV, eaveY, out), slate, o);
        b.mesh().addQuad(b.onWall(side, -b.halfWidth(), c6Soffit, 0.0), b.onWall(side, b.halfWidth(), c6Soffit, 0.0),
            b.onWall(side, b.halfWidth(), c6Soffit, out), b.onWall(side, -b.halfWidth(), c6Soffit, out), c6Underside,
            Vec3(0.0, -1.0, 0.0));
    }

    // Ridge tiles along the top, a little proud of both slopes
    void ridgeTiles(Builder& b, const Color& tile)
    {
        const double half = b.halfLength() + c6Overhang;
        const double edge = 0.13;
        const double low = b.roofHeight(edge) + 0.01;
        const double top = b.ridge() + 0.09;
        for (const double s : { 1.0, -1.0 })
        {
            b.mesh().addQuad(b.point(-half, s*edge, low), b.point(half, s*edge, low), b.point(half, 0.0, top),
                b.point(-half, 0.0, top), tile, b.point(0.0, s, 1.0) - b.point(0.0, 0.0, 0.0));
            b.mesh().addTriangle(b.point(s*half, -edge, low), b.point(s*half, edge, low), b.point(s*half, 0.0, top), tile,
                b.point(s, 0.0, 0.0) - b.point(0.0, 0.0, 0.0));
        }
    }

    // A pipe through the roof at (u, v), standing this far above it
    void pipe(Builder& b, double u, double v, double above, double radius, const Color& color)
    {
        const double foot = b.roofHeight(v) - 0.2;
        const double halfHeight = 0.5 * (above + 0.2);
        b.mesh().addBox(b.point(u, v, foot + halfHeight), normalize(b.point(1.0, 0.0, 0.0) - b.point(0.0, 0.0, 0.0)),
            Vec3(0.0, 1.0, 0.0), b.outward(Side::Front), Vec3(radius, halfHeight, radius), color);
    }

    // Paving before a side, from a0 to a1 along it and depth out from the wall, laid just above the ground in tiles
    // that follow it. Against the wall, but for a margin at either end, it is raised to the floor if the ground lies
    // lower there, and slopes down from it to the ground at its outer edge.
    void paving(Builder& b, Side side, double a0, double a1, double depth, double margin, const Color& color)
    {
        const double top = -0.02;   // just under the thresholds
        const double lift = 0.04;   // above the ground [m]
        const double tile = 1.0;    // the paving follows the ground in steps of at most this size [m]
        const double half = (side == Side::Front || side == Side::Back) ? b.halfLength() : b.halfWidth();
        const double level0 = std::max(a0 + margin, -half);
        const double level1 = std::min(a1 - margin, half);
        std::vector<double> ats;
        double from = a0;
        for (const double to : { level0, level1, a1 })
        {
            const int pieces = (int) std::ceil((to - from) / tile - 1.0e-9);
            for (int k = 0; k < pieces; ++k)
            {
                ats.push_back(mix(from, to, (double) k / (double) pieces));
            }
            from = std::max(from, to);
        }
        ats.push_back(a1);
        const int rows = std::max(1, (int) std::ceil(depth / tile));
        auto place = [&](double at, int row)
        {
            const double out = depth * (double) row / (double) rows;
            const Vec3 p = b.onWall(side, at, 0.0, out);
            const double ground = b.groundUnder(p) + lift;
            const bool raised = at > level0 - 1.0e-9 && at < level1 + 1.0e-9;
            const double rise = raised ? std::max(0.0, top - ground) * (1.0 - (double) row / (double) rows) : 0.0;
            return b.onWall(side, at, ground + rise, out);
        };
        // Lit as if level, so that the slight slopes between the tiles do not show
        const Color lit = b.mesh().litGround(color, Vec3(0.0, 1.0, 0.0));
        for (size_t i = 0; i + 1 < ats.size(); ++i)
        {
            for (int j = 0; j < rows; ++j)
            {
                const Vec3 p00 = place(ats[i], j);
                const Vec3 p10 = place(ats[i + 1], j);
                const Vec3 p11 = place(ats[i + 1], j + 1);
                const Vec3 p01 = place(ats[i], j + 1);
                b.mesh().addFlatTriangle(p00, p10, p11, lit, lit, lit);
                b.mesh().addFlatTriangle(p00, p11, p01, lit, lit, lit);
            }
        }
    }

    void buildComfort6C(Builder& b, const Paint& paint)
    {
        const double hl = b.halfLength();
        const double hw = b.halfWidth();
        // Places along a side as parts of its length from one end: the bedroom end of the entrance side, the living
        // room end of the terrace side, the entrance side of the living-room gable, the terrace side of the other
        const auto entrance = [&](double part) { return hl * (1.0 - 2.0 * part); };
        const auto terrace = [&](double part) { return hl * (2.0 * part - 1.0); };
        const auto livingEnd = [&](double part) { return hw * (2.0 * part - 1.0); };
        const auto bedroomEnd = [&](double part) { return hw * (1.0 - 2.0 * part); };
        b.roof(c6Underside);
        ridgeTiles(b, paint.m_roof.scaled(0.9f));
        slateGable(b, Side::Left, paint.m_gable);
        slateGable(b, Side::Right, paint.m_gable);
        { // Entrance side, from left to right for someone facing it: the bedroom window, the two doors with two
          // small windows over the brick between them, all in one white frame, and a narrow window
            const Opening bedroom = { entrance(0.136), entrance(0.028), c6WindowSill, c6Soffit, true };
            const Opening group = { entrance(0.674), entrance(0.285), 0.0, c6Soffit, false };
            const Opening narrow = { entrance(0.864), entrance(0.806), c6WindowSill, c6Soffit, true };
            wall(b, Side::Back, { bedroom, group, narrow }, paint);
            frame(b, Side::Back, bedroom);
            lights(b, Side::Back, { bedroom.m_a0, entrance(0.073), bedroom.m_a1 }, bedroom.m_y0, bedroom.m_y1, paint);
            frame(b, Side::Back, narrow);
            lights(b, Side::Back, { narrow.m_a0, narrow.m_a1 }, narrow.m_y0, narrow.m_y1, paint);
            frame(b, Side::Back, group);
            const double door1 = entrance(0.374);   // the first door from group.m_a1 to here
            const double door2 = entrance(0.584);   // the second from group.m_a0 to here
            door(b, Side::Back, door1, group.m_a1, paint.m_door, paint);
            fill(b, Side::Back, door1 + c6Bar + 0.11, group.m_a1 - c6Bar - 0.11, 1.03, 1.89, c6Frosted, true);
            handle(b, Side::Back, group.m_a1 - c6Bar - 0.10);
            door(b, Side::Back, group.m_a0, door2, paint.m_door, paint);
            handle(b, Side::Back, door2 - c6Bar - 0.10);
            brick(b, Side::Back, door2 + 0.5 * c6Bar, door1 - 0.5 * c6Bar, 0.0, c6Rail - c6Bar, -c6Reveal + 0.012, paint);
            lights(b, Side::Back, { door2, entrance(0.496), entrance(0.474), door1 }, c6Rail, c6Soffit, paint);
            b.downpipe(Side::Back, -hl + 0.10, c6White, false);
            paving(b, Side::Back, group.m_a0 - 0.90, group.m_a1 + 0.90, 1.20, 0.90, c6Paving);
        }
        { // Terrace side, from left to right: the living room glazed from its corner, three windows over white
          // panels and its door, brick, a window of four over a sill, brick
            const Opening living = { -hl + c6Corner, terrace(0.380), 0.0, c6Soffit, false };
            const Opening four = { terrace(0.507), terrace(0.792), 0.75, c6Soffit, true };
            wall(b, Side::Front, { living, four }, paint);
            frame(b, Side::Front, living);
            const double door0 = terrace(0.286);
            const double pane = (door0 - living.m_a0) / 3.0;
            lights(b, Side::Front, { living.m_a0, living.m_a0 + pane, living.m_a0 + 2.0*pane, door0 }, c6Kick, c6Soffit, paint);
            door(b, Side::Front, door0, living.m_a1, c6Blue, paint);
            fill(b, Side::Front, door0 + c6Bar + 0.08, living.m_a1 - c6Bar - 0.08, 0.45, 1.90, paint.m_glass, true);
            frame(b, Side::Front, four);
            const double quarter = 0.25 * (four.m_a1 - four.m_a0);
            lights(b, Side::Front, { four.m_a0, four.m_a0 + quarter, four.m_a0 + 2.0*quarter, four.m_a0 + 3.0*quarter, four.m_a1 },
                four.m_y0, four.m_y1, paint);
            b.downpipe(Side::Front, hl - 0.10, c6White);
            paving(b, Side::Front, -hl - 1.40, living.m_a1 + 0.60, 2.60, 0.60, c6Paving);
        }
        { // Living-room gable, from the entrance side: a glazed door and glass up to the corner
            const Opening glazedDoor = { livingEnd(0.50), livingEnd(0.65), 0.0, c6Soffit, false };
            const Opening corner = { livingEnd(0.87), hw - c6Corner, 0.0, c6Soffit, false };
            wall(b, Side::Left, { glazedDoor, corner }, paint);
            frame(b, Side::Left, glazedDoor);
            door(b, Side::Left, glazedDoor.m_a0, glazedDoor.m_a1, c6White, paint);
            fill(b, Side::Left, glazedDoor.m_a0 + 2.0*c6Bar, glazedDoor.m_a1 - 2.0*c6Bar, 0.45, 1.90, paint.m_glass, true);
            frame(b, Side::Left, corner);
            lights(b, Side::Left, { corner.m_a0, corner.m_a1 }, c6Kick, c6Soffit, paint);
            b.mesh().addBox(b.point(-hl, hw, 0.5 * c6Soffit), normalize(b.point(1.0, 0.0, 0.0) - b.point(0.0, 0.0, 0.0)),
                Vec3(0.0, 1.0, 0.0), b.outward(Side::Front), Vec3(c6Corner + 0.01, 0.5 * c6Soffit, c6Corner + 0.01), c6White);
        }
        { // Bedroom gable, from the terrace side: the meter box, a narrow window towards the entrance side
            const Opening window = { bedroomEnd(0.75), bedroomEnd(0.63), c6WindowSill, c6Soffit, true };
            wall(b, Side::Right, { window }, paint);
            frame(b, Side::Right, window);
            lights(b, Side::Right, { window.m_a0, window.m_a1 }, window.m_y0, window.m_y1, paint);
            b.mesh().addBox(b.onWall(Side::Right, bedroomEnd(0.17), 1.75, 0.06), along(b, Side::Right), Vec3(0.0, 1.0, 0.0),
                b.outward(Side::Right), Vec3(0.18, 0.22, 0.06), Color(0.78f, 0.78f, 0.76f));
        }
        // On the roof: the flue of the stove near the living-room end of the terrace slope, two flues and two
        // terracotta vents on the entrance slope
        pipe(b, terrace(0.15), 0.30*hw, 1.00, 0.08, paint.m_pipe);
        pipe(b, entrance(0.48), -0.25*hw, 0.60, 0.06, paint.m_pipe);
        pipe(b, entrance(0.76), -0.25*hw, 0.60, 0.06, paint.m_pipe);
        pipe(b, entrance(0.52), -0.62*hw, 0.25, 0.06, c6Vent);
        pipe(b, entrance(0.72), -0.62*hw, 0.25, 0.06, c6Vent);
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
