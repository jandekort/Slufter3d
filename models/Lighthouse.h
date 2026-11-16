// Slufter3D - the Eierland lighthouse (Vuurtoren Texel), background scenery north of the park
// Author: Jan de Kort
// Created: 2026-11-15
#pragma once

#include "Color.h"
#include "Math.h"

class Mesh;

// Class Lighthouse
// Drawn after photo Lighthouse: a red round tower, slightly tapering, a white band under the first gallery,
// the white watch room, a second gallery round the glazed lantern, a white dome and the radar antenna on top.
// Small white-framed windows climb the tower with the stairs. 34.7 m from the foot to the top.
class Lighthouse
{
public:
    Lighthouse();

    // Mutator
    void build(Mesh& mesh, const Vec3& foot) const;

    // Accessor
    double getHeight() const { return m_height; }
    double getFootRadius() const { return m_footRadius; }

private:
    // A tapering tube between two heights above the foot
    void addTube(Mesh& mesh, const Vec3& foot, double y0, double y1, double r0, double r1, const Color& color) const;
    // A level ring between two radii, facing up or down
    void addRing(Mesh& mesh, const Vec3& foot, double y, double inner, double outer, const Color& color, bool up) const;
    // A gallery: a deck with its edge, and a railing of posts and two rails
    void addGallery(Mesh& mesh, const Vec3& foot, double y, double inner, double outer) const;
    // A small window on the round wall at the given angle from the east, anticlockwise seen from above
    void addWindow(Mesh& mesh, const Vec3& foot, double angle, double y, double radius, double width, double height) const;
    Vec3 around(const Vec3& foot, double angle, double radius, double y) const;
    double radiusAt(double y) const; // of the red shaft and its white band

    double m_height;          // foot to the top of the antenna [m]
    double m_shaftTop;        // top of the red shaft [m]
    double m_bandTop;         // top of the white band, the deck of the first gallery [m]
    double m_footRadius;      // of the shaft at the foot [m]
    double m_topRadius;       // of the shaft under the first gallery [m]
    double m_galleryReach;    // the first gallery juts out this far [m]
    double m_watchTop;        // top of the watch room, deck of the second gallery [m]
    double m_watchRadius;
    double m_upperGallery;    // radius of the second gallery [m]
    double m_lanternBase;     // white wall at the foot of the lantern [m]
    double m_lanternTop;      // top of the glazing [m]
    double m_lanternRadius;
    double m_domeTop;         // [m]
    double m_antennaSpan;     // length of the radar bar [m]
    double m_railing;         // height of the railings [m]
    double m_embed;           // the shaft reaches this far into the dune [m]
    int m_segments;           // facets round the tower
    Color m_red;
    Color m_white;
    Color m_glass;
    Color m_iron;             // railings, antenna
    Color m_dome;
};
