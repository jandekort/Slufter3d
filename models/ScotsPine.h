// Slufter3D - a Scots pine (Pinus sylvestris), the tree of the woods around the park
// Author: Jan de Kort
// Created: 2026-11-16
#pragma once

#include "Color.h"
#include "Math.h"

class Mesh;

// Class ScotsPine
// A bare trunk, grey-brown below and orange-brown higher up, that leans a little and carries a flattened,
// blue-green crown high up. The crown is a handful of clumps of needles: a few around the trunk, each on a
// limb of its own, and one on top. A young tree is a bushy cone of two clumps; a low one is a shrub: one
// clump near the ground. No clump is round: every corner of it stands a little in or out.
// Height and width of the crown are those measured (data/trees.json); the rest follows from them.
class ScotsPine
{
public:
    ScotsPine();

    // Mutator
    // height: ground to top [m]; crownRadius [m]; no two trees are alike: turn [rad] sets which way it leans
    // and where its clumps stand
    void build(Mesh& mesh, const Vec3& foot, double height, double crownRadius, double turn) const;

private:
    // The trunk from under the ground to the given height, in two parts, the upper end standing at lean
    void addTrunk(Mesh& mesh, const Vec3& foot, double radius, double top, const Vec3& lean, double turn,
        int sides) const;
    // A branch that carries a clump: a bar that gets thinner towards its end
    void addLimb(Mesh& mesh, const Vec3& from, const Vec3& to, double radius) const;
    // A clump of needles: a wide ring low and a smaller one higher up, closed by a point above and a point
    // below; seed sets how far each corner stands in or out
    void addClump(Mesh& mesh, const Vec3& center, double radius, double up, double down, double turn,
        double seed, const Color& color) const;
    // A number from 0 to 1 that looks random but follows from the seed: the same tree on every run
    double scatter(double seed) const;

    int m_trunkSides;
    int m_shrubTrunkSides;
    int m_limbSides;
    int m_clumpSides;
    double m_trunkRadius;     // at the foot, as a part of the height
    double m_trunkRadiusMin;  // [m]
    double m_taper;           // radius of the trunk under the crown, as a part of that at the foot
    double m_lean;            // the top of the trunk stands this far from above the foot, as a part of the height
    double m_barkChange;      // the bark turns orange at this part of the trunk
    double m_crownFrom;       // the crown of a tall tree starts at this part of its height
    double m_shrubHeight;     // lower than this is a shrub, with its crown near the ground [m]
    double m_tallFrom;        // from this height a tree has a crown of clumps on limbs [m]
    double m_threeFrom;       // from this crown radius there are three clumps around the trunk [m]
    double m_fourFrom;        // and from this one four [m]
    double m_clumpRadius;     // radius of a clump around the trunk, as a part of that of the crown
    double m_topRadius;       // and of the clump on top
    double m_topShift;        // how far the clump on top stands to the side, as a part of the crown radius
    double m_upperRing;       // radius of the upper ring of a clump, as a part of the lower one
    double m_rugged;          // a corner of a clump stands this part of the radius in or out
    double m_root;            // the trunk reaches this far into the ground, for trees on a slope [m]
    Color m_barkLow;          // old bark, near the ground
    Color m_bark;             // the thin, flaking bark higher up
    Color m_needles;
    Color m_needlesTop;       // the upper clump catches more light
};
