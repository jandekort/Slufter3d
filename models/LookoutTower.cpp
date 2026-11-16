// Slufter3D - the wooden lookout tower (Uitkijktoren, Q on the park map)
// Author: Jan de Kort
// Created: 2026-11-15
#include "LookoutTower.h"

#include "Mesh.h"
#include "Ground.h"

namespace
{
    const double pi = 3.14159265358979323846;
}

LookoutTower::LookoutTower()
    :
    m_baseHalf(1.55)
    , m_topHalf(1.15)
    , m_lowerDeck(3.4)
    , m_upperDeck(6.1)
    , m_railing(1.15)
    , m_post(0.11)
    , m_slats(9)
    , m_fenceRadius(5.5)
    , m_fencePosts(12)
    , m_fenceHeight(0.9)
    , m_flagpole(8.5)
    , m_wood(0.62f, 0.60f, 0.56f)
    , m_flag(0.96f, 0.96f, 0.94f)
{
}

// A deck of planks with a railing of upright slats between two rails on all four sides
void LookoutTower::addDeck(Mesh& mesh, const Vec3& foot, double height, double half) const
{
    const Vec3 up(0.0, 1.0, 0.0);
    const Vec3 east(1.0, 0.0, 0.0);
    const Vec3 south(0.0, 0.0, 1.0);
    const double reach = half + 0.25; // the deck juts out beyond the posts
    mesh.addBox(foot + up * height, east, up, south, Vec3(reach, 0.07, reach), m_wood);
    const Vec3 corners[] = { Vec3(-reach, 0.0, -reach), Vec3(reach, 0.0, -reach), Vec3(reach, 0.0, reach),
        Vec3(-reach, 0.0, reach) };
    for (size_t k = 0; k < 4; ++k)
    {
        const Vec3 a = foot + corners[k] + up * height;
        const Vec3 b = foot + corners[(k + 1) % 4] + up * height;
        mesh.addBeam(a + up * 0.12, b + up * 0.12, 0.06, m_wood);
        mesh.addBeam(a + up * m_railing, b + up * m_railing, 0.06, m_wood);
        for (int s = 0; s < m_slats; ++s)
        {
            const double t = ((double) s + 0.5) / (double) m_slats;
            const Vec3 p = a + (b - a) * t;
            mesh.addBeam(p + up * 0.10, p + up * (m_railing - 0.02), 0.065, m_wood.scaled(0.94f));
        }
    }
}

// A low rail fence in a ring around the foot, open towards the path in the east
void LookoutTower::addFence(Mesh& mesh, const Ground& ground, double east, double north) const
{
    auto post = [&](int k)
    {
        const double angle = 2.0 * pi * (double) k / (double) m_fencePosts;
        const double e = east + m_fenceRadius * std::sin(angle);
        const double n = north + m_fenceRadius * std::cos(angle);
        return Vec3(e, ground.height(e, n), -n);
    };
    const int gateFirst = m_fencePosts / 4 - 1; // the gap lies around due east
    const int gateLast = m_fencePosts / 4 + 1;
    for (int k = 0; k < m_fencePosts; ++k)
    {
        const Vec3 a = post(k);
        const Vec3 b = post(k + 1);
        const Vec3 up(0.0, 1.0, 0.0);
        mesh.addBeam(a - up * 0.4, a + up * m_fenceHeight, 0.07, m_wood);
        if (k < gateFirst || k >= gateLast)
        {
            mesh.addBeam(a + up * (m_fenceHeight - 0.08), b + up * (m_fenceHeight - 0.08), 0.06, m_wood);
        }
    }
}

void LookoutTower::build(Mesh& mesh, const Ground& ground, double east, double north) const
{
    const Vec3 foot(east, ground.height(east, north), -north);
    const Vec3 up(0.0, 1.0, 0.0);
    const double top = m_upperDeck + m_railing;
    const double signs[4][2] = { { -1.0, -1.0 }, {  1.0, -1.0 }, {  1.0,  1.0 }, { -1.0,  1.0 } };
    auto postAt = [&](size_t k, double height)
    {
        const double half = mix(m_baseHalf, m_topHalf, height / top);
        return foot + Vec3(signs[k][0] * half, height, signs[k][1] * half);
    };
    for (size_t k = 0; k < 4; ++k)
    {
        // Posts lean inwards and stand a metre into the dune
        mesh.addBeam(postAt(k, -1.0), postAt(k, top), m_post, m_wood);
        // Crossed braces under the lower deck and a single one between the decks
        const size_t next = (k + 1) % 4;
        mesh.addBeam(postAt(k, 0.3), postAt(next, m_lowerDeck - 0.3), 0.07, m_wood);
        mesh.addBeam(postAt(next, 0.3), postAt(k, m_lowerDeck - 0.3), 0.07, m_wood);
        mesh.addBeam(postAt(k, m_lowerDeck - 0.25), postAt(next, m_lowerDeck - 0.25), 0.08, m_wood);
        mesh.addBeam(postAt(k, m_upperDeck - 0.25), postAt(next, m_upperDeck - 0.25), 0.08, m_wood);
    }
    addDeck(mesh, foot, m_lowerDeck, mix(m_baseHalf, m_topHalf, m_lowerDeck / top));
    addDeck(mesh, foot, m_upperDeck, mix(m_baseHalf, m_topHalf, m_upperDeck / top));

    { // Ladders: ground to lower deck, lower deck to upper deck
        const double runs[2][2] = { { 0.0, m_lowerDeck }, { m_lowerDeck, m_upperDeck } };
        for (size_t r = 0; r < 2; ++r)
        {
            const double side = r == 0 ? 0.5 : -0.5;
            const Vec3 low = foot + Vec3(side, runs[r][0], 0.9);
            const Vec3 high = foot + Vec3(side, runs[r][1], 0.2);
            const Vec3 half(0.25, 0.0, 0.0);
            mesh.addBeam(low - half, high - half, 0.035, m_wood);
            mesh.addBeam(low + half, high + half, 0.035, m_wood);
            const int rungs = 8;
            for (int k = 1; k < rungs; ++k)
            {
                const Vec3 p = low + (high - low) * ((double) k / (double) rungs);
                mesh.addBeam(p - half, p + half, 0.025, m_wood);
            }
        }
    }

    addFence(mesh, ground, east, north);

    { // Flagpole with a white flag, north-east of the tower
        const double e = east + 4.2;
        const double n = north + 1.5;
        const Vec3 base(e, ground.height(e, n), -n);
        mesh.addBeam(base - up * 0.5, base + up * m_flagpole, 0.05, Color(0.95f, 0.95f, 0.95f));
        const Vec3 hoist = base + up * (m_flagpole - 0.1);
        const Vec3 fly(-0.9, -0.25, 0.3);
        mesh.addQuad(hoist, hoist + fly, hoist + fly - up * 1.5, hoist - up * 1.5, m_flag, Vec3(0.3, 0.0, 1.0));
    }
}
