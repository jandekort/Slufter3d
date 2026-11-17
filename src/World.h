// Slufter3D - the park and the land around it: ground, houses, roads, water
// Author: Jan de Kort
// Created: 2026-11-16
#pragma once

#include "LookoutTower.h"
#include "Reception.h"
#include "Site.h"
#include "Terrain.h"
#include "WorldData.h"

#include <string>
#include <vector>

class Model;

// Struct House
struct House
{
    int m_number = 0;                 // 0->none on the park map
    std::string m_type;               // type on the park map, "8L"
    std::string m_name;               // name on the house, "" when it has none
    const Model* m_model = nullptr;   // draws it
    Site m_site;
    double m_east = 0.0;
    double m_north = 0.0;
    bool m_semi = false;              // true->half of a semi-detached block
    bool m_pitch = false;             // true->a camping pitch

    std::string debugString() const;
};

// Class World
class World
{
public:
    enum class Surface
    {
        Asphalt,
        Bricks,
        Shells,
        Sand,     // an unpaved road or path
        Water,
        Sea,
        Dunes     // anything else: no road underfoot
    };

    explicit World(const WorldData& data);

    // Accessor
    const WorldData& getData() const { return m_data; }
    const Terrain& getTerrain() const { return m_terrain; }
    const std::vector<House>& getHouses() const { return m_houses; }
    const Reception& getReception() const { return m_reception; }
    const LookoutTower& getTower() const { return m_tower; }
    const House* findHouse(int number) const;                  // nullptr->no such number
    const House* nearestHouse(double east, double north) const;
    // The house someone at this spot is looking at; the nearest one when none is in view
    const House* houseInView(double east, double north, double heading) const;
    bool insideHouse(const House& house, double east, double north, double margin) const;
    bool obstacle(double east, double north) const;            // true->a wall, or the edge of the world
    bool standable(double east, double north) const;           // true->dry ground to stand on
    bool blocked(double east, double north) const { return obstacle(east, north) || !standable(east, north); }
    bool inTower(double east, double north) const;             // true->within the walls' margin of the lookout tower
    double getClearance() const { return m_clearance; }        // above this, nothing is in the way [m above the ground]
    // Above this, nothing at this point is in the way [m above the ground]: over the lookout tower just above its
    // railings, elsewhere higher than any roof
    double clearanceAt(double east, double north) const;
    Surface surfaceAt(double east, double north) const;
    static const char* surfaceName(Surface surface);
    // A free spot on the front side of a house, and the heading [rad] that faces the house from there
    void viewpoint(const House& house, double& east, double& north, double& heading) const;

private:
    void joinSemiDetached(const WorldData& data);
    void indexRoads();

    WorldData m_data;
    Terrain m_terrain;
    Reception m_reception;
    LookoutTower m_tower;
    std::vector<House> m_houses;
    std::vector<size_t> m_roadStart;    // per tile of the finest grid: its first road triangle in m_roadOrder
    std::vector<size_t> m_roadOrder;    // road triangles, tile by tile
    double m_wallMargin;    // a pedestrian keeps this far from walls [m]
    double m_edgeMargin;    // and this far from the edge of the world [m]
    double m_semiStagger;   // one half of a 4C block stands this far forward of the other, as in the BGT [m]
    double m_clearance;     // higher than any roof [m]
    double m_towerHeadroom; // over the lookout tower the feet stay this far above its railings [m]
    double m_viewDistance;  // viewpoint in front of a house [m]
    double m_viewRange;     // a house counts as looked at up to this far [m]
    double m_viewAngle;     // and this far off the line of sight [rad]
};
