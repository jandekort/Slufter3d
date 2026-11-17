// Slufter3D - native tests of the world, the visitor and the picture, on the packed world file
// Author: Jan de Kort
// Created: 2026-11-16
#include "Game.h"
#include "Model.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
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

    // Lets time pass with the controls as they are; returns the distance covered over the ground
    double run(Game& game, double seconds)
    {
        const double dt = 1.0 / 60.0;
        const double east = game.getWalker().getEast();
        const double north = game.getWalker().getNorth();
        for (int k = 0; k < (int) std::lround(seconds / dt); ++k)
        {
            game.step(dt);
        }
        return std::hypot(game.getWalker().getEast() - east, game.getWalker().getNorth() - north);
    }

    Controls ahead()
    {
        Controls controls;
        controls.m_forward = 1.0;
        return controls;
    }

    // Colour of the first triangle after the sky that lies over a point, blended from its corners as the page
    // does: the ground there, as it is lit
    Color groundColourAt(const Scene& scene, double east, double north)
    {
        const Mesh& mesh = scene.getMesh();
        const float* v = mesh.data();
        for (size_t t = scene.skyVertexCount() / 3; t < mesh.vertexCount() / 3; ++t)
        {
            const float* p = v + 18 * t;
            const double ax = p[0], an = -p[2], bx = p[6], bn = -p[8], cx = p[12], cn = -p[14];
            const double d = (bn - cn) * (ax - cx) + (cx - bx) * (an - cn);
            if (std::fabs(d) < 1.0e-12)
            {
                continue;
            }
            const double wa = ((bn - cn) * (east - cx) + (cx - bx) * (north - cn)) / d;
            const double wb = ((cn - an) * (east - cx) + (ax - cx) * (north - cn)) / d;
            if (wa >= 0.0 && wb >= 0.0 && wa + wb <= 1.0)
            {
                const double wc = 1.0 - wa - wb;
                return Color((float) (wa * p[3] + wb * p[9] + wc * p[15]), (float) (wa * p[4] + wb * p[10] + wc * p[16]),
                    (float) (wa * p[5] + wb * p[11] + wc * p[17]));
            }
        }
        return Color(0.0f, 0.0f, 0.0f);
    }
}

