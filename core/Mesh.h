// Slufter - flat shaded triangle soup, ready to upload as a vertex buffer
// Author: Jan de Kort
// Created: 2026-11-15
#pragma once

#include "Color.h"
#include "Math.h"

#include <cstddef>
#include <vector>

// Class Mesh
// Vertex layout: x, y, z, r, g, b (six floats). Lighting is baked into the colour; the page blends the
// colours of a triangle's corners across it.
class Mesh
{
public:
    Mesh();

    // Mutator
    void clear() { m_vertices.clear(); }
    // The face is lit on the side that outward points to
    void addTriangle(const Vec3& a, const Vec3& b, const Vec3& c, const Color& color, const Vec3& outward);
    void addQuad(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, const Color& color, const Vec3& outward);
    void addBox(const Vec3& center, const Vec3& axisX, const Vec3& axisY, const Vec3& axisZ,
        const Vec3& halfSize, const Color& color);
    // A square bar from one end to the other
    void addBeam(const Vec3& from, const Vec3& to, double halfThickness, const Color& color);
    // The colours as given, one per corner: for the sky, for surfaces that must keep their colour, and for
    // the ground, lit beforehand with litGround()
    void addFlatTriangle(const Vec3& a, const Vec3& b, const Vec3& c, const Color& ca, const Color& cb, const Color& cc);

    // Accessor
    // The ground lit with its slope exaggerated, so that gentle relief shows
    Color litGround(const Color& color, const Vec3& normal) const;
    const float* data() const { return m_vertices.data(); }
    size_t vertexCount() const { return m_vertices.size() / m_floatsPerVertex; }
    size_t floatCount() const { return m_vertices.size(); }

private:
    void addVertex(const Vec3& p, const Color& color);
    Color shade(const Color& color, const Vec3& normal) const;

    std::vector<float> m_vertices;
    size_t m_floatsPerVertex;
    Vec3 m_light;        // direction towards the sun
    double m_ambient;    // light on faces turned away from the sun
    double m_relief;     // factor on the slope of the ground when it is lit
};
