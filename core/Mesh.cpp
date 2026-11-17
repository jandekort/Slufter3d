// Slufter - flat shaded triangle soup, ready to upload as a vertex buffer
// Author: Jan de Kort
// Created: 2026-11-15
#include "Mesh.h"

#include <algorithm>

Mesh::Mesh()
    :
    m_vertices()
    , m_floatsPerVertex(6)
    , m_light(normalize(Vec3(-0.45, 0.75, 0.50))) // sun in the south-west
    , m_ambient(0.68)
    , m_relief(3.0)
{
}

void Mesh::addVertex(const Vec3& p, const Color& color)
{
    m_vertices.push_back((float) p.x);
    m_vertices.push_back((float) p.y);
    m_vertices.push_back((float) p.z);
    m_vertices.push_back(color.r);
    m_vertices.push_back(color.g);
    m_vertices.push_back(color.b);
}

Color Mesh::shade(const Color& color, const Vec3& normal) const
{
    const double diffuse = std::max(0.0, dot(normal, m_light));
    return color.scaled((float) (m_ambient + (1.0 - m_ambient) * diffuse / m_light.y));
}

void Mesh::addTriangle(const Vec3& a, const Vec3& b, const Vec3& c, const Color& color, const Vec3& outward)
{
    Vec3 normal = normalize(cross(b - a, c - a), outward);
    if (dot(normal, outward) < 0.0)
    {
        normal = -normal;
    }
    const Color lit = shade(color, normal);
    addVertex(a, lit);
    addVertex(b, lit);
    addVertex(c, lit);
}

void Mesh::addQuad(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, const Color& color, const Vec3& outward)
{
    addTriangle(a, b, c, color, outward);
    addTriangle(a, c, d, color, outward);
}

void Mesh::addBox(const Vec3& center, const Vec3& axisX, const Vec3& axisY, const Vec3& axisZ,
    const Vec3& halfSize, const Color& color)
{
    const Vec3 x = axisX * halfSize.x;
    const Vec3 y = axisY * halfSize.y;
    const Vec3 z = axisZ * halfSize.z;
    addQuad(center + x - y - z, center + x + y - z, center + x + y + z, center + x - y + z, color, axisX);
    addQuad(center - x - y - z, center - x + y - z, center - x + y + z, center - x - y + z, color, -axisX);
    addQuad(center + y - x - z, center + y + x - z, center + y + x + z, center + y - x + z, color, axisY);
    addQuad(center - y - x - z, center - y + x - z, center - y + x + z, center - y - x + z, color, -axisY);
    addQuad(center + z - x - y, center + z + x - y, center + z + x + y, center + z - x + y, color, axisZ);
    addQuad(center - z - x - y, center - z + x - y, center - z + x + y, center - z - x + y, color, -axisZ);
}

void Mesh::addBeam(const Vec3& from, const Vec3& to, double halfThickness, const Color& color)
{
    const Vec3 along = normalize(to - from);
    const Vec3 reference = std::fabs(along.y) < 0.9 ? Vec3(0.0, 1.0, 0.0) : Vec3(1.0, 0.0, 0.0);
    const Vec3 side = normalize(cross(along, reference));
    const Vec3 other = cross(side, along);
    addBox((from + to) * 0.5, side, along, other, Vec3(halfThickness, 0.5 * length(to - from), halfThickness), color);
}

void Mesh::addFlatTriangle(const Vec3& a, const Vec3& b, const Vec3& c, const Color& ca, const Color& cb, const Color& cc)
{
    addVertex(a, ca);
    addVertex(b, cb);
    addVertex(c, cc);
}

Color Mesh::litGround(const Color& color, const Vec3& normal) const
{
    const Vec3 up = normal.y < 0.0 ? -normal : normal;
    const Vec3 steep = normalize(Vec3(m_relief * up.x, up.y, m_relief * up.z));
    return shade(color, steep);
}
