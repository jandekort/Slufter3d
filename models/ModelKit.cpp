// Slufter3D - what the models are drawn with
// Author: Jan de Kort
// Created: 2026-11-16
#include "ModelKit.h"

#include "Check.h"
#include "Ground.h"
#include "Mesh.h"

#include <algorithm>
#include <cmath>

namespace ModelKit
{
    double degrees(double angle)
    {
        return angle * pi / 180.0;
    }

    double groundLevel(const Site& site, const Vec3& p)
    {
        return site.m_ground != nullptr ? site.m_ground->height(p.x, -p.z) : site.m_floor.y;
    }

    void addDrum(Mesh& mesh, const Vec3& foot, double radius, double height, const Color& color)
    {
        const int sides = 10;
        const Vec3 up(0.0, height, 0.0);
        const auto rim = [&](int k)
            {
                const double angle = 2.0 * pi * (double) k / (double) sides;
                return Vec3(radius * std::cos(angle), 0.0, radius * std::sin(angle));
            };
        for (int k = 0; k < sides; ++k)
        {
            const Vec3 a = rim(k);
            const Vec3 b = rim(k + 1);
            mesh.addQuad(foot + a, foot + b, foot + b + up, foot + a + up, color, a + b);
            mesh.addTriangle(foot + up, foot + a + up, foot + b + up, color, Vec3(0.0, 1.0, 0.0));
        }
    }

    std::pair<double, double> comfortFootprint()
    {
        const double comfortLength = 9.0;
        const double comfortWidth = 6.0;
        return { comfortLength, comfortWidth };
    }

    std::pair<double, double> wallsUnderRoof(double outlineLength, double outlineWidth)
    {
        const double roofOverhang = 0.5;   // of the outline beyond the wall, on every side [m]
        const double smallest = 3.0;
        return { std::max(smallest, outlineLength - 2.0 * roofOverhang),
            std::max(smallest, outlineWidth - 2.0 * roofOverhang) };
    }

    // Class Builder
    Builder::Builder(Mesh& mesh, const Site& site, const Shape& shape, const Paint& paint)
        :
        m_mesh(mesh)
        , m_site(site)
        , m_shape(shape)
        , m_paint(paint)
        , m_halfLength(0.5 * site.m_length)
        , m_halfWidth(0.5 * site.m_width)
        , m_rise(m_halfWidth * std::tan(shape.m_pitch))
        , m_skin(0.03)
        , m_foundation(2.5)
    {
    }

    Vec3 Builder::point(double u, double v, double y) const
    {
        return m_site.m_floor + m_site.m_axisLength * u + m_site.m_axisWidth * v + Vec3(0.0, y, 0.0);
    }

    Vec3 Builder::outward(Side side) const
    {
        switch (side)
        {
        case Side::Front: return m_site.m_axisWidth;
        case Side::Back:  return -m_site.m_axisWidth;
        case Side::Left:  return -m_site.m_axisLength;
        case Side::Right: return m_site.m_axisLength;
        default: require(false, "Unknown side"); return Vec3();
        }
    }

    Vec3 Builder::onWall(Side side, double a, double y, double out) const
    {
        switch (side)
        {
        case Side::Front: return point(a, m_halfWidth + out, y);
        case Side::Back:  return point(a, -m_halfWidth - out, y);
        case Side::Left:  return point(-m_halfLength - out, a, y);
        case Side::Right: return point(m_halfLength + out, a, y);
        default: require(false, "Unknown side"); return Vec3();
        }
    }

    void Builder::panel(Side side, double a0, double a1, double y0, double y1, const Color& color, double out)
    {
        m_mesh.addQuad(onWall(side, a0, y0, out), onWall(side, a1, y0, out), onWall(side, a1, y1, out),
            onWall(side, a0, y1, out), color, outward(side));
    }

