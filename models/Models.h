// Slufter3D - the list of vector models
// Author: Jan de Kort
// Created: 2026-11-16
#pragma once

#include "Model.h"

#include <string>
#include <vector>

// One function per model, each defined in the file of its model: models/<Name>.cpp
namespace Models
{
    const Model& comfort6C();
    const Model& comfort4C();
    const Model& comfort8C();
    const Model& luxe8L();
    const Model& luxe12L();
    const Model& luxe6EL();
    const Model& luxe10L();
    const Model& luxe6L();
    const Model& luxe4EL();
    const Model& visdiefje();
    const Model& krimweg73();
    const Model& tent();
    const Model& safariTent();
    const Model& receptionWing();

    const std::vector<const Model*>& all();
    // The model of this name, as used in data/type_models.json; nullptr->there is none
    const Model* byName(const std::string& name);
}
