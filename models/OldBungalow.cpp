// Slufter3D - OldBungalow: the thatched bungalows that stood in the dunes before the park was built
// Author: Jan de Kort
// Created: 2026-11-18
// What the three types share (photos of Bungalow Geel, Bungalow Groen and Bungalow Rood): white painted brick on a
// black foot; thatch that is hipped all round and reaches 0.4 m beyond the walls, the ends steeper than front and
// back and about half a metre lower at their edge; a short ridge under a row of half-round orange tiles. Along
// the front two vent slits, the door in the colour of the type with four panes over boards in a heavy brown
// frame under a white globe, and a window of two casements of four panes on a broad sill. Before the door a step
// of red square tiles within a row of darker red bricks (DoosA_Tray2_029).
// In the right end the glazed doors to the terrace, where the thatch is cut back to clear them; in the left end a
// narrow window under a cut in the thatch (D16T2_039); at the back windows of two lights (DoosA_Tray2_042).
// The yardstick of every photograph is its door leaf, 0.89 m by 2.03 m (DoosA_Tray2_029, square on).
#include "OldBungalow.h"

#include "ModelKit.h"
#include "PlanKit.h"

#include "Ground.h"
#include "Mesh.h"

#include <algorithm>
#include <cmath>
#include <functional>

using namespace ModelKit;

namespace
{
    using Wall = PlanBuilder::Wall;

    const double obEave = 0.40;         // thatch beyond the walls [m]
    const double obEaveLow = 2.15;      // underside of the thatch at the front and back edges; the walls end here [m]
    const double obEndEaveLow = 1.70;   // underside of the thatch at the edges of the ends [m]
    const double obThatch = 0.30;       // thickness of the thatch at its edge [m]
    const double obPitch = 42.0;        // front and back: the roof a fifth higher than the walls, Rood 22.39.12 [deg]
    const double obEndPitch = 54.0;     // the ends: steeper, which leaves the short ridges of the photographs [deg]
    const double obPlinth = 0.10;       // black band at the foot of the wall [m]
    const double obFoundation = 2.5;    // walls reach this far into the dune [m]
    const double obHead = 2.10;         // top of the windows, just under the thatch [m]
    const double obNotch = 0.10;        // a cut in the thatch reaches this far beyond what it clears [m]
    const double obLeaf = 0.89;         // width of the door leaf [m]
    const double obTile = 0.2652;       // radius of a half-round ridge tile [m]
    const double obTileLength = 0.24;   // of a ridge tile, along the ridge [m]
    const double obTileJoint = 0.01872; // between two ridge tiles, which lie end to end [m]
    const double obTileEnd = 0.09;      // the straight part of an end tile, before it curves down [m]

    const Color obWall(0.95f, 0.95f, 0.92f);        // white painted brick
    const Color obFoot(0.17f, 0.17f, 0.17f);        // black foot
    const Color obRoof(0.44f, 0.40f, 0.36f);        // weathered thatch
    const Color obCut(0.36f, 0.32f, 0.28f);         // cut edge of the thatch
    const Color obFrame(0.33f, 0.23f, 0.18f);       // dark brown frames
    const Color obGlass(0.24f, 0.27f, 0.30f);
    const Color obTiles(0.84f, 0.53f, 0.38f);       // orange ridge tiles
    const Color obSlit(0.30f, 0.31f, 0.33f);
    const Color obLamp(0.92f, 0.92f, 0.90f);
    const Color obStepTile(0.66f, 0.42f, 0.36f);    // red tiles of the doorstep
    const Color obStepBrick(0.52f, 0.30f, 0.25f);   // the darker bricks round them
    const Color obStepJoint(0.40f, 0.30f, 0.27f);
    const Color obPaving(0.55f, 0.55f, 0.54f);      // grey concrete tiles of the terraces
    const Color obBrick(0.56f, 0.34f, 0.27f);       // the chimney
    const Color obSteel(0.74f, 0.74f, 0.72f);       // door handle, the vent in the wall

    double tanOf(double angle) { return std::tan(degrees(angle)); }

