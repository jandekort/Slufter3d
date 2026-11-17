// Slufter3D - model Visdiefje: 1950s bungalow, white walls under thatch that comes down low at the ends
// Author: Jan de Kort
// Created: 2026-11-16
// Visdiefje (photos IMG_1812, the house itself; DoosA_Tray2_034, D16T2_038, D16T2_039 and D16T2_039_2, others of its type)
// A bungalow of the 1950s: white painted brick on a black foot, under thatch that is hipped all round and reaches
// 0.4 m beyond the walls, so the walls stand that far within the outline of the roof. The ends are steeper than
// front and back and come down about half a metre lower, so that the thatch drops at every corner; the short ridge
// is capped with a straight row of half-round tiles that lie on the thatch, with an end tile at either end that
// curves down to their edges. Heights and pitches follow the frontal photograph D16T2_038: walls 2.15 m, a roof
// about as high again, a ridge of about a third of the eaves.
// Along the front, as in IMG_1812 and placed by its brick courses: two vent slits, a lamp, the door and the window.
// The right end has the glazed doors of DoosA_Tray2_034, under a cut in the thatch that clears them, before a
// terrace. Places along a wall are parts of its length, so that the picture keeps on every outline; heights are in
// metres. Brown frames of DoosA_Tray2_034 and the orange door of D16T2_038, each brightened as much as the white wall
// of its photograph needs to match IMG_1812.
#include "Models.h"

#include "ModelKit.h"
#include "PlanKit.h"

#include "Check.h"
#include "Ground.h"
#include "Mesh.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>

using namespace ModelKit;

namespace
{
    using Wall = PlanBuilder::Wall;

    const double vdEave = 0.40;         // thatch beyond the walls [m]
    const double vdEaveLow = 2.15;      // underside of the thatch at the front and back edges; the walls end here [m]
    const double vdEndEaveLow = 1.70;   // underside of the thatch at the edges of the ends, all photographs [m]
    const double vdThatch = 0.30;       // thickness of the thatch at its edge [m]
    const double vdPitch = 33.0;        // front and back: the ridge of D16T2_038 about 2.3 m above the walls [deg]
    const double vdEndPitch = 47.0;     // the ends: steeper, which leaves the ridge of that photo, a third of the eaves [deg]
    const double vdPlinth = 0.10;       // black band at the foot of the wall [m]
    const double vdFoundation = 2.5;    // walls reach this far into the dune [m]
    const double vdTile = 0.2652;       // radius of a half-round ridge tile, D16T2_039_2 [m]
    const double vdTileLength = 0.24;   // of a ridge tile, along the ridge [m]
    const double vdTileJoint = 0.01872; // between two ridge tiles, which lie end to end [m]
    const double vdTileEnd = 0.09;      // the straight part of an end tile, before it curves down [m]

    // Along the front, as parts of its length: the recent photograph, measured along its brick courses
    const double vdVents = 0.122;       // middle of the two vent slits
    const double vdDoor = 0.277;        // middle of the door
    const double vdDoorPart = 0.143;    // width of the door leaf, kept between a narrow and a wide door
    const double vdWindow0 = 0.508;     // the window, outside its frame
    const double vdWindow1 = 0.802;
    // Along the right end, as parts of its length from the front: the glazed doors of the older photograph
    const double vdGlazed0 = 0.19;
    const double vdGlazed1 = 0.63;
    const double vdNotch = 0.15;        // the cut in the thatch reaches this far beyond the glazed doors [m]

    const Paint paintVisdiefje = {
        Color(0.95f, 0.95f, 0.92f),   // white painted brick
        Color(0.17f, 0.17f, 0.17f),   // black foot
        Color(0.50f, 0.46f, 0.41f),   // weathered thatch
        Color(0.50f, 0.46f, 0.41f),
        Color(0.36f, 0.32f, 0.28f),   // cut edge of the thatch
        Color(0.46f, 0.37f, 0.31f),   // brown frames, older photograph: (107, 85, 73) brightened
        Color(0.24f, 0.27f, 0.30f),
        Color(0.68f, 0.35f, 0.25f),   // orange door, D16T2_038: (112, 57, 43) beside a wall of (157, 154, 162)
        Color(0.36f, 0.30f, 0.25f),   // not used: no shutters
        Color(0.12f, 0.12f, 0.13f) };
    const Color visdiefjeTiles(0.84f, 0.53f, 0.38f);   // D16T2_039_2: (185, 140, 120) in the shade, and IMG_1812
    const Color visdiefjeSlit(0.30f, 0.31f, 0.33f);
    const Color visdiefjeLamp(0.92f, 0.92f, 0.90f);
    const Color visdiefjePaving(0.55f, 0.52f, 0.47f);   // older photograph: (124, 116, 104) brightened

