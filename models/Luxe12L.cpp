// Slufter3D - model Luxe12L: a tall part under thatch with a low, wide wing at its far end, both under one roof
// Author: Jan de Kort
// Created: 2026-11-17
// Luxe12L (photos 12La, 12Lb, 12Lc and 12Ld; 12La is house 411)
// Every 12L stands on the same plan in the BGT: a tall part 8.2 m long and 8.1 m wide, its gable to the street,
// and at its far end a low wing 6.5 m long and 12.05 m wide, reaching 2 m beyond it on either side. One roof of
// 45 degrees covers both: from the ridge at 7.6 m it comes down to 3.2 m over the tall part and on to 1.2 m over
// the wing, so that where the wing begins its roof ends in a verge, over the entrance. The far end is hipped up to
// the ridge, at the same pitch, through tiles and thatch alike (12Ld); the ridge begins above the inner end of the
// wing. Thatch above 4.0 m, red tiles below, a ridge of red tiles.
// The tall part has long walls of brick and a street gable clad in dark boards, within white boarding, with dark
// frames (12La); the wing has low walls of dark boards. Places are measured on the photos with their perspective
// taken out. In the frame of the model the wing lies at -u and the street gable at +u; the garden side with the
// terrace and the French doors (12Ld) is the front, +v, and the entrance (12La, 12Lb, 12Lc) faces the street on
// the back, -v, in the end of the wing. Seen so, the photographs agree with each other; data/house_corrections.json
// turns the houses whose wing lies at the other end.
#include "ModelKit.h"
#include "Models.h"

#include "Mesh.h"

#include <cmath>

using namespace ModelKit;

namespace
{
    const double l12Length = 14.70;     // the walls from end to end [m]
    const double l12Width = 12.05;      // across the wing [m]
    const double l12Main = 4.05;        // half the width of the tall part [m]
    const double l12Wing = 6.50;        // length of the wing [m]
    const double l12Thatch = 3.95;      // the thatch starts at this height, red tiles below [m]
    const double l12Step = 0.10;        // the thatch stands this far proud of the tiles [m]
    const double l12Foundation = 2.5;   // walls reach this far into the dune, as those of the kit [m]
    const double l12Board = 0.22;       // depth of the boards along verges [m]

    // The wall height is that of the wing, where the roof meets its walls; the ridge follows from it
    const Shape shape12L = { 1.62, 0.45, degrees(45.0), 0.42, 0.35, 0.18, 0.0, 1.0 };
    const Paint paint12L = {
        Color(0.50f, 0.27f, 0.22f),   // red-brown brick of the tall part
        Color(0.86f, 0.86f, 0.82f),   // light plinth
        Color(0.66f, 0.22f, 0.16f),   // red tiles
        Color(0.38f, 0.35f, 0.31f),   // thatch
        Color(0.20f, 0.21f, 0.24f),   // dark boards of the street gable and the wing
        Color(0.94f, 0.93f, 0.88f),   // white boards, and the frames in the brick
        Color(0.36f, 0.44f, 0.50f),
        Color(0.18f, 0.20f, 0.23f),   // dark door
        Color(0.20f, 0.20f, 0.21f),
        Color(0.10f, 0.10f, 0.11f) }; // black vents
    const Color darkFrame(0.24f, 0.25f, 0.28f);
    const Color frosted(0.80f, 0.82f, 0.82f);
    const Color ridgeTiles(0.80f, 0.40f, 0.26f);

    // Class Roof12L: the heights and edges of the one roof
    class Roof12L
    {
    public:
        explicit Roof12L(Builder& b)
            :
            m_b(b)
            , m_tangent(std::tan(shape12L.m_pitch))
            , m_ridge(b.ridge())
            , m_thatchV((b.ridge() - l12Thatch) / std::tan(shape12L.m_pitch))
            , m_mainEaveV(l12Main + shape12L.m_eave)
            , m_wingEaveV(b.halfWidth() + shape12L.m_eave)
            , m_streetEnd(b.halfLength() + shape12L.m_verge)
            , m_farEnd(-b.halfLength() - shape12L.m_eave)
            , m_wingVerge(-b.halfLength() + l12Wing + shape12L.m_verge)
            , m_hipThatch(m_farEnd + (l12Thatch - height(m_wingEaveV)) / m_tangent)
            , m_apex(m_farEnd + (m_ridge - height(m_wingEaveV)) / m_tangent)
        {
        }