    void Builder::window(Side side, double a0, double a1, double y0, double y1, int columns, int rows,
        const Color& frame, const Color& glass)
    {
        const double bar = 0.09;
        panel(side, a0, a1, y0, y1, frame, m_skin);
        const double paneWidth = (a1 - a0 - bar) / (double) columns;
        const double paneHeight = (y1 - y0 - bar) / (double) rows;
        for (int c = 0; c < columns; ++c)
        {
            for (int r = 0; r < rows; ++r)
            {
                const double pa = a0 + bar + (double) c * paneWidth;
                const double py = y0 + bar + (double) r * paneHeight;
                panel(side, pa, pa + paneWidth - bar, py, py + paneHeight - bar, glass, 2.0 * m_skin);
            }
        }
    }

    void Builder::door(Side side, double a0, double a1, double y0, double y1, const Color& frame, const Color& leaf,
        bool glazed, const Color& pane)
    {
        const double bar = 0.08;
        panel(side, a0, a1, y0, y1, frame, m_skin);
        panel(side, a0 + bar, a1 - bar, y0, y1 - bar, leaf, 2.0 * m_skin);
        if (glazed)
        {
            const double mid = 0.5 * (y0 + y1);
            panel(side, a0 + 2.5*bar, a1 - 2.5*bar, mid, y1 - 3.0*bar, pane, 3.0 * m_skin);
        }
    }

    void Builder::skylight(double u0, double u1, double v0, double v1, double sign, const Color& frame, const Color& glass)
    {
        const double lift = 0.06;
        const double bar = 0.08;
        m_mesh.addQuad(point(u0, sign*v0, roofHeight(v0) + lift), point(u1, sign*v0, roofHeight(v0) + lift),
            point(u1, sign*v1, roofHeight(v1) + lift), point(u0, sign*v1, roofHeight(v1) + lift), frame, Vec3(0.0, 1.0, 0.0));
        m_mesh.addQuad(point(u0 + bar, sign*(v0 - bar), roofHeight(v0 - bar) + 2.0*lift),
            point(u1 - bar, sign*(v0 - bar), roofHeight(v0 - bar) + 2.0*lift),
            point(u1 - bar, sign*(v1 + bar), roofHeight(v1 + bar) + 2.0*lift),
            point(u0 + bar, sign*(v1 + bar), roofHeight(v1 + bar) + 2.0*lift), glass, Vec3(0.0, 1.0, 0.0));
    }

    void Builder::shutters(Side side, double a0, double a1, double y0, double y1, double leaf)
    {
        panel(side, a0 - leaf - 0.05, a0 - 0.05, y0, y1, m_paint.m_shutter, 2.0 * m_skin);
        panel(side, a1 + 0.05, a1 + leaf + 0.05, y0, y1, m_paint.m_shutter, 2.0 * m_skin);
    }

    void Builder::walls()
    {
        const Side sides[] = { Side::Front, Side::Back, Side::Left, Side::Right };
        for (const Side side : sides)
        {
            const double half = (side == Side::Front || side == Side::Back) ? m_halfLength : m_halfWidth;
            panel(side, -half, half, -m_foundation, m_shape.m_plinthHeight, m_paint.m_plinth, 0.0);
            panel(side, -half, half, m_shape.m_plinthHeight, m_shape.m_wallHeight, m_paint.m_wall, 0.0);
        }
    }

    void Builder::gable(Side side, const Color& color, double out, double inset)
    {
        const double top = m_shape.m_wallHeight + (1.0 - m_shape.m_hip) * m_rise;
        const double halfTop = m_halfWidth * m_shape.m_hip;
        const double base = m_shape.m_wallHeight + inset * std::tan(m_shape.m_pitch);
        const double shrink = inset / std::cos(m_shape.m_pitch);
        m_mesh.addQuad(onWall(side, -m_halfWidth + inset + shrink, base, out),
            onWall(side, m_halfWidth - inset - shrink, base, out),
            onWall(side, std::max(0.0, halfTop - shrink), top - inset, out),
            onWall(side, -std::max(0.0, halfTop - shrink), top - inset, out), color, outward(side));
    }