    // Top of the thatch at the ridge, above the floor, for a house whose walls are this wide
    double visdiefjeRidge(double width)
    {
        return vdEaveLow + vdThatch + (0.5 * width + vdEave) * std::tan(degrees(vdPitch));
    }

    // Top of the ridge tiles above the thatch: their edges lie on either slope of the ridge
    double visdiefjeTileTop()
    {
        return vdTile * (1.0 - std::tan(degrees(vdPitch)));
    }

    double lowest(const std::vector<RoofPlane>& planes, double u, double v)
    {
        double h = planes.front().at(u, v);
        for (const RoofPlane& plane : planes)
        {
            h = std::min(h, plane.at(u, v));
        }
        return h;
    }

    // Struct Thatch: the top and the underside of the thatch over the plan of one house
    struct Thatch
    {
        std::vector<RoofPlane> m_top;     // front, back, right end, left end, and level where the ridge tiles lie
        std::vector<RoofPlane> m_under;   // level under front and back; along the slope of either end near its edge

        double top(double u, double v) const { return lowest(m_top, u, v); }
        double under(double u, double v) const { return lowest(m_under, u, v); }
    };

    // The thatch whose edge lies eu, ev from the middle of the plan
    Thatch makeThatch(double eu, double ev)
    {
        const double front = std::tan(degrees(vdPitch));
        const double end = std::tan(degrees(vdEndPitch));
        const double edge = vdEaveLow + vdThatch;            // top of the thatch at the front and back edges
        const double endEdge = vdEndEaveLow + vdThatch;      // and at the edges of the ends
        Thatch thatch;
        const double ridge = edge + front * ev;
        thatch.m_top = {
            RoofPlane{ 0.0, -front, ridge },                 // front
            RoofPlane{ 0.0, front, ridge },                  // back
            RoofPlane{ -end, 0.0, endEdge + end * eu },      // right end
            RoofPlane{ end, 0.0, endEdge + end * eu },       // left end
            RoofPlane{ 0.0, 0.0, ridge - vdTile * front } }; // level under the ridge tiles: the thatch ends at their edges
        thatch.m_under = {
            RoofPlane{ 0.0, 0.0, vdEaveLow },
            RoofPlane{ -end, 0.0, vdEndEaveLow + end * eu },
            RoofPlane{ end, 0.0, vdEndEaveLow + end * eu } };
        return thatch;
    }

    Vec3 alongWall(PlanBuilder& b, const Wall& wall)
    {
        return normalize(b.onWall(wall, 1.0, 0.0, 0.0) - b.onWall(wall, 0.0, 0.0, 0.0));
    }

    // A face where the thatch is cut straight down, along a line of the plan from its underside up to its top
    void cutFace(PlanBuilder& b, const Wall& line, const Thatch& thatch, const Color& color)
    {
        std::vector<double> ts = b.kinks(line.m_left, line.m_right, thatch.m_top);
        const std::vector<double> more = b.kinks(line.m_left, line.m_right, thatch.m_under);
        ts.insert(ts.end(), more.begin(), more.end());
        std::sort(ts.begin(), ts.end());
        const double len = b.length(line);
        auto at = [&](double t)
        {
            return UV{ mix(line.m_left.u, line.m_right.u, t), mix(line.m_left.v, line.m_right.v, t) };
        };
        for (size_t k = 0; k + 1 < ts.size(); ++k)
        {
            if (ts[k + 1] - ts[k] < 1.0e-9)
            {
                continue;
            }
            const UV p = at(ts[k]);
            const UV q = at(ts[k + 1]);
            b.mesh().addQuad(b.onWall(line, ts[k] * len, thatch.under(p.u, p.v), 0.0),
                b.onWall(line, ts[k + 1] * len, thatch.under(q.u, q.v), 0.0),
                b.onWall(line, ts[k + 1] * len, thatch.top(q.u, q.v), 0.0),
                b.onWall(line, ts[k] * len, thatch.top(p.u, p.v), 0.0), color, b.outward(line));
        }
    }

