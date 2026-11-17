// Slufter3D - model Krimweg73: the white house at Krimweg 73, under hipped thatch on a skirt of orange tiles
// Author: Jan de Kort
// Created: 2026-11-18
// Krimweg73 (photos Krimweg_73_RD_118546574790, _b, _c and _d), drawn from the photographs alone; the plan from the
// BGT outline of the building, the heights from the AHN.
// Plan: the BGT outline is that of the walls: a main block of 11.05 m by 8.43 m and, against one long side, a lean-to
// of 6.55 m by 4.00 m, 0.63 m in from the entrance end. The roof of the main block reaches 0.35 m beyond its walls
// (photo _b). It has two layers, as in every photograph: below, a skirt of orange tiles at 40 deg; above it the
// thatch, which starts 0.3 m within the walls with a thick edge on the tiles and is hipped all round at 42 deg up to
// a ridge 7.0 m above the floor (AHN; the hips lie within 0.15 m of its profile), 2.6 m long, with a row of orange tiles on it and the chimney at its far end. In the middle of each
// side of the thatch stands a dormer with white framed windows, its cheeks and hood thatched (AHN: a bump in the
// middle of every side). Black brackets under the eaves. The lean-to has a grey roof that falls from under the eaves
// of the main block to 1.8 m (AHN), with a dark green fascia, and green doors at the entrance end (photo _d).
// Seen from the entrance (photo _d) the lean-to is on the left; the entrance has a glazed door beside a red one, then
// two windows in white frames. The far end has three tall windows (_b), the long side behind the lean-to a wide and
// a small window (_c), all in dark green frames. The fourth side is in no photograph: two windows, a guess.
// Colours: medians of the pixels of each kind in photos _b, _c and _d, scaled so that their white walls are 0.95;
// rendered, thatch and tiles stand to the walls as in photos _b and _d.
#include "Models.h"

#include "Color.h"
#include "Math.h"
#include "Mesh.h"
#include "Site.h"

#include <cmath>

namespace
{
    // Plan: a along the length of the main block towards the far end, b across it towards the lean-to; the origin
    // in the middle of the outline of main block and lean-to together, which is the site [m]
    const double krLength = 11.05;      // walls of the main block, along a (BGT)
    const double krDepth = 8.43;        // and across, along b (BGT)
    const double krMiddleB = -2.0;      // middle of the main block in b
    const double krLeanA0 = -4.895;     // the lean-to, along a: from 0.63 m in from the entrance end (BGT)
    const double krLeanA1 = 1.655;
    const double krLeanB1 = 6.215;      // its outer wall; its inner wall is the main block's (BGT)
    const double krOutlineLength = 12.43; // across both, along b: the length of the site
    const double krOutlineWidth = 11.05;  // along a: the width of the site

    // Heights above the floor, and the roof of the main block measured in from its edge [m]
    const double krFoundation = 1.5;    // walls reach this far into the ground
    const double krWallTop = 2.45;      // the walls end under the eaves (photo _b)
    const double krOverhang = 0.35;     // the roof beyond the walls: photo _b; the AHN has its edge 0.3 m to 0.8 m out
    const double krEdge = 2.57;         // top of the tiles at the edge of the roof
    const double krTilePitch = 40.0;    // of the skirt of tiles [deg]
    const double krTileDepth = 1.00;    // the skirt runs this far in, under the thatch
    const double krThatchIn = 0.65;     // the thatch starts this far in, 0.3 m within the walls: 0.6 m of tiles show (_b)
    const double krThatchEdge = 0.45;   // thickness of its edge
    const double krRidge = 7.00;        // top of the thatch at the ridge (AHN)
    const double krCap = 0.14;          // the ridge tiles above it
    // Dormers: the front stands on the edge of the thatch, its hood reaches beyond it and falls back into the roof
    const double krDormerFront = 0.70;  // in from the edge of the roof
    const double krDormerHead = 4.50;   // underside of the hood (photo _b)
    const double krHoodFront = 0.45;    // the hood reaches this far, in from the edge of the roof
    const double krHoodTop = 4.90;      // top of the hood at its front (AHN)
    const double krHoodPitch = 18.0;    // of the hood [deg]
    const double krCheek = 0.25;        // thatch beside the windows
    const double krDormerLong = 2.60;   // width of the windows of a dormer on a long side (AHN: a bump of about 2.5 m)
    const double krDormerEnd = 2.00;    // and on an end (photo _b)

