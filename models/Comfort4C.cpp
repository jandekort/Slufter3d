// Slufter3D - model Comfort4C: one half of a semi-detached pair of bungalows, light brick with darker courses at its
// foot, a dark tiled roof, green slate gables, white frames and green doors
// Author: Jan de Kort
// Created: 2026-11-17, in place of ComfortSemi4C
// Comfort4C (photos Type_4C_b, Type_4C_c, Type_4C_d, Type_4C_half and brick-fence)
// Every 4C block in the BGT is two rectangles of 8.84 by 6.44 m, end to end, one 1.22 m further forward than the
// other; the AHN puts their ridges 4.8 m above the ground. Each half has a gabled roof of 30 degrees over eaves and
// verges of 0.40 m, the boards along them 0.32 m high over a soffit at 2.33 m; at the outer gable the roof reaches
// over the verge and the green slate stands on a board as high as those along the sides, at the gable against the
// other half the roof ends flush with the wall. The darker courses end 0.48 m above the floor, eight courses,
// level all round. Places along the walls are measured on Type_4C_d with its camera worked out from the corners
// of the block, and on Type_4C_b and Type_4C_c, where the gable and the street side show square.
// In the frame of the model the outer gable is at +u and the gable against the other half at -u; the street side
// with the entrance is the front, +v, the garden with the terrace the back, -v. Seen from the street the right
// half stands forward; the world draws the left half as the mirror image of the model, and that half carries the
// brick fence between the two terraces, out into the garden from where the halves meet (brick-fence).
#include "ComfortKit.h"
#include "ModelKit.h"
#include "Models.h"

#include "Mesh.h"

#include <cmath>

using namespace ModelKit;
using ComfortKit::Opening;

namespace
{
    const double c4Length = 8.84;       // the walls, as the BGT has them [m]
    const double c4Width = 6.44;
    const double c4Soffit = 2.33;       // underside of the eaves all round; frames and doors reach up to it [m]
    const double c4Fascia = 0.32;       // the boards along the eaves [m]
    const double c4Overhang = 0.40;     // of eaves and the outer verge [m]
    const double c4Pitch = 30.0;        // [deg]
    const double c4Plinth = 0.48;       // top of the darker courses, eight of them [m]
    const double c4Foundation = 2.5;    // walls reach this far into the dune, as those of the kit [m]
    const double c4Reveal = 0.07;       // frames and doors stand this far back in the brick [m]
    const double c4Bar = 0.07;          // width of the white frames [m]
    const double c4DoorTop = 2.03;      // top of the door leaves, under a light up to the soffit [m]
    const double c4Kick = 0.50;         // the living room is glazed down to this height, white panels below [m]
    const double c4Sill = 1.00;         // bottom of the frames of the windows over a sill [m]
    const double c4Rail = 1.62;         // top of the brick beside the front door, under two small windows [m]
    const double c4Corner = 0.07;       // the white post at the glazed corner of the living room, either way [m]
    const double c4FenceLength = 2.50;  // the fence reaches this far out into the garden [m]
    const double c4FenceHigh = 2.20;    // its top against the house, just under the soffit ... [m]
    const double c4FenceLow = 1.45;     // ... and at its end [m]
    const double c4FenceThick = 0.22;   // one brick and a half [m]

