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
#include "ComfortKit.h"
#include "ModelKit.h"
#include "Models.h"

#include "Mesh.h"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace ModelKit;
using ComfortKit::Opening;

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

    const ComfortKit::Style style6C = { c6Soffit, c6Fascia, c6Overhang, c6Plinth, c6Foundation, c6Reveal, c6Bar, c6DoorTop,
        c6White, c6Underside, c6Stone };

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
        ComfortKit::Mason m(b, style6C, paint);
        b.roof(c6Underside);
        m.ridgeTiles(-(hl + c6Overhang), hl + c6Overhang, paint.m_roof.scaled(0.9f));
        m.slateGable(Side::Left, c6Overhang - 0.02, false);
        m.slateGable(Side::Right, c6Overhang - 0.02, false);
        { // Entrance side, from left to right for someone facing it: the bedroom window, the two doors with two
          // small windows over the brick between them, all in one white frame, and a narrow window
            const Opening bedroom = { entrance(0.136), entrance(0.028), c6WindowSill, c6Soffit, true };
            const Opening group = { entrance(0.674), entrance(0.285), 0.0, c6Soffit, false };
            const Opening narrow = { entrance(0.864), entrance(0.806), c6WindowSill, c6Soffit, true };
            m.wall(Side::Back, { bedroom, group, narrow });
            m.frame(Side::Back, bedroom);
            m.lights(Side::Back, { bedroom.m_a0, entrance(0.073), bedroom.m_a1 }, bedroom.m_y0, bedroom.m_y1);
            m.frame(Side::Back, narrow);
            m.lights(Side::Back, { narrow.m_a0, narrow.m_a1 }, narrow.m_y0, narrow.m_y1);
            m.frame(Side::Back, group);
            const double door1 = entrance(0.374);   // the first door from group.m_a1 to here
            const double door2 = entrance(0.584);   // the second from group.m_a0 to here
            m.door(Side::Back, door1, group.m_a1, paint.m_door);
            m.fill(Side::Back, door1 + c6Bar + 0.11, group.m_a1 - c6Bar - 0.11, 1.03, 1.89, c6Frosted, true);
            m.handle(Side::Back, group.m_a1 - c6Bar - 0.10);
            m.door(Side::Back, group.m_a0, door2, paint.m_door);
            m.handle(Side::Back, door2 - c6Bar - 0.10);
            m.brick(Side::Back, door2 + 0.5 * c6Bar, door1 - 0.5 * c6Bar, 0.0, c6Rail - c6Bar, -c6Reveal + 0.012);
            m.lights(Side::Back, { door2, entrance(0.496), entrance(0.474), door1 }, c6Rail, c6Soffit);
            b.downpipe(Side::Back, -hl + 0.10, c6White, false);
            m.paving(Side::Back, group.m_a0 - 0.90, group.m_a1 + 0.90, 1.20, 0.90, c6Paving);
        }
        { // Terrace side, from left to right: the living room glazed from its corner, three windows over white
          // panels and its door, brick, a window of four over a sill, brick
            const Opening living = { -hl + c6Corner, terrace(0.380), 0.0, c6Soffit, false };
            const Opening four = { terrace(0.507), terrace(0.792), 0.75, c6Soffit, true };
            m.wall(Side::Front, { living, four });
            m.frame(Side::Front, living);
            const double door0 = terrace(0.286);
            const double pane = (door0 - living.m_a0) / 3.0;
            m.lights(Side::Front, { living.m_a0, living.m_a0 + pane, living.m_a0 + 2.0*pane, door0 }, c6Kick, c6Soffit);
            m.door(Side::Front, door0, living.m_a1, c6Blue);
            m.fill(Side::Front, door0 + c6Bar + 0.08, living.m_a1 - c6Bar - 0.08, 0.45, 1.90, paint.m_glass, true);
            m.frame(Side::Front, four);
            const double quarter = 0.25 * (four.m_a1 - four.m_a0);
            m.lights(Side::Front, { four.m_a0, four.m_a0 + quarter, four.m_a0 + 2.0*quarter, four.m_a0 + 3.0*quarter, four.m_a1 },
                four.m_y0, four.m_y1);
            b.downpipe(Side::Front, hl - 0.10, c6White);
            m.paving(Side::Front, -hl - 1.40, living.m_a1 + 0.60, 2.60, 0.60, c6Paving);
        }
        { // Living-room gable, from the entrance side: a glazed door and glass up to the corner
            const Opening glazedDoor = { livingEnd(0.50), livingEnd(0.65), 0.0, c6Soffit, false };
            const Opening corner = { livingEnd(0.87), hw - c6Corner, 0.0, c6Soffit, false };
            m.wall(Side::Left, { glazedDoor, corner });
            m.frame(Side::Left, glazedDoor);
            m.door(Side::Left, glazedDoor.m_a0, glazedDoor.m_a1, c6White);
            m.fill(Side::Left, glazedDoor.m_a0 + 2.0*c6Bar, glazedDoor.m_a1 - 2.0*c6Bar, 0.45, 1.90, paint.m_glass, true);
            m.frame(Side::Left, corner);
            m.lights(Side::Left, { corner.m_a0, corner.m_a1 }, c6Kick, c6Soffit);
            b.mesh().addBox(b.point(-hl, hw, 0.5 * c6Soffit), normalize(b.point(1.0, 0.0, 0.0) - b.point(0.0, 0.0, 0.0)),
                Vec3(0.0, 1.0, 0.0), b.outward(Side::Front), Vec3(c6Corner + 0.01, 0.5 * c6Soffit, c6Corner + 0.01), c6White);
        }
        { // Bedroom gable, from the terrace side: the meter box, a narrow window towards the entrance side
            const Opening window = { bedroomEnd(0.75), bedroomEnd(0.63), c6WindowSill, c6Soffit, true };
            m.wall(Side::Right, { window });
            m.frame(Side::Right, window);
            m.lights(Side::Right, { window.m_a0, window.m_a1 }, window.m_y0, window.m_y1);
            b.mesh().addBox(b.onWall(Side::Right, bedroomEnd(0.17), 1.75, 0.06), m.along(Side::Right), Vec3(0.0, 1.0, 0.0),
                b.outward(Side::Right), Vec3(0.18, 0.22, 0.06), Color(0.78f, 0.78f, 0.76f));
        }
        // On the roof: the flue of the stove near the living-room end of the terrace slope, two flues and two
        // terracotta vents on the entrance slope
        m.pipe(terrace(0.15), 0.30*hw, 1.00, 0.08, paint.m_pipe);
        m.pipe(entrance(0.48), -0.25*hw, 0.60, 0.06, paint.m_pipe);
        m.pipe(entrance(0.76), -0.25*hw, 0.60, 0.06, paint.m_pipe);
        m.pipe(entrance(0.52), -0.62*hw, 0.25, 0.06, c6Vent);
        m.pipe(entrance(0.72), -0.62*hw, 0.25, 0.06, c6Vent);
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