    const Color krWhite(0.95f, 0.95f, 0.94f);       // painted brick, white frames
    const Color krThatch(0.32f, 0.31f, 0.27f);      // _b (0.31, 0.32, 0.34), _c (0.32, 0.31, 0.33), _d (0.29, 0.29, 0.26); less blue, as the world lights it bluer
    // _b (0.73, 0.47, 0.38), _c (0.68, 0.42, 0.36), _d (0.78, 0.48, 0.37), taken down by 0.72: the photographs are
    // overcast, and in the sun of the world the tiles, which face the sky, came out that much too bright beside the walls
    const Color krTiles(0.54f, 0.33f, 0.26f);
    const Color krSoffit(0.22f, 0.22f, 0.22f);
    const Color krBracket(0.12f, 0.12f, 0.12f);
    const Color krFrame(0.18f, 0.26f, 0.22f);       // dark green frames, _b and _c
    const Color krGreen(0.30f, 0.38f, 0.35f);       // the doors of the lean-to, _d
    const Color krGlass(0.24f, 0.27f, 0.30f);
    const Color krRed(0.62f, 0.16f, 0.14f);         // the front door, _d
    const Color krLeanRoof(0.42f, 0.44f, 0.47f);    // grey roof of the lean-to, _c and _d
    const Color krChimney(0.57f, 0.53f, 0.55f);     // _b

    double tanDegrees(double degrees) { return std::tan(degrees * 3.14159265358979323846 / 180.0); }

    // Top of the tiles and of the thatch, s in from the edge of the roof [m]
    double tilesAt(double s) { return krEdge + s * tanDegrees(krTilePitch); }
    double thatchStart() { return tilesAt(krThatchIn) - 0.08 + krThatchEdge; }
    double thatchSlope() { return (krRidge - thatchStart()) / (0.5 * krDepth + krOverhang - krThatchIn); }
    double thatchAt(double s) { return thatchStart() + (s - krThatchIn) * thatchSlope(); }
    double hoodAt(double s) { return krHoodTop + (s - krHoodFront) * tanDegrees(krHoodPitch); }

    // Struct Side: one side of the main block
    struct Side
    {
        double m_da;    // outward, in the plan
        double m_db;
        double m_reach; // from the middle of the main block to the wall
        double m_half;  // half the length of the wall
    };
    const Side krSides[] = {
        {  1.0,  0.0, 0.5 * krLength, 0.5 * krDepth },   // the far end, three windows
        { -1.0,  0.0, 0.5 * krLength, 0.5 * krDepth },   // the entrance
        {  0.0,  1.0, 0.5 * krDepth, 0.5 * krLength },   // the side of the lean-to
        {  0.0, -1.0, 0.5 * krDepth, 0.5 * krLength } }; // the side in no photograph

    // Class Plan: draws in the plan of the house
    class Plan
    {
    public:
        Plan(Mesh& mesh, const Site& site)
            :
            m_mesh(mesh)
            , m_site(site)
        {
        }

        // Accessor
        Vec3 at(double a, double b, double y) const
        {
            return m_site.m_floor + m_site.m_axisLength * (-b) + m_site.m_axisWidth * (-a) + Vec3(0.0, y, 0.0);
        }
        Vec3 dir(double da, double db) const { return m_site.m_axisLength * (-db) + m_site.m_axisWidth * (-da); }
        // A point s in from the edge of the roof on a side, x along it (to the right seen from outside)
        Vec3 onRoof(const Side& side, double s, double x, double y) const
        {
            const double out = side.m_reach + krOverhang - s;
            return at(side.m_da * out - side.m_db * x, krMiddleB + side.m_db * out + side.m_da * x, y);
        }
        Vec3 outward(const Side& side) const { return dir(side.m_da, side.m_db); }

