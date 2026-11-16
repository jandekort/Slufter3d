// Slufter3D - drawing a house on a polygonal plan, under a roof of several planes
// Author: Jan de Kort
// Created: 2026-11-16
#pragma once

#include "Color.h"
#include "Math.h"
#include "Site.h"

#include <functional>
#include <vector>

class Mesh;

namespace ModelKit
{
    // Struct UV: a point of a plan, u to the right seen from the front, v towards the front [m]
    struct UV
    {
        double u = 0.0;
        double v = 0.0;
    };

    // Struct RoofPlane: the height a*u + b*v + c of a plane roof face over the plan
    struct RoofPlane
    {
        double a = 0.0;
        double b = 0.0;
        double c = 0.0;

        double at(double u, double v) const { return a*u + b*v + c; }
    };

    double area(const std::vector<UV>& poly);

    // Splits a convex polygon by the line a*u + b*v + c = 0 into the parts below and above it
    void splitConvex(const std::vector<UV>& poly, double a, double b, double c, std::vector<UV>& below,
        std::vector<UV>& above);

    // Axis-parallel rectangles covering a region of the plan: the cells of the grid on us x vs whose middle lies in it
    std::vector<std::vector<UV>> gridCells(std::vector<double> us, std::vector<double> vs,
        const std::function<bool(double, double)>& inside);

    // Class PlanBuilder: draws a house on a polygonal plan
    class PlanBuilder
    {
    public:
        // Struct Wall: a straight wall, from left to right as seen from outside
        struct Wall
        {
            UV m_left;
            UV m_right;
        };

        PlanBuilder(Mesh& mesh, const Site& site);

        Vec3 point(double u, double v, double y) const;

        double length(const Wall& wall) const;

        Vec3 outward(const Wall& wall) const;

        // Point on a wall, a metres from its left end, out metres in front of it
        Vec3 onWall(const Wall& wall, double a, double y, double out) const;

        void panel(const Wall& wall, double a0, double a1, double y0, double y1, const Color& color, double out);

        void window(const Wall& wall, double a0, double a1, double y0, double y1, int columns, const Color& frame,
            const Color& glass);

        // A white bracket under the eaves
        void bracket(const Wall& wall, double a, double top, const Color& color);

        // A roof whose height over the plan is given by height(), which picks one of planes at every point.
        // The pieces are cut along every line where two planes are equally high, so that each final cell
        // lies on a single plane and the hips and valleys come out straight.
        void roof(const std::vector<std::vector<UV>>& pieces, const std::vector<RoofPlane>& planes,
            const std::function<double(double, double)>& height, const Color& color);

        // Horizontal underside of the eaves over the cells
        void soffit(const std::vector<std::vector<UV>>& cells, double y, const Color& color);

        // Parameters along the segment from p to q where two of the planes are equally high, with 0 and 1, in order
        std::vector<double> kinks(const UV& p, const UV& q, const std::vector<RoofPlane>& planes) const;

        // A wall from y0 up to the underside of the roof, which may slope along it
        void wallUpToRoof(const Wall& wall, double y0, const std::vector<RoofPlane>& planes,
            const std::function<double(double, double)>& height, const Color& color, double out);

        // A board along an edge of the roof, hanging below it
        void edgeBoard(const Wall& edge, double depth, const std::vector<RoofPlane>& planes,
            const std::function<double(double, double)>& height, const Color& color);

        Mesh& mesh() { return m_mesh; }
        const Site& site() const { return m_site; }

    private:
        Mesh& m_mesh;
        const Site& m_site;
        double m_skin;        // distance between layers on a wall [m]
    };
}
