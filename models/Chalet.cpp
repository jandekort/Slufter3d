// Slufter3D - model Chalet: the chalets of type B6 that stood north-east of the old park from 1971
// Author: Jan de Kort
// Created: 2026-11-21
// Chalet (photo IMG_4894, a small print: one chalet seen from the dune, its long side to the camera)
// A plain rectangular house: walls of ochre brown, a dark roof that is all but flat and reaches a little beyond
// the walls, a pale board along its edge, windows and a door in dark brown frames along the long side.
// Its floor is that of the white bungalow for six, the green one (6.4 m by 5.2 m), laid out two by three:
// 4.71 m by 7.07 m (Jan de Kort, 2026-11-21). The photograph is too small to measure by: the heights are those
// of a door leaf of 2.0 m under the eaves. Of the ends and the back there is no photograph: a window in each
// end and two at the back are assumed. The gutter along the front with its downpipe is drawn as asked for; the
// photograph does not show it.
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

    const double chArea = 6.40 * 5.20;      // the floor of the green bungalow [m2]
    const double chEave = 0.30;             // roof beyond the walls [m]
    const double chWallTop = 2.30;          // the walls end here, under the roof at front and back [m]
    const double chRoof = 0.10;             // thickness of the roof at its edge [m]
    const double chPitch = 8.0;             // of the roof, all but flat [deg]
    const double chFoundation = 2.5;        // walls reach this far into the dune [m]
    const double chHead = 2.05;             // top of the windows and of the door [m]
    const double chSill = 0.95;             // foot of the windows [m]
    const double chLeaf = 0.89;             // width of the door [m]
    const double chGutter = 0.07;           // half the width of the gutter [m]
    const double chPipe = 0.04;             // half the width of the downpipe [m]

    const Color chWall(0.66f, 0.50f, 0.30f);        // ochre brown
    const Color chRoofTop(0.20f, 0.21f, 0.23f);     // dark roofing
    const Color chBoard(0.84f, 0.82f, 0.76f);       // the pale board along the edge of the roof
    const Color chFrame(0.33f, 0.22f, 0.15f);       // dark brown frames
    const Color chGlass(0.22f, 0.25f, 0.28f);
    const Color chDoor(0.42f, 0.29f, 0.19f);        // the door leaf, a little lighter than its frame
    const Color chZinc(0.62f, 0.64f, 0.66f);        // gutter and downpipe
    const Color chSteel(0.74f, 0.74f, 0.72f);

    double tanOf(double angle) { return std::tan(degrees(angle)); }

    double chLength() { return std::sqrt(1.5 * chArea); }   // three to two
    double chWidth() { return chArea / chLength(); }

    double ridgeOf(double width)
    {
        return chWallTop + chRoof + (0.5 * width + chEave) * tanOf(chPitch);
    }

    // Class Chalet
    class Chalet : public Model
    {
    public:
        // Accessor
        const char* name() const override { return "Chalet"; }
        const char* label() const override { return "Chalet"; }

        std::pair<double, double> footprint(double, double) const override
        {
            return { chLength(), chWidth() };
        }

        double ridgeHeight(double width) const override
        {
            return ridgeOf(width);
        }

        void build(Mesh& mesh, const Site& site) const override
        {
            PlanBuilder b(mesh, site);
            const Vec3 up(0.0, 1.0, 0.0);
            const double hu = 0.5 * site.m_length;
            const double hv = 0.5 * site.m_width;
            const double eu = hu + chEave;
            const double ev = hv + chEave;
            const double pitch = tanOf(chPitch);
            const double ridge = ridgeOf(site.m_width);
            const std::vector<RoofPlane> top = { RoofPlane{ 0.0, -pitch, ridge }, RoofPlane{ 0.0, pitch, ridge } };
            const std::vector<RoofPlane> under = {
                RoofPlane{ 0.0, -pitch, ridge - chRoof }, RoofPlane{ 0.0, pitch, ridge - chRoof } };
            const auto topAt = [&](double u, double v) { return std::min(top[0].at(u, v), top[1].at(u, v)); };
            const auto underAt = [&](double u, double v) { return std::min(under[0].at(u, v), under[1].at(u, v)); };

            // Walls, each from left to right as seen from outside
            const Wall front{ UV{ -hu, hv }, UV{ hu, hv } };
            const Wall right{ UV{ hu, hv }, UV{ hu, -hv } };
            const Wall back{ UV{ hu, -hv }, UV{ -hu, -hv } };
            const Wall left{ UV{ -hu, -hv }, UV{ -hu, hv } };
            for (const Wall& wall : { front, right, back, left })
            {
                b.panel(wall, 0.0, b.length(wall), -chFoundation, 0.0, chWall, 0.0);
                b.wallUpToRoof(wall, 0.0, under, underAt, chWall, 0.0);
            }

            { // The long side of the photograph: a window, the door, a wide window
                const double length = b.length(front);
                b.window(front, 0.60, 1.90, chSill, chHead, 2, chFrame, chGlass);
                const double d0 = 0.40 * length - 0.5 * chLeaf;
                b.panel(front, d0 - 0.08, d0 + chLeaf + 0.08, 0.0, chHead + 0.08, chFrame, 0.01);
                b.panel(front, d0, d0 + chLeaf, 0.02, chHead, chDoor, 0.02);
                b.window(front, d0 + 0.14, d0 + chLeaf - 0.14, 1.15, chHead - 0.15, 1, chFrame, chGlass);
                b.mesh().addBox(b.onWall(front, d0 + chLeaf - 0.10, 1.02, 0.05), site.m_axisLength, up, site.m_axisWidth,
                    Vec3(0.05, 0.012, 0.02), chSteel);
                b.window(front, 0.56 * length, length - 0.60, chSill - 0.25, chHead, 3, chFrame, chGlass);
            }
            // The ends and the back: assumed
            b.window(right, 1.40, b.length(right) - 1.40, chSill, chHead, 2, chFrame, chGlass);
            b.window(left, 1.60, b.length(left) - 1.60, chSill + 0.15, chHead, 2, chFrame, chGlass);
            b.window(back, 1.00, 2.20, chSill + 0.15, chHead, 2, chFrame, chGlass);
            b.window(back, b.length(back) - 2.40, b.length(back) - 1.00, chSill + 0.15, chHead, 2, chFrame, chGlass);

            // The roof: two faces that barely slope, a pale board along their edge, their underside
            const std::vector<double> us = { -eu, -hu, hu, eu };
            const std::vector<double> vs = { -ev, -hv, hv, ev };
            b.roof(gridCells(us, vs, [](double, double) { return true; }), top, topAt, chRoofTop);
            b.edgeBoard(Wall{ UV{ -eu, ev }, UV{ eu, ev } }, chRoof + 0.06, top, topAt, chBoard);
            b.edgeBoard(Wall{ UV{ eu, ev }, UV{ eu, -ev } }, chRoof + 0.06, top, topAt, chBoard);
            b.edgeBoard(Wall{ UV{ eu, -ev }, UV{ -eu, -ev } }, chRoof + 0.06, top, topAt, chBoard);
            b.edgeBoard(Wall{ UV{ -eu, -ev }, UV{ -eu, ev } }, chRoof + 0.06, top, topAt, chBoard);
            for (const std::vector<UV>& cell : gridCells(us, vs, [&](double u, double v)
                { return std::fabs(u) > hu || std::fabs(v) > hv; }))
            {
                b.mesh().addQuad(b.point(cell[0].u, cell[0].v, underAt(cell[0].u, cell[0].v)),
                    b.point(cell[1].u, cell[1].v, underAt(cell[1].u, cell[1].v)),
                    b.point(cell[2].u, cell[2].v, underAt(cell[2].u, cell[2].v)),
                    b.point(cell[3].u, cell[3].v, underAt(cell[3].u, cell[3].v)), chBoard.scaled(0.8f), -up);
            }

            { // The rainwater: a gutter along the front edge of the roof, and a pipe down the right end of the front
                const double edge = topAt(0.0, ev);
                const double gutterY = edge - chRoof - chGutter;
                b.mesh().addBox(b.point(0.0, ev + chGutter, gutterY), site.m_axisLength, up, site.m_axisWidth,
                    Vec3(eu, chGutter, chGutter), chZinc);
                const double pipeU = hu - 0.15;
                // From the gutter back to the wall, then down it into the ground
                b.mesh().addBox(b.point(pipeU, 0.5 * (hv + ev) + 0.5 * chPipe, gutterY - chGutter - chPipe), site.m_axisLength,
                    up, site.m_axisWidth, Vec3(chPipe, chPipe, 0.5 * chEave + chPipe), chZinc);
                const Vec3 foot = b.point(pipeU, hv + chPipe + 0.01, 0.0);
                const double ground = groundLevel(site, foot) - site.m_floor.y;
                const double pipeTop = gutterY - chGutter;
                b.mesh().addBox(b.point(pipeU, hv + chPipe + 0.01, 0.5 * (pipeTop + ground) - 0.1), site.m_axisLength, up,
                    site.m_axisWidth, Vec3(chPipe, 0.5 * (pipeTop - ground) + 0.1, chPipe), chZinc);
                for (const double y : { 0.5, 1.6 })
                {
                    b.mesh().addBox(b.point(pipeU, hv + chPipe + 0.01, y), site.m_axisLength, up, site.m_axisWidth,
                        Vec3(chPipe + 0.015, 0.02, chPipe + 0.015), chZinc.scaled(0.8f));
                }
            }
        }
    };
}

const Model& Models::chalet()
{
    static const Chalet model;
    return model;
}