    // Paving laid on the ground before a wall, from a0 to a1 along it and depth out from it
    void paving(PlanBuilder& b, const Wall& wall, double a0, double a1, double depth, const Color& color)
    {
        const double tile = 0.6;     // the paving follows the ground in steps of this size [m]
        const double lift = 0.10;    // above the ground, below the roads [m]
        const int na = std::max(1, (int) std::ceil((a1 - a0) / tile));
        const int nd = std::max(1, (int) std::ceil(depth / tile));
        auto place = [&](int i, int j)
        {
            const Vec3 p = b.onWall(wall, mix(a0, a1, (double) i / (double) na), 0.0, depth * (double) j / (double) nd);
            return Vec3(p.x, groundLevel(b.site(), p) + lift, p.z);
        };
        for (int i = 0; i < na; ++i)
        {
            for (int j = 0; j < nd; ++j)
            {
                b.mesh().addQuad(place(i, j), place(i + 1, j), place(i + 1, j + 1), place(i, j + 1), color,
                    Vec3(0.0, 1.0, 0.0));
            }
        }
    }

    // The door of the photographs: a brown frame, a leaf of boards with four panes in its upper half
    void door(PlanBuilder& b, const Wall& wall, double d0, double d1, const Paint& paint)
    {
        const double frame = 0.06;
        const double top = 2.03;
        const double stile = 0.10;
        const double bar = 0.05;
        const double glazed0 = 0.88;
        const double glazed1 = 1.92;
        b.panel(wall, d0 - frame, d1 + frame, 0.0, top + frame, paint.m_trim, 0.02);
        b.panel(wall, d0, d1, 0.0, top, paint.m_door, 0.04);
        for (int k = 1; k <= 6; ++k)
        {
            const double y = 0.13 * (double) k;
            b.panel(wall, d0 + 0.03, d1 - 0.03, y - 0.008, y + 0.008, paint.m_door.scaled(0.82f), 0.05);
        }
        const double paneWidth = 0.5 * (d1 - d0 - 2.0*stile - bar);
        const double paneHeight = 0.5 * (glazed1 - glazed0 - bar);
        for (int c = 0; c < 2; ++c)
        {
            for (int r = 0; r < 2; ++r)
            {
                const double a = d0 + stile + (double) c * (paneWidth + bar);
                const double y = glazed0 + (double) r * (paneHeight + bar);
                b.panel(wall, a, a + paneWidth, y, y + paneHeight, paint.m_glass, 0.06);
            }
        }
        b.panel(wall, d1 - 0.15, d1 - 0.07, 1.00, 1.05, Color(0.75f, 0.75f, 0.73f), 0.07);
    }

    // A window of DoosA_Tray2_034: a brown frame on a broad sill, two casements of two by two panes
    void casementWindow(PlanBuilder& b, const Wall& wall, double a0, double a1, double y0, double y1, const Paint& paint)
    {
        const double border = 0.07;
        const double rail = 0.17;       // the bottom of the frame, broad in the photograph
        const double mullion = 0.10;
        const double bar = 0.04;
        b.panel(wall, a0, a1, y0, y1, paint.m_trim, 0.02);
        b.mesh().addBox(b.onWall(wall, 0.5 * (a0 + a1), y0 + 0.03, 0.05), alongWall(b, wall), Vec3(0.0, 1.0, 0.0),
            b.outward(wall), Vec3(0.5 * (a1 - a0) + 0.03, 0.04, 0.06), paint.m_trim);
        const double g0 = y0 + rail;
        const double g1 = y1 - border;
        const double casement = 0.5 * (a1 - a0 - 2.0*border - mullion);
        const double paneWidth = 0.5 * (casement - bar);
        const double paneHeight = 0.5 * (g1 - g0 - bar);
        for (int c = 0; c < 2; ++c)
        {
            const double c0 = a0 + border + (double) c * (casement + mullion);
            for (int i = 0; i < 2; ++i)
            {
                for (int j = 0; j < 2; ++j)
                {
                    const double a = c0 + (double) i * (paneWidth + bar);
                    const double y = g0 + (double) j * (paneHeight + bar);
                    b.panel(wall, a, a + paneWidth, y, y + paneHeight, paint.m_glass, 0.04);
                }
            }
        }
    }

