// Slufter3D - the ground: grids of heights, finer where the park is
// Author: Jan de Kort
// Created: 2026-11-16
#include "Terrain.h"

#include "Check.h"

#include <algorithm>

// Class TerrainLevel
TerrainLevel::TerrainLevel(const WorldData::Level& level)
    :
    m_nx(level.m_nx)
    , m_nz(level.m_nz)
    , m_cell(level.m_cell)
    , m_west(level.m_west)
    , m_south(level.m_south)
    , m_height(level.m_height)
    , m_water(level.m_water)
{
    require(m_nx > 1 && m_nz > 1 && m_cell > 0.0, "Terrain grid is empty");
    require(m_height.size() == m_nx * m_nz && m_water.size() == (m_nx - 1) * (m_nz - 1), "Terrain grid is incomplete");
}

// The file holds whole centimetres; here the edge is put exactly on the coarser grid, so that no crack shows
void TerrainLevel::joinTo(const TerrainLevel& coarser)
{
    for (size_t j = 0; j < m_nz; ++j)
    {
        for (size_t i = 0; i < m_nx; ++i)
        {
            if (i == 0 || j == 0 || i + 1 == m_nx || j + 1 == m_nz)
            {
                m_height[j*m_nx + i] = (float) coarser.height(m_west + (double) i * m_cell, m_south + (double) j * m_cell);
            }
        }
    }
}

double TerrainLevel::nodeHeight(size_t i, size_t j) const
{
    return m_height.at(j*m_nx + i);
}

bool TerrainLevel::hasWater(size_t i, size_t j) const
{
    return m_water.at(j*(m_nx - 1) + i) > 0.5f * WorldData::dry();
}

double TerrainLevel::waterLevel(size_t i, size_t j) const
{
    return m_water.at(j*(m_nx - 1) + i);
}

size_t TerrainLevel::tileOf(double east, double north) const
{
    const double u = clamp((east - m_west) / m_cell, 0.0, (double) (m_nx - 1));
    const double v = clamp((north - m_south) / m_cell, 0.0, (double) (m_nz - 1));
    return std::min((size_t) v, m_nz - 2) * (m_nx - 1) + std::min((size_t) u, m_nx - 2);
}

bool TerrainLevel::waterAt(double east, double north) const
{
    return m_water[tileOf(east, north)] > 0.5f * WorldData::dry();
}

double TerrainLevel::height(double east, double north) const
{
    const double u = clamp((east - m_west) / m_cell, 0.0, (double) (m_nx - 1));
    const double v = clamp((north - m_south) / m_cell, 0.0, (double) (m_nz - 1));
    const size_t i = std::min((size_t) u, m_nx - 2);
    const size_t j = std::min((size_t) v, m_nz - 2);
    const double fu = u - (double) i;
    const double fv = v - (double) j;
    const double h00 = nodeHeight(i, j);
    const double h10 = nodeHeight(i + 1, j);
    const double h01 = nodeHeight(i, j + 1);
    const double h11 = nodeHeight(i + 1, j + 1);
    if (fu >= fv)
    {
        return h00 + (h10 - h00)*fu + (h11 - h10)*fv; // south-east triangle
    }
    return h00 + (h11 - h01)*fu + (h01 - h00)*fv;     // north-west triangle
}

bool TerrainLevel::contains(double east, double north, double margin /* = 0.0 */) const
{
    return east >= getWest() + margin && east <= getEast() - margin
        && north >= getSouth() + margin && north <= getNorth() - margin;
}

// Class Terrain
Terrain::Terrain(const WorldData& data)
    :
    m_levels()
    , m_seaLevel(0.0)
    , m_seaBed(-1.5)
    , m_wading(0.25)
{
    require(!data.m_levels.empty(), "No terrain in the world file");
    m_levels.reserve(data.m_levels.size());
    std::transform(data.m_levels.begin(), data.m_levels.end(), std::back_inserter(m_levels),
        [&](const WorldData::Level& level)
        {
            return TerrainLevel(level);
        });
    for (size_t k = 0; k + 1 < m_levels.size(); ++k)
    {
        m_levels[k].joinTo(m_levels[k + 1]);
    }
}

const TerrainLevel* Terrain::levelAt(double east, double north) const
{
    const auto it = std::find_if(m_levels.begin(), m_levels.end(),
        [&](const TerrainLevel& level)
        {
            return level.contains(east, north);
        });
    return it != m_levels.end() ? &*it : nullptr;
}

double Terrain::height(double east, double north) const
{
    const TerrainLevel* level = levelAt(east, north);
    return level != nullptr ? level->height(east, north) : m_seaBed;
}

Vec3 Terrain::normal(double east, double north) const
{
    const double step = 0.05;
    const double dhEast = (height(east + step, north) - height(east - step, north)) / (2.0 * step);
    const double dhNorth = (height(east, north + step) - height(east, north - step)) / (2.0 * step);
    return normalize(Vec3(-dhEast, 1.0, dhNorth));
}

bool Terrain::contains(double east, double north, double margin /* = 0.0 */) const
{
    return m_levels.back().contains(east, north, margin);
}

bool Terrain::waterAt(double east, double north) const
{
    const TerrainLevel* level = levelAt(east, north);
    return level != nullptr && level->waterAt(east, north);
}

bool Terrain::seaAt(double east, double north) const
{
    return height(east, north) < m_seaLevel - m_wading;
}
