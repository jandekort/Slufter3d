// Slufter3D - a Scots pine (Pinus sylvestris), the tree of the woods around the park
// Author: Jan de Kort
// Created: 2026-11-16
#include "ScotsPine.h"

#include "Mesh.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
    const double fullTurn = 6.283185307179586;

    Vec3 around(double angle)
    {
        return Vec3(std::cos(angle), 0.0, std::sin(angle));
    }
}

// Class ScotsPine
ScotsPine::ScotsPine()
    :
    m_trunkSides(5)
    , m_shrubTrunkSides(4)
    , m_limbSides(3)
    , m_clumpSides(6)
    , m_trunkRadius(0.022)
    , m_trunkRadiusMin(0.06)
    , m_taper(0.55)
    , m_lean(0.035)
    , m_barkChange(0.45)
    , m_crownFrom(0.55)
    , m_shrubHeight(3.0)
    , m_tallFrom(6.0)
    , m_threeFrom(2.5)
    , m_fourFrom(4.0)
    , m_clumpRadius(0.56)
    , m_topRadius(0.62)
    , m_topShift(0.18)
    , m_upperRing(0.62)
    , m_rugged(0.16)
    , m_root(0.5)
    , m_barkLow(0.40f, 0.31f, 0.26f)
    , m_bark(0.60f, 0.37f, 0.23f)
    , m_needles(0.19f, 0.32f, 0.24f)
    , m_needlesTop(0.24f, 0.38f, 0.27f)
{
}

void ScotsPine::build(Mesh& mesh, const Vec3& foot, double height, double crownRadius, double turn) const
{
    const double trunkRadius = std::max(m_trunkRadiusMin, m_trunkRadius * height);

    if (height < m_shrubHeight)
    { // Shrub: one clump near the ground
        const double crownBase = 0.25 * height;
        const double crownHeight = height - crownBase;
        addTrunk(mesh, foot, trunkRadius, crownBase + 0.3 * crownHeight, Vec3(0.0, 0.0, 0.0), turn,
            m_shrubTrunkSides);
        addClump(mesh, foot + Vec3(0.0, crownBase + 0.4 * crownHeight, 0.0), crownRadius, 0.6 * crownHeight,
            0.4 * crownHeight, turn, turn, m_needles);
        return;
    }

    const Vec3 lean = around(turn + 2.0) * (m_lean * height * (0.4 + scatter(turn)));

    if (height < m_tallFrom)
    { // Young tree: a bushy cone, wide below and a smaller clump on top
        const double crownBase = 0.3 * height;
        const double crownHeight = height - crownBase;
        addTrunk(mesh, foot, trunkRadius, crownBase + 0.5 * crownHeight, lean, turn, m_trunkSides);
        const Vec3 axis = foot + lean;
        addClump(mesh, axis + Vec3(0.0, crownBase + 0.3 * crownHeight, 0.0), crownRadius, 0.4 * crownHeight,
            0.3 * crownHeight, turn, turn + 1.0, m_needles);
        addClump(mesh, axis + Vec3(0.0, crownBase + 0.66 * crownHeight, 0.0), 0.6 * crownRadius, 0.34 * crownHeight,
            0.2 * crownHeight, turn + 0.5, turn + 2.0, m_needlesTop);
        return;
    }

    // Tall tree: clumps around the trunk, each on a limb, and one on top
    const double crownBase = m_crownFrom * height;
    const double crownHeight = height - crownBase;
    const double trunkTop = crownBase + 0.5 * crownHeight;
    addTrunk(mesh, foot, trunkRadius, trunkTop, lean, turn, m_trunkSides);
    const Vec3 axis = foot + lean;

    const int clumps = crownRadius < m_threeFrom ? 2 : (crownRadius < m_fourFrom ? 3 : 4);
    const double radius = m_clumpRadius * crownRadius;
    for (int k = 0; k < clumps; ++k)
    {
        const double seed = turn + 7.0 * (double) (k + 1);
        const double angle = turn + fullTurn * ((double) k + 0.5 * (scatter(seed) - 0.5)) / (double) clumps;
        const double out = crownRadius - radius * (1.0 + 0.2 * scatter(seed + 1.0));
        const double level = crownBase + (0.22 + 0.2 * scatter(seed + 2.0)) * crownHeight;
        const Vec3 center = axis + around(angle) * out + Vec3(0.0, level, 0.0);
        const double size = radius * (0.9 + 0.2 * scatter(seed + 3.0));
        // The limb leaves the trunk under the crown and ends inside its clump
        const double fork = crownBase - (0.05 + 0.2 * scatter(seed + 4.0)) * crownHeight;
        addLimb(mesh, foot + lean * (fork / trunkTop) + Vec3(0.0, fork, 0.0), center, m_taper * trunkRadius);
        addClump(mesh, center, size, 0.3 * crownHeight, 0.2 * crownHeight, angle, seed,
            m_needles.mixed(m_needlesTop, (float) (0.5 * scatter(seed + 5.0))));
    }
    { // The clump on top: a little to one side, up to the top of the tree
        const double shift = m_topShift * crownRadius;
        const Vec3 center = axis + around(turn) * shift + Vec3(0.0, crownBase + 0.62 * crownHeight, 0.0);
        addClump(mesh, center, m_topRadius * crownRadius, 0.38 * crownHeight, 0.24 * crownHeight, turn + 0.5,
            turn + 3.0, m_needlesTop);
    }
}