    // Top of the thatch at the ridge, above the floor, for walls this wide
    double thatchRidge(double width)
    {
        return obEaveLow + obThatch + (0.5 * width + obEave) * tanOf(obPitch);
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
        const double front = tanOf(obPitch);
        const double end = tanOf(obEndPitch);
        const double ridge = obEaveLow + obThatch + front * ev;
        const double endEdge = obEndEaveLow + obThatch;
        Thatch thatch;
        thatch.m_top = {
            RoofPlane{ 0.0, -front, ridge },
            RoofPlane{ 0.0, front, ridge },
            RoofPlane{ -end, 0.0, endEdge + end * eu },
            RoofPlane{ end, 0.0, endEdge + end * eu },
            RoofPlane{ 0.0, 0.0, ridge - obTile * front } };
        thatch.m_under = {
            RoofPlane{ 0.0, 0.0, obEaveLow },
            RoofPlane{ -end, 0.0, obEndEaveLow + end * eu },
            RoofPlane{ end, 0.0, obEndEaveLow + end * eu } };
        return thatch;
    }

    // Struct Notch: a cut in the thatch beyond an end wall, between v0 and v1
    struct Notch
    {
        double m_side;  // 1->the right end, -1->the left end
        double m_v0;
        double m_v1;
    };

    Vec3 alongWall(PlanBuilder& b, const Wall& wall)
    {
        return normalize(b.onWall(wall, 1.0, 0.0, 0.0) - b.onWall(wall, 0.0, 0.0, 0.0));
    }

