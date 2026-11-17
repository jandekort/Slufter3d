// Slufter3D - where a model stands
// Author: Jan de Kort
// Created: 2026-11-16
#pragma once

#include "Math.h"

class Ground;

// Struct Site
// A model is drawn in the frame of a footprint: u along the long axis, v across it with the front at +v,
// y up from the floor. Positions are x east, y up, z south.
struct Site
{
    Vec3 m_floor;       // centre of the footprint at floor level
    Vec3 m_axisLength;  // level unit vector along the long axis
    Vec3 m_axisWidth;   // level unit vector towards the front
    double m_length = 10.0;
    double m_width = 7.0;
    const Ground* m_ground = nullptr; // terraces, pegs and the like are laid on it; none->level with the floor
    bool m_mirrored = false;    // true->m_axisLength turned round, so that the model is drawn as its mirror image
};
