// Slufter3D - the picture: the mesh of the world and the visitor's camera
// Author: Jan de Kort
// Created: 2026-11-16
#include "Scene.h"

#include "Check.h"
#include "Lighthouse.h"
#include "Model.h"
#include "Terrain.h"
#include "Walker.h"
#include "World.h"

#include <algorithm>
#include <deque>

namespace
{
    const double pi = 3.14159265358979323846;
}

Scene::Scene()
    :
    m_mesh()
    , m_skyVertexCount(0)
    , m_flatVertexCount(0)
    , m_fieldOfView(68.0 * pi / 180.0)
    , m_near(0.15)
    , m_nearPerHeight(0.03)
    , m_nearMax(6.0)
    , m_far(60000.0)
    , m_fogRange(7000.0)
    , m_skyRadius(1500.0)
    , m_skyRings(8)
    , m_skySegments(24)
    , m_zenith(0.13f, 0.36f, 0.80f)
    , m_horizon(0.62f, 0.78f, 0.95f)
    , m_grass(0.25f, 0.44f, 0.19f)
    , m_marram(0.78f, 0.74f, 0.44f)
    , m_meadow(0.36f, 0.56f, 0.24f)
    , m_sand(0.84f, 0.78f, 0.60f)
    , m_water(0.20f, 0.36f, 0.52f)
    , m_sea(0.18f, 0.33f, 0.46f)
    , m_asphalt(0.40f, 0.40f, 0.42f)
    , m_bricks(0.55f, 0.40f, 0.33f)
    , m_shell(0.86f, 0.82f, 0.70f)
    , m_track(0.74f, 0.67f, 0.50f)
    , m_meadowHeight(1.6)
    , m_duneHeight(9.0)
    , m_sandSlope(0.38)
    , m_beachHeight(2.6)
    , m_beachReach(600.0)
    , m_tileContrast(0.92f)
    , m_roadLift(0.06)
    , m_waterLift(0.05)
    , m_seaReach(40000.0)
{
}

void Scene::build(const World& world)
{
    const Terrain& terrain = world.getTerrain();
    const WorldData& data = world.getData();
    m_mesh.clear();
    addSky();
    m_skyVertexCount = m_mesh.vertexCount();

    addGround(terrain);
    world.getReception().build(m_mesh);
    world.getTower().build(m_mesh, terrain, data.m_towerEast, data.m_towerNorth);
    for (const House& house : world.getHouses())
    {
        house.m_model->build(m_mesh, house.m_site);
    }
    Lighthouse().build(m_mesh, Vec3(data.m_lighthouseEast, terrain.height(data.m_lighthouseEast, data.m_lighthouseNorth),
        -data.m_lighthouseNorth));

    const size_t before = m_mesh.vertexCount();
    addWater(terrain);
    addRoads(world);
    m_flatVertexCount = m_mesh.vertexCount() - before;
}

// High up the nearest things are far away, and a later near plane keeps distant ground sharp
Mat4 Scene::viewProjection(const Walker& walker, double aspect) const
{
    const Vec3 eye = walker.eye();
    const double near = std::min(m_nearMax, m_near + m_nearPerHeight * walker.getAltitude());
    return Mat4::perspective(m_fieldOfView, aspect, near, m_far)
        * Mat4::lookAt(eye, eye + walker.gaze(), Vec3(0.0, 1.0, 0.0));
}

// Low ground is green, dune tops are the yellow-green of marram, steep faces show sand, and so does
// low ground that the sea can reach. Every other tile of the park is a shade darker.
Color Scene::groundColor(double height, double slope, bool beach, bool odd) const
{
    const double dune = clamp((height - m_meadowHeight) / (m_duneHeight - m_meadowHeight), 0.0, 1.0);
    Color color = height < m_meadowHeight ? m_meadow : m_grass.mixed(m_marram, (float) dune);
    const double bare = clamp((slope - m_sandSlope) / m_sandSlope, 0.0, 1.0);
    color = color.mixed(m_sand, (float) (0.8 * bare));
    if (beach)
    {
        color = m_sand;
    }
    return odd ? color.scaled(m_tileContrast) : color;
}

