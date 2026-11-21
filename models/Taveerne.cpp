// Slufter3D - model Taveerne: the reception, restaurant and shop of the park as it was before 1993
// Author: Jan de Kort
// Created: 2026-11-21
// Taveerne (photo IMG_4893, a postcard taken from the midget golf; plan: the park map IMG_4902)
// The postcard shows the long front from the south-west: a white wall under a tall thatched roof, hipped at both
// ends, with a row of wide windows in yellow frames either side of the door, three chimneys on the ridge, a
// lower hipped part with the name on it at the left end, a paved terrace and a flagpole before the front.
// The park map gives the plan: the front 27 m long, a wing (the shop) that runs back from its left end.
// Yardsticks: the people on the terrace (the wall is one and a half of them high, 2.5 m) and the length of the
// front on the park map. Of the back and of the wing there is no photograph: they are assumed plain, the wing
// with a window every four metres.
#include "Models.h"

#include "ModelKit.h"
#include "PlanKit.h"

#include "Ground.h"
#include "Mesh.h"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace ModelKit;

namespace
{
    using Wall = PlanBuilder::Wall;

    const double tvLength = 27.0;       // the front [m]
    const double tvWidth = 12.0;        // from front to back [m]
    const double tvWingWidth = 8.0;     // the wing behind the left end, across [m]
    const double tvWingLength = 16.0;   // and how far it runs back beyond the back wall [m]
    const double tvAnnexLength = 4.5;   // the lower part at the left end, along the front [m]
    const double tvAnnexWidth = 7.0;    // and from front to back [m]
    const double tvEave = 0.50;         // thatch beyond the walls [m]
    const double tvEaveLow = 2.50;      // underside of the thatch at its edge; the walls end here [m]
    const double tvAnnexEaveLow = 2.20; // and that of the lower part [m]
    const double tvThatch = 0.35;       // thickness of the thatch at its edge [m]
    const double tvPitch = 40.0;        // the long sides of a roof [deg]
    const double tvEndPitch = 52.0;     // its hipped ends [deg]
    const double tvFoundation = 2.5;    // walls reach this far into the ground [m]
    const double tvSill = 0.85;         // foot of the windows [m]
    const double tvHead = 2.25;         // their top [m]
    const double tvTerrace = 6.0;       // depth of the paving before the front [m]
    const double tvPole = 9.0;          // height of the flagpole [m]

    const Color tvWall(0.95f, 0.95f, 0.92f);        // white painted brick
    const Color tvRoof(0.40f, 0.37f, 0.33f);        // weathered thatch
    const Color tvCut(0.33f, 0.30f, 0.26f);         // its edge
    const Color tvFrame(0.86f, 0.72f, 0.22f);       // yellow frames
    const Color tvGlass(0.24f, 0.27f, 0.30f);
    const Color tvDoor(0.30f, 0.22f, 0.17f);
    const Color tvBrick(0.56f, 0.34f, 0.27f);       // the chimneys
    const Color tvPaving(0.62f, 0.60f, 0.56f);      // the terrace
    const Color tvSteel(0.90f, 0.90f, 0.88f);       // the flagpole
    const Color tvFlag(0.85f, 0.87f, 0.92f);

    double tanOf(double angle) { return std::tan(degrees(angle)); }

    // Struct Block: a rectangular part of the building under its own hipped thatch
    struct Block
    {
        double m_u0;        // the walls, in the frame of the model
        double m_u1;
        double m_v0;
        double m_v1;
        double m_eaveLow;   // underside of the thatch at its edge
        bool m_alongU;      // true->the ridge runs along u, false->along v

        double ridge() const
        {
            const double across = m_alongU ? m_v1 - m_v0 : m_u1 - m_u0;
            return m_eaveLow + tvThatch + (0.5 * across + tvEave) * tanOf(tvPitch);
        }

        bool contains(double u, double v, double margin) const
        {
            return u > m_u0 - margin && u < m_u1 + margin && v > m_v0 - margin && v < m_v1 + margin;
        }
    };

    Block mainBlock()
    {
        return Block{ -0.5 * tvLength, 0.5 * tvLength, -0.5 * tvWidth, 0.5 * tvWidth, tvEaveLow, true };
    }

    Block wingBlock()
    {
        const double u0 = -0.5 * tvLength;
        return Block{ u0, u0 + tvWingWidth, -0.5 * tvWidth - tvWingLength, -0.5 * tvWidth + 1.0, tvEaveLow, false };
    }

