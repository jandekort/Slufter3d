// Slufter3D - the central building with the reception
// Author: Jan de Kort
// Created: 2026-11-15
#include "Reception.h"

#include "Mesh.h"
#include "Models.h"
#include "Terrain.h"
#include "WorldData.h"

#include <algorithm>
#include <numeric>

namespace
{
    const double pi = 3.14159265358979323846;
}

Reception::Reception(const WorldData& data, const Terrain& terrain)
    :
    m_wings()
    , m_door()
    , m_facing()
    , m_floorLift(0.15)
    , m_canopyHalf(3.6)
    , m_canopyReach(1.9)
    , m_canopyFoot(2.55)
    , m_canopyHead(3.25)
    , m_canopySegments(8)
    , m_white(0.95f, 0.94f, 0.90f)
    , m_glass(0.22f, 0.26f, 0.30f)
    , m_sign(0.80f, 0.56f, 0.42f)
    , m_column(0.16f, 0.22f, 0.26f)
    , m_paving(0.56f, 0.45f, 0.38f)
{
    // One floor for the whole building: the mean ground under the centres of its wings
    const double ground = std::accumulate(data.m_wings.begin(), data.m_wings.end(), 0.0,
        [&](double sum, const WorldData::WingRecord& wing)
        {
            return sum + terrain.height(wing.m_east, wing.m_north);
        }) / (double) std::max<size_t>(1, data.m_wings.size());
    const double floor = ground + m_floorLift;

    m_wings.reserve(data.m_wings.size());
    std::transform(data.m_wings.begin(), data.m_wings.end(), std::back_inserter(m_wings),
        [&](const WorldData::WingRecord& wing)
        {
            const double azimuth = wing.m_axis * pi / 180.0;
            Site site;
            site.m_floor = Vec3(wing.m_east, floor, -wing.m_north);
            site.m_axisLength = Vec3(std::sin(azimuth), 0.0, -std::cos(azimuth));
            site.m_axisWidth = Vec3(std::cos(azimuth), 0.0, std::sin(azimuth));
            site.m_length = wing.m_length;
            site.m_width = wing.m_width;
            return site;
        });

    const double facing = data.m_entranceAzimuth * pi / 180.0;
    m_door = Vec3(data.m_entranceEast, floor, -data.m_entranceNorth);
    m_facing = Vec3(std::sin(facing), 0.0, -std::cos(facing));
}

bool Reception::contains(double east, double north, double margin) const
{
    return std::any_of(m_wings.begin(), m_wings.end(),
        [&](const Site& wing)
        {
            const Vec3 offset(east - wing.m_floor.x, 0.0, -north - wing.m_floor.z);
            return std::fabs(dot(offset, wing.m_axisLength)) < 0.5 * wing.m_length + margin
                && std::fabs(dot(offset, wing.m_axisWidth)) < 0.5 * wing.m_width + margin;
        });
}

void Reception::build(Mesh& mesh) const
{
    for (const Site& wing : m_wings)
    {
        Models::receptionWing().build(mesh, wing);
    }
    addEntrance(mesh);
}

// White double doors with round panes under a white canopy that bows out from the wall,
// the sign board in the middle of it, a dark column either side, brick paving in front
void Reception::addEntrance(Mesh& mesh) const
{
    const Vec3 up(0.0, 1.0, 0.0);
    const Vec3 along = cross(up, m_facing); // to the right for someone walking in
    auto onWall = [&](double a, double y, double out)
    {
        return m_door + along * a + up * y + m_facing * out;
    };
    auto panel = [&](double a0, double a1, double y0, double y1, double out, const Color& color)
    {
        mesh.addQuad(onWall(a0, y0, out), onWall(a1, y0, out), onWall(a1, y1, out), onWall(a0, y1, out), color, m_facing);
    };

    { // Doors and the glass either side
        panel(-3.3, 3.3, 0.02, m_canopyFoot, 0.05, Color(0.10f, 0.10f, 0.11f));
        panel(-3.2, -1.3, 0.50, m_canopyFoot - 0.1, 0.08, m_glass);
        panel( 1.3,  3.2, 0.50, m_canopyFoot - 0.1, 0.08, m_glass);
        panel(-1.2, -0.05, 0.02, 2.30, 0.08, m_white);
        panel( 0.05, 1.2, 0.02, 2.30, 0.08, m_white);
        // The half-round panes, as two upright ovals of four panels each
        for (const double s : { -1.0, 1.0 })
        {
            const double widths[] = { 0.22, 0.34, 0.34, 0.22 };
            for (size_t k = 0; k < 4; ++k)
            {
                const double y0 = 0.75 + 0.33 * (double) k;
                const double inner = 0.14;
                panel(s > 0.0 ? inner : -inner - widths[k], s > 0.0 ? inner + widths[k] : -inner, y0, y0 + 0.33, 0.11, m_glass);
            }
        }
    }

    { // Canopy: a slab with a bowed edge, its fascia, the sign
        auto edge = [&](int k)
        {
            const double t = -1.0 + 2.0 * (double) k / (double) m_canopySegments;
            return Vec3(t * m_canopyHalf, 0.0, m_canopyReach * (0.45 + 0.55 * std::cos(0.5 * pi * t)));
        };
        for (int k = 0; k < m_canopySegments; ++k)
        {
            const Vec3 a = edge(k);
            const Vec3 b = edge(k + 1);
            mesh.addQuad(onWall(a.x, m_canopyFoot, a.z), onWall(b.x, m_canopyFoot, b.z), onWall(b.x, m_canopyHead, b.z),
                onWall(a.x, m_canopyHead, a.z), m_white, m_facing);
            mesh.addQuad(onWall(a.x, m_canopyHead, 0.0), onWall(b.x, m_canopyHead, 0.0), onWall(b.x, m_canopyHead, b.z),
                onWall(a.x, m_canopyHead, a.z), m_white.scaled(0.95f), up);
            mesh.addQuad(onWall(a.x, m_canopyFoot, 0.0), onWall(b.x, m_canopyFoot, 0.0), onWall(b.x, m_canopyFoot, b.z),
                onWall(a.x, m_canopyFoot, a.z), m_white.scaled(0.80f), -up);
        }
        for (const double s : { -1.0, 1.0 })
        {
            const Vec3 end = edge(s > 0.0 ? m_canopySegments : 0);
            mesh.addQuad(onWall(end.x, m_canopyFoot, 0.0), onWall(end.x, m_canopyFoot, end.z),
                onWall(end.x, m_canopyHead, end.z), onWall(end.x, m_canopyHead, 0.0), m_white, along * s);
        }
        panel(-0.75, 0.75, m_canopyFoot + 0.18, m_canopyHead - 0.18, m_canopyReach + 0.03, m_sign);
    }

    { // Columns and paving
        for (const double s : { -1.0, 1.0 })
        {
            mesh.addBeam(onWall(s * 2.3, -0.5, 1.25), onWall(s * 2.3, m_canopyFoot, 1.25), 0.11, m_column);
        }
        mesh.addQuad(onWall(-2.0, 0.03, 0.0), onWall(2.0, 0.03, 0.0), onWall(2.0, 0.03, 9.0), onWall(-2.0, 0.03, 9.0),
            m_paving, up);
    }
}
