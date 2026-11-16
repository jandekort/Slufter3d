// Slufter3D - a vector model of a building, drawn after photographs of the park
// Author: Jan de Kort
// Created: 2026-11-16
#pragma once

#include "Site.h"

#include <utility>

class Mesh;

// Class Model
class Model // Base class of the vector models; one derived class per model, each in its own file
{
public:
    virtual ~Model() = default;

    // Accessor
    virtual const char* name() const = 0;   // as in data/type_models.json: "Comfort6C"
    virtual const char* label() const = 0;  // short, for the screen: "6C"
    // Length and width of the model for a house whose outline is outlineLength by outlineWidth [m];
    // for a building that is the outline of its roof
    virtual std::pair<double, double> footprint(double outlineLength, double outlineWidth) const = 0;
    virtual double ridgeHeight(double width) const = 0; // above the floor [m]
    // true->the point (du, dv) in the frame of a model of this size (along its length, towards its front)
    // lies within margin of its walls
    virtual bool covers(double length, double width, double du, double dv, double margin) const;

    virtual void build(Mesh& mesh, const Site& site) const = 0; // Adds the model to the mesh
};
