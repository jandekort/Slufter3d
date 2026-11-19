// Slufter3D - the list of vector models
// Author: Jan de Kort
// Created: 2026-11-16
#include "Models.h"

const std::vector<const Model*>& Models::all()
{
    static const std::vector<const Model*> models = {
        &comfort6C(),
        &comfort4C(),
        &comfort8C(),
        &luxe8L(),
        &luxe12L(),
        &luxe6EL(),
        &luxe10L(),
        &luxe6L(),
        &luxe4EL(),
        &visdiefje(),
        &bungalowRood(),
        &bungalowGroen(),
        &bungalowGeel(),
        &krimweg73(),
        &tent(),
        &safariTent(),
        &receptionWing() };
    return models;
}

const Model* Models::byName(const std::string& name)
{
    for (const Model* model : all())
    {
        if (name == model->name())
        {
            return model;
        }
    }
    return nullptr;
}