    void Builder::roof()
    {
        roof(m_paint.m_trim.scaled(0.8f));
    }

    void Builder::roof(const Color& soffit)
    {
        const double tangent = std::tan(m_shape.m_pitch);
        const double eaveV = m_halfWidth + m_shape.m_eave;
        const double eaveY = m_shape.m_wallHeight - m_shape.m_eave * tangent;
        const double endU = m_halfLength + m_shape.m_verge;
        const double hipY = m_shape.m_wallHeight + (1.0 - m_shape.m_hip) * m_rise;
        const double hipV = m_halfWidth * m_shape.m_hip;
        const double hipBack = hipBackOf();                           // the ridge ends this far in
        const double bandY = m_shape.m_wallHeight + m_shape.m_band * (1.0 - m_shape.m_hip) * m_rise;
        const double bandV = (ridge() - bandY) / tangent;
        const double step = m_shape.m_band < 1.0 ? 0.10 : 0.0;       // thatch stands proud of the tiles
        const Vec3 up(0.0, 1.0, 0.0);
        const Color& top = m_shape.m_band < 1.0 ? m_paint.m_roofTop : m_paint.m_roof;
        for (const double s : { 1.0, -1.0 })
        {
            const Vec3 lift = (up + m_site.m_axisWidth * (s * tangent)) * (step * std::cos(m_shape.m_pitch));
            // Lower band from the eaves
            m_mesh.addQuad(point(-endU, s*eaveV, eaveY), point(endU, s*eaveV, eaveY),
                point(endU, s*bandV, bandY), point(-endU, s*bandV, bandY), m_paint.m_roof, up);
            if (m_shape.m_band < 1.0)
            {
                // Upper part up to the level of the hips
                m_mesh.addQuad(point(-endU, s*bandV, bandY) + lift, point(endU, s*bandV, bandY) + lift,
                    point(endU, s*hipV, hipY) + lift, point(-endU, s*hipV, hipY) + lift, top, up);
                // Edge of the thatch above the tiles
                m_mesh.addQuad(point(-endU, s*bandV, bandY), point(endU, s*bandV, bandY),
                    point(endU, s*bandV, bandY) + lift, point(-endU, s*bandV, bandY) + lift,
                    top.scaled(0.8f), m_site.m_axisWidth * s);
            }
            if (m_shape.m_hip > 0.0)
            {
                // Above the hips the slope narrows to the ridge
                m_mesh.addQuad(point(-endU, s*hipV, hipY) + lift, point(endU, s*hipV, hipY) + lift,
                    point(endU - hipBack, 0.0, ridge()) + lift, point(-endU + hipBack, 0.0, ridge()) + lift, top, up);
            }
            // Board along the eaves
            m_mesh.addQuad(point(-endU, s*eaveV, eaveY - m_shape.m_fascia), point(endU, s*eaveV, eaveY - m_shape.m_fascia),
                point(endU, s*eaveV, eaveY + 0.02), point(-endU, s*eaveV, eaveY + 0.02), m_paint.m_trim,
                m_site.m_axisWidth * s);
            // Underside of the eaves
            m_mesh.addQuad(point(-endU, s*m_halfWidth, eaveY - m_shape.m_fascia), point(endU, s*m_halfWidth, eaveY - m_shape.m_fascia),
                point(endU, s*eaveV, eaveY - m_shape.m_fascia), point(-endU, s*eaveV, eaveY - m_shape.m_fascia),
                soffit, -up);
            // Boards along the verges
            for (const double e : { 1.0, -1.0 })
            {
                const double board = 0.22;
                const Vec3 down(0.0, -board, 0.0);
                const Vec3 low = point(e*endU, s*eaveV, eaveY);
                const Vec3 high = point(e*endU, s*hipV, hipY);
                m_mesh.addQuad(low + down, high + down, high, low, m_paint.m_trim, m_site.m_axisLength * e);
            }
        }
        if (m_shape.m_hip > 0.0)
        {
            for (const double e : { 1.0, -1.0 })
            {
                m_mesh.addTriangle(point(e*endU, -hipV, hipY), point(e*endU, hipV, hipY),
                    point(e*(endU - hipBack), 0.0, ridge() + step), top, m_site.m_axisLength * e + up);
            }
        }
    }