        // Accessor
        double height(double v) const { return m_ridge - std::fabs(v) * m_tangent; } // of the roof plane
        // Away from the roof plane on the side of the given sign, this far
        Vec3 lift(double sign, double distance) const
        {
            return (Vec3(0.0, 1.0, 0.0) + m_b.outward(Side::Front) * (sign * m_tangent))
                * (distance * std::cos(shape12L.m_pitch));
        }
        Vec3 at(double u, double v, double above = 0.0) const { return m_b.point(u, v, height(v) + above); }

        // Mutator
        void build(const Paint& paint);
        // A window in a slope: from u0 to u1, from v0 out to v1 in, standing this far proud of the plane
        void skylight(double u0, double u1, double v0, double v1, double sign, double proud);

    private:
        Builder& m_b;
        double m_tangent;
        double m_ridge;
        double m_thatchV;     // where the thatch starts, off the ridge [m]
        double m_mainEaveV;   // eaves of the tall part, off the ridge [m]
        double m_wingEaveV;   // eaves of the wing [m]
        double m_streetEnd;   // verge of the street gable, along the house [m]
        double m_farEnd;      // eaves at the far end of the wing [m]
        double m_wingVerge;   // where the roof of the wing ends towards the street, over the entrance [m]
        double m_hipThatch;   // where the hip at the far end reaches the thatch, along the house [m]
        double m_apex;        // where it reaches the ridge [m]
    };

