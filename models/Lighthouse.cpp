// Slufter3D - the Eierland lighthouse (Vuurtoren Texel), background scenery north of the park
// Author: Jan de Kort
// Created: 2026-11-15
#include "Lighthouse.h"

#include "Mesh.h"

#include <algorithm>

namespace
{
    const double pi = 3.14159265358979323846;
}

Lighthouse::Lighthouse()
    :
    m_height(34.7)
    , m_shaftTop(25.0)
    , m_bandTop(26.6)
    , m_footRadius(4.25)
    , m_topRadius(3.70)
    , m_galleryReach(0.55)
    , m_watchTop(30.4)
    , m_watchRadius(2.65)
    , m_upperGallery(2.95)
    , m_lanternBase(31.0)
    , m_lanternTop(32.8)
    , m_lanternRadius(1.65)
    , m_domeTop(33.9)
    , m_antennaSpan(4.4)
    , m_railing(1.1)
    , m_embed(3.0)
    , m_segments(40)
    , m_red(0.78f, 0.15f, 0.12f)
    , m_white(0.95f, 0.95f, 0.93f)
    , m_glass(0.55f, 0.65f, 0.70f)
    , m_iron(0.20f, 0.21f, 0.23f)
    , m_dome(0.88f, 0.89f, 0.88f)
{
}

Vec3 Lighthouse::around(const Vec3& foot, double angle, double radius, double y) const
{
    return foot + Vec3(radius * std::cos(angle), y, -radius * std::sin(angle));
}

double Lighthouse::radiusAt(double y) const
{
    return mix(m_footRadius, m_topRadius, y / m_bandTop);
}

void Lighthouse::addTube(Mesh& mesh, const Vec3& foot, double y0, double y1, double r0, double r1, const Color& color) const
{
    for (int k = 0; k < m_segments; ++k)
    {
        const double a0 = 2.0 * pi * (double) k / (double) m_segments;
        const double a1 = 2.0 * pi * (double) (k + 1) / (double) m_segments;
        const double mid = 0.5 * (a0 + a1);
        mesh.addQuad(around(foot, a0, r0, y0), around(foot, a1, r0, y0), around(foot, a1, r1, y1), around(foot, a0, r1, y1),
            color, Vec3(std::cos(mid), 0.0, -std::sin(mid)));
    }
}

void Lighthouse::addRing(Mesh& mesh, const Vec3& foot, double y, double inner, double outer, const Color& color, bool up) const
{
    const Vec3 facing(0.0, up ? 1.0 : -1.0, 0.0);
    for (int k = 0; k < m_segments; ++k)
    {
        const double a0 = 2.0 * pi * (double) k / (double) m_segments;
        const double a1 = 2.0 * pi * (double) (k + 1) / (double) m_segments;
        mesh.addQuad(around(foot, a0, inner, y), around(foot, a1, inner, y), around(foot, a1, outer, y),
            around(foot, a0, outer, y), color, facing);
    }
}

void Lighthouse::addGallery(Mesh& mesh, const Vec3& foot, double y, double inner, double outer) const
{
    const double deck = 0.25;
    addRing(mesh, foot, y, inner, outer, m_white, true);
    addRing(mesh, foot, y - deck, inner, outer, m_white.scaled(0.85f), false);
    addTube(mesh, foot, y - deck, y, outer, outer, m_white);
    const double rail = outer - 0.06;
    const int posts = std::max(8, (int) (2.0 * pi * rail / 0.9));
    for (int k = 0; k < posts; ++k)
    {
        const double a0 = 2.0 * pi * (double) k / (double) posts;
        const double a1 = 2.0 * pi * (double) (k + 1) / (double) posts;
        mesh.addBeam(around(foot, a0, rail, y), around(foot, a0, rail, y + m_railing), 0.03, m_iron);
        mesh.addBeam(around(foot, a0, rail, y + m_railing), around(foot, a1, rail, y + m_railing), 0.03, m_iron);
        mesh.addBeam(around(foot, a0, rail, y + 0.5 * m_railing), around(foot, a1, rail, y + 0.5 * m_railing), 0.02, m_iron);
    }
}

