// Slufter3D - the ground a model stands on
// Author: Jan de Kort
// Created: 2026-11-16
#pragma once

// Class Ground
class Ground // Base class of anything that gives the height of the ground
{
public:
    virtual ~Ground() = default;
    virtual double height(double east, double north) const = 0; // [m]
};
