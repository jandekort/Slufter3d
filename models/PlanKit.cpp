// Slufter3D - drawing a house on a polygonal plan
// Author: Jan de Kort
// Created: 2026-11-16
#include "PlanKit.h"

#include "Check.h"
#include "Mesh.h"

#include <algorithm>
#include <cmath>

namespace ModelKit
{
    double area(const std::vector<UV>& poly)
    {
        double twice = 0.0;
        for (size_t k = 0; k < poly.size(); ++k)
        {
            const UV& p = poly[k];
            const UV& q = poly[(k + 1) % poly.size()];
            twice += p.u*q.v - q.u*p.v;
        }
        return 0.5 * std::fabs(twice);
    }

    void splitConvex(const std::vector<UV>& poly, double a, double b, double c, std::vector<UV>& below, std::vector<UV>& above)
    {
        const double eps = 1.0e-9;
        below.clear();
        above.clear();
        for (size_t k = 0; k < poly.size(); ++k)
        {
            const UV& p = poly[k];
            const UV& q = poly[(k + 1) % poly.size()];
            const double sp = a*p.u + b*p.v + c;
            const double sq = a*q.u + b*q.v + c;
            if (sp <= eps)
            {
                below.push_back(p);
            }
            if (sp >= -eps)
            {
                above.push_back(p);
            }
            if ((sp < -eps && sq > eps) || (sp > eps && sq < -eps))
            {
                const double t = sp / (sp - sq);
                const UV x{ p.u + t*(q.u - p.u), p.v + t*(q.v - p.v) };
                below.push_back(x);
                above.push_back(x);
            }
        }
    }

    std::vector<std::vector<UV>> gridCells(std::vector<double> us, std::vector<double> vs,
        const std::function<bool(double, double)>& inside)
    {
        std::sort(us.begin(), us.end());
        std::sort(vs.begin(), vs.end());
        std::vector<std::vector<UV>> cells;
        for (size_t i = 0; i + 1 < us.size(); ++i)
        {
            for (size_t j = 0; j + 1 < vs.size(); ++j)
            {
                if (us[i + 1] - us[i] > 1.0e-6 && vs[j + 1] - vs[j] > 1.0e-6
                    && inside(0.5 * (us[i] + us[i + 1]), 0.5 * (vs[j] + vs[j + 1])))
                {
                    cells.push_back({ UV{ us[i], vs[j] }, UV{ us[i + 1], vs[j] }, UV{ us[i + 1], vs[j + 1] }, UV{ us[i], vs[j + 1] } });
                }
            }
        }
        return cells;
    }

    // Class PlanBuilder
    PlanBuilder::PlanBuilder(Mesh& mesh, const Site& site)
        :
        m_mesh(mesh)
        , m_site(site)
        , m_skin(0.03)
    {
    }

    Vec3 PlanBuilder::point(double u, double v, double y) const
    {
        return m_site.m_floor + m_site.m_axisLength * u + m_site.m_axisWidth * v + Vec3(0.0, y, 0.0);
    }

    double PlanBuilder::length(const Wall& wall) const
    {
        return std::hypot(wall.m_right.u - wall.m_left.u, wall.m_right.v - wall.m_left.v);
    }

    Vec3 PlanBuilder::outward(const Wall& wall) const
    {
        const double len = length(wall);
        const double du = (wall.m_right.u - wall.m_left.u) / len;
        const double dv = (wall.m_right.v - wall.m_left.v) / len;
        return m_site.m_axisLength * (-dv) + m_site.m_axisWidth * du;
    }

    Vec3 PlanBuilder::onWall(const Wall& wall, double a, double y, double out) const
    {
        const double t = a / length(wall);
        return point(mix(wall.m_left.u, wall.m_right.u, t), mix(wall.m_left.v, wall.m_right.v, t), y) + outward(wall) * out;
    }

    void PlanBuilder::panel(const Wall& wall, double a0, double a1, double y0, double y1, const Color& color, double out)
    {
        m_mesh.addQuad(onWall(wall, a0, y0, out), onWall(wall, a1, y0, out), onWall(wall, a1, y1, out),
            onWall(wall, a0, y1, out), color, outward(wall));
    }

    void PlanBuilder::window(const Wall& wall, double a0, double a1, double y0, double y1, int columns, const Color& frame,
        const Color& glass)
    {
        const double bar = 0.09;
        panel(wall, a0, a1, y0, y1, frame, m_skin);
        const double pane = (a1 - a0 - bar) / (double) columns;
        for (int c = 0; c < columns; ++c)
        {
            const double pa = a0 + bar + (double) c * pane;
            panel(wall, pa, pa + pane - bar, y0 + bar, y1 - bar, glass, 2.0 * m_skin);
        }
    }

    void PlanBuilder::bracket(const Wall& wall, double a, double top, const Color& color)
    {
        const Vec3 out = outward(wall);
        const Vec3 up(0.0, 1.0, 0.0);
        const Vec3 foot = onWall(wall, a, top - 0.38, 0.0);
        const Vec3 along = normalize(onWall(wall, a + 1.0, 0.0, 0.0) - onWall(wall, a, 0.0, 0.0));
        m_mesh.addBox(foot + up * 0.19 + out * 0.06, along, up, out, Vec3(0.04, 0.19, 0.06), color);
        m_mesh.addBox(foot + up * 0.32 + out * 0.22, along, up, out, Vec3(0.04, 0.06, 0.22), color);
    }