    // The glazed doors of DoosA_Tray2_034: two leaves and a side light in brown frames, three panes high
    void glazedDoors(PlanBuilder& b, const Wall& wall, double a0, double a1, double top, const Paint& paint)
    {
        const double border = 0.07;
        const double mullion = 0.09;
        const double bar = 0.05;
        const double kick = 0.40;       // the frames are closed below this height [m]
        const double parts[] = { 0.345, 0.345, 0.31 };
        b.panel(wall, a0, a1, 0.0, top + 0.04, paint.m_trim, 0.02);   // a little into the cut of the thatch above
        const double inner = a1 - a0 - 2.0*border - 2.0*mullion;
        const double g1 = top - border;
        const double paneHeight = (g1 - kick - 2.0*bar) / 3.0;
        double a = a0 + border;
        for (const double part : parts)
        {
            const double width = part * inner;
            for (int r = 0; r < 3; ++r)
            {
                const double y = kick + (double) r * (paneHeight + bar);
                b.panel(wall, a, a + width, y, y + paneHeight, paint.m_glass, 0.04);
            }
            a += width + mullion;
        }
    }

    // The ridge of D16T2_039_2: half-round tiles laid end to end in a straight row from -half to half along the
    // ridge, straight on the thatch, their edges on either slope of it. Each end tile runs straight for vdTileEnd and
    // is then closed by a quarter sphere, which curves down to the level of the edges of the row.
    void ridgeTiles(PlanBuilder& b, double ridge, double half, const Color& tile)
    {
        const int facets = 10;                          // around the half circle of a tile
        const int rings = 5;                            // over the quarter sphere of an end tile
        const double base = ridge - vdTile * std::tan(degrees(vdPitch));   // the edges of the tiles, on the thatch
        const Site& site = b.site();
        const Vec3 up(0.0, 1.0, 0.0);
        const Color face = tile.scaled(0.85f);

        // A point of a tile: along the ridge at u, around its half circle at angle, and from the circle towards its
        // end by turn, a quarter turn being the tip of a quarter sphere
        auto at = [&](double u, double angle, double turn, double side)
        {
            const double r = vdTile * std::cos(turn);
            return b.point(u + side * vdTile * std::sin(turn), r * std::cos(angle), base + r * std::sin(angle));
        };
        auto angleOf = [&](double j) { return pi * j / (double) facets; };
        auto across = [&](double angle) { return site.m_axisWidth * std::cos(angle) + up * std::sin(angle); };
        // The rounded top of a tile from u0 to u1
        auto barrel = [&](double u0, double u1)
        {
            for (int j = 0; j < facets; ++j)
            {
                const double a0 = angleOf((double) j);
                const double a1 = angleOf((double) j + 1.0);
                b.mesh().addQuad(at(u0, a0, 0.0, 1.0), at(u1, a0, 0.0, 1.0), at(u1, a1, 0.0, 1.0), at(u0, a1, 0.0, 1.0),
                    tile, across(angleOf((double) j + 0.5)));
            }
        };
        // The end of a tile at u, facing along the ridge to one side (1: right, -1: left)
        auto halfDisc = [&](double u, double side)
        {
            for (int j = 0; j < facets; ++j)
            {
                b.mesh().addTriangle(b.point(u, 0.0, base), at(u, angleOf((double) j), 0.0, 1.0),
                    at(u, angleOf((double) j + 1.0), 0.0, 1.0), face, site.m_axisLength * side);
            }
        };
        // The quarter sphere that closes an end tile, from u towards one side
        auto dome = [&](double u, double side)
        {
            for (int i = 0; i < rings; ++i)
            {
                const double t0 = 0.5 * pi * (double) i / (double) rings;
                const double t1 = 0.5 * pi * ((double) i + 1.0) / (double) rings;
                const double middle = 0.5 * (t0 + t1);
                for (int j = 0; j < facets; ++j)
                {
                    const double a0 = angleOf((double) j);
                    const double a1 = angleOf((double) j + 1.0);
                    const Vec3 outward = site.m_axisLength * (side * std::sin(middle))
                        + across(angleOf((double) j + 0.5)) * std::cos(middle);
                    if (i + 1 < rings)
                    {
                        b.mesh().addQuad(at(u, a0, t0, side), at(u, a0, t1, side), at(u, a1, t1, side), at(u, a1, t0, side),
                            tile, outward);
                    }
                    else
                    {
                        b.mesh().addTriangle(at(u, a0, t0, side), at(u, a0, t1, side), at(u, a1, t0, side), tile, outward);
                    }
                }
            }
        };

        // The end tiles, then the others between them, end to end with a joint between each two
        const double domeStart = std::max(0.0, half - vdTile);
        const double endStart = std::max(0.0, domeStart - vdTileEnd);
        for (const double side : { -1.0, 1.0 })
        {
            barrel(std::min(side * endStart, side * domeStart), std::max(side * endStart, side * domeStart));
            halfDisc(side * endStart, -side);
            dome(side * domeStart, side);
        }
        const double span = 2.0 * (endStart - vdTileJoint);
        const int count = span > 0.5 * vdTileLength ? std::max(1, (int) std::lround(span / vdTileLength)) : 0;
        const double length = count > 0 ? (span - (double) (count - 1) * vdTileJoint) / (double) count : 0.0;
        for (int k = 0; k < count; ++k)
        {
            const double u0 = -endStart + vdTileJoint + (double) k * (length + vdTileJoint);
            const double u1 = u0 + length;
            barrel(u0, u1);
            halfDisc(u0, -1.0);
            halfDisc(u1, 1.0);
        }
    }