    void Roof12L::build(const Paint& paint)
    {
        Mesh& mesh = m_b.mesh();
        const Vec3 up(0.0, 1.0, 0.0);
        const Vec3 along = m_b.outward(Side::Right);
        const Vec3 across = m_b.outward(Side::Front);
        const Vec3 down(0.0, -l12Board, 0.0);
        const double fascia = shape12L.m_fascia;
        // The thatch stands proud of the tiles: lifted straight up, so that on slopes and hip, all of one pitch, it
        // keeps its edges in one line
        const Vec3 proud = up * (l12Step / std::cos(shape12L.m_pitch));
        const Vec3 hipSlope = up - along;
        for (const double s : { 1.0, -1.0 })
        {
            const Vec3 slope = up + across * s;
            { // Thatch, from its foot to the ridge and back to the hip, and its edge over the tiles
                mesh.addQuad(at(m_hipThatch, s*m_thatchV) + proud, at(m_streetEnd, s*m_thatchV) + proud,
                    at(m_streetEnd, 0.0) + proud, at(m_apex, 0.0) + proud, paint.m_roofTop, slope);
                mesh.addQuad(at(m_hipThatch, s*m_thatchV), at(m_streetEnd, s*m_thatchV),
                    at(m_streetEnd, s*m_thatchV) + proud, at(m_hipThatch, s*m_thatchV) + proud,
                    paint.m_roofTop.scaled(0.8f), across * s);
            }
            { // Red tiles: a band over the tall part, down to the low eaves over the wing, back to the hip
                mesh.addQuad(at(m_wingVerge, s*m_mainEaveV), at(m_streetEnd, s*m_mainEaveV),
                    at(m_streetEnd, s*m_thatchV), at(m_wingVerge, s*m_thatchV), paint.m_roof, slope);
                mesh.addQuad(at(m_farEnd, s*m_wingEaveV), at(m_wingVerge, s*m_wingEaveV),
                    at(m_wingVerge, s*m_thatchV), at(m_hipThatch, s*m_thatchV), paint.m_roof, slope);
            }
            { // White boards along the eaves, and the underside of the eaves
                const double mainY = height(m_mainEaveV);
                const double wingY = height(m_wingEaveV);
                mesh.addQuad(m_b.point(m_wingVerge, s*m_mainEaveV, mainY - fascia), m_b.point(m_streetEnd, s*m_mainEaveV, mainY - fascia),
                    m_b.point(m_streetEnd, s*m_mainEaveV, mainY + 0.02), m_b.point(m_wingVerge, s*m_mainEaveV, mainY + 0.02),
                    paint.m_trim, across * s);
                mesh.addQuad(m_b.point(m_farEnd, s*m_wingEaveV, wingY - fascia), m_b.point(m_wingVerge, s*m_wingEaveV, wingY - fascia),
                    m_b.point(m_wingVerge, s*m_wingEaveV, wingY + 0.02), m_b.point(m_farEnd, s*m_wingEaveV, wingY + 0.02),
                    paint.m_trim, across * s);
                const Color soffit = paint.m_trim.scaled(0.8f);
                mesh.addQuad(m_b.point(m_wingVerge, s*l12Main, mainY - fascia), m_b.point(m_streetEnd, s*l12Main, mainY - fascia),
                    m_b.point(m_streetEnd, s*m_mainEaveV, mainY - fascia), m_b.point(m_wingVerge, s*m_mainEaveV, mainY - fascia),
                    soffit, -up);
                mesh.addQuad(m_b.point(m_farEnd, s*m_b.halfWidth(), wingY - fascia), m_b.point(m_wingVerge, s*m_b.halfWidth(), wingY - fascia),
                    m_b.point(m_wingVerge, s*m_wingEaveV, wingY - fascia), m_b.point(m_farEnd, s*m_wingEaveV, wingY - fascia),
                    soffit, -up);
            }
            { // Boards along the verges: the street gable, the end of the wing over the entrance
                mesh.addQuad(at(m_streetEnd, s*m_mainEaveV) + down, at(m_streetEnd, 0.0) + down, at(m_streetEnd, 0.0),
                    at(m_streetEnd, s*m_mainEaveV), paint.m_trim, along);
                mesh.addQuad(at(m_wingVerge, s*m_wingEaveV) + down, at(m_wingVerge, s*m_mainEaveV) + down,
                    at(m_wingVerge, s*m_mainEaveV), at(m_wingVerge, s*m_wingEaveV), paint.m_trim, along);
            }
        }
        { // The hip at the far end: tiles from its eaves, thatch from the same height as on the slopes up to the ridge;
          // its board and underside
            const double wingY = height(m_wingEaveV);
            mesh.addQuad(m_b.point(m_farEnd, -m_wingEaveV, wingY), m_b.point(m_farEnd, m_wingEaveV, wingY),
                at(m_hipThatch, m_thatchV), at(m_hipThatch, -m_thatchV), paint.m_roof, hipSlope);
            mesh.addTriangle(at(m_hipThatch, -m_thatchV) + proud, at(m_hipThatch, m_thatchV) + proud,
                at(m_apex, 0.0) + proud, paint.m_roofTop, hipSlope);
            mesh.addQuad(at(m_hipThatch, -m_thatchV), at(m_hipThatch, m_thatchV), at(m_hipThatch, m_thatchV) + proud,
                at(m_hipThatch, -m_thatchV) + proud, paint.m_roofTop.scaled(0.8f), -along);
            mesh.addQuad(m_b.point(m_farEnd, -m_wingEaveV, wingY - fascia), m_b.point(m_farEnd, m_wingEaveV, wingY - fascia),
                m_b.point(m_farEnd, m_wingEaveV, wingY + 0.02), m_b.point(m_farEnd, -m_wingEaveV, wingY + 0.02),
                paint.m_trim, -along);
            mesh.addQuad(m_b.point(-m_b.halfLength(), -m_wingEaveV, wingY - fascia), m_b.point(-m_b.halfLength(), m_wingEaveV, wingY - fascia),
                m_b.point(m_farEnd, m_wingEaveV, wingY - fascia), m_b.point(m_farEnd, -m_wingEaveV, wingY - fascia),
                paint.m_trim.scaled(0.8f), -up);
        }
        { // Red tiles along the ridge
            const double half = 0.5 * (m_streetEnd - m_apex);
            mesh.addBox(m_b.point(m_apex + half, 0.0, m_ridge + 0.14), along, up, across, Vec3(half, 0.10, 0.16), ridgeTiles);
        }
    }