        // Mutator
        void quad(const Vec3& p, const Vec3& q, const Vec3& r, const Vec3& s, const Color& color, const Vec3& out)
        {
            m_mesh.addQuad(p, q, r, s, color, out);
        }
        void triangle(const Vec3& p, const Vec3& q, const Vec3& r, const Color& color, const Vec3& out)
        {
            m_mesh.addTriangle(p, q, r, color, out);
        }
        // A level band of the roof on a side, from s0 at height y0 to s1 at y1; its ends follow the hips
        void band(const Side& side, double s0, double y0, double s1, double y1, const Color& color, const Vec3& out)
        {
            const double h0 = side.m_half + krOverhang - s0;
            const double h1 = side.m_half + krOverhang - s1;
            if (h1 < 1.0e-6)
            {
                triangle(onRoof(side, s0, -h0, y0), onRoof(side, s0, h0, y0), onRoof(side, s1, 0.0, y1), color, out);
                return;
            }
            quad(onRoof(side, s0, -h0, y0), onRoof(side, s0, h0, y0), onRoof(side, s1, h1, y1),
                onRoof(side, s1, -h1, y1), color, out);
        }
        // A rectangle on the wall of the main block, out in front of it, from x0 to x1 along it
        void onWall(const Side& side, double x0, double x1, double y0, double y1, double out, const Color& color)
        {
            const double s = krOverhang - out;
            quad(onRoof(side, s, x0, y0), onRoof(side, s, x1, y0), onRoof(side, s, x1, y1), onRoof(side, s, x0, y1),
                color, outward(side));
        }
        // A window of columns by rows panes in a frame, on a wall of the main block
        void window(const Side& side, double x0, double x1, double y0, double y1, int columns, int rows,
            const Color& frame)
        {
            const double bar = 0.06;
            onWall(side, x0, x1, y0, y1, 0.02, frame);
            const double w = (x1 - x0 - (columns + 1) * bar) / columns;
            const double h = (y1 - y0 - (rows + 1) * bar) / rows;
            for (int i = 0; i < columns; ++i)
            {
                for (int j = 0; j < rows; ++j)
                {
                    const double px = x0 + bar + i * (w + bar);
                    const double py = y0 + bar + j * (h + bar);
                    onWall(side, px, px + w, py, py + h, 0.035, krGlass);
                }
            }
        }
        // A box of half sizes along a, up and along b
        void box(double a, double b, double y, double ha, double hy, double hb, const Color& color)
        {
            m_mesh.addBox(at(a, b, y), dir(1.0, 0.0), Vec3(0.0, 1.0, 0.0), dir(0.0, 1.0), Vec3(ha, hy, hb), color);
        }

    private:
        Mesh& m_mesh;
        const Site& m_site;
    };

    void walls(Plan& p)
    {
        for (const Side& side : krSides)
        {
            p.onWall(side, -side.m_half, side.m_half, -krFoundation, krWallTop, 0.0, krWhite);
        }
    }

    // The roof of the main block: soffit, tiles, the edge and slopes of the thatch, ridge tiles and chimney
    void roof(Plan& p)
    {
        const Vec3 up(0.0, 1.0, 0.0);
        for (const Side& side : krSides)
        {
            const Vec3 out = p.outward(side);
            const double h = side.m_half + krOverhang;
            p.band(side, 0.0, krWallTop, krOverhang, krWallTop, krSoffit, -up);
            p.quad(p.onRoof(side, 0.0, -h, krWallTop), p.onRoof(side, 0.0, h, krWallTop), p.onRoof(side, 0.0, h, krEdge),
                p.onRoof(side, 0.0, -h, krEdge), krTiles.scaled(0.8f), out);
            p.band(side, 0.0, krEdge, krTileDepth, tilesAt(krTileDepth), krTiles, out + up);
            const double t = h - krThatchIn;
            const double low = thatchStart() - krThatchEdge;
            p.quad(p.onRoof(side, krThatchIn, -t, low), p.onRoof(side, krThatchIn, t, low),
                p.onRoof(side, krThatchIn, t, thatchStart()), p.onRoof(side, krThatchIn, -t, thatchStart()),
                krThatch.scaled(0.85f), out);
            p.band(side, krThatchIn, thatchStart(), 0.5 * krDepth + krOverhang, krRidge, krThatch, out + up);
        }
        { // Ridge tiles: a low ridge of their own along the ridge, closed at the ends
            const double half = 0.5 * (krLength - krDepth) + 0.12;
            const double wide = 0.26;
            const Vec3 top0 = p.at(-half, krMiddleB, krRidge + krCap);
            const Vec3 top1 = p.at(half, krMiddleB, krRidge + krCap);
            for (double sign : { -1.0, 1.0 })
            {
                p.quad(top0, top1, p.at(half, krMiddleB + sign * wide, krRidge - 0.06),
                    p.at(-half, krMiddleB + sign * wide, krRidge - 0.06), krTiles, p.dir(0.0, sign) + up);
                p.triangle(p.at(sign * half, krMiddleB - wide, krRidge - 0.06), p.at(sign * half, krMiddleB + wide, krRidge - 0.06),
                    sign < 0.0 ? top0 : top1, krTiles, p.dir(sign, 0.0));
            }
        }
        { // Chimney at the far end of the ridge (AHN, photos _b and _c)
            p.box(1.20, krMiddleB, 7.05, 0.28, 0.60, 0.24, krChimney);
            p.box(1.20, krMiddleB, 7.68, 0.33, 0.05, 0.29, krBracket);
        }
    }

