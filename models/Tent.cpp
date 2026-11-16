// Slufter3D - model Tent: olive dome tent with a porch on a hoop, red guy lines
// Author: Jan de Kort
// Created: 2026-11-16
// Tent (photo Type_C2_tent)
// A dome tent for four with a porch on a hoop: u runs from the back of the dome to the foot of the porch
#include "Models.h"

#include "ModelKit.h"

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
    const double tentLength = 4.0;      // [m]
    const double tentWidth = 2.6;       // [m]
    const double tentHeight = 1.55;     // top of the dome above the ground [m]
    const double tentSkirt = 0.6;       // the cloth reaches this far below the floor, into the ground [m]

    const Shape shapeTent = { 0.0, 0.0, degrees(50.0), 0.0, 0.0, 0.0, 0.0, 1.0 };
    const Paint paintTent = {
        Color(0.36f, 0.41f, 0.35f),   // olive fly sheet
        Color(0.12f, 0.13f, 0.13f),   // black ground sheet (not seen: the fly sheet reaches the ground)
        Color(0.42f, 0.47f, 0.41f),   // lighter panel over the arch between dome and porch
        Color(0.42f, 0.47f, 0.41f),
        Color(0.30f, 0.34f, 0.30f),   // door panel of the porch, in shadow
        Color(0.30f, 0.34f, 0.30f),
        Color(0.20f, 0.22f, 0.21f),
        Color(0.30f, 0.34f, 0.30f),
        Color(0.36f, 0.41f, 0.35f),
        Color(0.74f, 0.25f, 0.18f) }; // red guy lines

    // Struct TentRib: one arch of the tent across its length
    struct TentRib
    {
        double m_u;           // position along the tent [m]
        double m_halfWidth;   // at the ground [m]
        double m_height;      // of the arch [m]
    };

    void buildTent(Mesh& mesh, const Site& site, const Paint& paint)
    {
        // From the back of the dome over its top, down to the saddle, up to the hoop of the porch and down to its foot
        const TentRib ribs[] = {
            {  2.00, 0.55, 0.02 },
            {  1.85, 0.95, 0.55 },
            {  1.45, 1.20, 1.10 },
            {  0.95, 1.30, 1.42 },
            {  0.40, 1.30, tentHeight },
            { -0.15, 1.30, 1.40 },
            { -0.60, 1.25, 1.22 },
            { -1.10, 1.18, 1.15 },
            { -1.60, 1.02, 0.62 },
            { -2.00, 0.85, 0.02 } };
        const size_t ribCount = sizeof(ribs) / sizeof(ribs[0]);
        const size_t archFirst = 4;   // the lighter panel lies between these two ribs
        const size_t archLast = 6;
        const size_t porchFirst = 7;  // from the hoop down: the door of the porch
        const int segments = 8;
        const auto point = [&](double u, double v, double y)
            {
                return site.m_floor + site.m_axisLength * u + site.m_axisWidth * v + Vec3(0.0, y, 0.0);
            };
        const auto onRib = [&](const TentRib& rib, int k)
            {
                const double angle = pi * (double) k / (double) segments;
                return point(rib.m_u, rib.m_halfWidth * std::cos(angle), rib.m_height * std::sin(angle));
            };
        for (size_t r = 0; r + 1 < ribCount; ++r)
        {
            const bool arch = r >= archFirst && r < archLast;
            const bool porch = r >= porchFirst;
            for (int k = 0; k < segments; ++k)
            {
                const bool top = k >= 2 && k < segments - 2;
                const Color& color = arch ? paint.m_roof : (porch && top ? paint.m_gable : paint.m_wall);
                const Vec3 a = onRib(ribs[r], k);
                const Vec3 b = onRib(ribs[r + 1], k);
                const Vec3 c = onRib(ribs[r + 1], k + 1);
                const Vec3 d = onRib(ribs[r], k + 1);
                const Vec3 middle = (a + b + c + d) * 0.25;
                const Vec3 axis = point(0.5 * (ribs[r].m_u + ribs[r + 1].m_u), 0.0, 0.3);
                mesh.addQuad(a, b, c, d, color, middle - axis);
            }
            // The cloth runs on into the ground on both sides
            for (const double side : { 1.0, -1.0 })
            {
                const Vec3 a = point(ribs[r].m_u, side * ribs[r].m_halfWidth, 0.0);
                const Vec3 b = point(ribs[r + 1].m_u, side * ribs[r + 1].m_halfWidth, 0.0);
                const Vec3 down(0.0, -tentSkirt, 0.0);
                mesh.addQuad(a + down, b + down, b, a, paint.m_wall.scaled(0.85f), site.m_axisWidth * side);
            }
        }
        for (const size_t r : { (size_t) 0, ribCount - 1 })
        {
            const double end = r == 0 ? 1.0 : -1.0;
            const Vec3 a = point(ribs[r].m_u, ribs[r].m_halfWidth, ribs[r].m_height);
            const Vec3 b = point(ribs[r].m_u, -ribs[r].m_halfWidth, ribs[r].m_height);
            const Vec3 down(0.0, -tentSkirt - ribs[r].m_height, 0.0);
            mesh.addQuad(a + down, b + down, b, a, paint.m_wall.scaled(0.85f), site.m_axisLength * end);
        }
        // Guy lines: from the hoop of the porch forwards, from the dome sideways and backwards
        const double guys[][5] = {
            // u, v, y on the tent; u, v of the peg
            { -1.10,  1.00, 0.62, -2.70,  1.70 },
            { -1.10, -1.00, 0.62, -2.70, -1.70 },
            {  0.95,  0.95, 0.97,  1.90,  2.50 },
            {  0.95, -0.95, 0.97,  1.90, -2.50 } };
        for (const auto& guy : guys)
        {
            const Vec3 peg = point(guy[3], guy[4], 0.0);
            mesh.addBeam(point(guy[0], guy[1], guy[2]), Vec3(peg.x, groundLevel(site, peg), peg.z), 0.012, paint.m_pipe);
        }
    }

    // Class Tent
    class Tent : public Model
    {
    public:
        // Accessor
        const char* name() const override { return "Tent"; }
        const char* label() const override { return "C2"; }
        std::pair<double, double> footprint(double, double) const override { return { tentLength, tentWidth }; }
        double ridgeHeight(double) const override { return tentHeight; }

        void build(Mesh& mesh, const Site& site) const override { buildTent(mesh, site, paintTent); }
    };
}

const Model& Models::tent()
{
    static const Tent model;
    return model;
}