    const Shape shape4C = { c4Soffit + c4Fascia + c4Overhang * std::tan(degrees(c4Pitch)), c4Plinth, degrees(c4Pitch),
        c4Overhang, c4Overhang, c4Fascia, 0.0, 1.0 };
    // Off the overcast Type_4C_c, brightened until its white frames are white; the roof in its dark version
    const Paint paint4C = {
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
    const Color c4White(0.92f, 0.92f, 0.90f);       // frames, panels, downpipes
    const Color c4Underside(0.86f, 0.86f, 0.83f);   // the soffits, light boards
    const Color c4Stone(0.80f, 0.80f, 0.78f);       // sills and thresholds
    const Color c4Frosted(0.62f, 0.68f, 0.70f);     // pane of the front door
    const Color c4Paving(0.46f, 0.46f, 0.47f);
    const Color c4Bronze(0.45f, 0.31f, 0.21f);      // the plate with the number beside the front door
    const ComfortKit::Style style4C = { c4Soffit, c4Fascia, c4Overhang, c4Plinth, c4Foundation, c4Reveal, c4Bar, c4DoorTop,
        c4White, c4Underside, c4Stone };

    // The roof: both slopes from the gable against the other half, where it ends flush with the wall, to over the
    // verge of the outer gable, with the boards along the eaves and the verges and the soffits under the eaves
    void roof(Builder& b, const Paint& paint)
    {
        Mesh& mesh = b.mesh();
        const Vec3 up(0.0, 1.0, 0.0);
        const Vec3 alongU = normalize(b.point(1.0, 0.0, 0.0) - b.point(0.0, 0.0, 0.0));
        const Vec3 across = b.outward(Side::Front);
        const double hw = b.halfWidth();
        const double eaveV = hw + c4Overhang;
        const double eaveY = c4Soffit + c4Fascia;
        const double u0 = -b.halfLength();
        const double u1 = b.halfLength() + c4Overhang;
        const double board = 0.22;
        for (const double s : { 1.0, -1.0 })
        {
            mesh.addQuad(b.point(u0, s*eaveV, eaveY), b.point(u1, s*eaveV, eaveY), b.point(u1, 0.0, b.ridge()),
                b.point(u0, 0.0, b.ridge()), paint.m_roof, up);
            mesh.addQuad(b.point(u0, s*eaveV, c4Soffit), b.point(u1, s*eaveV, c4Soffit), b.point(u1, s*eaveV, eaveY + 0.02),
                b.point(u0, s*eaveV, eaveY + 0.02), paint.m_trim, across * s);
            mesh.addQuad(b.point(u0, s*hw, c4Soffit), b.point(u1, s*hw, c4Soffit), b.point(u1, s*eaveV, c4Soffit),
                b.point(u0, s*eaveV, c4Soffit), c4Underside, -up);
            // Verge boards: over the outer gable, and against the other half just outside the wall, where the two
            // roofs cross
            for (const double e : { 1.0, -1.0 })
            {
                const double u = e > 0.0 ? u1 : u0 - 0.01;
                const Vec3 low = b.point(u, s*eaveV, eaveY);
                const Vec3 high = b.point(u, 0.0, b.ridge());
                const Vec3 down(0.0, -board, 0.0);
                mesh.addQuad(low + down, high + down, high, low, paint.m_trim, alongU * e);
            }
        }
    }

    // The brick fence between the two terraces: from the back wall where the halves meet out into the garden, its
    // top sloping down from just under the eaves, its foot in the darker courses
    void fence(Builder& b, const Paint& paint)
    {
        Mesh& mesh = b.mesh();
        const Vec3 up(0.0, 1.0, 0.0);
        const Vec3 alongU = normalize(b.point(1.0, 0.0, 0.0) - b.point(0.0, 0.0, 0.0));
        const Vec3 back = b.outward(Side::Back);
        const double u0 = -b.halfLength() - 0.5 * c4FenceThick;
        const double u1 = -b.halfLength() + 0.5 * c4FenceThick;
        const double v0 = -b.halfWidth();
        const double v1 = v0 - c4FenceLength;
        const double foot = -c4Foundation;
        for (const double u : { u0, u1 })
        {
            const Vec3 facing = alongU * (u < -b.halfLength() ? -1.0 : 1.0);
            mesh.addQuad(b.point(u, v0, foot), b.point(u, v1, foot), b.point(u, v1, c4Plinth), b.point(u, v0, c4Plinth),
                paint.m_plinth, facing);
            mesh.addQuad(b.point(u, v0, c4Plinth), b.point(u, v1, c4Plinth), b.point(u, v1, c4FenceLow), b.point(u, v0, c4FenceHigh),
                paint.m_wall, facing);
        }
        mesh.addQuad(b.point(u0, v1, foot), b.point(u1, v1, foot), b.point(u1, v1, c4Plinth), b.point(u0, v1, c4Plinth),
            paint.m_plinth, back);
        mesh.addQuad(b.point(u0, v1, c4Plinth), b.point(u1, v1, c4Plinth), b.point(u1, v1, c4FenceLow), b.point(u0, v1, c4FenceLow),
            paint.m_wall, back);
        // A row of bricks on edge along the top
        mesh.addQuad(b.point(u0, v0, c4FenceHigh), b.point(u1, v0, c4FenceHigh), b.point(u1, v1, c4FenceLow),
            b.point(u0, v1, c4FenceLow), paint.m_plinth, up);
    }

    void buildComfort4C(Builder& b, const Paint& paint)
    {
        const double hl = b.halfLength();
        const double hw = b.halfWidth();
        // Places along a side, in metres from its outer end: along the street side and the garden side from the
        // outer gable, along the outer gable from the street side
        const auto fromOuter = [&](double d) { return hl - d; };
        const auto fromStreet = [&](double d) { return hw - d; };
        ComfortKit::Mason m(b, style4C, paint);
        roof(b, paint);
        m.ridgeTiles(-hl, hl + c4Overhang, paint.m_roof.scaled(0.9f));
        m.slateGable(Side::Right, c4Overhang - 0.02, true);
        m.slateGable(Side::Left, -0.02, true);
        { // Street side, from the outer gable: a window of two over a sill, two small windows over brick beside the
          // front door, all in one frame, and a narrow window near the other half
            const Opening window = { fromOuter(2.55), fromOuter(1.35), c4Sill, c4Soffit, true };
            const Opening group = { fromOuter(6.00), fromOuter(3.00), 0.0, c4Soffit, false };
            const Opening narrow = { fromOuter(8.30), fromOuter(7.40), c4Sill, c4Soffit, true };
            m.wall(Side::Front, { window, group, narrow });
            m.frame(Side::Front, window);
            m.lights(Side::Front, { window.m_a0, fromOuter(1.95), window.m_a1 }, window.m_y0, window.m_y1);
            m.frame(Side::Front, narrow);
            m.lights(Side::Front, { narrow.m_a0, narrow.m_a1 }, narrow.m_y0, narrow.m_y1);
            m.frame(Side::Front, group);
            const double door = fromOuter(5.00);    // the door from group.m_a0 to here, hung on the side of the other half
            m.door(Side::Front, group.m_a0, door, paint.m_door);
            m.fill(Side::Front, group.m_a0 + c4Bar + 0.12, door - c4Bar - 0.12, 1.00, 1.88, c4Frosted, true);
            m.handle(Side::Front, door - c4Bar - 0.10);
            m.brick(Side::Front, door + 0.5 * c4Bar, group.m_a1 - 0.5 * c4Bar, 0.0, c4Rail - c4Bar, -c4Reveal + 0.012);
            m.lights(Side::Front, { door, fromOuter(4.00), group.m_a1 }, c4Rail, c4Soffit);
            b.mesh().addBox(b.onWall(Side::Front, fromOuter(6.45), 1.65, 0.04), m.along(Side::Front), Vec3(0.0, 1.0, 0.0),
                b.outward(Side::Front), Vec3(0.09, 0.12, 0.04), c4Bronze);
            b.downpipe(Side::Front, fromOuter(0.12), c4White, false);
            m.paving(Side::Front, group.m_a0 - 0.30, door + 0.30, 1.00, 0.30, c4Paving);
        }
        { // Garden side, from the outer gable: the living room glazed from its corner, with its door, over white
          // panels; a second door; a window of two over a sill; the terrace before the living room
            const Opening living = { fromOuter(3.30), hl - c4Corner, 0.0, c4Soffit, false };
            const Opening second = { fromOuter(4.55), fromOuter(3.75), 0.0, c4Soffit, false };
            const Opening window = { fromOuter(6.85), fromOuter(5.95), c4Sill, c4Soffit, true };
            m.wall(Side::Back, { living, second, window });
            m.frame(Side::Back, living);
            const double door0 = fromOuter(2.15);
            const double door1 = fromOuter(1.20);
            m.lights(Side::Back, { living.m_a0, door0 }, c4Kick, c4Soffit);
            m.lights(Side::Back, { door1, living.m_a1 }, c4Kick, c4Soffit);
            m.door(Side::Back, door0, door1, paint.m_door);
            m.fill(Side::Back, door0 + c4Bar + 0.08, door1 - c4Bar - 0.08, 0.45, 1.90, paint.m_glass, true);
            m.handle(Side::Back, door0 + c4Bar + 0.10);
            m.frame(Side::Back, second);
            m.door(Side::Back, second.m_a0, second.m_a1, paint.m_door);
            m.frame(Side::Back, window);
            m.lights(Side::Back, { window.m_a0, fromOuter(6.40), window.m_a1 }, window.m_y0, window.m_y1);
            b.downpipe(Side::Back, -hl + 0.25, c4White, false);
            m.paving(Side::Back, fromOuter(6.50), hl + 0.50, 3.60, 0.50, c4Paving);
        }
        { // Outer gable, from the street side: brick, a window of two over white panels, brick, and the glazed
          // corner of the living room
            const Opening wide = { fromStreet(3.75), fromStreet(1.45), 0.0, c4Soffit, false };
            const Opening corner = { -hw + c4Corner, fromStreet(5.05), 0.0, c4Soffit, false };
            m.wall(Side::Right, { wide, corner });
            m.frame(Side::Right, wide);
            m.lights(Side::Right, { wide.m_a0, fromStreet(2.60), wide.m_a1 }, c4Kick, c4Soffit);
            m.frame(Side::Right, corner);
            m.lights(Side::Right, { corner.m_a0, corner.m_a1 }, c4Kick, c4Soffit);
            b.mesh().addBox(b.point(hl, -hw, 0.5 * c4Soffit), normalize(b.point(1.0, 0.0, 0.0) - b.point(0.0, 0.0, 0.0)),
                Vec3(0.0, 1.0, 0.0), b.outward(Side::Front), Vec3(c4Corner + 0.01, 0.5 * c4Soffit, c4Corner + 0.01), c4White);
        }
        // The gable against the other half: brick, where it stands free of it
        m.wall(Side::Left, {});
        if (b.mirrored())
        {
            fence(b, paint);
        }
        // On the roof: two flues near the ridge and two vents on the street slope, the flue of the stove near the
        // outer gable on the garden slope
        m.pipe(fromOuter(4.20), 0.20*hw, 0.75, 0.07, paint.m_pipe);
        m.pipe(fromOuter(5.10), 0.45*hw, 0.55, 0.07, paint.m_pipe);
        m.pipe(fromOuter(3.10), 0.75*hw, 0.25, 0.07, paint.m_pipe);
        m.pipe(fromOuter(6.00), 0.75*hw, 0.25, 0.07, paint.m_pipe);
        m.pipe(fromOuter(1.40), -0.20*hw, 1.00, 0.08, paint.m_pipe);
    }

    // Class Comfort4C
    class Comfort4C : public GableModel
    {
    public:
        Comfort4C()
            :
            GableModel(shape4C, paint4C)
        {
        }

        // Accessor
        const char* name() const override { return "Comfort4C"; }
        const char* label() const override { return "4C"; }
        // The plan of the BGT, the same for every half of every 4C block
        std::pair<double, double> footprint(double, double) const override { return { c4Length, c4Width }; }
        // The walls, and the fence where the halves meet behind them: of the half that stands back, or along the
        // free end of the other half's gable
        bool covers(double length, double width, double du, double dv, double margin) const override
        {
            const bool inFence = std::fabs(du + 0.5 * length) < 0.5 * c4FenceThick + margin
                && dv < -0.5 * width + margin && dv > -0.5 * width - c4FenceLength - margin;
            return Model::covers(length, width, du, dv, margin) || inFence;
        }

    protected:
        void draw(Builder& builder, const Paint& paint) const override { buildComfort4C(builder, paint); }
    };
}

const Model& Models::comfort4C()
{
    static const Comfort4C model;
    return model;
}
