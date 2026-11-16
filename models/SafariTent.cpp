// Slufter3D - model SafariTent: canvas on a timber frame, on a deck, the veranda under the same roof
// Author: Jan de Kort
// Created: 2026-11-16
// SafariTent (photo Type_6ST_Safaritent)
// Canvas on a frame of round timber, on a deck; the roof runs on over the veranda at the +u end.
// Heights are above the floor, which is the ground at the middle of the deck.
#include "Models.h"

#include "ModelKit.h"

#include "Check.h"
#include "Ground.h"
#include "Mesh.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>

using namespace ModelKit;

namespace
{
    const double safariLength = 10.2;       // deck, along the ridge [m]
    const double safariWidth = 6.1;         // deck [m]
    const double safariDeck = 0.45;         // top of the deck [m]
    const double safariTentHalf = 2.5;      // half the width of the canvas room [m]
    const double safariTentBack = -5.0;     // the canvas room runs from here ...
    const double safariTentFront = 2.0;     // ... to here; the veranda takes the rest [m]
    const double safariEaveHalf = 3.2;      // the roof reaches this far from the ridge [m]
    const double safariEave = 2.50;         // edge of the roof [m]
    const double safariRidge = 3.70;        // [m]
    const double safariFoundation = 1.2;    // the boards under the deck reach this far into the dune [m]

    const Paint paintSafari = {
        Color(0.72f, 0.58f, 0.43f),   // tan canvas
        Color(0.40f, 0.29f, 0.22f),   // brown band at the foot of the canvas
        Color(0.56f, 0.57f, 0.51f),   // grey-green roof sheet
        Color(0.31f, 0.32f, 0.29f),   // its underside
        Color(0.72f, 0.58f, 0.43f),
        Color(0.60f, 0.41f, 0.25f),   // round timber of the frame
        Color(0.25f, 0.28f, 0.30f),   // clear plastic windows
        Color(0.14f, 0.12f, 0.11f),   // open door
        Color(0.79f, 0.67f, 0.47f),   // planks of the deck
        Color(0.72f, 0.73f, 0.72f) }; // steel flue