    void Builder::ridgeCap(const Color& color)
    {
        const double hipBack = hipBackOf();
        const double half = m_halfLength + m_shape.m_verge - hipBack;
        m_mesh.addBox(point(0.0, 0.0, ridge() + 0.12), m_site.m_axisLength, Vec3(0.0, 1.0, 0.0), m_site.m_axisWidth,
            Vec3(half, 0.10, 0.16), color);
    }

    void Builder::pipe(double u, double v, double above, double radius)
    {
        const double foot = roofHeight(v) - 0.2;
        const double halfHeight = 0.5 * (above + 0.2);
        m_mesh.addBox(point(u, v, foot + halfHeight), m_site.m_axisLength, Vec3(0.0, 1.0, 0.0), m_site.m_axisWidth,
            Vec3(radius, halfHeight, radius), m_paint.m_pipe);
    }

    void Builder::dormer(double u0, double u1, double sill, double head, int panes, const Color& cheek, const Color& top)
    {
        const double tangent = std::tan(m_shape.m_pitch);
        const double faceV = (ridge() - sill) / tangent;           // where the sill meets the roof
        const double backV = std::max(0.0, (ridge() - head - 0.25) / tangent - 0.6);
        const double mid = 0.5 * (u0 + u1);
        const double half = 0.5 * (u1 - u0);
        m_mesh.addBox(point(mid, 0.5 * (faceV + backV), 0.5 * (sill + head)), m_site.m_axisLength, Vec3(0.0, 1.0, 0.0),
            m_site.m_axisWidth, Vec3(half, 0.5 * (head - sill), 0.5 * (faceV - backV)), cheek);
        // Roof of the dormer, falling to the front
        m_mesh.addQuad(point(u0 - 0.15, faceV + 0.25, head + 0.02), point(u1 + 0.15, faceV + 0.25, head + 0.02),
            point(u1 + 0.15, backV, head + 0.30), point(u0 - 0.15, backV, head + 0.30), top, Vec3(0.0, 1.0, 0.0));
        const double pane = (u1 - u0 - 0.2) / (double) panes;
        for (int k = 0; k < panes; ++k)
        {
            const double a = u0 + 0.1 + (double) k * pane;
            m_mesh.addQuad(point(a, faceV + m_skin, sill + 0.12), point(a + pane, faceV + m_skin, sill + 0.12),
                point(a + pane, faceV + m_skin, head - 0.08), point(a, faceV + m_skin, head - 0.08), m_paint.m_trim,
                m_site.m_axisWidth);
            m_mesh.addQuad(point(a + 0.07, faceV + 2.0*m_skin, sill + 0.19), point(a + pane - 0.07, faceV + 2.0*m_skin, sill + 0.19),
                point(a + pane - 0.07, faceV + 2.0*m_skin, head - 0.15), point(a + 0.07, faceV + 2.0*m_skin, head - 0.15),
                m_paint.m_glass, m_site.m_axisWidth);
        }
    }

    void Builder::pergola(Side side, double a0, double a1, double depth, double height, const Color& wood)
    {
        const double post = 0.07;
        for (const double a : { a0, 0.5 * (a0 + a1), a1 })
        {
            m_mesh.addBeam(onWall(side, a, 0.0, depth), onWall(side, a, height, depth), post, wood);
            m_mesh.addBeam(onWall(side, a, height, 0.0), onWall(side, a, height, depth + 0.3), post, wood);
        }
        m_mesh.addBeam(onWall(side, a0 - 0.3, height, depth), onWall(side, a1 + 0.3, height, depth), post, wood);
        const int laths = 14;
        for (int k = 0; k <= laths; ++k)
        {
            const double a = mix(a0, a1, (double) k / (double) laths);
            m_mesh.addBeam(onWall(side, a, height + 0.10, 0.3), onWall(side, a, height + 0.10, depth + 0.3), 0.03, wood);
        }
    }

