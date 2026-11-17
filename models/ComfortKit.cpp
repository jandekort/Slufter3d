// Slufter3D - ComfortKit: the parts the Comfort bungalows are built of
// Author: Jan de Kort
// Created: 2026-11-17, from what Comfort6C drew on its own
#include "ComfortKit.h"

#include "Mesh.h"

#include <algorithm>
#include <cmath>

using namespace ModelKit;

namespace ComfortKit
{
    Mason::Mason(Builder& builder, const Style& style, const Paint& paint)
        :
        m_builder(builder)
        , m_style(style)
        , m_paint(paint)
    {
    }

    Vec3 Mason::along(Side side) const
    {
        return normalize(m_builder.onWall(side, 1.0, 0.0, 0.0) - m_builder.onWall(side, 0.0, 0.0, 0.0));
    }

    void Mason::brick(Side side, double a0, double a1, double y0, double y1, double out)
    {
        if (a1 - a0 < 1.0e-6 || y1 - y0 < 1.0e-6)
        {
            return;
        }
        if (y0 < m_style.m_plinth)
        {
            m_builder.panel(side, a0, a1, y0, std::min(y1, m_style.m_plinth), m_paint.m_plinth, out);
        }
        if (y1 > m_style.m_plinth)
        {
            m_builder.panel(side, a0, a1, std::max(y0, m_style.m_plinth), y1, m_paint.m_wall, out);
        }
    }

    void Mason::reveal(Side side, const Opening& o)
    {
        Mesh& mesh = m_builder.mesh();
        const Vec3 a = along(side);
        const Vec3 up(0.0, 1.0, 0.0);
        const double reveal = m_style.m_reveal;
        auto jamb = [&](double at, double y0, double y1, const Color& color, const Vec3& facing)
        {
            if (y1 - y0 > 1.0e-6)
            {
                mesh.addQuad(m_builder.onWall(side, at, y0, 0.0), m_builder.onWall(side, at, y0, -reveal),
                    m_builder.onWall(side, at, y1, -reveal), m_builder.onWall(side, at, y1, 0.0), color, facing);
            }
        };
        for (const double s : { 1.0, -1.0 })
        {
            const double at = s > 0.0 ? o.m_a0 : o.m_a1;
            jamb(at, o.m_y0, std::min(o.m_y1, m_style.m_plinth), m_paint.m_plinth, a * s);
            jamb(at, std::max(o.m_y0, m_style.m_plinth), o.m_y1, m_paint.m_wall, a * s);
        }
        auto flat = [&](double y, const Color& color, const Vec3& facing)
        {
            mesh.addQuad(m_builder.onWall(side, o.m_a0, y, 0.0), m_builder.onWall(side, o.m_a1, y, 0.0),
                m_builder.onWall(side, o.m_a1, y, -reveal), m_builder.onWall(side, o.m_a0, y, -reveal), color, facing);
        };
        flat(o.m_y1, o.m_y1 >= m_style.m_soffit ? m_style.m_underside : m_paint.m_wall, -up);
        if (!o.m_sill)
        {
            flat(o.m_y0, m_style.m_stone, up);
        }
    }

    void Mason::wall(Side side, std::vector<Opening> openings)
    {
        std::sort(openings.begin(), openings.end(), [](const Opening& p, const Opening& q) { return p.m_a0 < q.m_a0; });
        const double half = (side == Side::Front || side == Side::Back) ? m_builder.halfLength() : m_builder.halfWidth();
        double from = -half;
        for (const Opening& o : openings)
        {
            brick(side, from, o.m_a0, -m_style.m_foundation, m_style.m_soffit, 0.0);
            brick(side, o.m_a0, o.m_a1, -m_style.m_foundation, o.m_y0, 0.0);
            brick(side, o.m_a0, o.m_a1, o.m_y1, m_style.m_soffit, 0.0);
            reveal(side, o);
            if (o.m_sill)
            {
                const double out = 0.06;
                m_builder.mesh().addBox(m_builder.onWall(side, 0.5 * (o.m_a0 + o.m_a1), o.m_y0 - 0.03, 0.5 * (out - m_style.m_reveal)),
                    along(side), Vec3(0.0, 1.0, 0.0), m_builder.outward(side),
                    Vec3(0.5 * (o.m_a1 - o.m_a0) + 0.04, 0.03, 0.5 * (out + m_style.m_reveal)), m_style.m_stone);
            }
            from = o.m_a1;
        }
        brick(side, from, half, -m_style.m_foundation, m_style.m_soffit, 0.0);
    }