    Block annexBlock()
    {
        const double u1 = -0.5 * tvLength + 0.5;
        const double v1 = 0.5 * tvWidth - 1.0;
        return Block{ u1 - 0.5 - tvAnnexLength, u1, v1 - tvAnnexWidth, v1, tvAnnexEaveLow, true };
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

    // The top of the thatch of a block: two long sides and two hipped ends
    std::vector<RoofPlane> thatchOf(const Block& block)
    {
        const double cu = 0.5 * (block.m_u0 + block.m_u1);
        const double cv = 0.5 * (block.m_v0 + block.m_v1);
        const double eu = 0.5 * (block.m_u1 - block.m_u0) + tvEave;
        const double ev = 0.5 * (block.m_v1 - block.m_v0) + tvEave;
        const double edge = block.m_eaveLow + tvThatch;
        const double tu = block.m_alongU ? tanOf(tvEndPitch) : tanOf(tvPitch);
        const double tv = block.m_alongU ? tanOf(tvPitch) : tanOf(tvEndPitch);
        return {
            RoofPlane{ 0.0, -tv, edge + tv * (ev + cv) },
            RoofPlane{ 0.0, tv, edge + tv * (ev - cv) },
            RoofPlane{ -tu, 0.0, edge + tu * (eu + cu) },
            RoofPlane{ tu, 0.0, edge + tu * (eu - cu) } };
    }

    // The walls of a block, white from the ground up under the thatch, and its thatch
    void buildBlock(PlanBuilder& b, const Block& block)
    {
        const std::vector<RoofPlane> planes = thatchOf(block);
        const auto top = [&](double u, double v) { return lowest(planes, u, v); };
        const std::vector<RoofPlane> level = { RoofPlane{ 0.0, 0.0, block.m_eaveLow } };
        const auto under = [&](double, double) { return block.m_eaveLow; };
        const std::vector<Wall> walls = {
            Wall{ UV{ block.m_u0, block.m_v1 }, UV{ block.m_u1, block.m_v1 } },
            Wall{ UV{ block.m_u1, block.m_v1 }, UV{ block.m_u1, block.m_v0 } },
            Wall{ UV{ block.m_u1, block.m_v0 }, UV{ block.m_u0, block.m_v0 } },
            Wall{ UV{ block.m_u0, block.m_v0 }, UV{ block.m_u0, block.m_v1 } } };
        for (const Wall& wall : walls)
        {
            b.panel(wall, 0.0, b.length(wall), -tvFoundation, 0.0, tvWall, 0.0);
            b.wallUpToRoof(wall, 0.0, level, under, tvWall, 0.0);
        }
        const double u0 = block.m_u0 - tvEave;
        const double u1 = block.m_u1 + tvEave;
        const double v0 = block.m_v0 - tvEave;
        const double v1 = block.m_v1 + tvEave;
        const std::vector<double> us = { u0, block.m_u0, block.m_u1, u1 };
        const std::vector<double> vs = { v0, block.m_v0, block.m_v1, v1 };
        b.roof(gridCells(us, vs, [](double, double) { return true; }), planes, top, tvRoof);
        b.edgeBoard(Wall{ UV{ u0, v1 }, UV{ u1, v1 } }, tvThatch, planes, top, tvCut);
        b.edgeBoard(Wall{ UV{ u1, v1 }, UV{ u1, v0 } }, tvThatch, planes, top, tvCut);
        b.edgeBoard(Wall{ UV{ u1, v0 }, UV{ u0, v0 } }, tvThatch, planes, top, tvCut);
        b.edgeBoard(Wall{ UV{ u0, v0 }, UV{ u0, v1 } }, tvThatch, planes, top, tvCut);
        b.soffit(gridCells(us, vs, [&](double u, double v) { return !block.contains(u, v, 0.0); }), block.m_eaveLow,
            tvCut.scaled(0.8f));
    }

    // A chimney through the ridge of the main block, u from its middle
    void chimney(PlanBuilder& b, double u, double ridge, double half, double height)
    {
        const Site& site = b.site();
        const Vec3 up(0.0, 1.0, 0.0);
        b.mesh().addBox(b.point(u, 0.0, ridge - 0.5 + 0.5 * (height + 0.5)), site.m_axisLength, up, site.m_axisWidth,
            Vec3(half, 0.5 * (height + 0.5), half), tvBrick);
        b.mesh().addBox(b.point(u, 0.0, ridge + height + 0.04), site.m_axisLength, up, site.m_axisWidth,
            Vec3(half + 0.04, 0.04, half + 0.04), tvBrick.scaled(0.8f));
    }

    // Paving laid on the ground before a wall, from a0 to a1 along it and depth out from it
    void paving(PlanBuilder& b, const Wall& wall, double a0, double a1, double depth)
    {
        const double tile = 1.0;     // the paving follows the ground in steps of this size [m]
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
                b.mesh().addQuad(place(i, j), place(i + 1, j), place(i + 1, j + 1), place(i, j + 1), tvPaving,
                    Vec3(0.0, 1.0, 0.0));
            }
        }
    }

    // Class Taveerne
    class Taveerne : public Model
    {
    public:
        // Accessor
        const char* name() const override { return "Taveerne"; }
        const char* label() const override { return "Taveerne"; }

        std::pair<double, double> footprint(double, double) const override
        {
            return { tvLength, tvWidth };
        }

        double ridgeHeight(double) const override
        {
            return mainBlock().ridge();
        }

        bool covers(double, double, double du, double dv, double margin) const override
        {
            return mainBlock().contains(du, dv, margin) || wingBlock().contains(du, dv, margin)
                || annexBlock().contains(du, dv, margin);
        }

        void build(Mesh& mesh, const Site& site) const override
        {
            PlanBuilder b(mesh, site);
            const Vec3 up(0.0, 1.0, 0.0);
            const Block main = mainBlock();
            const Block wing = wingBlock();
            const Block annex = annexBlock();
            buildBlock(b, wing);
            buildBlock(b, annex);
            buildBlock(b, main);

            { // The front: wide windows in yellow frames either side of the door, the terrace before it
                const Wall front{ UV{ main.m_u0, main.m_v1 }, UV{ main.m_u1, main.m_v1 } };
                const std::vector<std::pair<double, double>> windows = {
                    { 2.0, 6.0 }, { 7.2, 11.2 }, { 14.6, 18.6 }, { 19.8, 23.8 } };
                for (const auto& [a0, a1] : windows)
                {
                    b.window(front, a0, a1, tvSill, tvHead, 4, tvFrame, tvGlass);
                }
                b.panel(front, 12.0, 13.8, 0.0, tvHead, tvFrame, 0.01);
                b.panel(front, 12.1, 13.7, 0.0, tvHead - 0.1, tvDoor, 0.02);
                b.window(front, 24.6, 26.0, tvSill + 0.3, tvHead, 2, tvFrame, tvGlass);
                paving(b, front, 1.0, tvLength - 1.0, tvTerrace);
                // The flagpole, at the edge of the terrace right of the door
                const Vec3 foot = b.onWall(front, 16.5, 0.0, tvTerrace + 0.5);
                b.mesh().addBox(Vec3(foot.x, foot.y + 0.5 * tvPole - 0.5, foot.z), site.m_axisLength, up, site.m_axisWidth,
                    Vec3(0.05, 0.5 * tvPole + 0.5, 0.05), tvSteel);
                b.mesh().addBox(Vec3(foot.x, foot.y + tvPole - 0.6, foot.z) - site.m_axisLength * 0.65, site.m_axisLength,
                    up, site.m_axisWidth, Vec3(0.60, 0.40, 0.01), tvFlag);
            }
            { // The lower part at the left end: the way in, under the name
                const Wall face{ UV{ annex.m_u0, annex.m_v1 }, UV{ annex.m_u1, annex.m_v1 } };
                b.window(face, 0.5, 2.3, tvSill, 2.0, 3, tvFrame, tvGlass);
                b.panel(face, 2.8, 3.8, 0.0, 2.0, tvDoor, 0.01);
            }
            { // The right end, the back and the wing: plain windows, assumed
                const Wall right{ UV{ main.m_u1, main.m_v1 }, UV{ main.m_u1, main.m_v0 } };
                b.window(right, 2.0, 4.4, tvSill, tvHead, 3, tvFrame, tvGlass);
                b.window(right, 7.6, 10.0, tvSill, tvHead, 3, tvFrame, tvGlass);
                const Wall back{ UV{ main.m_u1, main.m_v0 }, UV{ wing.m_u1, main.m_v0 } };
                for (double a = 2.0; a + 1.6 < b.length(back); a += 4.0)
                {
                    b.window(back, a, a + 1.6, 1.0, tvHead, 2, tvFrame, tvGlass);
                }
                const Wall wingRight{ UV{ wing.m_u1, main.m_v0 }, UV{ wing.m_u1, wing.m_v0 } };
                const Wall wingLeft{ UV{ wing.m_u0, wing.m_v0 }, UV{ wing.m_u0, annex.m_v0 } };
                for (const Wall& wall : { wingRight, wingLeft })
                {
                    for (double a = 2.0; a + 1.6 < b.length(wall); a += 4.0)
                    {
                        b.window(wall, a, a + 1.6, 1.0, tvHead, 2, tvFrame, tvGlass);
                    }
                }
                const Wall wingEnd{ UV{ wing.m_u1, wing.m_v0 }, UV{ wing.m_u0, wing.m_v0 } };
                b.panel(wingEnd, 3.2, 4.8, 0.0, tvHead, tvDoor, 0.01);
            }

            // The chimneys of the postcard: a broad one near the left end, two smaller ones further on
            const double ridge = main.ridge();
            chimney(b, -0.5 * tvLength + 5.0, ridge, 0.40, 1.1);
            chimney(b, -1.0, ridge, 0.30, 0.6);
            chimney(b, 4.5, ridge, 0.25, 0.6);
        }
    };
}

const Model& Models::taveerne()
{
    static const Taveerne model;
    return model;
}