    // A dormer in the middle of a side: white framed windows under a thatched hood, between thatched cheeks
    void dormer(Plan& p, const Side& side, double width, int panes)
    {
        const Vec3 up(0.0, 1.0, 0.0);
        const Vec3 out = p.outward(side);
        const Vec3 right = p.dir(-side.m_db, side.m_da);
        const double in = 0.5 * width;
        const double cheek = in + krCheek;
        const double sill = thatchAt(krDormerFront) - 0.15;
        // The hood falls back into the thatch where they are as high
        const double back = (krHoodTop - krHoodFront * tanDegrees(krHoodPitch) - thatchStart() + krThatchIn * thatchSlope())
            / (thatchSlope() - tanDegrees(krHoodPitch));
        { // Windows: a white frame, panes in it
            p.quad(p.onRoof(side, krDormerFront, -in, sill), p.onRoof(side, krDormerFront, in, sill),
                p.onRoof(side, krDormerFront, in, krDormerHead), p.onRoof(side, krDormerFront, -in, krDormerHead), krWhite, out);
            const double bar = 0.08;
            const double w = (width - (panes + 1) * bar) / panes;
            const double s = krDormerFront - 0.02;
            for (int i = 0; i < panes; ++i)
            {
                const double x = -in + bar + i * (w + bar);
                p.quad(p.onRoof(side, s, x, sill + 0.12), p.onRoof(side, s, x + w, sill + 0.12),
                    p.onRoof(side, s, x + w, krDormerHead - 0.10), p.onRoof(side, s, x, krDormerHead - 0.10), krGlass, out);
            }
        }
        for (double sign : { -1.0, 1.0 })
        { // Cheeks: their front beside the windows, and their side down to the thatch
            const double x0 = sign * in;
            const double x1 = sign * cheek;
            p.quad(p.onRoof(side, krDormerFront, x0, sill), p.onRoof(side, krDormerFront, x1, sill),
                p.onRoof(side, krHoodFront, x1, krDormerHead), p.onRoof(side, krHoodFront, x0, krDormerHead), krThatch, out);
            p.quad(p.onRoof(side, krDormerFront, x1, sill), p.onRoof(side, krHoodFront, x1, krDormerHead),
                p.onRoof(side, krHoodFront, x1, krHoodTop), p.onRoof(side, back, x1, hoodAt(back)), krThatch, right * sign);
        }
        { // Hood: its edge, its underside, its top
            p.quad(p.onRoof(side, krHoodFront, -cheek, krDormerHead), p.onRoof(side, krHoodFront, cheek, krDormerHead),
                p.onRoof(side, krHoodFront, cheek, krHoodTop), p.onRoof(side, krHoodFront, -cheek, krHoodTop),
                krThatch.scaled(0.85f), out);
            p.quad(p.onRoof(side, krHoodFront, -in, krDormerHead), p.onRoof(side, krHoodFront, in, krDormerHead),
                p.onRoof(side, krDormerFront, in, krDormerHead), p.onRoof(side, krDormerFront, -in, krDormerHead),
                krThatch.scaled(0.7f), -up);
            p.quad(p.onRoof(side, krHoodFront, -cheek, krHoodTop), p.onRoof(side, krHoodFront, cheek, krHoodTop),
                p.onRoof(side, back, cheek, hoodAt(back)), p.onRoof(side, back, -cheek, hoodAt(back)), krThatch, out + up);
        }
    }