int main(int argc, char** argv)
{
    const std::string path = argc > 1 ? argv[1] : "docs/data/world.bin";
    std::ifstream in(path, std::ios::binary);
    const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    check(!bytes.empty(), "the world file opens: " + path);
    if (bytes.empty())
    {
        return 1;
    }
    const WorldData data = WorldData::parse(bytes.data(), bytes.size());
    Game game(data);
    const World& world = game.getWorld();
    const Terrain& terrain = world.getTerrain();
    const double pi = 3.14159265358979323846;

    { // The ground
        check(terrain.levelCount() == 3, "three grids of ground");
        const TerrainLevel& park = terrain.getLevel(0);
        check(park.getCell() == 3.0, "the park in tiles of 3 m");
        double low = 1.0e9;
        double high = -1.0e9;
        for (size_t j = 0; j < park.getNz(); ++j)
        {
            for (size_t i = 0; i < park.getNx(); ++i)
            {
                low = std::min(low, park.nodeHeight(i, j));
                high = std::max(high, park.nodeHeight(i, j));
            }
        }
        check(low > -2.0 && high > 10.0 && high < 25.0, "measured heights, nothing scaled: " + std::to_string(low) + " to "
            + std::to_string(high) + " m NAP");
        // Along the edge of every grid the next, coarser one is equally high: no cracks
        for (size_t k = 0; k + 1 < terrain.levelCount(); ++k)
        {
            const TerrainLevel& fine = terrain.getLevel(k);
            const TerrainLevel& coarse = terrain.getLevel(k + 1);
            double worst = 0.0;
            for (int s = 0; s <= 400; ++s)
            {
                const double t = (double) s / 400.0;
                const double east = mix(fine.getWest(), fine.getEast(), t);
                const double north = mix(fine.getSouth(), fine.getNorth(), t);
                for (const double e : { fine.getWest(), fine.getEast() })
                {
                    worst = std::max(worst, std::fabs(fine.height(e, north) - coarse.height(e, north)));
                }
                for (const double n : { fine.getSouth(), fine.getNorth() })
                {
                    worst = std::max(worst, std::fabs(fine.height(east, n) - coarse.height(east, n)));
                }
            }
            check(worst < 1.0e-6, "grid " + std::to_string(k) + " meets the next without a crack: " + std::to_string(worst) + " m");
            check(coarse.contains(fine.getWest(), fine.getSouth()) && coarse.contains(fine.getEast(), fine.getNorth()),
                "grid " + std::to_string(k) + " lies within the next");
        }
        const TerrainLevel& horizon = terrain.getLevel(2);
        check(terrain.seaAt(horizon.getWest() + 500.0, 0.0) && !terrain.seaAt(0.0, 0.0), "sea in the west, land in the park");
        check(terrain.height(-1.0e6, 0.0) < 0.0, "beyond the grids there is only sea");
        check(!terrain.seaAt(397.5, 130.5) && terrain.height(397.5, 130.5) < -0.1,
            "the shore of the pond in the polder lies below 0 m NAP, and is no sea");
        check(!terrain.seaAt(499.5, 49.5), "the polder in the south-east is no sea");
    }

    { // Houses
        const std::vector<House>& houses = world.getHouses();
        check(houses.size() == 395, "395 houses and pitches");
        check(std::count_if(houses.begin(), houses.end(), [](const House& h) { return h.m_pitch; }) == 60, "60 of them pitches");
        double worst = 0.0;
        for (const House& house : houses)
        {
            worst = std::max(worst, std::fabs(house.m_site.m_floor.y - terrain.height(house.m_east, house.m_north)));
        }
        check(worst < 2.0, "every floor near the ground it stands on: at most " + std::to_string(worst) + " m");
        const House* h449 = world.findHouse(449);
        check(h449 != nullptr && h449->m_type == "8L" && std::string(h449->m_model->name()) == "Luxe8L", "449 is an 8L, drawn as Luxe8L");
        const House* h26 = world.findHouse(26);
        check(h26 != nullptr && h26->m_type == "6CK" && std::string(h26->m_model->name()) == "Comfort8C", "26 is a 6CK, drawn as Comfort8C");
        const House* h14 = world.findHouse(14);
        const House* h15 = world.findHouse(15);
        check(h14 != nullptr && h15 != nullptr && h14->m_semi && h15->m_semi
            && std::fabs(std::hypot(h14->m_east - h15->m_east, h14->m_north - h15->m_north) - h14->m_site.m_length) < 1.2,
            "14 and 15 are the two halves of one block, side by side");
        check(std::any_of(houses.begin(), houses.end(), [](const House& h)
            {
                return h.m_name == "Visdiefje" && std::fabs(h.m_site.m_length - 5.8) < 0.01 && std::fabs(h.m_site.m_width - 5.2) < 0.01;
            }), "the Visdiefje is there, with walls of 5.8 m by 5.2 m");
        check(std::count_if(houses.begin(), houses.end(), [](const House& h)
            {
                return h.m_type == "Visdiefje" && std::string(h.m_model->name()) == "Visdiefje";
            }) == 2, "two bungalows of the Visdiefje type, both drawn as Visdiefje");
        check(std::any_of(houses.begin(), houses.end(), [](const House& h)
            {
                return h.m_type == "Visdiefje" && std::hypot(h.m_east - 73.8, h.m_north - 10.5) < 3.0;   // RD 118774, 574711
            }), "the second Visdiefje stands at RD 118774, 574711");
        check(std::none_of(houses.begin(), houses.end(), [](const House& h)
            {
                return std::hypot(h.m_east - 40.0, h.m_north - 41.3) < 3.0;   // RD 118740, 574741
            }), "the bicycle rental is left out");
        check(std::none_of(houses.begin(), houses.end(), [](const House& h)
            {
                return std::hypot(h.m_east - 41.9, h.m_north - 17.7) < 3.0;   // RD 118742, 574718
            }), "the building once taken for a second Visdiefje is left out");
        check(world.findHouse(9999) == nullptr, "no house 9999");
        // The front of a house is free of its own walls
        const House* h17 = world.findHouse(17);
        check(h17 != nullptr && world.insideHouse(*h17, h17->m_east, h17->m_north, 0.0)
            && !world.insideHouse(*h17, h17->m_east + 8.0 * h17->m_site.m_axisWidth.x, h17->m_north - 8.0 * h17->m_site.m_axisWidth.z, 0.0),
            "a house holds its centre and not the spot 8 m in front of it");
    }

    { // Roads and water
        check(data.m_roadKinds.size() > 10000, "roads of the BGT: " + std::to_string(data.m_roadKinds.size()) + " triangles");
        bool found[4] = { false, false, false, false };
        bool right = true;
        for (size_t k = 0; k < data.m_roadKinds.size(); k += 97)
        {
            const float* t = data.m_roadCorners.data() + 6*k;
            const World::Surface surface = world.surfaceAt((t[0] + t[2] + t[4]) / 3.0, (t[1] + t[3] + t[5]) / 3.0);
            const World::Surface expected[] = { World::Surface::Asphalt, World::Surface::Bricks, World::Surface::Shells,
                World::Surface::Sand };
            found[data.m_roadKinds[k]] = true;
            right = right && (surface == expected[data.m_roadKinds[k]] || surface == World::Surface::Water);
        }
        check(right, "the surface underfoot is that of the road triangle there");
        check(found[0] && found[1] && found[2] && found[3], "asphalt, bricks, shells and sand all occur");
        // The pond in the middle of the park, RD 118927, 575100
        const double pondEast = 118927.0 - data.m_originEast;
        const double pondNorth = 575100.0 - data.m_originNorth;
        check(terrain.waterAt(pondEast, pondNorth) && !world.standable(pondEast, pondNorth) && world.blocked(pondEast, pondNorth),
            "the pond in the park is water, and nobody stands in it");
        check(world.surfaceAt(pondEast, pondNorth) == World::Surface::Water, "its surface is water");
    }

    { // Walking
        game.reset();
        const Walker& walker = game.getWalker();
        check(!world.blocked(walker.getEast(), walker.getNorth()), "the start is free ground");
        check(walker.getPace() == Walker::Pace::Cycle && !walker.isFlying(), "the visit starts on foot at 20 km/h");
        game.setPace(Walker::Pace::Walk);
        const House* h17 = world.findHouse(17);
        check(game.goTo(17), "goes to house 17");
        const double toHouse = std::atan2(h17->m_east - walker.getEast(), h17->m_north - walker.getNorth());
        check(std::fabs(std::remainder(toHouse - walker.getHeading(), 2.0 * pi)) < 0.3, "and faces it");
        game.setControls(ahead());
        run(game, 60.0);
        check(!world.insideHouse(*h17, walker.getEast(), walker.getNorth(), 0.0)
            && std::hypot(h17->m_east - walker.getEast(), h17->m_north - walker.getNorth()) < 12.0,
            "walking on, its wall stops the visitor");
        check(std::fabs(walker.eye().y - terrain.height(walker.getEast(), walker.getNorth()) - walker.getEyeHeight()) < 0.05,
            "the eyes are 1.7 m above the ground");
        game.setControls(Controls());
    }

    { // Flying
        game.reset();
        const Walker& walker = game.getWalker();
        game.rise(1.0);
        run(game, 4.0);
        check(walker.isFlying() && std::fabs(walker.getAltitude() - 3.0) < 0.1, "one notch up: 3 m above the ground");
        game.rise(12.0);
        run(game, 6.0);
        const double high = walker.getAltitude();
        check(high > 50.0 && high < 100.0, "twelve more: " + std::to_string(high) + " m");
        check(walker.flightSpeed() > 2.0 * walker.paceSpeed(), "the higher, the faster");
        // Straight over house 17, which stopped the visitor on the ground
        const House* h17 = world.findHouse(17);
        game.place(h17->m_east - 40.0 * h17->m_site.m_axisWidth.x, h17->m_north + 40.0 * h17->m_site.m_axisWidth.z,
            std::atan2(h17->m_site.m_axisWidth.x, -h17->m_site.m_axisWidth.z) * 180.0 / pi);
        game.rise(8.0);
        run(game, 5.0);
        game.setControls(ahead());
        double nearest = 1.0e9;
        for (int k = 0; k < 1200; ++k)
        {
            game.step(1.0 / 60.0);
            nearest = std::min(nearest, std::hypot(h17->m_east - walker.getEast(), h17->m_north - walker.getNorth()));
        }
        check(nearest < 2.0, "above the roofs the visitor flies straight over a house");
        // Coming down over a house, the visitor stays above its roof
        game.setControls(Controls());
        game.place(h17->m_east, h17->m_north, 0.0);
        game.rise(8.0);
        run(game, 4.0);
        game.land();
        run(game, 8.0);
        check(std::fabs(walker.getAltitude() - world.getClearance()) < 0.2, "landing on a house ends above its roof");
        // And over the pond above the water
        game.place(118927.0 - data.m_originEast, 575100.0 - data.m_originNorth, 0.0);
        game.rise(5.0);
        run(game, 3.0);
        game.land();
        run(game, 8.0);
        check(walker.isFlying() && walker.getAltitude() < 3.0, "landing on the pond ends just above the water");
        game.reset();
        game.rise(6.0);
        run(game, 3.0);
        game.land();
        run(game, 8.0);
        check(!walker.isFlying() && walker.getAltitude() == 0.0, "landing on free ground ends on the ground");
        // Far out at sea the world ends
        game.rise(40.0);
        game.setControls(ahead());
        run(game, 600.0);
        check(terrain.contains(walker.getEast(), walker.getNorth()), "flying on and on, the visitor stays over the world");
        check(walker.eye().y > terrain.height(walker.getEast(), walker.getNorth()), "and above its ground");
        game.setControls(Controls());
        game.reset();
    }

    { // The picture
        const Scene& scene = game.getScene();
        const Mesh& mesh = scene.getMesh();
        const float* v = mesh.data();
        check(mesh.vertexCount() > 500000, "the mesh holds the world: " + std::to_string(mesh.vertexCount()) + " vertices");
        check(scene.skyVertexCount() > 0 && scene.flatVertexCount() > 0
            && scene.skyVertexCount() + scene.flatVertexCount() < mesh.vertexCount(), "sky first, roads and water last");
        check(std::all_of(v, v + mesh.floatCount(), [](float x) { return std::isfinite(x); }), "all vertices are finite");
        const size_t trees = world.getData().m_trees.size();
        check(trees > 1000, "the trees of the AHN are in the world: " + std::to_string(trees));
        check(scene.treeVertexCount() >= 54 * trees && scene.treeVertexCount() % 3 == 0
            && scene.skyVertexCount() + scene.treeVertexCount() + scene.flatVertexCount() < mesh.vertexCount(),
            "the trees are drawn, just before the roads and the water: " + std::to_string(scene.treeVertexCount())
            + " vertices");
        { // The land comes right after the sky and holds no sea, which follows it
            const size_t from = scene.skyVertexCount();
            const size_t to = from + scene.groundVertexCount();
            const auto colour = [&](size_t k) { return Color(v[6*k + 3], v[6*k + 4], v[6*k + 5]); };
            const Color sea = colour(to);
            bool seaInLand = false;
            for (size_t k = from; k < to; ++k)
            {
                const Color c = colour(k);
                seaInLand = seaInLand || (c.r == sea.r && c.g == sea.g && c.b == sea.b);
            }
            check(scene.groundVertexCount() > 100000 && scene.groundVertexCount() % 3 == 0
                && to + scene.treeVertexCount() + scene.flatVertexCount() < mesh.vertexCount() && !seaInLand
                && v[6*to + 1] == 0.0f && v[6*(to + 1) + 1] == 0.0f && v[6*(to + 2) + 1] == 0.0f,
                "the land right after the sky, the sea after it: " + std::to_string(scene.groundVertexCount()) + " vertices");
            check(scene.getTileContrast() > 0.0 && scene.getTileContrast() < 0.1, "the page draws faint tiles on the land");
        }
        // The lighthouse stands on its dune, its top near 54 m NAP
        double top = 0.0;
        for (size_t k = scene.skyVertexCount(); k < mesh.vertexCount(); ++k)
        {
            if (std::hypot(v[6*k] - data.m_lighthouseEast, -v[6*k + 2] - data.m_lighthouseNorth) < 10.0)
            {
                top = std::max(top, (double) v[6*k + 1]);
            }
        }
        check(top > 50.0 && top < 60.0, "the lighthouse reaches " + std::to_string(top) + " m NAP");
        // Only the sea starts a beach: the polder below 0 m NAP is meadow, the beach by the lighthouse sand
        const Color polder = groundColourAt(scene, 499.5, 49.5);
        check(polder.g > polder.r, "the polder in the south-east is drawn green, not as beach");
        const Color sand = groundColourAt(scene, 666.0, 2814.0);
        check(sand.r > sand.g, "the beach by the lighthouse is drawn as sand");
        // The ground shows no tiles: on either side of the edge between two tiles of the park it looks the same
        const TerrainLevel& park = game.getWorld().getTerrain().getLevel(0);
        double jump = 0.0;
        for (size_t k = 1; k < 40; ++k)
        {
            const double edge = park.getWest() + (double) (7 * k) * park.getCell();
            const double north = park.getSouth() + (7.5 * (double) k + 0.5) * park.getCell();
            const Color west = groundColourAt(scene, edge - 0.01, north);
            const Color east = groundColourAt(scene, edge + 0.01, north);
            jump = std::max(jump, (double) std::max(std::fabs(west.r - east.r), std::max(std::fabs(west.g - east.g), std::fabs(west.b - east.b))));
        }
        check(jump < 0.01, "the ground runs on across the edges of the tiles, at most " + std::to_string(jump) + " apart");
        const Mat4 matrix = scene.viewProjection(game.getWalker(), 1.6);
        check(std::all_of(matrix.m, matrix.m + 16, [](float x) { return std::isfinite(x); }), "the view is finite");
        const std::string hud = game.hudString();
        check(std::count(hud.begin(), hud.end(), '|') == 10, "the HUD has eleven fields: " + hud);
    }

    std::cout << (failures == 0 ? "All tests passed" : "Tests failed: " + std::to_string(failures)) << std::endl;
    return failures == 0 ? 0 : 1;
}
