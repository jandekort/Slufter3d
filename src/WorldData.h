// Slufter3D - the world as data, read from docs/data/world.bin
// Author: Jan de Kort
// Created: 2026-11-16
#pragma once

#include <cstddef>
#include <string>
#include <vector>

// Struct WorldData
// What tools/pack_world.py wrote, in the same order. Positions are metres east and north of the origin,
// heights metres above NAP, azimuths degrees clockwise from north.
struct WorldData
{
    // Struct Level: a grid of ground heights; every tile may hold water
    struct Level
    {
        double m_west = 0.0;            // east coordinate of the first column
        double m_south = 0.0;           // north coordinate of the first row
        double m_cell = 1.0;            // tile size [m]
        size_t m_nx = 0;                // nodes from west to east
        size_t m_nz = 0;                // nodes from south to north
        std::vector<float> m_height;    // at the nodes, rows from south to north
        std::vector<float> m_water;     // level of the water on every tile, (m_nx - 1) * (m_nz - 1);
                                        // dry()->none, sea()->the tile is sea
    };

    // Struct HouseRecord
    struct HouseRecord
    {
        int m_number = 0;               // 0->none on the park map
        int m_semiWith = 0;             // number of the other half of the block, 0->detached
        std::string m_type;             // type on the park map, "8L"
        std::string m_name;             // name on the house, "" when it has none
        std::string m_model;            // name of the model that draws it, "Luxe8L"
        bool m_semi = false;            // true->half of a semi-detached block
        bool m_pitch = false;           // true->a camping pitch, false->a building
        bool m_outline = false;         // true->drawn at the size of its outline, false->the model decides
        unsigned m_eras = 0;            // bit k set->it stands in era k of the park, see eraCount()
        double m_east = 0.0;            // centre of the outline
        double m_north = 0.0;
        double m_length = 0.0;          // outline; for a building that of its roof
        double m_width = 0.0;
        double m_axis = 0.0;            // azimuth of the long axis
        double m_front = 0.0;           // azimuth the front faces
        double m_floor = 0.0;           // height of the floor
    };

    // Struct WingRecord: a wing of the central building
    struct WingRecord
    {
        double m_east = 0.0;
        double m_north = 0.0;
        double m_length = 0.0;
        double m_width = 0.0;
        double m_axis = 0.0;
    };

    // Struct TreeRecord: a tree found in the AHN (tools/make_trees.py)
    struct TreeRecord
    {
        double m_east = 0.0;
        double m_north = 0.0;
        double m_height = 0.0;          // ground to top [m]
        double m_crownRadius = 0.0;     // [m]
    };

    static float dry() { return -1.0e9f; }  // value of Level::m_water on a tile without water
    static float sea() { return -2.0e9f; }  // value of Level::m_water on a tile of the sea
    static int roadKinds() { return 4; }    // 0->asphalt, 1->bricks, 2->shells, 3->sand
    // The eras of the park: 0->1957 to 1971, the white bungalows; 1->1971 to 1993, with the chalets;
    // 2->1993 to 2013, the Comfort bungalows; 3->2013 to today, with the Luxe bungalows
    static int eraCount() { return 4; }
    static int today() { return 3; }

    // Reads the file; anything wrong with it stops the program
    static WorldData parse(const unsigned char* bytes, size_t size);

    double m_originEast = 0.0;          // RD New of the origin
    double m_originNorth = 0.0;
    std::vector<Level> m_levels;        // finest first; each lies within the next
    std::vector<HouseRecord> m_houses;
    std::vector<WingRecord> m_wings;
    double m_entranceEast = 0.0;        // entrance of the central building, on a wall of a wing
    double m_entranceNorth = 0.0;
    double m_entranceAzimuth = 0.0;     // the way it faces
    double m_towerEast = 0.0;           // lookout tower
    double m_towerNorth = 0.0;
    double m_startEast = 0.0;           // where the walk starts
    double m_startNorth = 0.0;
    double m_startHeading = 0.0;
    double m_lighthouseEast = 0.0;
    double m_lighthouseNorth = 0.0;
    double m_lighthouseHeight = 0.0;    // foot to top [m]
    std::vector<float> m_roadCorners;   // six per triangle: east, north of its three corners
    std::vector<unsigned char> m_roadKinds; // one per triangle, below roadKinds()
    std::vector<unsigned char> m_roadEras;  // one per triangle: bit k set->it lies there in era k
    std::vector<TreeRecord> m_trees;
};