    // Black brackets under the eaves, about 0.9 m apart; none where the lean-to stands
    void brackets(Plan& p)
    {
        for (const Side& side : krSides)
        {
            const int count = (int) std::floor(2.0 * side.m_half / 0.9);
            const double step = 2.0 * side.m_half / count;
            for (int i = 0; i <= count; ++i)
            {
                const double x = -side.m_half + i * step;
                const double a = side.m_da * (side.m_reach + 0.17) - side.m_db * x;
                const double b = krMiddleB + side.m_db * (side.m_reach + 0.17) + side.m_da * x;
                if (side.m_db > 0.0 && a > krLeanA0 - 0.3 && a < krLeanA1 + 0.3)
                {
                    continue;
                }
                const bool across = side.m_da != 0.0;
                p.box(a, b, krWallTop - 0.10, across ? 0.17 : 0.035, 0.10, across ? 0.035 : 0.17, krBracket);
            }
        }
    }

    // The windows and doors of the main block
    void openings(Plan& p)
    {
        const Side& far = krSides[0];
        const Side& entrance = krSides[1];
        const Side& lean = krSides[2];
        const Side& hidden = krSides[3];
        // Along a side x runs to the right seen from outside: at the far end x = b + 2, at the entrance -(b + 2), on
        // the side of the lean-to -a, on the fourth side a
        { // Far end, photo _b: three tall windows with a transom
            p.window(far, -2.355, -1.395, 0.62, 2.15, 1, 2, krFrame);
            p.window(far, -0.755, 0.325, 0.62, 2.15, 1, 2, krFrame);
            p.window(far, 1.875, 2.975, 0.62, 2.15, 1, 2, krFrame);
        }
        { // Entrance, photo _d: glazed door and red door in a white frame, then two windows in white frames
            p.onWall(entrance, -3.085, -0.685, 0.0, 2.25, 0.02, krWhite);
            p.onWall(entrance, -2.985, -1.785, 0.55, 2.15, 0.035, krGlass);
            p.onWall(entrance, -1.665, -0.765, 0.0, 2.15, 0.035, krRed);
            p.onWall(entrance, -1.58, -0.85, 1.35, 2.00, 0.045, krGlass);
            p.window(entrance, 0.425, 1.685, 0.95, 2.15, 2, 1, krWhite);
            p.window(entrance, 2.245, 2.755, 1.55, 2.10, 1, 1, krWhite);
        }
        { // Side of the lean-to, beyond it, photo _c: a wide window and a small one
            p.window(lean, -4.755, -2.955, 0.95, 2.15, 2, 1, krFrame);
            p.window(lean, -2.405, -2.105, 1.55, 2.10, 1, 1, krFrame);
        }
        { // The fourth side, in no photograph: two windows
            p.window(hidden, -3.045, -1.645, 0.95, 2.15, 2, 1, krFrame);
            p.window(hidden, 1.655, 3.055, 0.95, 2.15, 2, 1, krFrame);
        }
    }

