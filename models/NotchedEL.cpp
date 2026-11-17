// Slufter3D - the houses of the EL line
// Author: Jan de Kort
// Created: 2026-11-16
// Luxe4EL (photos B453_Type_4EL, 4EL-2,
// images, images-1). A hipped roof over the whole rectangle; at the entrance a corner of the house is cut away.
// The roof is cut away with it, so that its edges rise from the eaves to the inner corner of the cut-out:
// with their white boards they form the white ^ of the photographs. The walls in the cut-out are boarded
// black up to those edges; the double doors with the white half-circle are in the cut-out.
#include "NotchedEL.h"

#include "ModelKit.h"
#include "PlanKit.h"

#include "Check.h"
#include "Mesh.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>

using namespace ModelKit;

namespace
{
    // Struct NotchedPlan: a rectangle with a corner cut away at the front right, and on the 4EL also at the back left
    struct NotchedPlan
    {
        double m_halfLength = 5.3;    // walls, along u [m]
        double m_halfWidth = 5.3;     // walls, along v [m]
        UV m_front{ 1.9, 1.6 };       // walls give way to the front-right of this corner
        bool m_backNotch = false;
        UV m_back{ -2.2, -2.55 };     // and, with m_backNotch, to the back-left of this one

        bool inWalls(double u, double v, double margin) const
        {
            return std::fabs(u) < m_halfLength + margin && std::fabs(v) < m_halfWidth + margin
                && !(u > m_front.u + margin && v > m_front.v + margin)
                && !(m_backNotch && u < m_back.u - margin && v < m_back.v - margin);
        }
    };

    // The 4EL, after the height map of houses 421, 424, 438, 453 and 456 and the owner's sketch: walls 10.6 m by
    // 10.6 m, the front-right cut-out 3.4 m wide and 3.7 m deep, the back-left 3.1 m wide and 2.75 m deep
    NotchedPlan makePlan4EL()
    {
        NotchedPlan plan;
        plan.m_backNotch = true;
        return plan;
    }

    const NotchedPlan plan4EL = makePlan4EL();

    const double elWallHeight = 2.55;
    const double elPlinth = 0.60;
    const double elOverhang = 0.50;   // roof beyond the walls on every side [m]
    const double elPitch = 35.0;      // [deg]
    const double elFoundation = 2.5;  // walls reach this far into the dune [m]
    const double elFascia = 0.18;
    const double elBoard = 0.32;      // depth of the white boards along the cut-out [m]

    const Shape shape4EL = { elWallHeight, elPlinth, degrees(elPitch), elOverhang, elOverhang, elFascia, 1.0, 1.0 };
    const Paint paint4EL = {
        Color(0.46f, 0.23f, 0.19f),   // red-brown brick
        Color(0.88f, 0.87f, 0.83f),   // white plinth
        Color(0.17f, 0.17f, 0.19f),   // black tiles
        Color(0.17f, 0.17f, 0.19f),
        Color(0.14f, 0.15f, 0.17f),   // black boarding in the cut-outs
        Color(0.96f, 0.95f, 0.91f),   // white boards, brackets and frames
        Color(0.30f, 0.36f, 0.40f),
        Color(0.13f, 0.14f, 0.16f),   // black doors
        Color(0.13f, 0.14f, 0.16f),
        Color(0.12f, 0.12f, 0.13f) };