// A dome around the eye: pale at the horizon, deep blue overhead
void Scene::addSky()
{
    const double lowest = -0.12; // a little below the horizon [rad]
    auto at = [&](int ring, int segment)
    {
        const double t = (double) ring / (double) m_skyRings;
        const double elevation = lowest + (0.5 * pi - lowest) * t*t; // rings crowd near the horizon
        const double azimuth = 2.0 * pi * (double) segment / (double) m_skySegments;
        return Vec3(std::cos(elevation) * std::sin(azimuth), std::sin(elevation), std::cos(elevation) * std::cos(azimuth))
            * m_skyRadius;
    };
    auto tint = [&](int ring)
    {
        const double t = (double) ring / (double) m_skyRings;
        return m_horizon.mixed(m_zenith, (float) std::sqrt(clamp(1.6 * t, 0.0, 1.0)));
    };
    for (int ring = 0; ring < m_skyRings; ++ring)
    {
        for (int segment = 0; segment < m_skySegments; ++segment)
        {
            const Vec3 a = at(ring, segment);
            const Vec3 b = at(ring, segment + 1);
            const Vec3 c = at(ring + 1, segment + 1);
            const Vec3 d = at(ring + 1, segment);
            m_mesh.addFlatTriangle(a, b, c, tint(ring), tint(ring), tint(ring + 1));
            m_mesh.addFlatTriangle(a, c, d, tint(ring), tint(ring + 1), tint(ring + 1));
        }
    }
}

void Scene::addGround(const Terrain& terrain)
{
    for (size_t k = 0; k < terrain.levelCount(); ++k)
    {
        addLevel(terrain.getLevel(k), k > 0 ? &terrain.getLevel(k - 1) : nullptr, k == 0);
    }
    addSeaAround(terrain.getLevel(terrain.levelCount() - 1));
}

// Starting from the tiles under the sea, over neighbouring tiles of low ground, as far as the beach reaches
std::vector<bool> Scene::beachTiles(const TerrainLevel& level) const
{
    const size_t nx = level.getNx() - 1;
    const size_t nz = level.getNz() - 1;
    const int reach = (int) std::ceil(m_beachReach / level.getCell());
    const auto top = [&](size_t i, size_t j)
        {
            return std::max(std::max(level.nodeHeight(i, j), level.nodeHeight(i + 1, j)),
                std::max(level.nodeHeight(i, j + 1), level.nodeHeight(i + 1, j + 1)));
        };
    std::vector<int> steps(nx * nz, -1);
    std::deque<size_t> open;
    for (size_t j = 0; j < nz; ++j)
    {
        for (size_t i = 0; i < nx; ++i)
        {
            if (top(i, j) < 0.0)
            {
                steps[j*nx + i] = 0;
                open.push_back(j*nx + i);
            }
        }
    }
    while (!open.empty())
    {
        const size_t tile = open.front();
        open.pop_front();
        if (steps[tile] >= reach)
        {
            continue;
        }
        const size_t i = tile % nx;
        const size_t j = tile / nx;
        const size_t ni[] = { i > 0 ? i - 1 : i, i + 1 < nx ? i + 1 : i, i, i };
        const size_t nj[] = { j, j, j > 0 ? j - 1 : j, j + 1 < nz ? j + 1 : j };
        for (size_t k = 0; k < 4; ++k)
        {
            const size_t next = nj[k]*nx + ni[k];
            if (steps[next] < 0 && top(ni[k], nj[k]) < m_beachHeight)
            {
                steps[next] = steps[tile] + 1;
                open.push_back(next);
            }
        }
    }
    std::vector<bool> beach(nx * nz);
    std::transform(steps.begin(), steps.end(), beach.begin(),
        [](int s)
        {
            return s > 0;
        });
    return beach;
}

