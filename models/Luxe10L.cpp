// Slufter3D - model Luxe10L: the 10L and the 10LT, one house: black boarding, a black tiled roof with a dormer,
// brick gables, a pergola over the terrace at one of them
// Author: Jan de Kort
// Created: 2026-11-16, as Luxe10L and Luxe10LT; made one 2026-11-17
// Luxe10L (photos B405, B405_b and B405_c of house 405, a 10LT; 10L, 10La and 10Lb, of 10Ls, 10Lb of house 420)
// From outside the 10L and the 10LT are the same house. The BGT gives both the same plan, 12.4 to 13.0 m by 8.0 to
// 8.1 m, and the AHN a ridge 7.7 to 7.9 m above the ground; hence walls of 12.6 by 8.05 m, eaves at 2.6 m, as the
// photos show them over the doors, and a roof of 49 degrees. Places along the walls are measured on the photos with
// their perspective taken out, and where photos of the same wall differ, averaged.
// Front (+v), the street side (B405_b, 10L): black boarding between brick corners. From the end with the pergola: a
// tall window from 0.20 to 0.33 of the wall, the dormer over the middle, then a frosted light and the door from 0.59
// to 0.73; vents near either gable.
// Right-hand gable (+u, B405): brick within white boarding; a dark door near the front, a wide window with shutters
// towards the back; two windows above, with a shutter either side of the pair, as on the other gable.
// Left-hand gable (-u, 10La, 10Lb, B405_c): French doors towards the back, a window with one large shutter towards
// the front; the pergola over the terrace, as far from the one corner of the gable as from the other, and a glass screen at
// its front end.
// Back (-v): not photographed; a wide window, as the earlier models had it.
#include "ModelKit.h"
#include "Models.h"

#include "Mesh.h"

using namespace ModelKit;

namespace
{
    const Shape shape10L = { 3.00, 0.50, degrees(49.0), 0.35, 0.30, 0.15, 0.0, 1.0 };
    const Paint paint10L = {
        Color(0.11f, 0.11f, 0.12f),   // black boarding on the long walls
        Color(0.84f, 0.83f, 0.78f),   // pale plinth
        Color(0.17f, 0.18f, 0.20f),   // black tiles
        Color(0.17f, 0.18f, 0.20f),
        Color(0.47f, 0.25f, 0.20f),   // red-brown brick gables
        Color(0.94f, 0.93f, 0.88f),   // white verge boards and frames
        Color(0.32f, 0.38f, 0.42f),
        Color(0.22f, 0.25f, 0.27f),   // dark grey doors
        Color(0.22f, 0.23f, 0.25f),   // dark grey shutters
        Color(0.11f, 0.11f, 0.12f) }; // black vents
    const double l10Length = 12.60;   // the plan of the BGT [m]
    const double l10Width = 8.05;
    const double l10Sill2 = 3.50;     // windows of the upper floor in the gables [m]
    const double l10Head2 = 4.65;
    const Color darkFrame(0.24f, 0.25f, 0.28f);
    const Color frosted(0.80f, 0.82f, 0.82f);
    const Color wood(0.55f, 0.52f, 0.47f);
    const Color screenGlass(0.70f, 0.76f, 0.78f);
    const Color pipeGrey(0.60f, 0.62f, 0.64f);

    // A glass screen in a dark frame, standing out from a side at a, from the wall to depth
    void glassScreen(Builder& b, Side side, double a, double depth, double height)
    {
        Mesh& mesh = b.mesh();
        const double bar = 0.03;
        const double foot = 0.10;
        mesh.addBeam(b.onWall(side, a, 0.0, depth), b.onWall(side, a, height, depth), bar, darkFrame);
        mesh.addBeam(b.onWall(side, a, height, 0.05), b.onWall(side, a, height, depth), bar, darkFrame);
        mesh.addBeam(b.onWall(side, a, foot, 0.05), b.onWall(side, a, foot, depth), bar, darkFrame);
        mesh.addQuad(b.onWall(side, a, foot, 0.05), b.onWall(side, a, foot, depth), b.onWall(side, a, height, depth),
            b.onWall(side, a, height, 0.05), screenGlass, b.outward(Side::Front));
    }