    void buildVisdiefje(Mesh& mesh, const Site& site, const Paint& paint)
    {
        PlanBuilder b(mesh, site);
        const double hu = 0.5 * site.m_length;
        const double hv = 0.5 * site.m_width;
        const double eu = hu + vdEave;                       // edge of the thatch
        const double ev = hv + vdEave;
        const Thatch thatch = makeThatch(eu, ev);
        auto top = [&](double u, double v) { return thatch.top(u, v); };
        auto under = [&](double u, double v) { return thatch.under(u, v); };
        const Color& frame = paint.m_trim;

        // Walls, each from left to right as seen from outside: white over the black foot, up under the thatch
        const Wall walls[] = {
            Wall{ UV{ -hu, hv }, UV{ hu, hv } },             // front, the entrance
            Wall{ UV{ hu, hv }, UV{ hu, -hv } },             // right, the glazed doors
            Wall{ UV{ hu, -hv }, UV{ -hu, -hv } },           // back
            Wall{ UV{ -hu, -hv }, UV{ -hu, hv } } };         // left
        for (const Wall& wall : walls)
        {
            b.panel(wall, 0.0, b.length(wall), -vdFoundation, vdPlinth, paint.m_plinth, 0.0);
            b.wallUpToRoof(wall, vdPlinth, thatch.m_under, under, paint.m_wall, 0.0);
        }

        { // Front as in IMG_1812, from the left: two vent slits, the lamp, the door, the window
            const Wall& wall = walls[0];
            const double len = b.length(wall);
            for (const double side : { -1.0, 1.0 })
            {
                const double a = vdVents * len + side * 0.075;
                b.panel(wall, a - 0.035, a + 0.035, 1.50, 1.92, visdiefjeSlit, 0.01);
            }
            const double doorWidth = clamp(vdDoorPart * len, 0.72, 0.85);
            const double d0 = vdDoor * len - 0.5 * doorWidth;
            const double d1 = d0 + doorWidth;
            b.mesh().addBox(b.onWall(wall, d0 - 0.24, 2.02, 0.05), alongWall(b, wall), Vec3(0.0, 1.0, 0.0),
                b.outward(wall), Vec3(0.06, 0.06, 0.05), visdiefjeLamp);
            door(b, wall, d0, d1, paint);
            paving(b, wall, d0 - 0.25, d1 + 0.25, 0.70, visdiefjePaving);
            casementWindow(b, wall, vdWindow0 * len, vdWindow1 * len, 0.50, 2.10, paint);
        }

        // The right end: the glazed doors up to the walls' top, where the thatch is cut back to clear them; a terrace
        const double endLen = b.length(walls[1]);
        const double g0 = vdGlazed0 * endLen;
        const double g1 = vdGlazed1 * endLen;
        const double notchFront = hv - (g0 - vdNotch);       // the cut in the thatch, in v
        const double notchBack = hv - (g1 + vdNotch);
        auto inNotch = [&](double u, double v)
        {
            return u > hu && v > notchBack && v < notchFront;
        };
        glazedDoors(b, walls[1], g0, g1, under(hu, 0.5 * (notchFront + notchBack)), paint);
        paving(b, walls[1], 0.10 * endLen, 0.95 * endLen, 2.20, visdiefjePaving);

        { // The back has no photograph: a window like the front one is assumed; the left end has the narrow one of IMG_1812
            const double len = b.length(walls[2]);
            casementWindow(b, walls[2], 0.37 * len, 0.63 * len, 0.80, 2.10, paint);
            const double leftLen = b.length(walls[3]);
            b.window(walls[3], 0.5 * leftLen - 0.27, 0.5 * leftLen + 0.27, 0.70, 2.08, 1, frame, paint.m_glass);
        }

        // The thatch: the planes over the eaves but for the cut, its edge all round, the underside, the cut, the ridge tiles, a flue
        const double end = std::tan(degrees(vdEndPitch));
        const double bend = (vdEaveLow - vdEndEaveLow) / end;       // the underside of an end slopes this far in from its edge
        const std::vector<double> us = { -eu, -eu + bend, -hu, hu, eu - bend, eu };
        const std::vector<double> vs = { -ev, -hv, notchBack, notchFront, hv, ev };
        b.roof(gridCells(us, vs, [&](double u, double v) { return !inNotch(u, v); }), thatch.m_top, top, paint.m_roof);
        const Wall edges[] = {
            Wall{ UV{ -eu, ev }, UV{ eu, ev } }, Wall{ UV{ eu, ev }, UV{ eu, notchFront } },
            Wall{ UV{ eu, notchBack }, UV{ eu, -ev } }, Wall{ UV{ eu, -ev }, UV{ -eu, -ev } },
            Wall{ UV{ -eu, -ev }, UV{ -eu, ev } } };
        for (const Wall& edge : edges)
        {
            b.edgeBoard(edge, vdThatch + 0.03, thatch.m_top, top, paint.m_gable);
        }
        auto underEaves = [&](double u, double v)
        {
            return (std::fabs(u) > hu || std::fabs(v) > hv) && !inNotch(u, v);
        };
        const Color soffit = paint.m_gable.scaled(0.8f);
        for (const std::vector<UV>& cell : gridCells(us, vs, underEaves))
        {
            b.mesh().addQuad(b.point(cell[0].u, cell[0].v, under(cell[0].u, cell[0].v)),
                b.point(cell[1].u, cell[1].v, under(cell[1].u, cell[1].v)),
                b.point(cell[2].u, cell[2].v, under(cell[2].u, cell[2].v)),
                b.point(cell[3].u, cell[3].v, under(cell[3].u, cell[3].v)), soffit, Vec3(0.0, -1.0, 0.0));
        }
        cutFace(b, Wall{ UV{ eu, notchFront }, UV{ hu, notchFront } }, thatch, paint.m_gable);
        cutFace(b, Wall{ UV{ hu, notchFront }, UV{ hu, notchBack } }, thatch, paint.m_gable);
        cutFace(b, Wall{ UV{ hu, notchBack }, UV{ eu, notchBack } }, thatch, paint.m_gable);

        const double ridge = visdiefjeRidge(site.m_width);
        const double endEdge = vdEndEaveLow + vdThatch;
        const double ridgeHalf = std::max(0.0, eu - (ridge - endEdge) / end);   // where the ends reach the ridge
        const double tileBase = ridge - vdTile * std::tan(degrees(vdPitch));
        // The tiles cover the level top of the thatch, to where the ends rise to it
        ridgeTiles(b, ridge, std::max(0.0, eu - (tileBase - endEdge) / end), visdiefjeTiles);
        const double flueU = -ridgeHalf - 0.3;
        const double flueV = 0.35;
        const double flueFoot = top(flueU, flueV) - 0.2;
        b.mesh().addBox(b.point(flueU, flueV, flueFoot + 0.35), site.m_axisLength, Vec3(0.0, 1.0, 0.0), site.m_axisWidth,
            Vec3(0.06, 0.35, 0.06), paint.m_pipe);
    }

    // Class Visdiefje
    class Visdiefje : public Model
    {
    public:
        // Accessor
        const char* name() const override { return "Visdiefje"; }
        const char* label() const override { return "Visdiefje"; }
        std::pair<double, double> footprint(double outlineLength, double outlineWidth) const override
        {
            // The outline is that of the thatch, which reaches vdEave beyond the walls
            const double smallest = 3.0;
            return { std::max(smallest, outlineLength - 2.0 * vdEave), std::max(smallest, outlineWidth - 2.0 * vdEave) };
        }
        double ridgeHeight(double width) const override { return visdiefjeRidge(width) + visdiefjeTileTop(); }

        void build(Mesh& mesh, const Site& site) const override { buildVisdiefje(mesh, site, paintVisdiefje); }
    };
}

const Model& Models::visdiefje()
{
    static const Visdiefje model;
    return model;
}
