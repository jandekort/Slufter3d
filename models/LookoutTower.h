// Slufter3D - the wooden lookout tower (Uitkijktoren, Q on the park map)
// Author: Jan de Kort
// Created: 2026-11-15
#pragma once

#include "Color.h"
#include "Math.h"

class Mesh;
class Ground;

// Class LookoutTower
// Drawn after photo Uitkijktoren: four leaning posts, two decks with slatted railings,
// crossed braces underneath, a ladder, a rail fence around the foot and a flagpole beside it.
class LookoutTower
{
public:
    LookoutTower();

    // Mutator
    void build(Mesh& mesh, const Ground& ground, double east, double north) const;

    // Accessor
    double getFootprint() const { return m_baseHalf; } // half the side of the square it stands on [m]
    double getHeight() const { return m_upperDeck + m_railing; }

private:
    void addDeck(Mesh& mesh, const Vec3& foot, double height, double half) const;
    void addFence(Mesh& mesh, const Ground& ground, double east, double north) const;

    double m_baseHalf;     // posts at the ground, from the centre [m]
    double m_topHalf;      // posts at the top [m]
    double m_lowerDeck;    // above the ground [m]
    double m_upperDeck;
    double m_railing;      // height of the railings [m]
    double m_post;         // half thickness of a post [m]
    int m_slats;           // per side of a railing
    double m_fenceRadius;
    int m_fencePosts;
    double m_fenceHeight;
    double m_flagpole;     // height [m]
    Color m_wood;          // weathered grey
    Color m_flag;
};