    void buildLuxe10L(Builder& b, const Paint& paint)
    {
        const double hl = b.halfLength();
        const double hw = b.halfWidth();
        const double plinth = shape10L.m_plinthHeight;
        const double wall = shape10L.m_wallHeight;
        b.walls();
        // Gables of brick within white boarding, two windows above with a shutter either side of the pair; brick
        // round the corners of the long walls
        for (const Side side : { Side::Left, Side::Right })
        {
            b.panel(side, -hw, hw, plinth, wall, paint.m_gable, 0.01);
            b.gable(side, paint.m_trim, 0.01, 0.0);
            b.gable(side, paint.m_gable, 0.03, 0.18);
            b.window(side, -0.34*hw, -0.06*hw, l10Sill2, l10Head2, 1, 1, paint.m_trim, paint.m_glass, 0.01);
            b.window(side,  0.06*hw,  0.34*hw, l10Sill2, l10Head2, 1, 1, paint.m_trim, paint.m_glass, 0.01);
            b.shutters(side, -0.34*hw, 0.34*hw, l10Sill2, l10Head2, 0.75);
        }
        for (const Side side : { Side::Front, Side::Back })
        {
            b.panel(side, -hl, -hl + 0.25, plinth, wall, paint.m_gable, 0.01);
            b.panel(side, hl - 0.25, hl, plinth, wall, paint.m_gable, 0.01);
            b.downpipe(side, -hl + 0.12, pipeGrey, false);
            b.downpipe(side, hl - 0.35, pipeGrey, false);
        }
        b.roof();
        // Right-hand gable as in B405: the dark door near the front corner, with a small window, and the wide
        // window with its shutters
        b.door(Side::Right, 0.19*hw, 0.42*hw, 0.05, 2.30, paint.m_trim, paint.m_door, false, paint.m_glass, 0.01);
        b.panel(Side::Right, 0.27*hw, 0.33*hw, 1.75, 1.95, paint.m_glass, 0.11);
        b.window(Side::Right, -0.49*hw, -0.02*hw, 1.05, 2.25, 2, 1, paint.m_trim, paint.m_glass, 0.01);
        b.shutters(Side::Right, -0.49*hw, -0.02*hw, 1.05, 2.25, 0.70);
        // Left-hand gable as in 10La, 10Lb and B405_c: French doors in a white frame towards the back, the window
        // with one large shutter towards the front, the terrace under the pergola and the glass screen
        b.panel(Side::Left, -0.90*hw, -0.34*hw, 0.0, 2.41, paint.m_trim, 0.02);
        b.window(Side::Left, -0.88*hw, -0.36*hw, 0.05, 2.35, 2, 1, darkFrame, paint.m_glass, 0.02);
        b.window(Side::Left, 0.40*hw, 0.62*hw, 1.15, 2.35, 1, 1, paint.m_trim, paint.m_glass, 0.01);
        b.panel(Side::Left, 0.17*hw, 0.39*hw, 1.15, 2.35, paint.m_shutter, 0.06);
        // The pergola leaves a tenth of the gable free at either corner; the terrace lies under it, out to the ends
        // of its beam and its laths, 0.3 m beyond its posts
        b.terrace(Side::Left, -0.80*hw - 0.3, 0.80*hw + 0.3, 3.3, terraceGrey);
        b.pergola(Side::Left, -0.80*hw, 0.80*hw, 3.0, 2.95, wood);
        glassScreen(b, Side::Left, 0.66*hw, 2.0, 1.60);
        // Front as in B405_b and 10L: the tall window near the pergola, the frosted light and the door, the dormer
        // over the middle, vents near either gable
        b.window(Side::Front, -0.60*hl, -0.34*hl, 0.05, 2.35, 2, 1, darkFrame, paint.m_glass);
        b.window(Side::Front, 0.17*hl, 0.32*hl, 0.05, 2.35, 1, 1, paint.m_trim, frosted);
        b.door(Side::Front, 0.32*hl, 0.46*hl, 0.05, 2.35, paint.m_trim, paint.m_door, false, paint.m_glass);
        b.dormer(-0.18*hl, 0.18*hl, 4.20, 5.35, 3, paint.m_roof.scaled(0.85f), paint.m_roof);
        b.pipe( 0.68*hl, 0.62*hw, 0.55, 0.09);
        b.pipe( 0.74*hl, 0.62*hw, 0.70, 0.07);
        b.pipe( 0.79*hl, 0.62*hw, 0.50, 0.07);
        b.pipe(-0.78*hl, 0.70*hw, 0.60, 0.06);
        b.pipe(-0.74*hl, 0.70*hw, 0.35, 0.05);
        // Back: not photographed
        b.window(Side::Back, -0.55*hl, 0.55*hl, 0.62, 2.35, 4, 1, paint.m_trim, paint.m_glass);
    }

    // Class Luxe10L
    class Luxe10L : public GableModel
    {
    public:
        Luxe10L()
            :
            GableModel(shape10L, paint10L)
        {
        }

        // Accessor
        const char* name() const override { return "Luxe10L"; }
        const char* label() const override { return "10L"; }
        // The plan of the BGT, the same for every 10L and 10LT, whatever the outline of the roof in the AHN
        std::pair<double, double> footprint(double, double) const override { return { l10Length, l10Width }; }

    protected:
        void draw(Builder& builder, const Paint& paint) const override { buildLuxe10L(builder, paint); }
    };
}

const Model& Models::luxe10L()
{
    static const Luxe10L model;
    return model;
}