    void Mason::frame(Side side, const Opening& o)
    {
        m_builder.panel(side, o.m_a0, o.m_a1, o.m_y0, o.m_y1, m_style.m_white, -m_style.m_reveal);
    }

    void Mason::fill(Side side, double a0, double a1, double y0, double y1, const Color& color, bool inLeaf /* = false */)
    {
        m_builder.panel(side, a0, a1, y0, y1, color, -m_style.m_reveal + (inLeaf ? 0.024 : 0.012));
    }

    void Mason::lights(Side side, const std::vector<double>& bars, double y0, double y1)
    {
        const double bar = m_style.m_bar;
        for (size_t k = 0; k + 1 < bars.size(); ++k)
        {
            fill(side, bars[k] + 0.5 * bar, bars[k + 1] - 0.5 * bar, y0 + 0.5 * bar, y1 - bar, m_paint.m_glass);
        }
    }

    void Mason::door(Side side, double a0, double a1, const Color& leaf)
    {
        const double bar = m_style.m_bar;
        fill(side, a0 + bar, a1 - bar, 0.0, m_style.m_doorTop, leaf);
        fill(side, a0 + bar, a1 - bar, m_style.m_doorTop + bar, m_style.m_soffit - bar, m_paint.m_glass);
    }

    void Mason::handle(Side side, double a)
    {
        m_builder.mesh().addBox(m_builder.onWall(side, a, 1.05, -m_style.m_reveal + 0.05), along(side), Vec3(0.0, 1.0, 0.0),
            m_builder.outward(side), Vec3(0.07, 0.015, 0.025), m_style.m_stone);
    }

    void Mason::slateGable(Side side, double out, bool board)
    {
        Builder& b = m_builder;
        const double eaveV = b.halfWidth() + m_style.m_overhang;
        const double eaveY = m_style.m_soffit + m_style.m_fascia;
        const double foot = board ? eaveY : m_style.m_soffit;
        const Vec3 o = b.outward(side);
        if (board)
        {
            b.mesh().addQuad(b.onWall(side, -eaveV, m_style.m_soffit, out), b.onWall(side, eaveV, m_style.m_soffit, out),
                b.onWall(side, eaveV, eaveY, out), b.onWall(side, -eaveV, eaveY, out), m_paint.m_trim, o);
            b.mesh().addTriangle(b.onWall(side, -eaveV, eaveY, out), b.onWall(side, eaveV, eaveY, out),
                b.onWall(side, 0.0, b.ridge(), out), m_paint.m_gable, o);
        }
        else
        {
            b.mesh().addQuad(b.onWall(side, -eaveV, foot, out), b.onWall(side, eaveV, foot, out),
                b.onWall(side, eaveV, eaveY, out), b.onWall(side, 0.0, b.ridge(), out), m_paint.m_gable, o);
            b.mesh().addTriangle(b.onWall(side, -eaveV, foot, out), b.onWall(side, 0.0, b.ridge(), out),
                b.onWall(side, -eaveV, eaveY, out), m_paint.m_gable, o);
        }
        if (out > 0.0)
        {
            b.mesh().addQuad(b.onWall(side, -b.halfWidth(), m_style.m_soffit, 0.0), b.onWall(side, b.halfWidth(), m_style.m_soffit, 0.0),
                b.onWall(side, b.halfWidth(), m_style.m_soffit, out), b.onWall(side, -b.halfWidth(), m_style.m_soffit, out),
                m_style.m_underside, Vec3(0.0, -1.0, 0.0));
        }
    }