    void Roof12L::skylight(double u0, double u1, double v0, double v1, double sign, double proud)
    {
        const double bar = 0.08;
        const Vec3 normal = lift(sign, 1.0);
        m_b.mesh().addQuad(at(u0, sign*v0) + normal * proud, at(u1, sign*v0) + normal * proud,
            at(u1, sign*v1) + normal * proud, at(u0, sign*v1) + normal * proud, darkFrame, normal);
        m_b.mesh().addQuad(at(u0 + bar, sign*(v0 - bar)) + normal * (proud + 0.02), at(u1 - bar, sign*(v0 - bar)) + normal * (proud + 0.02),
            at(u1 - bar, sign*(v1 + bar)) + normal * (proud + 0.02), at(u0 + bar, sign*(v1 + bar)) + normal * (proud + 0.02),
            paint12L.m_glass, normal);
    }

    // The walls: brick along the tall part, dark boards on its street gable and all round the wing
    void walls(Builder& b, const Roof12L& roof, const Paint& paint)
    {
        const double hl = b.halfLength();
        const double hw = b.halfWidth();
        const double plinth = shape12L.m_plinthHeight;
        const double junction = -hl + l12Wing;              // where the tall part begins
        const double mainTop = roof.height(l12Main);        // the roof meets its long walls here
        const double wingTop = roof.height(hw);
        const double mainOut = l12Main - hw;                // its long walls stand this far in from those of the wing
        for (const Side side : { Side::Front, Side::Back })
        {
            b.panel(side, junction, hl, -l12Foundation, plinth, paint.m_plinth, mainOut);
            b.panel(side, junction, hl, plinth, mainTop, paint.m_wall, mainOut);
            b.panel(side, -hl, junction, -l12Foundation, plinth, paint.m_plinth, 0.0);
            b.panel(side, -hl, junction, plinth, wingTop, paint.m_gable, 0.0);
        }
        b.panel(Side::Left, -hw, hw, -l12Foundation, plinth, paint.m_plinth, 0.0);
        b.panel(Side::Left, -hw, hw, plinth, wingTop, paint.m_gable, 0.0);
        { // Street gable: white boarding behind, dark boards within it
            Mesh& mesh = b.mesh();
            const Vec3 along = b.outward(Side::Right);
            const double inset = 0.16;
            b.panel(Side::Right, -l12Main, l12Main, -l12Foundation, plinth, paint.m_plinth, 0.0);
            mesh.addTriangle(b.onWall(Side::Right, -l12Main, mainTop, 0.02), b.onWall(Side::Right, l12Main, mainTop, 0.02),
                b.onWall(Side::Right, 0.0, b.ridge(), 0.02), paint.m_trim, along);
            b.panel(Side::Right, -l12Main, l12Main, plinth, mainTop, paint.m_gable, 0.04);
            mesh.addTriangle(b.onWall(Side::Right, -l12Main, mainTop, 0.04), b.onWall(Side::Right, l12Main, mainTop, 0.04),
                b.onWall(Side::Right, 0.0, b.ridge() - inset / std::cos(shape12L.m_pitch), 0.04), paint.m_gable, along);
        }
        { // The ends of the wing towards the street, either side of the tall part, up to the roof
            const double out = junction - hl;
            for (const double s : { 1.0, -1.0 })
            {
                b.panel(Side::Right, s > 0.0 ? l12Main : -hw, s > 0.0 ? hw : -l12Main, -l12Foundation, plinth, paint.m_plinth, out);
                b.mesh().addQuad(b.onWall(Side::Right, s*l12Main, plinth, out), b.onWall(Side::Right, s*hw, plinth, out),
                    b.onWall(Side::Right, s*hw, wingTop, out), b.onWall(Side::Right, s*l12Main, mainTop, out),
                    paint.m_gable, b.outward(Side::Right));
            }
        }
    }