void ScotsPine::addTrunk(Mesh& mesh, const Vec3& foot, double radius, double top, const Vec3& lean, double turn,
    int sides) const
{
    // A tree does not lean in a straight line: most of the bend is in the upper part
    const double levels[3] = { -m_root, m_barkChange * top, top };
    const double shifts[3] = { 0.0, 0.3, 1.0 };
    const double radii[3] = { radius, mix(radius, m_taper * radius, 0.6), m_taper * radius };
    for (int part = 0; part < 2; ++part)
    {
        const Vec3 low = foot + lean * shifts[part] + Vec3(0.0, levels[part], 0.0);
        const Vec3 high = foot + lean * shifts[part + 1] + Vec3(0.0, levels[part + 1], 0.0);
        for (int k = 0; k < sides; ++k)
        {
            const Vec3 d0 = around(turn + fullTurn * (double) k / (double) sides);
            const Vec3 d1 = around(turn + fullTurn * (double) (k + 1) / (double) sides);
            mesh.addQuad(low + d0 * radii[part], low + d1 * radii[part], high + d1 * radii[part + 1],
                high + d0 * radii[part + 1], part == 0 ? m_barkLow : m_bark, d0 + d1);
        }
    }
}

void ScotsPine::addLimb(Mesh& mesh, const Vec3& from, const Vec3& to, double radius) const
{
    const Vec3 along = normalize(to - from);
    const Vec3 reference = std::fabs(along.y) < 0.9 ? Vec3(0.0, 1.0, 0.0) : Vec3(1.0, 0.0, 0.0);
    const Vec3 side = normalize(cross(along, reference));
    const Vec3 other = cross(side, along);
    for (int k = 0; k < m_limbSides; ++k)
    {
        const double a0 = fullTurn * (double) k / (double) m_limbSides;
        const double a1 = fullTurn * (double) (k + 1) / (double) m_limbSides;
        const Vec3 d0 = side * std::cos(a0) + other * std::sin(a0);
        const Vec3 d1 = side * std::cos(a1) + other * std::sin(a1);
        mesh.addQuad(from + d0 * (0.6 * radius), from + d1 * (0.6 * radius), to + d1 * (0.3 * radius),
            to + d0 * (0.3 * radius), m_bark, d0 + d1);
    }
}

void ScotsPine::addClump(Mesh& mesh, const Vec3& center, double radius, double up, double down, double turn,
    double seed, const Color& color) const
{
    // The lower ring is the width of the clump; the upper ring stands half a step further round
    std::vector<Vec3> lower;
    std::vector<Vec3> upper;
    for (int k = 0; k < m_clumpSides; ++k)
    {
        const double a0 = turn + fullTurn * (double) k / (double) m_clumpSides;
        const double a1 = turn + fullTurn * ((double) k + 0.5) / (double) m_clumpSides;
        const double r0 = radius * (1.0 + m_rugged * (2.0 * scatter(seed + 11.0 * (double) k) - 1.0));
        const double r1 = radius * m_upperRing * (1.0 + m_rugged * (2.0 * scatter(seed + 13.0 * (double) k + 5.0) - 1.0));
        const double y0 = -down * (0.15 + 0.3 * scatter(seed + 17.0 * (double) k + 1.0));
        const double y1 = up * (0.45 + 0.25 * scatter(seed + 19.0 * (double) k + 2.0));
        lower.push_back(center + around(a0) * r0 + Vec3(0.0, y0, 0.0));
        upper.push_back(center + around(a1) * r1 + Vec3(0.0, y1, 0.0));
    }
    const Vec3 top = center + Vec3(0.0, up, 0.0) + around(seed) * (0.15 * radius);
    const Vec3 bottom = center - Vec3(0.0, down, 0.0);
    for (int k = 0; k < m_clumpSides; ++k)
    {
        const int next = (k + 1) % m_clumpSides;
        const Vec3& a = lower[k];
        const Vec3& b = lower[next];
        const Vec3& c = upper[k];
        const Vec3& d = upper[next];
        mesh.addTriangle(b, a, bottom, color, (a + b) * 0.5 - center - Vec3(0.0, radius, 0.0));
        mesh.addTriangle(a, b, c, color, (a + b + c) * (1.0 / 3.0) - center);
        mesh.addTriangle(b, d, c, color, (b + c + d) * (1.0 / 3.0) - center);
        mesh.addTriangle(c, d, top, color, (c + d) * 0.5 - center + Vec3(0.0, radius, 0.0));
    }
}

double ScotsPine::scatter(double seed) const
{
    const double value = std::sin(seed * 12.9898) * 43758.5453;
    return value - std::floor(value);
}