    // A face where the thatch is cut straight down, along a line of the plan from its underside up to its top
    void cutFace(PlanBuilder& b, const Wall& line, const Thatch& thatch)
    {
        std::vector<double> ts = b.kinks(line.m_left, line.m_right, thatch.m_top);
        const std::vector<double> more = b.kinks(line.m_left, line.m_right, thatch.m_under);
        ts.insert(ts.end(), more.begin(), more.end());
        std::sort(ts.begin(), ts.end());
        const double len = b.length(line);
        const auto at = [&](double t)
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
                b.onWall(line, ts[k] * len, thatch.top(p.u, p.v), 0.0), obCut, b.outward(line));
        }
    }

    // Paving laid on the ground before a wall, from a0 to a1 along it and depth out from it
    void paving(PlanBuilder& b, const Wall& wall, double a0, double a1, double depth, const Color& color)
    {
        const double tile = 0.6;     // the paving follows the ground in steps of this size [m]
        const double lift = 0.04;    // above the ground [m]
        const int na = std::max(1, (int) std::ceil((a1 - a0) / tile));
        const int nd = std::max(1, (int) std::ceil(depth / tile));
        const auto place = [&](int i, int j)
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

    // The doorstep (DoosA_Tray2_029): four by three red tiles of 0.30 m against the wall, within a row of darker
    // bricks on three sides, the tiles a tenth of the height of the step lower than the bricks. It lies level, just
    // under the threshold or on the highest ground under it, and its sides reach down into the ground, so that it
    // stands on the dune and does not float over it
    void doorstep(PlanBuilder& b, const Wall& wall, double middle)
    {
        const double tile = 0.30;
        const int across = 4;
        const int deep = 3;
        const double brick = 0.10;      // width of the row of bricks, and of each brick along it [m]
        const double joint = 0.012;
        const double half = 0.5 * (double) across * tile + brick;
        const double depth = (double) deep * tile + brick;
        const Vec3 up(0.0, 1.0, 0.0);
        const Vec3 along = alongWall(b, wall);
        const Vec3 out = b.outward(wall);
        double high = -1.0e9;
        double low = 1.0e9;
        for (const double a : { middle - half, middle, middle + half })
        {
            for (const double o : { 0.0, 0.5 * depth, depth })
            {
                const Vec3 p = b.onWall(wall, a, 0.0, o);
                const double ground = groundLevel(b.site(), p) - b.site().m_floor.y;
                high = std::max(high, ground);
                low = std::min(low, ground);
            }
        }
        const double top = std::max(-0.02, high + 0.03);
        const double foot = low - 0.30;
        const auto at = [&](double a, double o, double y) { return b.onWall(wall, a, y, o); };
        const auto flat = [&](double a0, double a1, double o0, double o1, double y, const Color& color)
        {
            b.mesh().addQuad(at(a0, o0, y), at(a1, o0, y), at(a1, o1, y), at(a0, o1, y), color, up);
        };
        // The tiles lie a tenth of the height of the step, above the lowest ground under it, below the bricks
        const double sunk = clamp(0.10 * (top - low), 0.01, 0.04);
        const double tileTop = top - sunk;
        const double inner = half - brick;      // the tiles, either way from the middle
        const double edge = depth - brick;      // and out from the wall
        // The beds with their joints: under the tiles, and under the bricks along the edge and down either side
        flat(middle - inner, middle + inner, 0.0, edge, tileTop - 0.004, obStepJoint);
        flat(middle - half, middle + half, edge, depth, top - 0.004, obStepJoint);
        flat(middle - half, middle - inner, 0.0, edge, top - 0.004, obStepJoint);
        flat(middle + inner, middle + half, 0.0, edge, top - 0.004, obStepJoint);
        // The inner faces of the bricks, down to the tiles
        b.mesh().addQuad(at(middle - inner, edge, tileTop), at(middle + inner, edge, tileTop), at(middle + inner, edge, top),
            at(middle - inner, edge, top), obStepBrick.scaled(0.8f), -out);
        for (const double side : { -1.0, 1.0 })
        {
            b.mesh().addQuad(at(middle + side * inner, 0.0, tileTop), at(middle + side * inner, edge, tileTop),
                at(middle + side * inner, edge, top), at(middle + side * inner, 0.0, top), obStepBrick.scaled(0.8f), along * (-side));
        }
        // The outer sides, down into the ground
        b.mesh().addQuad(at(middle - half, depth, foot), at(middle + half, depth, foot), at(middle + half, depth, top),
            at(middle - half, depth, top), obStepBrick.scaled(0.9f), out);
        for (const double side : { -1.0, 1.0 })
        {
            b.mesh().addQuad(at(middle + side * half, 0.0, foot), at(middle + side * half, depth, foot),
                at(middle + side * half, depth, top), at(middle + side * half, 0.0, top), obStepBrick.scaled(0.9f), along * side);
        }
        // The tiles, every other one a little darker
        for (int i = 0; i < across; ++i)
        {
            for (int j = 0; j < deep; ++j)
            {
                const double a0 = middle - half + brick + (double) i * tile;
                const double o0 = (double) j * tile;
                flat(a0 + 0.5 * joint, a0 + tile - 0.5 * joint, o0 + 0.5 * joint, o0 + tile - 0.5 * joint, tileTop,
                    obStepTile.scaled((i + j) % 2 == 0 ? 1.0f : 0.95f));
            }
        }
        // The bricks: along the outer edge, and down either side
        const int alongEdge = (int) std::lround(2.0 * half / brick);
        for (int k = 0; k < alongEdge; ++k)
        {
            const double a0 = middle - half + (double) k * brick;
            flat(a0 + 0.5 * joint, a0 + brick - 0.5 * joint, depth - brick + 0.5 * joint, depth, top, obStepBrick);
        }
        const int alongSide = (int) std::lround((depth - brick) / brick);
        for (const double side : { -1.0, 1.0 })
        {
            const double a0 = side > 0.0 ? middle + half - brick : middle - half;
            for (int k = 0; k < alongSide; ++k)
            {
                const double o0 = (double) k * brick;
                flat(a0 + (side > 0.0 ? 0.5 * joint : 0.0), a0 + brick - (side > 0.0 ? 0.0 : 0.5 * joint), o0 + 0.5 * joint,
                    o0 + brick - 0.5 * joint, top, obStepBrick);
            }
        }
    }

    // The front door (DoosA_Tray2_029): a heavy brown frame up to the thatch, a leaf of five boards under four
    // panes, the handle on the right
    void frontDoor(PlanBuilder& b, const Wall& wall, double d0, double d1, const Color& leaf)
    {
        const double frame = 0.10;
        const double top = 2.03;
        const double stile = 0.11;
        const double bar = 0.05;
        const double glazed0 = 0.83;
        const double glazed1 = 1.93;
        b.panel(wall, d0 - frame, d1 + frame, 0.0, obEaveLow, obFrame, 0.02);
        b.panel(wall, d0, d1, 0.0, top, leaf, 0.04);
        for (int k = 1; k <= 5; ++k)
        {
            const double y = 0.14 * (double) k;
            b.panel(wall, d0 + 0.03, d1 - 0.03, y - 0.008, y + 0.008, leaf.scaled(0.82f), 0.05);
        }
        const double paneWidth = 0.5 * (d1 - d0 - 2.0 * stile - bar);
        const double paneHeight = 0.5 * (glazed1 - glazed0 - bar);
        for (int c = 0; c < 2; ++c)
        {
            for (int r = 0; r < 2; ++r)
            {
                const double a = d0 + stile + (double) c * (paneWidth + bar);
                const double y = glazed0 + (double) r * (paneHeight + bar);
                b.panel(wall, a, a + paneWidth, y, y + paneHeight, obGlass, 0.06);
            }
        }
        b.panel(wall, d1 - 0.09, d1 - 0.04, 0.93, 1.13, obSteel, 0.065);
        b.panel(wall, d1 - 0.20, d1 - 0.06, 1.03, 1.06, obSteel, 0.075);
    }

    // A window in a brown frame on a broad sill: casements side by side, each of columns by rows panes
    void casementWindow(PlanBuilder& b, const Wall& wall, double a0, double a1, double y0, double y1, int casements,
        int columns, int rows)
    {
        const double border = 0.07;
        const double rail = 0.15;       // the bottom of the frame, broad in the photographs
        const double mullion = 0.10;
        const double bar = 0.04;
        b.panel(wall, a0, a1, y0, y1, obFrame, 0.02);
        b.mesh().addBox(b.onWall(wall, 0.5 * (a0 + a1), y0 + 0.03, 0.05), alongWall(b, wall), Vec3(0.0, 1.0, 0.0),
            b.outward(wall), Vec3(0.5 * (a1 - a0) + 0.03, 0.04, 0.06), obFrame);
        const double g0 = y0 + rail;
        const double g1 = y1 - border;
        const double casement = (a1 - a0 - 2.0 * border - (double) (casements - 1) * mullion) / (double) casements;
        const double paneWidth = (casement - (double) (columns - 1) * bar) / (double) columns;
        const double paneHeight = (g1 - g0 - (double) (rows - 1) * bar) / (double) rows;
        for (int c = 0; c < casements; ++c)
        {
            const double c0 = a0 + border + (double) c * (casement + mullion);
            for (int i = 0; i < columns; ++i)
            {
                for (int j = 0; j < rows; ++j)
                {
                    const double a = c0 + (double) i * (paneWidth + bar);
                    const double y = g0 + (double) j * (paneHeight + bar);
                    b.panel(wall, a, a + paneWidth, y, y + paneHeight, obGlass, 0.04);
                }
            }
        }
    }

    // The glazed doors to the terrace, in brown frames closed at their foot: two leaves of two by three panes
    // (Rood 23.05.13), or two leaves and a side light of one by three (DoosA_Tray2_034, where they only seem to
    // stand back from the wall: they are in its shadow under the thatch)
    void glazedDoors(PlanBuilder& b, const Wall& wall, double a0, double a1, double top, int leaves)
    {
        const double border = 0.07;
        const double mullion = 0.09;
        const double bar = 0.05;
        const double kick = 0.40;       // the frames are closed below this height [m]
        const std::vector<double> parts = leaves == 3 ? std::vector<double>{ 0.345, 0.345, 0.31 } : std::vector<double>{ 0.5, 0.5 };
        const int columns = leaves == 3 ? 1 : 2;
        b.panel(wall, a0, a1, 0.0, top + 0.04, obFrame, 0.02);
        const double inner = a1 - a0 - 2.0 * border - (double) (parts.size() - 1) * mullion;
        const double paneHeight = (top - border - kick - 2.0 * bar) / 3.0;
        double a = a0 + border;
        for (const double part : parts)
        {
            const double width = part * inner;
            const double paneWidth = (width - (double) (columns - 1) * bar) / (double) columns;
            for (int c = 0; c < columns; ++c)
            {
                for (int r = 0; r < 3; ++r)
                {
                    const double p = a + (double) c * (paneWidth + bar);
                    const double y = kick + (double) r * (paneHeight + bar);
                    b.panel(wall, p, p + paneWidth, y, y + paneHeight, obGlass, 0.04);
                }
            }
            a += width + mullion;
        }
    }

    // Half-round tiles laid end to end in a straight row from -half to half along the ridge, their edges on either
    // slope of the thatch; each end tile is closed by a quarter sphere that curves down to those edges
    void ridgeTiles(PlanBuilder& b, double ridge, double half)
    {
        const int facets = 10;                          // around the half circle of a tile
        const int rings = 5;                            // over the quarter sphere of an end tile
        const double base = ridge - obTile * tanOf(obPitch);    // the edges of the tiles, on the thatch
        const Site& site = b.site();
        const Vec3 up(0.0, 1.0, 0.0);
        const Color face = obTiles.scaled(0.85f);
        const auto at = [&](double u, double angle, double turn, double side)
        {
            const double r = obTile * std::cos(turn);
            return b.point(u + side * obTile * std::sin(turn), r * std::cos(angle), base + r * std::sin(angle));
        };
        const auto angleOf = [&](double j) { return pi * j / (double) facets; };
        const auto across = [&](double angle) { return site.m_axisWidth * std::cos(angle) + up * std::sin(angle); };
        const auto barrel = [&](double u0, double u1)
        {
            for (int j = 0; j < facets; ++j)
            {
                const double a0 = angleOf((double) j);
                const double a1 = angleOf((double) j + 1.0);
                b.mesh().addQuad(at(u0, a0, 0.0, 1.0), at(u1, a0, 0.0, 1.0), at(u1, a1, 0.0, 1.0), at(u0, a1, 0.0, 1.0),
                    obTiles, across(angleOf((double) j + 0.5)));
            }
        };
        const auto halfDisc = [&](double u, double side)
        {
            for (int j = 0; j < facets; ++j)
            {
                b.mesh().addTriangle(b.point(u, 0.0, base), at(u, angleOf((double) j), 0.0, 1.0),
                    at(u, angleOf((double) j + 1.0), 0.0, 1.0), face, site.m_axisLength * side);
            }
        };
        const auto dome = [&](double u, double side)
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
                            obTiles, outward);
                    }
                    else
                    {
                        b.mesh().addTriangle(at(u, a0, t0, side), at(u, a0, t1, side), at(u, a1, t0, side), obTiles, outward);
                    }
                }
            }
        };
        const double domeStart = std::max(0.0, half - obTile);
        const double endStart = std::max(0.0, domeStart - obTileEnd);
        for (const double side : { -1.0, 1.0 })
        {
            barrel(std::min(side * endStart, side * domeStart), std::max(side * endStart, side * domeStart));
            halfDisc(side * endStart, -side);
            dome(side * domeStart, side);
        }
        const double span = 2.0 * (endStart - obTileJoint);
        const int count = span > 0.5 * obTileLength ? std::max(1, (int) std::lround(span / obTileLength)) : 0;
        const double length = count > 0 ? (span - (double) (count - 1) * obTileJoint) / (double) count : 0.0;
        for (int k = 0; k < count; ++k)
        {
            const double u0 = -endStart + obTileJoint + (double) k * (length + obTileJoint);
            const double u1 = u0 + length;
            barrel(u0, u1);
            halfDisc(u0, -1.0);
            halfDisc(u1, 1.0);
        }
    }

    void buildOldBungalow(Mesh& mesh, const Site& site, const OldBungalowPlan& plan)
    {
        PlanBuilder b(mesh, site);
        const Vec3 up(0.0, 1.0, 0.0);
        const double hu = 0.5 * site.m_length;
        const double hv = 0.5 * site.m_width;
        const double eu = hu + obEave;                       // edge of the thatch
        const double ev = hv + obEave;
        const Thatch thatch = makeThatch(eu, ev);
        const auto top = [&](double u, double v) { return thatch.top(u, v); };
        const auto under = [&](double u, double v) { return thatch.under(u, v); };
        const double glazedFront = hv - plan.m_glazed0;      // the glazed doors, in v
        const double glazedBack = hv - plan.m_glazed1;
        const double windowBack = -hv + plan.m_endWindow0;   // the window of the left end, in v
        const double windowFront = -hv + plan.m_endWindow1;

        // The cuts in the thatch: over the glazed doors, and over the window of the left end
        const std::vector<Notch> notches = {
            Notch{ 1.0, glazedBack - obNotch, glazedFront + obNotch },
            Notch{ -1.0, windowBack - obNotch, windowFront + obNotch } };
        const auto inNotch = [&](double u, double v)
        {
            return std::any_of(notches.begin(), notches.end(), [&](const Notch& n)
                {
                    return n.m_side * u > hu && v > n.m_v0 && v < n.m_v1;
                });
        };
        // Walls, each from left to right as seen from outside: white over the black foot, up under the thatch
        const Wall front{ UV{ -hu, hv }, UV{ hu, hv } };
        const Wall right{ UV{ hu, hv }, UV{ hu, -hv } };
        const Wall back{ UV{ hu, -hv }, UV{ -hu, -hv } };
        const Wall left{ UV{ -hu, -hv }, UV{ -hu, hv } };
        const std::vector<Wall> walls = { front, right, back, left };
        for (const Wall& wall : walls)
        {
            b.panel(wall, 0.0, b.length(wall), -obFoundation, obPlinth, obFoot, 0.0);
            b.wallUpToRoof(wall, obPlinth, thatch.m_under, under, obWall, 0.0);
        }

        { // The front, from the left: two vent slits, the door under its globe, a vent low in the wall, the window;
          // the doorstep, and the terrace round it
            for (const double side : { -1.0, 1.0 })
            {
                const double a = plan.m_vents + side * 0.075;
                b.panel(front, a - 0.035, a + 0.035, 1.50, 1.95, obSlit, 0.01);
            }
            const double d0 = plan.m_doorMiddle - 0.5 * obLeaf;
            const double d1 = d0 + obLeaf;
            frontDoor(b, front, d0, d1, plan.m_door);
            b.mesh().addBox(b.onWall(front, d0 + 0.20, 2.07, 0.06), alongWall(b, front), up, b.outward(front),
                Vec3(0.07, 0.07, 0.06), obLamp);
            if (plan.m_ventBox)
            {
                b.mesh().addBox(b.onWall(front, plan.m_window0 + 0.15, 0.27, 0.03), alongWall(b, front), up, b.outward(front),
                    Vec3(0.12, 0.12, 0.03), obSteel);
            }
            casementWindow(b, front, plan.m_window0, plan.m_window1, plan.m_sill, obHead, 2, 2, 2);
            if (plan.m_frontTerrace > 0.0)
            {
                paving(b, front, plan.m_doorMiddle + 0.80, plan.m_length + 0.80, plan.m_frontTerrace, obPaving);
            }
            doorstep(b, front, plan.m_doorMiddle);
        }

        { // The right end: the glazed doors in the wall, up to where the thatch is cut back, and the terrace
            glazedDoors(b, right, plan.m_glazed0, plan.m_glazed1, under(hu, 0.5 * (glazedFront + glazedBack)) - 0.04,
                plan.m_leaves);
            if (plan.m_endTerrace > 0.0)
            {
                paving(b, right, 0.3, plan.m_width + 0.3, plan.m_endTerrace, obPaving);
            }
        }

        // The back: windows of two lights, each with one bar across
        for (const auto& [a0, a1] : plan.m_backWindows)
        {
            casementWindow(b, back, a0, a1, plan.m_backSill, obHead, 2, 1, 2);
        }
        // The left end: a narrow window of four panes, up into the cut in the thatch
        casementWindow(b, left, plan.m_endWindow0, plan.m_endWindow1, plan.m_backSill, obHead, 1, 2, 2);

        // The thatch: its planes but for the cuts, its edge all round, its underside, the faces of the cuts
        const double end = tanOf(obEndPitch);
        const double bend = (obEaveLow - obEndEaveLow) / end;       // the underside of an end slopes this far in from its edge
        const std::vector<double> us = { -eu, -eu + bend, -hu, hu, eu - bend, eu };
        std::vector<double> vs = { -ev, -hv, hv, ev };
        for (const Notch& n : notches)
        {
            vs.push_back(n.m_v0);
            vs.push_back(n.m_v1);
        }
        b.roof(gridCells(us, vs, [&](double u, double v) { return !inNotch(u, v); }), thatch.m_top, top, obRoof);
        { // The edge: along front and back, and along either end between its cuts
            b.edgeBoard(Wall{ UV{ -eu, ev }, UV{ eu, ev } }, obThatch + 0.03, thatch.m_top, top, obCut);
            b.edgeBoard(Wall{ UV{ eu, -ev }, UV{ -eu, -ev } }, obThatch + 0.03, thatch.m_top, top, obCut);
            for (const double side : { 1.0, -1.0 })
            {
                // Along the right end v runs down, along the left end up: from left to right as seen from outside
                double from = side * ev;
                for (const Notch& n : notches)
                {
                    if (n.m_side == side)
                    {
                        const double near = side > 0.0 ? n.m_v1 : n.m_v0;
                        const double far = side > 0.0 ? n.m_v0 : n.m_v1;
                        b.edgeBoard(Wall{ UV{ side * eu, from }, UV{ side * eu, near } }, obThatch + 0.03, thatch.m_top, top, obCut);
                        from = far;
                    }
                }
                b.edgeBoard(Wall{ UV{ side * eu, from }, UV{ side * eu, -side * ev } }, obThatch + 0.03, thatch.m_top, top, obCut);
            }
        }
        const auto underThatch = [&](double u, double v)
        {
            return (std::fabs(u) > hu || std::fabs(v) > hv) && !inNotch(u, v);
        };
        const Color soffit = obCut.scaled(0.8f);
        for (const std::vector<UV>& cell : gridCells(us, vs, underThatch))
        {
            b.mesh().addQuad(b.point(cell[0].u, cell[0].v, under(cell[0].u, cell[0].v)),
                b.point(cell[1].u, cell[1].v, under(cell[1].u, cell[1].v)),
                b.point(cell[2].u, cell[2].v, under(cell[2].u, cell[2].v)),
                b.point(cell[3].u, cell[3].v, under(cell[3].u, cell[3].v)), soffit, -up);
        }
        for (const Notch& n : notches)
        {
            // Round the cut, each face from left to right as seen from within it
            const double s = n.m_side;
            const double near = s > 0.0 ? n.m_v1 : n.m_v0;
            const double far = s > 0.0 ? n.m_v0 : n.m_v1;
            cutFace(b, Wall{ UV{ s * eu, near }, UV{ s * hu, near } }, thatch);
            cutFace(b, Wall{ UV{ s * hu, near }, UV{ s * hu, far } }, thatch);
            cutFace(b, Wall{ UV{ s * hu, far }, UV{ s * eu, far } }, thatch);
        }

        // The ridge: its tiles where the thatch lies level under them, and the chimney at their left end
        const double ridge = thatchRidge(site.m_width);
        const double endEdge = obEndEaveLow + obThatch;
        const double tileBase = ridge - obTile * tanOf(obPitch);
        const double tilesHalf = std::max(0.0, eu - (tileBase - endEdge) / end);
        ridgeTiles(b, ridge, tilesHalf);
        if (plan.m_chimney)
        {
            const double u = -tilesHalf + 0.10;
            b.mesh().addBox(b.point(u, 0.0, ridge + 0.08), site.m_axisLength, up, site.m_axisWidth, Vec3(0.28, 0.38, 0.28), obBrick);
            b.mesh().addBox(b.point(u, 0.0, ridge + 0.49), site.m_axisLength, up, site.m_axisWidth, Vec3(0.31, 0.03, 0.31),
                obBrick.scaled(0.8f));
            b.mesh().addBox(b.point(u, 0.0, ridge + 0.58), site.m_axisLength, up, site.m_axisWidth, Vec3(0.10, 0.06, 0.10), obSteel);
        }
    }
}

// Class OldBungalow
OldBungalow::OldBungalow(const OldBungalowPlan& plan)
    :
    m_plan(plan)
{
}

std::pair<double, double> OldBungalow::footprint(double /* outlineLength */, double /* outlineWidth */) const
{
    return { m_plan.m_length, m_plan.m_width };
}

double OldBungalow::ridgeHeight(double width) const
{
    // The top of the ridge tiles, whose edges lie on either slope of the thatch
    return thatchRidge(width) + obTile * (1.0 - tanOf(obPitch));
}

void OldBungalow::build(Mesh& mesh, const Site& site) const
{
    buildOldBungalow(mesh, site, m_plan);
}