void Lighthouse::addWindow(Mesh& mesh, const Vec3& foot, double angle, double y, double radius, double width,
    double height) const
{
    const Vec3 out(std::cos(angle), 0.0, -std::sin(angle));
    const Vec3 along(-std::sin(angle), 0.0, -std::cos(angle));
    const Vec3 up(0.0, 1.0, 0.0);
    const double frame = 0.10;
    const Vec3 centre = around(foot, angle, radius, y + 0.5 * height);
    auto pane = [&](double halfWidth, double halfHeight, double lift, const Color& color)
    {
        const Vec3 c = centre + out * lift;
        mesh.addQuad(c - along * halfWidth - up * halfHeight, c + along * halfWidth - up * halfHeight,
            c + along * halfWidth + up * halfHeight, c - along * halfWidth + up * halfHeight, color, out);
    };
    pane(0.5 * width + frame, 0.5 * height + frame, 0.04, m_white);
    pane(0.5 * width, 0.5 * height, 0.08, Color(0.16f, 0.19f, 0.23f));
}

void Lighthouse::build(Mesh& mesh, const Vec3& foot) const
{
    const Vec3 up(0.0, 1.0, 0.0);

    // Shaft: red up to the white band, which carries the first gallery
    addTube(mesh, foot, -m_embed, m_shaftTop, radiusAt(-m_embed), radiusAt(m_shaftTop), m_red);
    addTube(mesh, foot, m_shaftTop, m_bandTop, radiusAt(m_shaftTop), m_topRadius, m_white);
    { // The windows of the stair wind up the tower
        const int windows = 11;
        for (int k = 0; k < windows; ++k)
        {
            const double y = 2.8 + 2.05 * (double) k;
            const double angle = (250.0 + 47.0 * (double) k) * pi / 180.0;
            addWindow(mesh, foot, angle, y, radiusAt(y + 0.45), 0.55, 0.85);
        }
    }

    // First gallery, round the watch room
    addGallery(mesh, foot, m_bandTop, m_watchRadius, m_topRadius + m_galleryReach);
    addTube(mesh, foot, m_bandTop, m_watchTop, m_watchRadius, m_watchRadius, m_white);
    for (int k = 0; k < 4; ++k)
    {
        addWindow(mesh, foot, (45.0 + 90.0 * (double) k) * pi / 180.0, m_bandTop + 1.4, m_watchRadius, 0.6, 0.9);
    }
    addWindow(mesh, foot, 1.5 * pi, m_bandTop + 0.05, m_watchRadius, 0.8, 2.0); // door to the gallery, south

    // Second gallery, round the lantern: a white foot, the glazing between white bars, a white rim
    addGallery(mesh, foot, m_watchTop, m_lanternRadius, m_upperGallery);
    addRing(mesh, foot, m_watchTop, 0.0, m_lanternRadius, m_white, true);
    addTube(mesh, foot, m_watchTop, m_lanternBase, m_lanternRadius, m_lanternRadius, m_white);
    addTube(mesh, foot, m_lanternBase, m_lanternTop, m_lanternRadius, m_lanternRadius, m_glass);
    const int bars = 8;
    for (int k = 0; k < bars; ++k)
    {
        const double angle = 2.0 * pi * (double) k / (double) bars;
        mesh.addBeam(around(foot, angle, m_lanternRadius + 0.03, m_lanternBase),
            around(foot, angle, m_lanternRadius + 0.03, m_lanternTop), 0.05, m_white);
    }
    addTube(mesh, foot, m_lanternTop, m_lanternTop + 0.15, m_lanternRadius + 0.08, m_lanternRadius + 0.08, m_white);
    addRing(mesh, foot, m_lanternTop, 0.0, m_lanternRadius + 0.08, m_white.scaled(0.85f), false);

    { // Dome: rings closing in towards the top
        const int rings = 5;
        const double base = m_lanternTop + 0.15;
        auto radius = [&](int r) { return (m_lanternRadius + 0.08) * std::cos(0.5 * pi * 0.88 * (double) r / (double) rings); };
        auto height = [&](int r) { return base + (m_domeTop - base) * std::sin(0.5 * pi * (double) r / (double) rings); };
        for (int r = 0; r < rings; ++r)
        {
            addTube(mesh, foot, height(r), height(r + 1), radius(r), radius(r + 1), m_dome);
        }
        addRing(mesh, foot, m_domeTop, 0.0, radius(rings), m_dome, true);
    }

    // Mast with the radar bar on top
    mesh.addBeam(foot + up * (m_domeTop - 0.05), foot + up * m_height, 0.05, m_iron);
    const double barHeight = m_height - 0.12;
    const Vec3 half = Vec3(std::cos(0.35), 0.0, -std::sin(0.35)) * (0.5 * m_antennaSpan);
    mesh.addBeam(foot + up * barHeight - half, foot + up * barHeight + half, 0.08, m_iron);
}