    void Mason::ridgeTiles(double u0, double u1, const Color& tile)
    {
        Builder& b = m_builder;
        const double edge = 0.13;
        const double low = b.roofHeight(edge) + 0.01;
        const double top = b.ridge() + 0.09;
        for (const double s : { 1.0, -1.0 })
        {
            const double end = s > 0.0 ? u1 : u0;
            b.mesh().addQuad(b.point(u0, s*edge, low), b.point(u1, s*edge, low), b.point(u1, 0.0, top),
                b.point(u0, 0.0, top), tile, b.point(0.0, s, 1.0) - b.point(0.0, 0.0, 0.0));
            b.mesh().addTriangle(b.point(end, -edge, low), b.point(end, edge, low), b.point(end, 0.0, top), tile,
                b.point(s, 0.0, 0.0) - b.point(0.0, 0.0, 0.0));
        }
    }

    void Mason::pipe(double u, double v, double above, double radius, const Color& color)
    {
        Builder& b = m_builder;
        const double foot = b.roofHeight(v) - 0.2;
        const double halfHeight = 0.5 * (above + 0.2);
        b.mesh().addBox(b.point(u, v, foot + halfHeight), normalize(b.point(1.0, 0.0, 0.0) - b.point(0.0, 0.0, 0.0)),
            Vec3(0.0, 1.0, 0.0), b.outward(Side::Front), Vec3(radius, halfHeight, radius), color);
    }

    void Mason::paving(Side side, double a0, double a1, double depth, double margin, const Color& color)
    {
        Builder& b = m_builder;
        const double top = -0.02;   // just under the thresholds
        const double lift = 0.04;   // above the ground [m]
        const double tile = 1.0;    // the paving follows the ground in steps of at most this size [m]
        const double half = (side == Side::Front || side == Side::Back) ? b.halfLength() : b.halfWidth();
        const double level0 = std::max(a0 + margin, -half);
        const double level1 = std::min(a1 - margin, half);
        std::vector<double> ats;
        double from = a0;
        for (const double to : { level0, level1, a1 })
        {
            const int pieces = (int) std::ceil((to - from) / tile - 1.0e-9);
            for (int k = 0; k < pieces; ++k)
            {
                ats.push_back(mix(from, to, (double) k / (double) pieces));
            }
            from = std::max(from, to);
        }
        ats.push_back(a1);
        const int rows = std::max(1, (int) std::ceil(depth / tile));
        auto place = [&](double at, int row)
        {
            const double out = depth * (double) row / (double) rows;
            const Vec3 p = b.onWall(side, at, 0.0, out);
            const double ground = b.groundUnder(p) + lift;
            const bool raised = at > level0 - 1.0e-9 && at < level1 + 1.0e-9;
            const double rise = raised ? std::max(0.0, top - ground) * (1.0 - (double) row / (double) rows) : 0.0;
            return b.onWall(side, at, ground + rise, out);
        };
        // Lit as if level, so that the slight slopes between the tiles do not show
        const Color lit = b.mesh().litGround(color, Vec3(0.0, 1.0, 0.0));
        for (size_t i = 0; i + 1 < ats.size(); ++i)
        {
            for (int j = 0; j < rows; ++j)
            {
                const Vec3 p00 = place(ats[i], j);
                const Vec3 p10 = place(ats[i + 1], j);
                const Vec3 p11 = place(ats[i + 1], j + 1);
                const Vec3 p01 = place(ats[i], j + 1);
                b.mesh().addFlatTriangle(p00, p10, p11, lit, lit, lit);
                b.mesh().addFlatTriangle(p00, p11, p01, lit, lit, lit);
            }
        }
    }
}
