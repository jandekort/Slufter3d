// Slufter3D - model Visdiefje: 1950s bungalow, white walls under low thatch with a short ridge of tiles
// Author: Jan de Kort
// Created: 2026-11-16
// Visdiefje (photos Visdiefje, Type_Visdiefje_older_picture)
// A bungalow of the 1950s: 7 m square, white painted brick on a black foot, under thatch that comes down thick
// and low. The thatch is hipped all round; its ends are steeper than front and back, which leaves a short ridge,
// capped with orange tiles. Door and frames have the colours of the 1990s photograph, an orange door and brown
// frames: sampled there, and brightened as much as its white wall needs to match the recent photograph.
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
    const double vdEave = 0.40;         // thatch beyond the walls [m]
    const double vdEaveLow = 2.22;      // underside of the thatch at its edge, above the floor; the walls end here [m]
    const double vdThatch = 0.30;       // thickness of the thatch at its edge [m]
    const double vdPitch = 36.0;        // front and back: roof and walls equally high in the 1990s photo [deg]
    const double vdEndPitch = 46.0;     // the two ends: steeper, which leaves the 2.3 m ridge of that photo [deg]
    const double vdPlinth = 0.10;       // black band at the foot of the wall [m]
    const double vdFoundation = 2.5;    // walls reach this far into the dune [m]
    const double vdTiles = 0.20;        // the ridge tiles stand this far above the thatch [m]

    const Shape shapeVisdiefje = { vdEaveLow, vdPlinth, degrees(vdPitch), vdEave, vdEave, vdThatch, 1.0, 1.0 };
    const Paint paintVisdiefje = {
        Color(0.95f, 0.95f, 0.92f),   // white painted brick
        Color(0.17f, 0.17f, 0.17f),   // black foot
        Color(0.50f, 0.46f, 0.41f),   // weathered thatch
        Color(0.50f, 0.46f, 0.41f),
        Color(0.36f, 0.32f, 0.28f),   // cut edge of the thatch
        Color(0.40f, 0.32f, 0.26f),   // brown frames, 1990s photograph: (62, 47, 42) brightened
        Color(0.24f, 0.27f, 0.30f),
        Color(0.69f, 0.39f, 0.27f),   // orange door, 1990s photograph: (111, 58, 44) brightened
        Color(0.36f, 0.30f, 0.25f),   // the name board
        Color(0.12f, 0.12f, 0.13f) };
    const Color visdiefjeTiles(0.80f, 0.47f, 0.32f);

    // Top of the thatch at the ridge, above the floor, for a house of this width
    double visdiefjeRidge(double width)
    {
        return vdEaveLow + vdThatch + (0.5 * width + vdEave) * std::tan(degrees(vdPitch));
    }

    void buildVisdiefje(Mesh& mesh, const Site& site, const Paint& paint)
    {
        using Wall = PlanBuilder::Wall;
        PlanBuilder b(mesh, site);
        const double hu = 0.5 * site.m_length;
        const double hv = 0.5 * site.m_width;
        const double eu = hu + vdEave;                       // edge of the thatch
        const double ev = hv + vdEave;
        const double edge = vdEaveLow + vdThatch;            // top of the thatch at its edge
        const double front = std::tan(degrees(vdPitch));
        const double end = std::tan(degrees(vdEndPitch));
        const std::vector<RoofPlane> planes = {
            RoofPlane{ 0.0, -front, edge + front * ev },     // front
            RoofPlane{ 0.0, front, edge + front * ev },      // back
            RoofPlane{ -end, 0.0, edge + end * eu },         // right end
            RoofPlane{ end, 0.0, edge + end * eu } };        // left end
        auto height = [&](double u, double v)
        {
            return std::min(std::min(planes[0].at(u, v), planes[1].at(u, v)), std::min(planes[2].at(u, v), planes[3].at(u, v)));
        };
        const Color& frame = paint.m_trim;

        // Walls, each from left to right as seen from outside: white over the black foot, up under the thatch
        const Wall walls[] = {
            Wall{ UV{ -hu, hv }, UV{ hu, hv } },             // front, the entrance
            Wall{ UV{ hu, hv }, UV{ hu, -hv } },             // right
            Wall{ UV{ hu, -hv }, UV{ -hu, -hv } },           // back
            Wall{ UV{ -hu, -hv }, UV{ -hu, hv } } };         // left
        for (const Wall& wall : walls)
        {
            const double len = b.length(wall);
            b.panel(wall, 0.0, len, -vdFoundation, vdPlinth, paint.m_plinth, 0.0);
            b.panel(wall, 0.0, len, vdPlinth, vdEaveLow, paint.m_wall, 0.0);
        }

        { // Front as in the photographs, from the left: two vent slits, the door, the name board, the window
            const Wall& wall = walls[0];
            for (const double a : { 1.52, 1.68 })
            {
                b.panel(wall, a, a + 0.07, 1.64, 2.05, Color(0.42f, 0.42f, 0.43f), 0.01);
            }
            // The door: brown frame, orange leaf with four panes in its upper half, a handle
            const double d0 = 2.08;
            const double d1 = 3.01;
            b.panel(wall, d0 - 0.07, d1 + 0.07, 0.0, 2.17, frame, 0.02);
            b.panel(wall, d0, d1, 0.0, 2.10, paint.m_door, 0.04);
            const double bar = 0.08;
            const double glazed0 = 0.95;
            const double glazed1 = 1.92;
            const double paneWidth = 0.5 * (d1 - d0 - 0.24 - bar);
            const double paneHeight = 0.5 * (glazed1 - glazed0 - bar);
            for (int c = 0; c < 2; ++c)
            {
                for (int r = 0; r < 2; ++r)
                {
                    const double a = d0 + 0.12 + (double) c * (paneWidth + bar);
                    const double y = glazed0 + (double) r * (paneHeight + bar);
                    b.panel(wall, a, a + paneWidth, y, y + paneHeight, paint.m_glass, 0.06);
                }
            }
            b.panel(wall, d1 - 0.16, d1 - 0.08, 1.00, 1.06, Color(0.75f, 0.75f, 0.73f), 0.07);
            b.panel(wall, 3.22, 3.79, 1.59, 1.84, paint.m_shutter, 0.03);
            // Two equal lights in a brown frame, on a sill of dark tiles
            b.panel(wall, 4.00, 5.82, 0.47, 0.55, Color(0.17f, 0.17f, 0.16f), 0.05);
            b.window(wall, 4.06, 5.76, 0.55, 2.15, 2, frame, paint.m_glass);
        }

        { // A narrow window near the front of either side, as in the photographs; the back has none, so one is assumed
            b.window(walls[1], 0.60, 1.10, 0.80, 1.85, 1, frame, paint.m_glass);
            const double leftLen = b.length(walls[3]);
            b.window(walls[3], leftLen - 1.10, leftLen - 0.60, 0.80, 1.85, 1, frame, paint.m_glass);
            b.window(walls[2], 2.20, 3.40, 1.00, 2.00, 2, frame, paint.m_glass);
        }

        // The thatch: four planes over the eaves, its cut edge all round with the underside, the ridge tiles, a flue
        b.roof({ { UV{ -eu, -ev }, UV{ eu, -ev }, UV{ eu, ev }, UV{ -eu, ev } } }, planes, height, paint.m_roof);
        const Wall eaves[] = {
            Wall{ UV{ -eu, ev }, UV{ eu, ev } }, Wall{ UV{ eu, ev }, UV{ eu, -ev } },
            Wall{ UV{ eu, -ev }, UV{ -eu, -ev } }, Wall{ UV{ -eu, -ev }, UV{ -eu, ev } } };
        for (const Wall& eave : eaves)
        {
            b.panel(eave, 0.0, b.length(eave), vdEaveLow, edge + 0.01, paint.m_gable, 0.0);
        }
        auto underEaves = [&](double u, double v)
        {
            return std::fabs(u) > hu || std::fabs(v) > hv;
        };
        b.soffit(gridCells({ -eu, -hu, hu, eu }, { -ev, -hv, hv, ev }, underEaves), vdEaveLow, paint.m_gable.scaled(0.8f));
        const double ridge = visdiefjeRidge(site.m_width);
        const double ridgeHalf = eu - (ridge - edge) / end;  // where the ends reach the ridge
        b.mesh().addBox(b.point(0.0, 0.0, ridge + 0.5 * vdTiles - 0.02), site.m_axisLength, Vec3(0.0, 1.0, 0.0),
            site.m_axisWidth, Vec3(ridgeHalf + 0.08, 0.5 * vdTiles + 0.02, 0.16), visdiefjeTiles);
        const double flueU = -ridgeHalf - 0.3;
        const double flueV = 0.35;
        const double flueFoot = height(flueU, flueV) - 0.2;
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
            return { outlineLength, outlineWidth }; // given as walls, not as the outline of the roof
        }
        double ridgeHeight(double width) const override { return visdiefjeRidge(width) + vdTiles; }

        void build(Mesh& mesh, const Site& site) const override { buildVisdiefje(mesh, site, paintVisdiefje); }
    };
}

const Model& Models::visdiefje()
{
    static const Visdiefje model;
    return model;
}