    void buildNotchedEL(Mesh& mesh, const Site& site, const Paint& paint, const NotchedPlan& plan)
    {
        using Wall = PlanBuilder::Wall;
        PlanBuilder b(mesh, site);
        const double hu = plan.m_halfLength;
        const double hv = plan.m_halfWidth;
        const double eu = hu + elOverhang;                                   // the eaves
        const double ev = hv + elOverhang;
        const UV fn = plan.m_front;
        const UV bn = plan.m_back;
        const UV fe{ fn.u + elOverhang, fn.v + elOverhang };                // the cut-outs at the eaves
        const UV be{ bn.u - elOverhang, bn.v - elOverhang };
        const bool backNotch = plan.m_backNotch;
        const double tangent = std::tan(degrees(elPitch));
        const double eave = elWallHeight - elOverhang * tangent;            // height of the eaves
        const Color& white = paint.m_trim;

        // The roof: four faces of one pitch over the whole rectangle
        const std::vector<RoofPlane> planes = {
            RoofPlane{ 0.0, -tangent, eave + tangent * ev },     // front face
            RoofPlane{ 0.0, tangent, eave + tangent * ev },      // back face
            RoofPlane{ -tangent, 0.0, eave + tangent * eu },     // right face
            RoofPlane{ tangent, 0.0, eave + tangent * eu } };    // left face
        auto height = [&](double u, double v)
        {
            return std::min(std::min(planes[0].at(u, v), planes[1].at(u, v)), std::min(planes[2].at(u, v), planes[3].at(u, v)));
        };
        auto underRoof = [&](double u, double v)
        {
            return std::fabs(u) < eu && std::fabs(v) < ev && !(u > fe.u && v > fe.v) && !(backNotch && u < be.u && v < be.v);
        };

        // Struct Face: a wall, and whether it stands in a cut-out
        struct Face
        {
            Wall m_wall;
            bool m_cutOut;
        };
        std::vector<Face> faces = {
            { Wall{ UV{ -hu, hv }, UV{ fn.u, hv } }, false },       // front, brick
            { Wall{ UV{ fn.u, hv }, UV{ fn.u, fn.v } }, true },     // in the front cut-out, facing right
            { Wall{ UV{ fn.u, fn.v }, UV{ hu, fn.v } }, true },     // in the front cut-out, facing front: the doors
            { Wall{ UV{ hu, fn.v }, UV{ hu, -hv } }, false } };     // right
        if (backNotch)
        {
            faces.push_back({ Wall{ UV{ hu, -hv }, UV{ bn.u, -hv } }, false });       // back of the right part
            faces.push_back({ Wall{ UV{ bn.u, -hv }, UV{ bn.u, bn.v } }, true });     // in the back cut-out, facing left
            faces.push_back({ Wall{ UV{ bn.u, bn.v }, UV{ -hu, bn.v } }, true });     // in the back cut-out, facing back
            faces.push_back({ Wall{ UV{ -hu, bn.v }, UV{ -hu, hv } }, false });       // left
        }
        else
        {
            faces.push_back({ Wall{ UV{ hu, -hv }, UV{ -hu, -hv } }, false });        // back
            faces.push_back({ Wall{ UV{ -hu, -hv }, UV{ -hu, hv } }, false });        // left
        }

        for (const Face& face : faces)
        {
            const Wall& wall = face.m_wall;
            const double len = b.length(wall);
            b.panel(wall, 0.0, len, -elFoundation, elPlinth, paint.m_plinth, 0.0);
            b.wallUpToRoof(wall, elPlinth, planes, height, face.m_cutOut ? paint.m_gable : paint.m_wall, 0.0);
            if (!face.m_cutOut)
            {
                const int brackets = std::max(1, (int) (len / 0.9));
                for (int k = 0; k <= brackets; ++k)
                {
                    b.bracket(wall, 0.25 + (len - 0.5) * (double) k / (double) brackets, eave - elFascia, white);
                }
            }
        }

        { // The entrance in the front cut-out
            const Wall& side = faces[1].m_wall;
            const Wall& front = faces[2].m_wall;
            const double len = b.length(front);
            b.panel(front, len - 0.45, len, elPlinth, elWallHeight, paint.m_wall, 0.01);          // brick at the corner
            const double doors0 = 0.25;
            const double doors1 = std::min(1.60, len - 0.9);
            b.panel(front, doors0 - 0.08, doors1 + 0.08, -0.2, 2.53, white, 0.04);
            b.panel(front, doors0, 0.5 * (doors0 + doors1) - 0.02, -0.2, 2.45, paint.m_door, 0.07);
            b.panel(front, 0.5 * (doors0 + doors1) + 0.02, doors1, -0.2, 2.45, paint.m_door, 0.07);
            const int steps = 10;          // the white half-circle across both leaves, from the outer edges up
            const double span = 0.5 * (doors1 - doors0) - 0.10;
            auto archHeight = [&](int k)
            {
                const double t = (double) k / (double) steps;
                return 1.30 + 0.90 * std::sqrt(1.0 - (1.0 - t)*(1.0 - t));
            };
            for (int k = 0; k < steps; ++k)
            {
                const double a = doors0 + 0.08 + span * (double) k / (double) steps;
                const double width = span / (double) steps;
                const double low = archHeight(k);
                const double high = archHeight(k + 1) + 0.08;
                b.panel(front, a, a + width, low, high, white, 0.10);
                b.panel(front, doors0 + doors1 - a - width, doors0 + doors1 - a, low, high, white, 0.10);
            }
            b.panel(front, doors1 + 0.30, doors1 + 0.44, 1.95, 2.15, Color(0.22f, 0.22f, 0.23f), 0.12);   // lamp
            const double sideLen = b.length(side);
            b.window(side, 0.45 * sideLen, 0.45 * sideLen + 0.45, 1.75, 2.60, 1, white, paint.m_glass);
            // Paving from the doors to the front
            b.mesh().addQuad(b.onWall(front, doors0 - 0.3, 0.03, 0.0), b.onWall(front, doors1 + 0.3, 0.03, 0.0),
                b.onWall(front, doors1 + 0.3, 0.03, sideLen + 3.0), b.onWall(front, doors0 - 0.3, 0.03, sideLen + 3.0),
                Color(0.62f, 0.55f, 0.40f), Vec3(0.0, 1.0, 0.0));
        }

        { // Windows in the brick walls, downpipe, terrace behind
            const Wall& front = faces[0].m_wall;
            const double frontLen = b.length(front);
            b.window(front, 1.20, 2.30, 1.05, 2.05, 2, white, paint.m_glass);
            if (frontLen > 9.0)
            {
                b.window(front, 0.45 * frontLen, 0.45 * frontLen + 1.1, 1.05, 2.05, 2, white, paint.m_glass);
            }
            b.panel(front, frontLen - 0.20, frontLen - 0.12, -0.2, eave - elFascia, Color(0.62f, 0.63f, 0.64f), 0.08);
            const Wall& right = faces[3].m_wall;
            const double rightLen = b.length(right);
            b.window(right, 0.18 * rightLen, 0.18 * rightLen + 1.1, 1.05, 2.05, 2, white, paint.m_glass);
            b.window(right, 0.62 * rightLen, 0.62 * rightLen + 1.1, 1.05, 2.05, 2, white, paint.m_glass);
            const Wall& back = faces[4].m_wall;
            const double backLen = b.length(back);
            b.window(back, 0.20 * backLen, 0.75 * backLen, 0.62, 2.20, 4, white, paint.m_glass);
            b.mesh().addQuad(b.onWall(back, 0.15 * backLen, 0.03, 0.0), b.onWall(back, 0.85 * backLen, 0.03, 0.0),
                b.onWall(back, 0.85 * backLen, 0.03, 3.0), b.onWall(back, 0.15 * backLen, 0.03, 3.0), terraceGrey,
                Vec3(0.0, 1.0, 0.0));
            const Wall& left = faces.back().m_wall;
            const double leftLen = b.length(left);
            b.window(left, 0.20 * leftLen, 0.20 * leftLen + 1.1, 1.05, 2.05, 2, white, paint.m_glass);
            b.window(left, 0.60 * leftLen, 0.60 * leftLen + 1.1, 1.05, 2.05, 2, white, paint.m_glass);
            if (backNotch)
            {
                const Wall& backSide = faces[5].m_wall;
                const Wall& backLeft = faces[6].m_wall;
                b.window(backSide, 0.30 * b.length(backSide), 0.30 * b.length(backSide) + 1.0, 0.02, 2.20, 1, white, paint.m_glass);
                b.window(backLeft, 0.30 * b.length(backLeft), 0.30 * b.length(backLeft) + 1.1, 1.05, 2.05, 2, white, paint.m_glass);
            }
        }

        // The roof, the white boards along its edges over the cut-outs, the dark fascia along the other eaves,
        // the underside of the eaves along the outer walls
        b.roof(gridCells({ -eu, be.u, 0.0, fe.u, eu }, { -ev, be.v, 0.0, fe.v, ev }, underRoof), planes, height, paint.m_roof);
        std::vector<Wall> boards = { Wall{ UV{ fe.u, ev }, UV{ fe.u, fe.v } }, Wall{ UV{ fe.u, fe.v }, UV{ eu, fe.v } } };
        std::vector<Wall> eaves = { Wall{ UV{ -eu, ev }, UV{ fe.u, ev } }, Wall{ UV{ eu, fe.v }, UV{ eu, -ev } } };
        if (backNotch)
        {
            boards.push_back(Wall{ UV{ be.u, -ev }, UV{ be.u, be.v } });
            boards.push_back(Wall{ UV{ be.u, be.v }, UV{ -eu, be.v } });
            eaves.push_back(Wall{ UV{ eu, -ev }, UV{ be.u, -ev } });
            eaves.push_back(Wall{ UV{ -eu, be.v }, UV{ -eu, ev } });
        }
        else
        {
            eaves.push_back(Wall{ UV{ eu, -ev }, UV{ -eu, -ev } });
            eaves.push_back(Wall{ UV{ -eu, -ev }, UV{ -eu, ev } });
        }
        for (const Wall& board : boards)
        {
            b.edgeBoard(board, elBoard, planes, height, white);
        }
        for (const Wall& edge : eaves)
        {
            b.panel(edge, 0.0, b.length(edge), eave - elFascia, eave + 0.02, Color(0.20f, 0.20f, 0.22f), 0.0);
        }
        auto underEaves = [&](double u, double v)
        {
            return underRoof(u, v) && (std::fabs(u) > hu || std::fabs(v) > hv);
        };
        b.soffit(gridCells({ -eu, -hu, be.u, fe.u, hu, eu }, { -ev, -hv, be.v, fe.v, hv, ev }, underEaves), eave - elFascia, white);

        { // Three flues on the roof behind the cut-out
            for (int k = 0; k < 3; ++k)
            {
                const double u = fe.u + 0.7 + 0.32 * (double) k;
                const double v = fe.v - 0.8;
                const double foot = height(u, v) - 0.2;
                const double rise = 0.55 + 0.12 * (double) (k % 2);
                b.mesh().addBox(b.point(u, v, foot + 0.5 * rise), site.m_axisLength, Vec3(0.0, 1.0, 0.0), site.m_axisWidth,
                    Vec3(0.08, 0.5 * rise, 0.08), paint.m_pipe);
            }
        }
    }
}

std::pair<double, double> NotchedEL::size4EL()
{
    return { 2.0 * plan4EL.m_halfLength, 2.0 * plan4EL.m_halfWidth };
}

double NotchedEL::ridgeHeight(double width)
{
    return shape4EL.m_wallHeight + 0.5 * width * std::tan(shape4EL.m_pitch);
}

bool NotchedEL::covers4EL(double du, double dv, double margin)
{
    return plan4EL.inWalls(du, dv, margin);
}

void NotchedEL::build4EL(Mesh& mesh, const Site& site)
{
    buildNotchedEL(mesh, site, paint4EL, plan4EL);
}
