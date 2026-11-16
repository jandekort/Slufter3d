// Slufter3D - native tests of the vector models: each draws the same triangles as in the first application
// Author: Jan de Kort
// Created: 2026-11-16
//
// tests/golden/ holds, for every model on two sites, what the first application (Slufter, src/HouseModel.cpp)
// produced: its triangles as written to the vertex buffer (<id>.f32: x, y, z, r, g, b as 32-bit floats),
// and in models.txt the site, the footprint, the height of the ridge and where its walls are.
#include "Ground.h"
#include "Mesh.h"
#include "Models.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    int failures = 0;

    void check(bool condition, const std::string& what)
    {
        std::cout << (condition ? "  ok    " : "  FAIL  ") << what << std::endl;
        failures += condition ? 0 : 1;
    }

    bool near(double a, double b, double tolerance)
    {
        return std::fabs(a - b) <= tolerance;
    }

    // Class WavyGround: the ground the sloping sites of the golden files were drawn on
    class WavyGround : public Ground
    {
    public:
        explicit WavyGround(double level)
            :
            m_level(level)
        {
        }

        double height(double east, double north) const override
        {
            return m_level - 0.12 + 0.30 * std::sin(0.21 * east) + 0.25 * std::cos(0.17 * north);
        }

    private:
        double m_level; // of the floor of the site [m]
    };

    std::vector<float> readFloats(const std::string& path)
    {
        std::ifstream in(path, std::ios::binary);
        std::vector<float> values;
        float value = 0.0f;
        while (in.read((char*) &value, sizeof(float)))
        {
            values.push_back(value);
        }
        return values;
    }
}

int main(int argc, char** argv)
{
    const std::string dir = argc > 1 ? argv[1] : "tests/golden";
    const double pi = 3.14159265358979323846;
    const double tolerance = 1.0e-4;   // on positions [m] and colours; compilers round differently
    std::ifstream index(dir + "/models.txt");
    check(index.good(), "the list of golden files opens: " + dir + "/models.txt");

    std::set<std::string> tested;
    std::string line;
    while (std::getline(index, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }
        std::istringstream fields(line);
        std::string id;
        std::string name;
        std::string covers;
        double x = 0.0, y = 0.0, z = 0.0, heading = 0.0, length = 0.0, width = 0.0;
        double outlineLength = 0.0, outlineWidth = 0.0, footLength = 0.0, footWidth = 0.0, ridge = 0.0;
        int ground = 0;
        size_t vertices = 0;
        fields >> id >> name >> x >> y >> z >> heading >> length >> width >> ground >> vertices
            >> outlineLength >> outlineWidth >> footLength >> footWidth >> ridge >> covers;
        const Model* model = Models::byName(name);
        check(model != nullptr, id + ": there is a model " + name);
        if (model == nullptr)
        {
            continue;
        }
        tested.insert(name);

        const auto [modelLength, modelWidth] = model->footprint(outlineLength, outlineWidth);
        check(near(modelLength, footLength, 1.0e-9) && near(modelWidth, footWidth, 1.0e-9), id + ": the same footprint");
        check(near(model->ridgeHeight(width), ridge, 1.0e-9), id + ": the same height of the ridge");
        std::string walls;
        for (int i = -8; i <= 8; ++i)
        {
            for (int j = -8; j <= 8; ++j)
            {
                walls += model->covers(length, width, (double) i, (double) j, 0.4) ? '1' : '0';
            }
        }
        check(walls == covers, id + ": the same walls to stop at");

        const double azimuth = heading * pi / 180.0;
        const WavyGround wavy(y);
        Site site;
        site.m_floor = Vec3(x, y, z);
        site.m_axisLength = Vec3(std::sin(azimuth), 0.0, -std::cos(azimuth));
        site.m_axisWidth = Vec3(std::cos(azimuth), 0.0, std::sin(azimuth));
        site.m_length = length;
        site.m_width = width;
        site.m_ground = ground != 0 ? &wavy : nullptr;
        Mesh mesh;
        model->build(mesh, site);
        const std::vector<float> golden = readFloats(dir + "/" + id + ".f32");
        const bool sameCount = mesh.vertexCount() == vertices && mesh.floatCount() == golden.size();
        check(sameCount, id + ": " + std::to_string(vertices / 3) + " triangles");
        if (sameCount)
        {
            double worst = 0.0;
            for (size_t k = 0; k < golden.size(); ++k)
            {
                worst = std::max(worst, std::fabs((double) mesh.data()[k] - (double) golden[k]));
            }
            check(worst <= tolerance, id + ": every vertex and colour as in the first application");
        }
    }

    for (const Model* model : Models::all())
    {
        check(tested.count(model->name()) == 1, std::string(model->name()) + " has golden files");
        check(Models::byName(model->name()) == model, std::string(model->name()) + " is found by its name");
    }
    check(Models::byName("NoSuchModel") == nullptr, "an unknown name gives no model");

    std::cout << (failures == 0 ? "All tests passed" : "Tests failed: " + std::to_string(failures)) << std::endl;
    return failures == 0 ? 0 : 1;
}
