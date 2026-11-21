// Slufter3D - C interface exported to JavaScript
// Author: Jan de Kort
// Created: 2026-11-16
#include "Game.h"

#include "Check.h"

#include <memory>
#include <string>
#include <vector>

#define SLUFTER_EXPORT(name) __attribute__((export_name(#name)))

namespace
{
    // Struct App: everything the page talks to
    struct App
    {
        std::vector<unsigned char> m_file;  // the world file, as the page handed it over
        std::unique_ptr<Game> m_game;
        Mat4 m_viewProjection;
        float m_eye[3] = { 0.0f, 0.0f, 0.0f };
        float m_horizon[3] = { 0.0f, 0.0f, 0.0f };
        float m_tiles[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        std::string m_hud;
    };

    App& app()
    {
        static App instance;
        return instance;
    }

    Game& game()
    {
        require(app().m_game != nullptr, "slufter_init has not been called");
        return *app().m_game;
    }
}

extern "C"
{

// Room for the world file (docs/data/world.bin); the page copies the file to the address returned
SLUFTER_EXPORT(slufter_alloc) unsigned char* slufter_alloc(int size)
{
    app().m_file.assign((size_t) (size > 0 ? size : 0), 0);
    return app().m_file.data();
}

// Reads the world file and builds the world and its mesh; the mesh does not change afterwards
SLUFTER_EXPORT(slufter_init) void slufter_init()
{
    App& a = app();
    a.m_game = std::make_unique<Game>(WorldData::parse(a.m_file.data(), a.m_file.size()));
    a.m_file.clear();
    a.m_file.shrink_to_fit();
}

SLUFTER_EXPORT(slufter_reset) void slufter_reset()
{
    game().reset();
}

// forward: +1->ahead, strafe: +1->right, turn: +1->clockwise, climb: +1->up
SLUFTER_EXPORT(slufter_set_controls) void slufter_set_controls(float forward, float strafe, float turn, float climb)
{
    Controls controls;
    controls.m_forward = forward;
    controls.m_strafe = strafe;
    controls.m_turn = turn;
    controls.m_climb = climb;
    game().setControls(controls);
}

// 0->5 km/h, 1->20 km/h, 2->50 km/h; other values are taken as the nearest of these
SLUFTER_EXPORT(slufter_set_pace) void slufter_set_pace(int pace)
{
    game().setPace(pace <= 0 ? Walker::Pace::Walk : (pace == 1 ? Walker::Pace::Cycle : Walker::Pace::Drive));
}

// Turns the head [rad]: turn clockwise, lift upwards
SLUFTER_EXPORT(slufter_look) void slufter_look(float turn, float lift)
{
    game().look(turn, lift);
}

// Higher (+) or lower (-) by a number of notches; the last notch down lands
SLUFTER_EXPORT(slufter_rise) void slufter_rise(float notches)
{
    game().rise(notches);
}

SLUFTER_EXPORT(slufter_land) void slufter_land()
{
    game().land();
}

// Puts the visitor in front of a house; returns 0 when the number does not exist
SLUFTER_EXPORT(slufter_goto) int slufter_goto(int houseNumber)
{
    return game().goTo(houseNumber) ? 1 : 0;
}

// Puts the visitor down at a spot given in metres east and north of the origin
SLUFTER_EXPORT(slufter_place) void slufter_place(float east, float north, float headingDegrees)
{
    game().place(east, north, headingDegrees);
}

// The park as it was in an era: 0->1957 to 1971, 1->1971 to 1993, 2->1993 to 2013, 3->2013 to today. The mesh
// changes: the page reads the vertices and their counts again
SLUFTER_EXPORT(slufter_set_era) void slufter_set_era(int era)
{
    game().setEra(era);
}

SLUFTER_EXPORT(slufter_era) int slufter_era()
{
    return game().getWorld().getEra();
}

SLUFTER_EXPORT(slufter_step) void slufter_step(float elapsed)
{
    game().step(elapsed);
}

SLUFTER_EXPORT(slufter_vertex_count) int slufter_vertex_count()
{
    return (int) game().getScene().getMesh().vertexCount();
}

// The first vertices are the sky, to be drawn around the eye
SLUFTER_EXPORT(slufter_sky_vertex_count) int slufter_sky_vertex_count()
{
    return (int) game().getScene().skyVertexCount();
}

// The last vertices lie flat on the ground (roads, water), to be drawn after the rest, nudged towards the eye
SLUFTER_EXPORT(slufter_flat_vertex_count) int slufter_flat_vertex_count()
{
    return (int) game().getScene().flatVertexCount();
}

// The land comes just after the sky, so that the page can draw its tiles on it
SLUFTER_EXPORT(slufter_ground_vertex_count) int slufter_ground_vertex_count()
{
    return (int) game().getScene().groundVertexCount();
}

// The trees come just before the flat vertices, so that the page can leave them out
SLUFTER_EXPORT(slufter_tree_vertex_count) int slufter_tree_vertex_count()
{
    return (int) game().getScene().treeVertexCount();
}

// Six floats per vertex: x, y, z, r, g, b
SLUFTER_EXPORT(slufter_vertices) const float* slufter_vertices()
{
    return game().getScene().getMesh().data();
}

// The lens: the angle the picture spans from top to bottom [deg]. 68 is the wide view the visit starts with;
// a 50 mm lens on 35 mm film sees 27, a 100 mm lens 14
SLUFTER_EXPORT(slufter_set_field_of_view) void slufter_set_field_of_view(float degrees)
{
    game().setFieldOfView(degrees);
}

SLUFTER_EXPORT(slufter_field_of_view) float slufter_field_of_view()
{
    return (float) game().getScene().getFieldOfView();
}

// Sixteen floats, column-major
SLUFTER_EXPORT(slufter_view_projection) const float* slufter_view_projection(float aspect)
{
    App& a = app();
    a.m_viewProjection = game().getScene().viewProjection(game().getWalker(), aspect);
    return a.m_viewProjection.m;
}

// Three floats
SLUFTER_EXPORT(slufter_eye) const float* slufter_eye()
{
    App& a = app();
    const Vec3 eye = game().getWalker().eye();
    a.m_eye[0] = (float) eye.x;
    a.m_eye[1] = (float) eye.y;
    a.m_eye[2] = (float) eye.z;
    return a.m_eye;
}

// Three floats: the colour of the sky at the horizon, which distance fades to
SLUFTER_EXPORT(slufter_horizon) const float* slufter_horizon()
{
    App& a = app();
    const Color& horizon = game().getScene().getHorizon();
    a.m_horizon[0] = horizon.r;
    a.m_horizon[1] = horizon.g;
    a.m_horizon[2] = horizon.b;
    return a.m_horizon;
}

// Six floats: the finest grid of the ground, its west, south, east and north in metres east and north of the
// origin and the size of its tiles [m], and how much lighter and darker the page draws these tiles in turn
SLUFTER_EXPORT(slufter_tiles) const float* slufter_tiles()
{
    App& a = app();
    const TerrainLevel& finest = game().getWorld().getTerrain().getLevel(0);
    a.m_tiles[0] = (float) finest.getWest();
    a.m_tiles[1] = (float) finest.getSouth();
    a.m_tiles[2] = (float) finest.getEast();
    a.m_tiles[3] = (float) finest.getNorth();
    a.m_tiles[4] = (float) finest.getCell();
    a.m_tiles[5] = (float) game().getScene().getTileContrast();
    return a.m_tiles;
}

// Distance over which the land fades into the horizon [m]
SLUFTER_EXPORT(slufter_fog_range) float slufter_fog_range()
{
    return (float) game().getScene().getFogRange();
}

// Zero terminated, fields separated by |; see Game::hudString
SLUFTER_EXPORT(slufter_hud) const char* slufter_hud()
{
    app().m_hud = game().hudString();
    return app().m_hud.c_str();
}

}
