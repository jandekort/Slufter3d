// Slufter3D - the picture: the mesh of the world and the visitor's camera
// Author: Jan de Kort
// Created: 2026-11-16
#pragma once

#include "Math.h"
#include "Mesh.h"

#include <cstddef>
#include <vector>

class Terrain;
class TerrainLevel;
class Walker;
class World;

// Class Scene
// The world does not change, so its mesh is built once. The buffer holds the sky, then the land with
// everything that stands on it, then the trees (so that the page can leave them out), then what lies flat on the land: roads and water. The sky is drawn
// around the eye; the flat things are drawn last, nudged towards the eye, so that they never flicker
// against the ground they lie on.
class Scene
{
public:
    Scene();

    // Mutator
    void build(const World& world);

    // Accessor
    const Mesh& getMesh() const { return m_mesh; }
    size_t skyVertexCount() const { return m_skyVertexCount; }
    size_t flatVertexCount() const { return m_flatVertexCount; }   // the last ones of the buffer
    size_t treeVertexCount() const { return m_treeVertexCount; }   // just before the flat ones
    Mat4 viewProjection(const Walker& walker, double aspect) const;
    // beach: share of sand that the sea brings, in [0, 1]; dry and light: the patch the ground lies in, in [-1, 1]
    Color groundColor(double height, double slope, double beach, double dry, double light) const;
    const Color& getHorizon() const { return m_horizon; }
    double getFogRange() const { return m_fogRange; }

private:
    void addSky();
    void addGround(const Terrain& terrain);
    void addLevel(const TerrainLevel& level, const TerrainLevel* finer);
    void addSeaAround(const TerrainLevel& coarsest);
    void addWater(const Terrain& terrain);
    void addRoads(const World& world);
    void addTrees(const World& world);
    // Steps from the sea to every tile of a grid over low ground: 0 on the sea, more on beach and salt marsh;
    // -1 where the sea does not reach
    std::vector<int> stepsFromSea(const TerrainLevel& level) const;
    // The ground at every node of a grid, lit, rows from south to north
    std::vector<Color> nodeColors(const TerrainLevel& level, const std::vector<int>& steps) const;
    // Gentle, irregular patches in [-1, 1], the same on every run; every seed gives other patches
    double patches(double east, double north, double cell, unsigned seed) const;

    // Struct Patch: one layer of the patches on the ground
    struct Patch
    {
        double m_size;      // distance between the bumps of its noise [m]
        double m_weight;
        double m_turn;      // its lattice is turned this far, so that no layer runs along the grid [rad]
    };

    Mesh m_mesh;
    size_t m_skyVertexCount;
    size_t m_flatVertexCount;
    size_t m_treeVertexCount;
    double m_fieldOfView;
    double m_near;          // nearest distance drawn, on the ground [m]
    double m_nearPerHeight; // and what it grows by per metre of altitude
    double m_nearMax;
    double m_far;
    double m_fogRange;      // distance over which the land fades into the horizon [m]
    double m_skyRadius;
    int m_skyRings;
    int m_skySegments;
    Color m_zenith;         // deep blue overhead, as in photo B449a
    Color m_horizon;
    Color m_grass;          // low, sheltered ground
    Color m_marram;         // dune tops
    Color m_meadow;         // the polder
    Color m_sand;           // steep, bare slopes and the beach
    Color m_dry;            // straw: patches of drier vegetation
    Color m_lush;           // patches of deeper green
    Color m_water;
    Color m_sea;
    Color m_asphalt;
    Color m_bricks;
    Color m_shell;          // shell paths
    Color m_track;          // unpaved roads
    double m_meadowHeight;  // ground below this is polder [m NAP]
    double m_duneHeight;    // ground above this is all marram [m NAP]
    double m_sandSlope;     // slope from which sand shows
    double m_beachHeight;   // ground below this that the sea can reach is beach [m NAP]
    double m_beachReach;    // and no further from the sea than this [m]
    std::vector<Patch> m_patches; // broad to fine
    double m_patchTint;     // how far a patch shifts the vegetation towards m_dry or m_lush
    double m_patchLight;    // how much lighter or darker a patch makes the ground
    double m_roadLift;      // roads lie this far above the ground [m]
    double m_waterLift;     // water above its measured level [m]
    double m_seaReach;      // the sea is drawn this far beyond the last grid [m]
};
