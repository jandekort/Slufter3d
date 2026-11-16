// Slufter3D - the central building with the reception
// Author: Jan de Kort
// Created: 2026-11-15
#pragma once

#include "Color.h"
#include "Site.h"

#include <vector>

class Mesh;
class Terrain;
struct WorldData;

// Class Reception
// Drawn after photo Receptie. The surveyed outline of the building is covered with
// rectangular wings (WorldData::m_wings); every wing is white brick on a black plinth
// under its own red tiled roof with black boarded gables. Where wings overlap the
// roofs cross, as on the real building. The entrance has the round white canopy.
class Reception
{
public:
    Reception(const WorldData& data, const Terrain& terrain);

    // Mutator
    void build(Mesh& mesh) const;

    // Accessor
    const std::vector<Site>& getWings() const { return m_wings; }
    bool contains(double east, double north, double margin) const;

private:
    void addEntrance(Mesh& mesh) const;

    std::vector<Site> m_wings;
    Vec3 m_door;            // foot of the entrance, on the wall
    Vec3 m_facing;          // level unit vector out of the entrance
    double m_floorLift;     // floor above the mean ground [m]
    double m_canopyHalf;    // half the width of the canopy [m]
    double m_canopyReach;   // how far its middle stands out from the wall [m]
    double m_canopyFoot;    // underside above the floor [m]
    double m_canopyHead;    // top above the floor [m]
    int m_canopySegments;
    Color m_white;
    Color m_glass;
    Color m_sign;           // the board with RECEPTIE on it
    Color m_column;
    Color m_paving;
};
