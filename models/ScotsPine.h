// Slufter3D - a Scots pine (Pinus sylvestris), the tree of the woods around the park
// Author: Jan de Kort
// Created: 2026-11-16
#pragma once

#include "Color.h"
#include "Math.h"

class Mesh;

// Class ScotsPine
// A bare, orange-brown trunk that carries a flattened, blue-green crown high up: a wide lobe, and on a tree
// of some height a smaller one on top of it, a little to the side. A low one is a shrub: one lobe near the ground.
// Height and width of the crown are those measured (data/trees.json); the rest follows from them.
class ScotsPine
{
public:
    ScotsPine();

    // Mutator
    // height: ground to top [m]; crownRadius [m]; no two trees are alike: turn [rad] sets which way it leans
    void build(Mesh& mesh, const Vec3& foot, double height, double crownRadius, double turn) const;

private:
    // A lobe of needles: a ring of the given radius, closed by a point above and a point below
    void addLobe(Mesh& mesh, const Vec3& center, double radius, double up, double down, double turn,
        const Color& color) const;

    int m_trunkSides;
    int m_lobeSides;
    double m_trunkRadius;     // at the foot, as a part of the height
    double m_trunkRadiusMin;  // [m]
    double m_taper;           // radius of the trunk under the crown, as a part of that at the foot
    double m_crownFrom;       // the crown of a tall tree starts at this part of its height
    double m_shrubHeight;     // lower than this is a shrub, with its crown near the ground [m]
    double m_twoLobesFrom;    // from this height a tree has two lobes [m]
    double m_topLobe;         // radius of the upper lobe, as a part of that of the lower one
    double m_topShift;        // and how far it stands to the side, as a part of the radius
    double m_root;            // the trunk reaches this far into the ground, for trees on a slope [m]
    Color m_bark;
    Color m_needles;
    Color m_needlesTop;       // the upper lobe catches more light
};
