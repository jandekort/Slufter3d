// Slufter3D - the ground: grids of heights, finer where the park is
// Author: Jan de Kort
// Created: 2026-11-16
#pragma once

#include "Ground.h"
#include "Math.h"
#include "WorldData.h"

#include <cstddef>
#include <vector>

// Class TerrainLevel
// One grid. Every tile is two triangles split along the diagonal from its south-west to its
// north-east corner; height() follows those triangles, so walker and picture agree.
class TerrainLevel
{
public:
    explicit TerrainLevel(const WorldData::Level& level);

    // Mutator
    void joinTo(const TerrainLevel& coarser); // Sets the nodes along the edge on the ground of the coarser grid

    // Accessor
    double height(double east, double north) const;      // positions outside take the nearest edge
    double nodeHeight(size_t i, size_t j) const;
    bool hasWater(size_t i, size_t j) const;             // on the tile north-east of node (i, j)
    double waterLevel(size_t i, size_t j) const;
    bool waterAt(double east, double north) const;
    bool contains(double east, double north, double margin = 0.0) const;
    size_t tileOf(double east, double north) const;      // index of the tile under a point, j * (nx - 1) + i
    size_t getNx() const { return m_nx; }
    size_t getNz() const { return m_nz; }
    double getCell() const { return m_cell; }
    double getWest() const { return m_west; }
    double getSouth() const { return m_south; }
    double getEast() const { return m_west + (double) (m_nx - 1) * m_cell; }
    double getNorth() const { return m_south + (double) (m_nz - 1) * m_cell; }

private:
    size_t m_nx;
    size_t m_nz;
    double m_cell;
    double m_west;
    double m_south;
    std::vector<float> m_height;
    std::vector<float> m_water;
};

// Class Terrain
// The grids lie inside each other, finest first. A point takes its height from the finest grid it lies in;
// the grids meet without a crack, because tools/pack_world.py eases each into the next.
class Terrain : public Ground
{
public:
    explicit Terrain(const WorldData& data);

    // Accessor
    double height(double east, double north) const override; // [m NAP]; beyond the last grid the bed of the sea
    Vec3 normal(double east, double north) const;            // unit normal of the ground
    bool contains(double east, double north, double margin = 0.0) const; // within the coarsest grid
    bool waterAt(double east, double north) const;           // inland water on the tile underneath
    bool seaAt(double east, double north) const;             // the ground lies under the sea
    size_t levelCount() const { return m_levels.size(); }
    const TerrainLevel& getLevel(size_t k) const { return m_levels.at(k); }
    double getSeaLevel() const { return m_seaLevel; }

private:
    const TerrainLevel* levelAt(double east, double north) const; // the finest grid a point lies in; nullptr->none

    std::vector<TerrainLevel> m_levels;
    double m_seaLevel;    // [m NAP]
    double m_seaBed;      // height beyond the last grid [m NAP]
    double m_wading;      // ground this far under the sea level counts as sea [m]
};
