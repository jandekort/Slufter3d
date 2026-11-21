// Slufter3D - the picture: the mesh of the world and the visitor's camera
// Author: Jan de Kort
// Created: 2026-11-16
#include "Scene.h"

#include "ScotsPine.h"

#include "Check.h"
#include "Lighthouse.h"
#include "Model.h"
#include "Terrain.h"
#include "Walker.h"
#include "World.h"

#include <algorithm>
#include <cstdint>
#include <deque>

namespace
{
    const double pi = 3.14159265358979323846;

    // A number in [0, 1] for every corner of a lattice, the same on every run and on every machine
    double lattice(int i, int j, uint32_t seed)
    {
        uint32_t h = (uint32_t) i * 374761393u + (uint32_t) j * 668265263u + seed * 2246822519u;
        h = (h ^ (h >> 13)) * 1274126177u;
        h ^= h >> 16;
        return (double) h / 4294967295.0;
    }

    // Smooth noise in [-1, 1], with bumps about one lattice step apart
    double valueNoise(double x, double y, uint32_t seed)
    {
        const double x0 = std::floor(x);
        const double y0 = std::floor(y);
        const int i = (int) x0;
        const int j = (int) y0;
        const double u = (x - x0) * (x - x0) * (3.0 - 2.0 * (x - x0));
        const double v = (y - y0) * (y - y0) * (3.0 - 2.0 * (y - y0));
        const double south = lattice(i, j, seed) + (lattice(i + 1, j, seed) - lattice(i, j, seed)) * u;
        const double north = lattice(i, j + 1, seed) + (lattice(i + 1, j + 1, seed) - lattice(i, j + 1, seed)) * u;
        return 2.0 * (south + (north - south) * v) - 1.0;
    }
}

Scene::Scene()
    :
    m_mesh()
    , m_skyVertexCount(0)
    , m_flatVertexCount(0)
    , m_treeVertexCount(0)
    , m_groundVertexCount(0)
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
    , m_dry(0.66f, 0.62f, 0.36f)
    , m_lush(0.17f, 0.40f, 0.20f)
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
    , m_patches({ { 60.0, 0.40, 0.4 }, { 15.0, 0.35, 1.1 }, { 5.0, 0.25, 2.0 } })
    , m_patchTint(0.9)
    , m_patchLight(0.35)
    , m_tileContrast(0.04)
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
    if (world.hasReception())
    {
        world.getReception().build(m_mesh);
    }
    world.getTower().build(m_mesh, terrain, data.m_towerEast, data.m_towerNorth);
    for (const House& house : world.getHouses())
    {
        house.m_model->build(m_mesh, house.m_site);
    }
    Lighthouse().build(m_mesh, Vec3(data.m_lighthouseEast, terrain.height(data.m_lighthouseEast, data.m_lighthouseNorth),
        -data.m_lighthouseNorth));

    const size_t beforeTrees = m_mesh.vertexCount();
    addTrees(world);
    m_treeVertexCount = m_mesh.vertexCount() - beforeTrees;

    const size_t before = m_mesh.vertexCount();
    addWater(terrain);
    addRoads(world);
    m_flatVertexCount = m_mesh.vertexCount() - before;
}

void Scene::setFieldOfView(double degrees)
{
    m_fieldOfView = clamp(degrees, 5.0, 100.0) * pi / 180.0;
}