    void buildSafariTent(Mesh& mesh, const Site& site, const Paint& paint)
    {
        const double hl = 0.5 * safariLength;
        const double hw = 0.5 * safariWidth;
        const double slope = (safariRidge - safariEave) / safariEaveHalf;
        const double roofEnd = hl + 0.25;
        const double band = 0.40;       // height of the brown band [m]
        const double pole = 0.06;
        const Vec3 up(0.0, 1.0, 0.0);
        const auto point = [&](double u, double v, double y)
            {
                return site.m_floor + site.m_axisLength * u + site.m_axisWidth * v + Vec3(0.0, y, 0.0);
            };
        const auto roofAt = [&](double v)
            {
                return safariRidge - std::fabs(v) * slope;
            };
        // Deck: planks on boards that run into the dune
        mesh.addBox(point(0.0, 0.0, 0.5 * (safariDeck - safariFoundation)), site.m_axisLength, up, site.m_axisWidth,
            Vec3(hl, 0.5 * (safariDeck + safariFoundation), hw), paint.m_shutter);
        // Canvas walls along the sides, with three windows each
        const double wallTop = roofAt(safariTentHalf) - 0.02;
        for (const double s : { 1.0, -1.0 })
        {
            const Vec3 out = site.m_axisWidth * s;
            const double v = s * safariTentHalf;
            mesh.addQuad(point(safariTentBack, v, safariDeck), point(safariTentFront, v, safariDeck),
                point(safariTentFront, v, safariDeck + band), point(safariTentBack, v, safariDeck + band), paint.m_plinth, out);
            mesh.addQuad(point(safariTentBack, v, safariDeck + band), point(safariTentFront, v, safariDeck + band),
                point(safariTentFront, v, wallTop), point(safariTentBack, v, wallTop), paint.m_wall, out);
            for (int k = 0; k < 3; ++k)
            {
                const double u0 = safariTentBack + 0.75 + 2.25 * (double) k;
                const double vv = v + s * 0.03;
                mesh.addQuad(point(u0, vv, safariDeck + 0.75), point(u0 + 1.0, vv, safariDeck + 0.75),
                    point(u0 + 1.0, vv, safariDeck + 2.05), point(u0, vv, safariDeck + 2.05), paint.m_glass, out);
            }
        }
        // Canvas ends up to the roof; the front one has the door between two windows
        for (const double u : { safariTentBack, safariTentFront })
        {
            const Vec3 out = site.m_axisLength * (u > 0.0 ? 1.0 : -1.0);
            mesh.addQuad(point(u, -safariTentHalf, safariDeck), point(u, safariTentHalf, safariDeck),
                point(u, safariTentHalf, safariDeck + band), point(u, -safariTentHalf, safariDeck + band), paint.m_plinth, out);
            mesh.addQuad(point(u, -safariTentHalf, safariDeck + band), point(u, safariTentHalf, safariDeck + band),
                point(u, safariTentHalf, wallTop), point(u, -safariTentHalf, wallTop), paint.m_wall, out);
            mesh.addTriangle(point(u, -safariTentHalf, wallTop), point(u, safariTentHalf, wallTop),
                point(u, 0.0, safariRidge - 0.02), paint.m_wall, out);
        }
        {
            const double u = safariTentFront + 0.03;
            const Vec3 out = site.m_axisLength;
            mesh.addQuad(point(u, -0.55, safariDeck), point(u, 0.55, safariDeck), point(u, 0.55, safariDeck + 2.05),
                point(u, -0.55, safariDeck + 2.05), paint.m_door, out);
            for (const double s : { 1.0, -1.0 })
            {
                mesh.addQuad(point(u, s * 1.00, safariDeck + 0.75), point(u, s * 1.95, safariDeck + 0.75),
                    point(u, s * 1.95, safariDeck + 2.05), point(u, s * 1.00, safariDeck + 2.05), paint.m_glass, out);
            }
        }
        // Roof sheet, lit from above and dark from below
        for (const double s : { 1.0, -1.0 })
        {
            const Vec3 ridgeBack = point(-roofEnd, 0.0, safariRidge);
            const Vec3 ridgeFront = point(roofEnd, 0.0, safariRidge);
            const Vec3 eaveBack = point(-roofEnd, s * safariEaveHalf, safariEave);
            const Vec3 eaveFront = point(roofEnd, s * safariEaveHalf, safariEave);
            const Vec3 under(0.0, -0.04, 0.0);
            mesh.addQuad(eaveBack, eaveFront, ridgeFront, ridgeBack, paint.m_roof, up);
            mesh.addQuad(eaveBack + under, eaveFront + under, ridgeFront + under, ridgeBack + under, paint.m_roofTop, -up);
        }
        // Frame: ridge pole, poles along the eaves, a post and a leaning brace at each bay, rafters at the veranda
        mesh.addBeam(point(-roofEnd, 0.0, safariRidge - 0.10), point(roofEnd, 0.0, safariRidge - 0.10), pole, paint.m_trim);
        const double bays[] = { safariTentBack, -2.67, -0.33, safariTentFront, hl - 0.10 };
        for (const double s : { 1.0, -1.0 })
        {
            const double edge = s * (hw - 0.10);
            mesh.addBeam(point(-roofEnd, s * (safariEaveHalf - 0.10), roofAt(safariEaveHalf - 0.10) - 0.08),
                point(roofEnd, s * (safariEaveHalf - 0.10), roofAt(safariEaveHalf - 0.10) - 0.08), pole, paint.m_trim);
            for (const double u : bays)
            {
                mesh.addBeam(point(u, edge, safariDeck), point(u, edge, roofAt(edge) - 0.06), pole, paint.m_trim);
                if (u < safariTentFront + 0.01)
                {
                    mesh.addBeam(point(u + 0.9, s * (safariTentHalf + 0.06), safariDeck),
                        point(u + 0.25, edge, roofAt(edge) - 0.10), 0.8 * pole, paint.m_trim);
                }
            }
            for (const double u : { safariTentFront, hl - 0.10 })
            {
                mesh.addBeam(point(u, edge, roofAt(edge) - 0.08), point(u, 0.0, safariRidge - 0.10), pole, paint.m_trim);
            }
        }
        mesh.addBeam(point(hl - 0.10, 0.0, safariDeck), point(hl - 0.10, 0.0, safariRidge - 0.10), pole, paint.m_trim);
        mesh.addBeam(point(hl - 0.10, -(hw - 0.10), safariEave + 0.05), point(hl - 0.10, hw - 0.10, safariEave + 0.05), pole,
            paint.m_trim);
        // Railing of the veranda: along the back, across the end beside the middle post, a short piece at the front
        const double railTop = safariDeck + 0.95;
        const auto rail = [&](double u0, double v0, double u1, double v1)
            {
                for (const double y : { railTop, safariDeck + 0.50 })
                {
                    mesh.addBeam(point(u0, v0, y), point(u1, v1, y), 0.035, paint.m_shutter);
                }
            };
        rail(safariTentFront, -(hw - 0.10), hl - 0.10, -(hw - 0.10));
        rail(hl - 0.10, -(hw - 0.10), hl - 0.10, 0.0);
        rail(safariTentFront, hw - 0.10, safariTentFront + 1.3, hw - 0.10);
        // Steps at the front, beside the veranda
        for (int k = 0; k < 2; ++k)
        {
            const double tread = 0.30;
            const double top = safariDeck - 0.15 * (double) (k + 1);
            mesh.addBox(point(3.9, hw + tread * ((double) k + 0.5), 0.5 * (top - safariFoundation)), site.m_axisLength, up,
                site.m_axisWidth, Vec3(0.55, 0.5 * (top + safariFoundation), 0.5 * tread), paint.m_shutter.scaled(0.92f));
        }
        // Table with benches on the veranda
        const Color furniture = paint.m_shutter.scaled(1.05f);
        mesh.addBox(point(3.5, -0.9, safariDeck + 0.72), site.m_axisLength, up, site.m_axisWidth, Vec3(0.9, 0.04, 0.42), furniture);
        for (const double s : { 1.0, -1.0 })
        {
            mesh.addBox(point(3.5, -0.9 + s * 0.68, safariDeck + 0.22), site.m_axisLength, up, site.m_axisWidth,
                Vec3(0.9, 0.22, 0.17), furniture);
            mesh.addBox(point(3.5 + s * 0.7, -0.9, safariDeck + 0.35), site.m_axisLength, up, site.m_axisWidth,
                Vec3(0.05, 0.35, 0.35), furniture);
        }
        // Flue of the stove: up the front wall and through the eaves, with a cap
        const double flueU = -0.75;
        const double flueV = safariTentHalf + 0.22;
        mesh.addBox(point(flueU, flueV, 0.5 * (safariDeck + safariRidge + 0.5)), site.m_axisLength, up, site.m_axisWidth,
            Vec3(0.07, 0.5 * (safariRidge + 0.5 - safariDeck), 0.07), paint.m_pipe);
        mesh.addBox(point(flueU, flueV, safariRidge + 0.56), site.m_axisLength, up, site.m_axisWidth,
            Vec3(0.13, 0.03, 0.13), paint.m_pipe.scaled(0.8f));
    }

    // Class SafariTent
    class SafariTent : public Model
    {
    public:
        // Accessor
        const char* name() const override { return "SafariTent"; }
        const char* label() const override { return "6ST"; }
        std::pair<double, double> footprint(double, double) const override { return { safariLength, safariWidth }; }
        double ridgeHeight(double) const override { return safariRidge; }

        void build(Mesh& mesh, const Site& site) const override { buildSafariTent(mesh, site, paintSafari); }
    };
}

const Model& Models::safariTent()
{
    static const SafariTent model;
    return model;
}
