// Slufter3D - a Scots pine (Pinus sylvestris), the tree of the woods around the park
// Author: Jan de Kort
// Created: 2026-11-16
#include "ScotsPine.h"

#include "Mesh.h"

#include <algorithm>
#include <cmath>

namespace
{
    const double fullTurn = 6.283185307179586;
}

// Class ScotsPine
ScotsPine::ScotsPine()
    :
    m_trunkSides(4)
    , m_lobeSides(6)
    , m_trunkRadius(0.022)
    , m_trunkRadiusMin(0.06)
    , m_taper(0.55)
    , m_crownFrom(0.55)
    , m_shrubHeight(3.0)
    , m_twoLobesFrom(6.0)
    , m_topLobe(0.68)
    , m_topShift(0.3)
    , m_root(0.5)
    , m_bark(0.56f, 0.36f, 0.24f)
    , m_needles(0.19f, 0.32f, 0.24f)
    , m_needlesTop(0.24f, 0.38f, 0.27f)
{
}

void ScotsPine::build(Mesh& mesh, const Vec3& foot, double height, double crownRadius, double turn) const
{
    const bool shrub = height < m_shrubHeight;
    const double crownBase = shrub ? 0.25 * height : m_crownFrom * height;

    { // Trunk
        const double r0 = std::max(m_trunkRadiusMin, m_trunkRadius * height);
        const double r1 = m_taper * r0;
        const double top = crownBase + 0.3 * (height - crownBase);
        for (int k = 0; k < m_trunkSides; ++k)
        {
            const double a0 = turn + fullTurn * (double) k / (double) m_trunkSides;
            const double a1 = turn + fullTurn * (double) (k + 1) / (double) m_trunkSides;
            const Vec3 d0(std::cos(a0), 0.0, std::sin(a0));
            const Vec3 d1(std::cos(a1), 0.0, std::sin(a1));
            const Vec3 low(0.0, -m_root, 0.0);
            const Vec3 high(0.0, top, 0.0);
            mesh.addQuad(foot + low + d0 * r0, foot + low + d1 * r0, foot + high + d1 * r1, foot + high + d0 * r1,
                m_bark, d0 + d1);
        }
    }

    const double crownHeight = height - crownBase;
    if (shrub || height < m_twoLobesFrom)
    {
        const Vec3 center = foot + Vec3(0.0, crownBase + 0.4 * crownHeight, 0.0);
        addLobe(mesh, center, crownRadius, 0.6 * crownHeight, 0.4 * crownHeight, turn, m_needles);
        return;
    }
    { // Lower lobe: wide and flat
        const Vec3 center = foot + Vec3(0.0, crownBase + 0.3 * crownHeight, 0.0);
        addLobe(mesh, center, crownRadius, 0.35 * crownHeight, 0.3 * crownHeight, turn, m_needles);
    }
    { // Upper lobe: smaller, to one side, up to the top of the tree
        const double shift = m_topShift * crownRadius;
        const Vec3 center = foot + Vec3(shift * std::cos(turn), crownBase + 0.62 * crownHeight, shift * std::sin(turn));
        addLobe(mesh, center, m_topLobe * crownRadius, 0.38 * crownHeight, 0.25 * crownHeight, turn + 0.5,
            m_needlesTop);
    }
}

void ScotsPine::addLobe(Mesh& mesh, const Vec3& center, double radius, double up, double down, double turn,
    const Color& color) const
{
    const Vec3 top = center + Vec3(0.0, up, 0.0);
    const Vec3 bottom = center - Vec3(0.0, down, 0.0);
    for (int k = 0; k < m_lobeSides; ++k)
    {
        const double a0 = turn + fullTurn * (double) k / (double) m_lobeSides;
        const double a1 = turn + fullTurn * (double) (k + 1) / (double) m_lobeSides;
        const Vec3 d0(std::cos(a0), 0.0, std::sin(a0));
        const Vec3 d1(std::cos(a1), 0.0, std::sin(a1));
        const Vec3 outward = d0 + d1;
        mesh.addTriangle(center + d0 * radius, center + d1 * radius, top, color, outward * up + Vec3(0.0, radius, 0.0));
        mesh.addTriangle(center + d1 * radius, center + d0 * radius, bottom, color,
            outward * down - Vec3(0.0, radius, 0.0));
    }
}