    void buildLuxe12L(Builder& b, const Paint& paint)
    {
        const double hl = b.halfLength();
        const double hw = b.halfWidth();
        const double junction = -hl + l12Wing;
        const double mainOut = l12Main - hw;
        Roof12L roof(b);
        walls(b, roof, paint);
        roof.build(paint);
        // Street gable as in 12La: three windows below, the middle one wide, two above; dark frames
        b.window(Side::Right,  2.53,  3.65, 0.50, 2.05, 1, 1, darkFrame, paint.m_glass, 0.02);
        b.window(Side::Right, -0.94,  1.41, 0.50, 2.05, 2, 1, darkFrame, paint.m_glass, 0.02);
        b.window(Side::Right, -3.47, -2.29, 0.50, 2.05, 1, 1, darkFrame, paint.m_glass, 0.02);
        b.window(Side::Right,  0.47,  1.35, 4.00, 5.24, 1, 1, darkFrame, paint.m_glass, 0.02);
        b.window(Side::Right, -1.35, -0.53, 4.00, 5.24, 1, 1, darkFrame, paint.m_glass, 0.02);
        // Entrance side as in 12Lb and 12Lc: two windows in the brick, the door and a frosted light in the end of
        // the wing, facing the street
        b.window(Side::Back, 1.10, 2.23, 0.65, 2.15, 1, 1, paint.m_trim, paint.m_glass, mainOut);
        b.window(Side::Back, 3.98, 5.30, 0.65, 2.15, 1, 1, paint.m_trim, paint.m_glass, mainOut);
        b.door(Side::Right, -4.93, -4.10, 0.05, 2.25, paint.m_trim, paint.m_door, false, paint.m_glass, junction - hl);
        b.window(Side::Right, -5.45, -5.00, 0.05, 2.00, 1, 1, paint.m_trim, frosted, junction - hl);
        // Garden side as in 12Ld: a window either side of the French doors, the terrace before them
        b.window(Side::Front, -0.54, 0.38, 0.75, 2.00, 2, 1, paint.m_trim, paint.m_glass, mainOut);
        b.window(Side::Front,  1.45, 2.60, 0.05, 2.05, 2, 1, darkFrame, paint.m_glass, mainOut);
        b.window(Side::Front,  4.82, 5.75, 0.75, 2.00, 2, 1, paint.m_trim, paint.m_glass, mainOut);
        b.terrace(Side::Front, -0.20, 4.20, 3.2, terraceGrey, mainOut);
        // Windows in the roof and vents
        roof.skylight(0.20, 1.00, 2.50, 1.60, -1.0, l12Step + 0.04);
        roof.skylight(-1.40, -0.60, 2.50, 1.60, 1.0, l12Step + 0.04);
        roof.skylight(-4.20, -3.40, 4.90, 4.00, -1.0, 0.04);
        b.pipe(0.60, -3.60, 0.55, 0.07);
        b.pipe(-2.90, 3.60, 0.55, 0.07);
        b.pipe(-5.50, 5.00, 0.45, 0.07);
    }

    // Class Luxe12L
    class Luxe12L : public GableModel
    {
    public:
        Luxe12L()
            :
            GableModel(shape12L, paint12L)
        {
        }

        // Accessor
        const char* name() const override { return "Luxe12L"; }
        const char* label() const override { return "12L"; }
        // Its own plan, whatever the outline of its roof in the AHN, which misses the low eaves of the wing
        std::pair<double, double> footprint(double, double) const override { return { l12Length, l12Width }; }
        // The tall part and the wide wing at -u
        bool covers(double length, double width, double du, double dv, double margin) const override
        {
            const double junction = -0.5 * length + l12Wing;
            const bool inMain = du > junction - margin && du < 0.5 * length + margin && std::fabs(dv) < l12Main + margin;
            const bool inWing = du > -0.5 * length - margin && du < junction + margin && std::fabs(dv) < 0.5 * width + margin;
            return inMain || inWing;
        }

    protected:
        void draw(Builder& builder, const Paint& paint) const override { buildLuxe12L(builder, paint); }
    };
}

const Model& Models::luxe12L()
{
    static const Luxe12L model;
    return model;
}