    void PlanBuilder::roof(const std::vector<std::vector<UV>>& pieces, const std::vector<RoofPlane>& planes,
        const std::function<double(double, double)>& height, const Color& color)
    {
        std::vector<std::vector<UV>> cells = pieces;
        std::vector<UV> below;
        std::vector<UV> above;
        for (size_t i = 0; i < planes.size(); ++i)
        {
            for (size_t j = i + 1; j < planes.size(); ++j)
            {
                const double a = planes[i].a - planes[j].a;
                const double b = planes[i].b - planes[j].b;
                const double c = planes[i].c - planes[j].c;
                if (std::fabs(a) + std::fabs(b) < 1.0e-12)
                {
                    continue;
                }
                std::vector<std::vector<UV>> next;
                next.reserve(cells.size());
                for (const std::vector<UV>& cell : cells)
                {
                    splitConvex(cell, a, b, c, below, above);
                    if (below.size() >= 3 && area(below) > 1.0e-7)
                    {
                        next.push_back(below);
                    }
                    if (above.size() >= 3 && area(above) > 1.0e-7)
                    {
                        next.push_back(above);
                    }
                }
                cells.swap(next);
            }
        }
        const Vec3 up(0.0, 1.0, 0.0);
        for (const std::vector<UV>& cell : cells)
        {
            UV middle;
            for (const UV& p : cell)
            {
                middle.u += p.u / (double) cell.size();
                middle.v += p.v / (double) cell.size();
            }
            const double h = height(middle.u, middle.v);
            const auto plane = std::min_element(planes.begin(), planes.end(),
                [&](const RoofPlane& x, const RoofPlane& y)
                {
                    return std::fabs(x.at(middle.u, middle.v) - h) < std::fabs(y.at(middle.u, middle.v) - h);
                });
            for (size_t k = 1; k + 1 < cell.size(); ++k)
            {
                m_mesh.addTriangle(point(cell[0].u, cell[0].v, plane->at(cell[0].u, cell[0].v)),
                    point(cell[k].u, cell[k].v, plane->at(cell[k].u, cell[k].v)),
                    point(cell[k + 1].u, cell[k + 1].v, plane->at(cell[k + 1].u, cell[k + 1].v)), color, up);
            }
        }
    }

    void PlanBuilder::soffit(const std::vector<std::vector<UV>>& cells, double y, const Color& color)
    {
        for (const std::vector<UV>& cell : cells)
        {
            m_mesh.addQuad(point(cell[0].u, cell[0].v, y), point(cell[1].u, cell[1].v, y), point(cell[2].u, cell[2].v, y),
                point(cell[3].u, cell[3].v, y), color, Vec3(0.0, -1.0, 0.0));
        }
    }

    std::vector<double> PlanBuilder::kinks(const UV& p, const UV& q, const std::vector<RoofPlane>& planes) const
    {
        std::vector<double> ts = { 0.0, 1.0 };
        for (size_t i = 0; i < planes.size(); ++i)
        {
            for (size_t j = i + 1; j < planes.size(); ++j)
            {
                const double at0 = planes[i].at(p.u, p.v) - planes[j].at(p.u, p.v);
                const double at1 = planes[i].at(q.u, q.v) - planes[j].at(q.u, q.v);
                if ((at0 < 0.0 && at1 > 0.0) || (at0 > 0.0 && at1 < 0.0))
                {
                    ts.push_back(at0 / (at0 - at1));
                }
            }
        }
        std::sort(ts.begin(), ts.end());
        return ts;
    }

    void PlanBuilder::wallUpToRoof(const Wall& wall, double y0, const std::vector<RoofPlane>& planes,
        const std::function<double(double, double)>& height, const Color& color, double out)
    {
        const double len = length(wall);
        const std::vector<double> ts = kinks(wall.m_left, wall.m_right, planes);
        for (size_t k = 0; k + 1 < ts.size(); ++k)
        {
            const double t0 = ts[k];
            const double t1 = ts[k + 1];
            if (t1 - t0 < 1.0e-9)
            {
                continue;
            }
            const double top0 = height(mix(wall.m_left.u, wall.m_right.u, t0), mix(wall.m_left.v, wall.m_right.v, t0));
            const double top1 = height(mix(wall.m_left.u, wall.m_right.u, t1), mix(wall.m_left.v, wall.m_right.v, t1));
            m_mesh.addQuad(onWall(wall, t0 * len, y0, out), onWall(wall, t1 * len, y0, out), onWall(wall, t1 * len, top1, out),
                onWall(wall, t0 * len, top0, out), color, outward(wall));
        }
    }

    void PlanBuilder::edgeBoard(const Wall& edge, double depth, const std::vector<RoofPlane>& planes,
        const std::function<double(double, double)>& height, const Color& color)
    {
        const double len = length(edge);
        const std::vector<double> ts = kinks(edge.m_left, edge.m_right, planes);
        for (size_t k = 0; k + 1 < ts.size(); ++k)
        {
            const double t0 = ts[k];
            const double t1 = ts[k + 1];
            if (t1 - t0 < 1.0e-9)
            {
                continue;
            }
            const double top0 = height(mix(edge.m_left.u, edge.m_right.u, t0), mix(edge.m_left.v, edge.m_right.v, t0)) + 0.03;
            const double top1 = height(mix(edge.m_left.u, edge.m_right.u, t1), mix(edge.m_left.v, edge.m_right.v, t1)) + 0.03;
            m_mesh.addQuad(onWall(edge, t0 * len, top0 - depth, 0.0), onWall(edge, t1 * len, top1 - depth, 0.0),
                onWall(edge, t1 * len, top1, 0.0), onWall(edge, t0 * len, top0, 0.0), color, outward(edge));
        }
    }
}