// One grid, without the part that a finer grid covers. Tiles that reach below the sea level get the sea
// as well, in strips from west to east; tiles wholly under it get the sea only.
void Scene::addLevel(const TerrainLevel& level, const TerrainLevel* finer, bool chequered)
{
    const double cell = level.getCell();
    const double seaLevel = 0.0;
    const std::vector<bool> beach = beachTiles(level);
    auto node = [&](size_t i, size_t j)
    {
        return Vec3(level.getWest() + (double) i * cell, level.nodeHeight(i, j), -(level.getSouth() + (double) j * cell));
    };
    for (size_t j = 0; j + 1 < level.getNz(); ++j)
    {
        const double south = level.getSouth() + (double) j * cell;
        size_t stripFrom = 0;
        bool inStrip = false;
        const auto endStrip = [&](size_t to)
            {
                if (inStrip)
                {
                    const Vec3 a(level.getWest() + (double) stripFrom * cell, seaLevel, -south);
                    const Vec3 b(level.getWest() + (double) to * cell, seaLevel, -south);
                    const Vec3 c(b.x, seaLevel, -(south + cell));
                    const Vec3 d(a.x, seaLevel, -(south + cell));
                    m_mesh.addFlatTriangle(a, b, c, m_sea, m_sea, m_sea);
                    m_mesh.addFlatTriangle(a, c, d, m_sea, m_sea, m_sea);
                    inStrip = false;
                }
            };
        for (size_t i = 0; i + 1 < level.getNx(); ++i)
        {
            const Vec3 sw = node(i, j);
            const Vec3 se = node(i + 1, j);
            const Vec3 ne = node(i + 1, j + 1);
            const Vec3 nw = node(i, j + 1);
            const double lowest = std::min(std::min(sw.y, se.y), std::min(ne.y, nw.y));
            const double highest = std::max(std::max(sw.y, se.y), std::max(ne.y, nw.y));
            const bool covered = finer != nullptr && finer->contains(0.5 * (sw.x + ne.x), -0.5 * (sw.z + ne.z));
            if (covered || lowest >= seaLevel)
            {
                endStrip(i);
            }
            else if (!inStrip)
            {
                inStrip = true;
                stripFrom = i;
            }
            if (covered || highest < seaLevel)
            {
                continue;   // drawn by the finer grid, or all of it under the sea
            }
            const double mean = 0.25 * (sw.y + se.y + ne.y + nw.y);
            const double slope = std::hypot(0.5 * (se.y - sw.y + ne.y - nw.y), 0.5 * (nw.y - sw.y + ne.y - se.y)) / cell;
            const Color color = groundColor(mean, slope, beach[j*(level.getNx() - 1) + i], chequered && ((i + j) % 2) == 1);
            m_mesh.addGroundTriangle(sw, se, ne, color);
            m_mesh.addGroundTriangle(sw, ne, nw, color);
        }
        endStrip(level.getNx() - 1);
    }
}

// Beyond the last grid there is only sea: four sheets around it
void Scene::addSeaAround(const TerrainLevel& coarsest)
{
    const double w = coarsest.getWest();
    const double e = coarsest.getEast();
    const double s = coarsest.getSouth();
    const double n = coarsest.getNorth();
    const double far = m_seaReach;
    const auto sheet = [&](double west, double south, double east, double north)
        {
            const Vec3 a(west, 0.0, -south);
            const Vec3 b(east, 0.0, -south);
            const Vec3 c(east, 0.0, -north);
            const Vec3 d(west, 0.0, -north);
            m_mesh.addFlatTriangle(a, b, c, m_sea, m_sea, m_sea);
            m_mesh.addFlatTriangle(a, c, d, m_sea, m_sea, m_sea);
        };
    sheet(w - far, s - far, e + far, s);
    sheet(w - far, n, e + far, n + far);
    sheet(w - far, s, w, n);
    sheet(e, s, e + far, n);
}

// Ponds and ditches: every tile that holds water, at its own level; the finest grid that has the tile draws it
void Scene::addWater(const Terrain& terrain)
{
    for (size_t k = 0; k < terrain.levelCount(); ++k)
    {
        const TerrainLevel& level = terrain.getLevel(k);
        const TerrainLevel* finer = k > 0 ? &terrain.getLevel(k - 1) : nullptr;
        const double cell = level.getCell();
        for (size_t j = 0; j + 1 < level.getNz(); ++j)
        {
            for (size_t i = 0; i + 1 < level.getNx(); ++i)
            {
                const double west = level.getWest() + (double) i * cell;
                const double south = level.getSouth() + (double) j * cell;
                if (!level.hasWater(i, j) || (finer != nullptr && finer->contains(west + 0.5 * cell, south + 0.5 * cell)))
                {
                    continue;
                }
                const double y = level.waterLevel(i, j) + m_waterLift;
                const Vec3 a(west, y, -south);
                const Vec3 b(west + cell, y, -south);
                const Vec3 c(west + cell, y, -(south + cell));
                const Vec3 d(west, y, -(south + cell));
                m_mesh.addFlatTriangle(a, b, c, m_water, m_water, m_water);
                m_mesh.addFlatTriangle(a, c, d, m_water, m_water, m_water);
            }
        }
    }
}

// Every road triangle lies within one triangle of the ground, so lifting its corners lays it flat on it
void Scene::addRoads(const World& world)
{
    const WorldData& data = world.getData();
    const Terrain& terrain = world.getTerrain();
    const Color colours[] = { m_asphalt, m_bricks, m_shell, m_track };
    for (size_t k = 0; k < data.m_roadKinds.size(); ++k)
    {
        const float* t = data.m_roadCorners.data() + 6*k;
        const Color& color = colours[data.m_roadKinds[k]];
        const auto place = [&](double east, double north)
            {
                return Vec3(east, terrain.height(east, north) + m_roadLift, -north);
            };
        m_mesh.addFlatTriangle(place(t[0], t[1]), place(t[2], t[3]), place(t[4], t[5]), color, color, color);
    }
}
