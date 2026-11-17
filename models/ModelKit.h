// Slufter3D - what the models are drawn with: colours, proportions, and a builder of gabled houses
// Author: Jan de Kort
// Created: 2026-11-16
#pragma once

#include "Color.h"
#include "Math.h"
#include "Model.h"
#include "Site.h"

#include <utility>

class Mesh;

namespace ModelKit
{
    const double pi = 3.14159265358979323846;

    enum class Side
    {
        Front,  // +v
        Back,   // -v
        Left,   // -u
        Right   // +u
    };

    // Struct Paint: the colours of one model, read off its photograph
    struct Paint
    {
        Color m_wall;
        Color m_plinth;
        Color m_roof;
        Color m_roofTop;   // upper part of the roof, where it differs
        Color m_gable;     // cladding of the gable above the eaves
        Color m_trim;      // fascia, verges and window frames
        Color m_glass;
        Color m_door;
        Color m_shutter;
        Color m_pipe;
    };

    // Struct Shape: the proportions of one model
    struct Shape
    {
        double m_wallHeight;
        double m_plinthHeight;
        double m_pitch;       // of the roof [rad]
        double m_eave;        // overhang at the eaves [m]
        double m_verge;       // overhang at the gables [m]
        double m_fascia;      // height of the board along the eaves [m]
        double m_hip;         // part of the rise that is hipped back at the gables, 0->plain gable
        double m_band;        // part of the rise roofed in m_roof below m_roofTop, 1->all
    };

    double degrees(double angle); // to radians

    // Height of the ground under a point of a site; without a ground the floor is the ground
    double groundLevel(const Site& site, const Vec3& p);

    // An upright drum standing on its foot, closed at the top
    void addDrum(Mesh& mesh, const Vec3& foot, double radius, double height, const Color& color);

    inline const Color terraceGrey(0.38f, 0.39f, 0.42f);

    // The comfort bungalows are 9 m wide and 6 m deep, whatever their outline
    std::pair<double, double> comfortFootprint();
    // Walls of a house whose roof has this outline: the roof reaches about half a metre beyond them
    std::pair<double, double> wallsUnderRoof(double outlineLength, double outlineWidth);

    // Class Builder: draws faces in the frame of one footprint
    class Builder
    {
    public:
        Builder(Mesh& mesh, const Site& site, const Shape& shape, const Paint& paint);

        // Accessor
        double halfLength() const { return m_halfLength; }
        double halfWidth() const { return m_halfWidth; }
        double ridge() const { return m_shape.m_wallHeight + m_rise; }
        double roofHeight(double v) const { return ridge() - std::fabs(v) * std::tan(m_shape.m_pitch); }
        // A full hip slopes like the long sides, so its ridge ends half a width in; a half hip is steeper
        double hipBackOf() const { return m_shape.m_hip >= 1.0 ? m_halfWidth + m_shape.m_verge : m_shape.m_hip * m_rise * 0.8; }

        Vec3 point(double u, double v, double y) const;
        bool mirrored() const { return m_site.m_mirrored; }    // true->drawn as the mirror image of the model

        // The mesh drawn into, for the parts of a model that the builder has no method for
        Mesh& mesh() { return m_mesh; }

        Vec3 outward(Side side) const;

        // Point on the wall of a side: a runs along the wall, out is the distance in front of it
        Vec3 onWall(Side side, double a, double y, double out) const;

        // Mutator
        void panel(Side side, double a0, double a1, double y0, double y1, const Color& color, double out);

        // A framed window of columns by rows panes; out->its frame stands this far further out, as on a bay
        void window(Side side, double a0, double a1, double y0, double y1, int columns, int rows,
            const Color& frame, const Color& glass, double out = 0.0);

        // A door in a frame; glazed->a pane in its upper half; out->it stands this far further out
        void door(Side side, double a0, double a1, double y0, double y1, const Color& frame, const Color& leaf,
            bool glazed, const Color& pane, double out = 0.0);

        // A window in the roof between u0 and u1, from v0 up to v1 on the slope of the given sign
        void skylight(double u0, double u1, double v0, double v1, double sign, const Color& frame, const Color& glass);

        // A leaf either side of the opening from a0 to a1
        void shutters(Side side, double a0, double a1, double y0, double y1, double leaf);

        void walls();

        // The gable above the eaves, cut off level where the roof is hipped back
        void gable(Side side, const Color& color, double out, double inset);

        // Both slopes, the hips, the boards along eaves and verges, and the ridge
        void roof();
        // The same with the underside of the eaves in its own colour
        void roof(const Color& soffit);

        // A row of ridge tiles
        void ridgeCap(const Color& color);

        // A flue through the roof at (u, v), standing this far above the roof
        void pipe(double u, double v, double above, double radius);

        // A dormer on the front slope between u0 and u1 with a row of windows
        void dormer(double u0, double u1, double sill, double head, int panes, const Color& cheek, const Color& top);

        // An open timber frame over a terrace against a side
        void pergola(Side side, double a0, double a1, double depth, double height, const Color& wood);

        // A lean-to against a side
        void annex(Side side, double a0, double a1, double depth, double height, const Color& color);

        // Height of the ground under a point, above the floor; without a ground the floor is the ground
        double groundUnder(const Vec3& p) const;

        // A terrace in front of a side: paving laid on the ground, at the foot of the wall; out->of a wall this far
        // further out, back in when negative
        void terrace(Side side, double a0, double a1, double depth, const Color& color, double out = 0.0);

        // A downpipe from the gutter at position a of a side; drum->into a drum standing on the ground, else into the ground
        void downpipe(Side side, double a, const Color& color, bool drum = true);

    private:
        Mesh& m_mesh;
        const Site& m_site;
        const Shape& m_shape;
        const Paint& m_paint;
        double m_halfLength;
        double m_halfWidth;
        double m_rise;        // of the roof from eaves to ridge
        double m_skin;        // distance between layers on a wall [m]
        double m_foundation;  // walls reach this far below the floor, into the dune
    };

    // Class GableModel
    class GableModel : public Model // Base class of the models drawn with Builder: walls under one gabled roof
    {
    public:
        GableModel(const Shape& shape, const Paint& paint);

        // Accessor
        double ridgeHeight(double width) const override;

        void build(Mesh& mesh, const Site& site) const override;

    protected:
        virtual Shape shapeFor(const Site& site) const; // The proportions on this site
        virtual void draw(Builder& builder, const Paint& paint) const = 0; // Draws the model with the builder

    private:
        const Shape& m_shape;
        const Paint& m_paint;
    };
}