double Scene::getFieldOfView() const
{
    return m_fieldOfView * 180.0 / pi;
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
// low ground that the sea can reach. Patches make the vegetation drier or lusher, the ground lighter or darker.
Color Scene::groundColor(double height, double slope, double beach, double dry, double light) const
{
    const double dune = clamp((height - m_meadowHeight) / (m_duneHeight - m_meadowHeight), 0.0, 1.0);
    Color color = height < m_meadowHeight ? m_meadow : m_grass.mixed(m_marram, (float) dune);
    color = color.mixed(dry > 0.0 ? m_dry : m_lush, (float) (m_patchTint * std::fabs(dry)));
    const double bare = clamp((slope - m_sandSlope) / m_sandSlope, 0.0, 1.0);
    color = color.mixed(m_sand, (float) (0.8 * bare));
    color = color.mixed(m_sand, (float) beach);
    return color.scaled((float) (1.0 + m_patchLight * light));
}

// A sum of layers of smooth noise. A grid cannot show bumps closer together than two of its tiles, so on the
// coarser grids the fine layers fade out.
double Scene::patches(double east, double north, double cell, unsigned seed) const
{
    double sum = 0.0;
    for (size_t k = 0; k < m_patches.size(); ++k)
    {
        const Patch& patch = m_patches[k];
        const double fade = clamp(patch.m_size / (2.0 * cell), 0.0, 1.0);
        const double c = std::cos(patch.m_turn);
        const double s = std::sin(patch.m_turn);
        sum += fade * patch.m_weight * valueNoise((c * east - s * north) / patch.m_size, (s * east + c * north) / patch.m_size,
            (uint32_t) (seed * m_patches.size() + k));
    }
    return sum;
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

// The land of every grid first, so that the page can draw it apart, then the sea
void Scene::addGround(const Terrain& terrain)
{
    std::vector<Vec3> sea;
    for (size_t k = 0; k < terrain.levelCount(); ++k)
    {
        addLevel(terrain.getLevel(k), k > 0 ? &terrain.getLevel(k - 1) : nullptr, sea);
    }
    m_groundVertexCount = m_mesh.vertexCount() - m_skyVertexCount;
    for (size_t k = 0; k + 3 < sea.size(); k += 4)
    {
        m_mesh.addFlatTriangle(sea[k], sea[k + 1], sea[k + 2], m_sea, m_sea, m_sea);
        m_mesh.addFlatTriangle(sea[k], sea[k + 2], sea[k + 3], m_sea, m_sea, m_sea);
    }
    addSeaAround(terrain.getLevel(terrain.levelCount() - 1));
}

// Starting from the tiles of the sea, over neighbouring tiles of low ground, as far as the beach reaches. Only
// the sea counts: low ground inland, the polder below 0 m NAP with its ditches, does not start a beach.
std::vector<int> Scene::stepsFromSea(const TerrainLevel& level) const
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
            if (level.isSea(i, j))
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
    return steps;
}

// Colour and light belong to the nodes, and the triangles blend them between their corners, so the ground shows
// no tiles. The slope at a node comes from its neighbours on either side, at the edge of the grid from the one
// inside. A node takes the beach from the land around it, and nodes amidst the sea are sand.
std::vector<Color> Scene::nodeColors(const TerrainLevel& level, const std::vector<int>& steps) const
{
    const size_t nx = level.getNx();
    const size_t nz = level.getNz();
    const double cell = level.getCell();
    std::vector<Color> colors;
    colors.reserve(nx * nz);
    for (size_t j = 0; j < nz; ++j)
    {
        for (size_t i = 0; i < nx; ++i)
        {
            const size_t west = i > 0 ? i - 1 : i;
            const size_t east = i + 1 < nx ? i + 1 : i;
            const size_t south = j > 0 ? j - 1 : j;
            const size_t north = j + 1 < nz ? j + 1 : j;
            const double alongEast = (level.nodeHeight(east, j) - level.nodeHeight(west, j)) / ((double) (east - west) * cell);
            const double alongNorth = (level.nodeHeight(i, north) - level.nodeHeight(i, south)) / ((double) (north - south) * cell);
            int land = 0;
            int beach = 0;
            for (size_t tj = std::min(south, nz - 2); tj <= std::min(j, nz - 2); ++tj)
            {
                for (size_t ti = std::min(west, nx - 2); ti <= std::min(i, nx - 2); ++ti)
                {
                    const int step = steps[tj*(nx - 1) + ti];
                    land += step != 0 ? 1 : 0;
                    beach += step > 0 ? 1 : 0;
                }
            }
            const double x = level.getWest() + (double) i * cell;
            const double y = level.getSouth() + (double) j * cell;
            const Color color = groundColor(level.nodeHeight(i, j), std::hypot(alongEast, alongNorth),
                land > 0 ? (double) beach / (double) land : 1.0, patches(x, y, cell, 1u), patches(x, y, cell, 2u));
            colors.push_back(m_mesh.litGround(color, normalize(Vec3(-alongEast, 1.0, alongNorth))));
        }
    }
    return colors;
}

// One grid, without the part that a finer grid covers. Tiles of the sea and of the beach that reach below the
// sea level get the sea as well, in strips from west to east; such tiles wholly under it get the sea only.
// Low ground inland keeps its ground, however low.
void Scene::addLevel(const TerrainLevel& level, const TerrainLevel* finer, std::vector<Vec3>& sea)
{
    const double cell = level.getCell();
    const double seaLevel = 0.0;
    const std::vector<int> steps = stepsFromSea(level);
    const std::vector<Color> colors = nodeColors(level, steps);
    auto node = [&](size_t i, size_t j)
    {
        return Vec3(level.getWest() + (double) i * cell, level.nodeHeight(i, j), -(level.getSouth() + (double) j * cell));
    };
    auto color = [&](size_t i, size_t j)
    {
        return colors[j*level.getNx() + i];
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
                    sea.insert(sea.end(), { a, b, c, d });
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
            const int step = steps[j*(level.getNx() - 1) + i];
            const bool coast = step >= 0;   // the sea, or the beach it reaches
            if (covered || lowest >= seaLevel || !coast)
            {
                endStrip(i);
            }
            else if (!inStrip)
            {
                inStrip = true;
                stripFrom = i;
            }
            if (covered || (coast && highest < seaLevel))
            {
                continue;   // drawn by the finer grid, or all of it under the sea
            }
            m_mesh.addFlatTriangle(sw, se, ne, color(i, j), color(i + 1, j), color(i + 1, j + 1));
            m_mesh.addFlatTriangle(sw, ne, nw, color(i, j), color(i + 1, j + 1), color(i, j + 1));
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
    const unsigned int era = 1u << world.getEra();
    for (size_t k = 0; k < data.m_roadKinds.size(); ++k)
    {
        if ((data.m_roadEras[k] & era) == 0)
        {
            continue;       // this road was not there, or is there no more
        }
        const float* t = data.m_roadCorners.data() + 6*k;
        const Color& color = colours[data.m_roadKinds[k]];
        const auto place = [&](double east, double north)
            {
                return Vec3(east, terrain.height(east, north) + m_roadLift, -north);
            };
        m_mesh.addFlatTriangle(place(t[0], t[1]), place(t[2], t[3]), place(t[4], t[5]), color, color, color);
    }
}

void Scene::addTrees(const World& world)
{
    const Terrain& terrain = world.getTerrain();
    const ScotsPine pine;
    for (const WorldData::TreeRecord& tree : world.getData().m_trees)
    {
        // Which way a tree leans follows from where it stands: the same on every run
        const double turn = std::fmod(std::abs(tree.m_east * 12.9898 + tree.m_north * 78.233), 6.283185307179586);
        pine.build(m_mesh, Vec3(tree.m_east, terrain.height(tree.m_east, tree.m_north), -tree.m_north),
            tree.m_height, tree.m_crownRadius, turn);
    }
}
