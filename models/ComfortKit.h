// Slufter3D - ComfortKit: the parts the Comfort bungalows are built of
// Author: Jan de Kort
// Created: 2026-11-17, from what Comfort6C drew on its own
// Light brick with darker courses at its foot, white frames set back in it, slate gables flush with the boards of
// the eaves, ridge tiles, flues, and paving that lies on the ground. A Style gives the measures and colours; a
// Mason lays the parts on a builder in that style.
#pragma once

#include "ModelKit.h"

#include <vector>

namespace ComfortKit
{
    // Struct Style: the measures and colours of the parts
    struct Style
    {
        double m_soffit;      // underside of the eaves all round; frames and doors reach up to it [m]
        double m_fascia;      // height of the boards along the eaves [m]
        double m_overhang;    // of eaves and verges [m]
        double m_plinth;      // top of the darker courses [m]
        double m_foundation;  // walls reach this far into the dune [m]
        double m_reveal;      // frames and doors stand this far back in the brick [m]
        double m_bar;         // width of the white frames [m]
        double m_doorTop;     // top of the door leaves, under a light up to the soffit [m]
        Color m_white;        // frames, panels, downpipes
        Color m_underside;    // the soffits
        Color m_stone;        // sills and thresholds
    };

    // Struct Opening: a hole in a wall from a0 to a1 along it, y0 to y1 high, with a1 > a0
    struct Opening
    {
        double m_a0;
        double m_a1;
        double m_y0;
        double m_y1;
        bool m_sill;    // true->a sill under it, false->a threshold
    };

    // Class Mason: lays the parts of a Comfort bungalow on a builder, in a style and its paint
    class Mason
    {
    public:
        Mason(ModelKit::Builder& builder, const Style& style, const ModelKit::Paint& paint);

        // Accessor
        Vec3 along(ModelKit::Side side) const;     // unit vector along a side, the way a runs

        // Mutator
        // Brick from y0 to y1 between a0 and a1, out in front of the wall, darker below the plinth line
        void brick(ModelKit::Side side, double a0, double a1, double y0, double y1, double out);
        // A wall from deep in the dune up to the soffit, with its openings in it
        void wall(ModelKit::Side side, std::vector<Opening> openings);
        // The white frame that fills an opening at the back of its reveal
        void frame(ModelKit::Side side, const Opening& o);
        // Glass, a leaf or a panel in the frame, just before it; inLeaf->a pane in a leaf, just before that
        void fill(ModelKit::Side side, double a0, double a1, double y0, double y1, const Color& color, bool inLeaf = false);
        // Lights of glass between the bars of a frame at the given places along the wall, from y0 to y1
        void lights(ModelKit::Side side, const std::vector<double>& bars, double y0, double y1);
        // A door from a0 to a1 in its frame: its leaf up to the top of the doors, a light over it up to the soffit
        void door(ModelKit::Side side, double a0, double a1, const Color& leaf);
        // A lever on a door, at a along the wall
        void handle(ModelKit::Side side, double a);
        // The slate of a gable this far out of the wall, from the soffit up under the roof, and the soffit between it
        // and the wall; board->a board along the eaves under the slate, as high as those along the sides
        void slateGable(ModelKit::Side side, double out, bool board);
        // Ridge tiles from u0 to u1 along the top, a little proud of both slopes
        void ridgeTiles(double u0, double u1, const Color& tile);
        // A pipe through the roof at (u, v), standing this far above it
        void pipe(double u, double v, double above, double radius, const Color& color);
        // Paving before a side, from a0 to a1 along it and depth out from the wall, laid just above the ground in
        // tiles that follow it. Against the wall, but for a margin at either end, it is raised to the floor if the
        // ground lies lower there, and slopes down from it to the ground at its outer edge.
        void paving(ModelKit::Side side, double a0, double a1, double depth, double margin, const Color& color);

    private:
        // The sides of an opening, back to its frame: brick at either side, the soffit over it when it reaches up
        // to that, else brick, and stone at its foot unless a sill covers it
        void reveal(ModelKit::Side side, const Opening& o);

        ModelKit::Builder& m_builder;
        const Style& m_style;
        const ModelKit::Paint& m_paint;
    };
}