    void Builder::annex(Side side, double a0, double a1, double depth, double height, const Color& color)
    {
        const Vec3 centre = onWall(side, 0.5 * (a0 + a1), 0.5 * (height - m_foundation), 0.5 * depth);
        const Vec3 along = (side == Side::Front || side == Side::Back) ? m_site.m_axisLength : m_site.m_axisWidth;
        m_mesh.addBox(centre, along, Vec3(0.0, 1.0, 0.0), outward(side),
            Vec3(0.5 * (a1 - a0), 0.5 * (height + m_foundation), 0.5 * depth), color);
    }

    double Builder::groundUnder(const Vec3& p) const
    {
        return groundLevel(m_site, p) - m_site.m_floor.y;
    }

    void Builder::terrace(Side side, double a0, double a1, double depth, const Color& color)
    {
        const double tile = 0.6;     // the paving follows the ground in steps of this size [m]
        const double lift = 0.10;    // above the ground, below the roads [m]
        const int na = std::max(1, (int) std::ceil((a1 - a0) / tile));
        const int nd = std::max(1, (int) std::ceil(depth / tile));
        const auto place = [&](int i, int j)
            {
                const Vec3 p = onWall(side, mix(a0, a1, (double) i / (double) na), 0.0, depth * (double) j / (double) nd);
                return Vec3(p.x, groundLevel(m_site, p) + lift, p.z);
            };
        for (int i = 0; i < na; ++i)
        {
            for (int j = 0; j < nd; ++j)
            {
                m_mesh.addQuad(place(i, j), place(i + 1, j), place(i + 1, j + 1), place(i, j + 1), color,
                    Vec3(0.0, 1.0, 0.0));
            }
        }
    }

    void Builder::downpipe(Side side, double a, const Color& color, bool drum /* = true */)
    {
        const double out = 0.12;
        const double drumRadius = 0.21;
        const double drumHeight = 0.50;
        const double eaveY = m_shape.m_wallHeight - m_shape.m_eave * std::tan(m_shape.m_pitch) - m_shape.m_fascia;
        const Vec3 foot = onWall(side, a, 0.0, out);
        const double ground = groundUnder(foot);
        const double drumTop = ground + drumHeight;
        // From the gutter back to the wall, then down
        m_mesh.addBeam(onWall(side, a, eaveY, m_shape.m_eave - 0.05), onWall(side, a, eaveY - 0.35, out), 0.04, color);
        m_mesh.addBeam(onWall(side, a, eaveY - 0.35, out), onWall(side, a, drum ? drumTop : ground - 0.2, out), 0.04, color);
        if (drum)
        {
            addDrum(m_mesh, Vec3(foot.x, m_site.m_floor.y + ground - 0.3, foot.z), drumRadius, drumHeight + 0.3, color.scaled(0.93f));
        }
    }

    // Class GableModel
    GableModel::GableModel(const Shape& shape, const Paint& paint)
        :
        m_shape(shape)
        , m_paint(paint)
    {
    }

    double GableModel::ridgeHeight(double width) const
    {
        return m_shape.m_wallHeight + 0.5 * width * std::tan(m_shape.m_pitch);
    }

    Shape GableModel::shapeFor(const Site&) const
    {
        return m_shape;
    }

    void GableModel::build(Mesh& mesh, const Site& site) const
    {
        const Shape shape = shapeFor(site);
        Builder builder(mesh, site, shape, m_paint);
        draw(builder, m_paint);
    }
}