    // The lean-to: white walls under a grey roof that falls away from the main block, green doors at its end
    void leanTo(Plan& p)
    {
        const Vec3 up(0.0, 1.0, 0.0);
        const double b0 = krMiddleB + 0.5 * krDepth;
        const double reach = 0.15;
        const double high = 2.38;   // top of the roof against the main block, under its eaves
        const double low = 1.80;    // and at its outer edge (AHN)
        const double slope = (high - low) / (krLeanB1 + reach - b0);
        const auto top = [&](double b) { return high - (b - b0) * slope; };
        const auto under = [&](double b) { return top(b) - 0.10; };
        { // Walls
            p.quad(p.at(krLeanA0, krLeanB1, -krFoundation), p.at(krLeanA1, krLeanB1, -krFoundation),
                p.at(krLeanA1, krLeanB1, under(krLeanB1)), p.at(krLeanA0, krLeanB1, under(krLeanB1)), krWhite, p.dir(0.0, 1.0));
            for (double a : { krLeanA0, krLeanA1 })
            {
                p.quad(p.at(a, b0, -krFoundation), p.at(a, krLeanB1, -krFoundation), p.at(a, krLeanB1, under(krLeanB1)),
                    p.at(a, b0, under(b0)), krWhite, p.dir(a < 0.0 ? -1.0 : 1.0, 0.0));
            }
        }
        { // Roof: top, underside and a dark green fascia round it
            const double a0 = krLeanA0 - reach;
            const double a1 = krLeanA1 + reach;
            const double b1 = krLeanB1 + reach;
            p.quad(p.at(a0, b0, top(b0)), p.at(a1, b0, top(b0)), p.at(a1, b1, top(b1)), p.at(a0, b1, top(b1)), krLeanRoof, up);
            p.quad(p.at(a0, b0, under(b0)), p.at(a1, b0, under(b0)), p.at(a1, b1, under(b1)), p.at(a0, b1, under(b1)),
                krSoffit, -up);
            p.quad(p.at(a0, b1, under(b1) - 0.04), p.at(a1, b1, under(b1) - 0.04), p.at(a1, b1, top(b1)), p.at(a0, b1, top(b1)),
                krFrame, p.dir(0.0, 1.0));
            for (double a : { a0, a1 })
            {
                p.quad(p.at(a, b0, under(b0) - 0.04), p.at(a, b1, under(b1) - 0.04), p.at(a, b1, top(b1)), p.at(a, b0, top(b0)),
                    krFrame, p.dir(a < 0.0 ? -1.0 : 1.0, 0.0));
            }
        }
        { // Green doors at the entrance end, two leaves with white rails, photo _d
            const Vec3 out = p.dir(-1.0, 0.0);
            const double a = krLeanA0 - 0.03;
            const double door0 = 2.715;
            const double door1 = 5.215;
            p.quad(p.at(a, door0, 0.0), p.at(a, door1, 0.0), p.at(a, door1, 1.72), p.at(a, door0, 1.72), krGreen, out);
            for (double rail : { 0.30, 1.30 })
            {
                p.quad(p.at(a - 0.01, door0, rail), p.at(a - 0.01, door1, rail), p.at(a - 0.01, door1, rail + 0.08),
                    p.at(a - 0.01, door0, rail + 0.08), krWhite, out);
            }
            const double middle = 0.5 * (door0 + door1);
            p.quad(p.at(a - 0.01, middle - 0.02, 0.0), p.at(a - 0.01, middle + 0.02, 0.0), p.at(a - 0.01, middle + 0.02, 1.72),
                p.at(a - 0.01, middle - 0.02, 1.72), krFrame, out);
        }
        { // Two small windows in its outer wall near the entrance end, photos _c and _d
            const Vec3 out = p.dir(0.0, 1.0);
            const double b = krLeanB1 + 0.02;
            for (double a0 : { -4.345, -3.245 })
            {
                p.quad(p.at(a0, b, 0.80), p.at(a0 + 0.50, b, 0.80), p.at(a0 + 0.50, b, 1.45), p.at(a0, b, 1.45), krWhite, out);
                p.quad(p.at(a0 + 0.06, b + 0.015, 0.86), p.at(a0 + 0.44, b + 0.015, 0.86), p.at(a0 + 0.44, b + 0.015, 1.39),
                    p.at(a0 + 0.06, b + 0.015, 1.39), krGlass, out);
            }
        }
    }

    // Class Krimweg73
    class Krimweg73 : public Model
    {
    public:
        // Accessor
        const char* name() const override { return "Krimweg73"; }
        const char* label() const override { return "K73"; }
        // As measured on the BGT outline, whatever the outline of the AHN
        std::pair<double, double> footprint(double, double) const override
        {
            return { krOutlineLength, krOutlineWidth };
        }
        double ridgeHeight(double) const override { return krRidge + krCap; }
        // The main block and the lean-to; not the empty corners beside the lean-to
        bool covers(double, double, double du, double dv, double margin) const override
        {
            const double a = -dv;
            const double b = -du;
            const bool main = std::fabs(a) < 0.5 * krLength + margin && std::fabs(b - krMiddleB) < 0.5 * krDepth + margin;
            const bool lean = a > krLeanA0 - margin && a < krLeanA1 + margin && b > krMiddleB && b < krLeanB1 + margin;
            return main || lean;
        }

        void build(Mesh& mesh, const Site& site) const override
        {
            Plan p(mesh, site);
            walls(p);
            openings(p);
            brackets(p);
            roof(p);
            for (const Side& side : krSides)
            {
                dormer(p, side, side.m_da != 0.0 ? krDormerEnd : krDormerLong, side.m_da != 0.0 ? 2 : 3);
            }
            leanTo(p);
        }
    };
}

const Model& Models::krimweg73()
{
    static const Krimweg73 model;
    return model;
}
