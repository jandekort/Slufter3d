// Slufter3D - the world as data, read from docs/data/world.bin
// Author: Jan de Kort
// Created: 2026-11-16
#include "WorldData.h"

#include "Check.h"

#include <cstdint>
#include <cstring>

namespace
{
    const std::uint32_t magic = 0x33464c53;     // "SLF3"
    const std::uint32_t version = 4;
    const std::int16_t noWater = -32768;
    const std::int16_t seaMark = -32767;        // in place of a water level: the tile is sea
    const double centimetre = 0.01;

    // Class Reader: takes little-endian values off a block of bytes, one after the other
    class Reader
    {
    public:
        Reader(const unsigned char* bytes, size_t size)
            :
            m_bytes(bytes)
            , m_size(size)
            , m_at(0)
        {
        }

        template <typename T> T take()
        {
            require(m_at + sizeof(T) <= m_size, "World file ends too soon");
            T value;
            std::memcpy(&value, m_bytes + m_at, sizeof(T));
            m_at += sizeof(T);
            return value;
        }

        std::string text()
        {
            const size_t length = take<std::uint8_t>();
            require(m_at + length <= m_size, "World file ends too soon");
            const std::string value((const char*) m_bytes + m_at, length);
            m_at += length;
            return value;
        }

        // Accessor
        bool atEnd() const { return m_at == m_size; }

    private:
        const unsigned char* m_bytes;
        size_t m_size;
        size_t m_at;    // next byte to take
    };
}

// Struct WorldData
WorldData WorldData::parse(const unsigned char* bytes, size_t size)
{
    require(bytes != nullptr, "No world file");
    Reader in(bytes, size);
    require(in.take<std::uint32_t>() == magic, "Not a world file");
    require(in.take<std::uint32_t>() == version, "World file of another version; run tools/pack_world.py");
    WorldData data;
    data.m_originEast = in.take<double>();
    data.m_originNorth = in.take<double>();

    { // Ground
        const size_t count = in.take<std::uint32_t>();
        require(count >= 1 && count <= 8, "World file: number of grids");
        data.m_levels.resize(count);
        for (Level& level : data.m_levels)
        {
            level.m_west = in.take<double>();
            level.m_south = in.take<double>();
            level.m_cell = in.take<double>();
            level.m_nx = in.take<std::uint32_t>();
            level.m_nz = in.take<std::uint32_t>();
            require(level.m_nx > 1 && level.m_nz > 1 && level.m_cell > 0.0, "World file: empty grid");
            level.m_height.reserve(level.m_nx * level.m_nz);
            for (size_t k = 0; k < level.m_nx * level.m_nz; ++k)
            {
                level.m_height.push_back((float) (centimetre * (double) in.take<std::int16_t>()));
            }
            const size_t tiles = (level.m_nx - 1) * (level.m_nz - 1);
            level.m_water.reserve(tiles);
            for (size_t k = 0; k < tiles; ++k)
            {
                const std::int16_t water = in.take<std::int16_t>();
                level.m_water.push_back(water == noWater ? dry()
                    : water == seaMark ? sea() : (float) (centimetre * (double) water));
            }
        }
    }

    { // Houses
        const size_t count = in.take<std::uint32_t>();
        data.m_houses.resize(count);
        for (HouseRecord& house : data.m_houses)
        {
            house.m_number = in.take<std::int32_t>();
            house.m_semiWith = in.take<std::int32_t>();
            house.m_type = in.text();
            house.m_name = in.text();
            house.m_model = in.text();
            house.m_semi = in.take<std::uint8_t>() != 0;
            house.m_pitch = in.take<std::uint8_t>() != 0;
            house.m_outline = in.take<std::uint8_t>() != 0;
            house.m_eras = in.take<std::uint8_t>();
            require(house.m_eras != 0 && house.m_eras < (1u << eraCount()), "World file: eras of a house");
            house.m_east = in.take<float>();
            house.m_north = in.take<float>();
            house.m_length = in.take<float>();
            house.m_width = in.take<float>();
            house.m_axis = in.take<float>();
            house.m_front = in.take<float>();
            house.m_floor = in.take<float>();
        }
    }

    { // Central building and landmarks
        const size_t count = in.take<std::uint32_t>();
        data.m_wings.resize(count);
        for (WingRecord& wing : data.m_wings)
        {
            wing.m_east = in.take<float>();
            wing.m_north = in.take<float>();
            wing.m_length = in.take<float>();
            wing.m_width = in.take<float>();
            wing.m_axis = in.take<float>();
        }
        data.m_entranceEast = in.take<float>();
        data.m_entranceNorth = in.take<float>();
        data.m_entranceAzimuth = in.take<float>();
        data.m_towerEast = in.take<float>();
        data.m_towerNorth = in.take<float>();
        data.m_startEast = in.take<float>();
        data.m_startNorth = in.take<float>();
        data.m_startHeading = in.take<float>();
        data.m_lighthouseEast = in.take<float>();
        data.m_lighthouseNorth = in.take<float>();
        data.m_lighthouseHeight = in.take<float>();
    }

    { // Roads
        const size_t count = in.take<std::uint32_t>();
        data.m_roadCorners.reserve(6 * count);
        for (size_t k = 0; k < 6 * count; ++k)
        {
            data.m_roadCorners.push_back(in.take<float>());
        }
        data.m_roadKinds.reserve(count);
        for (size_t k = 0; k < count; ++k)
        {
            const unsigned char kind = in.take<std::uint8_t>();
            require(kind < roadKinds(), "World file: kind of road");
            data.m_roadKinds.push_back(kind);
        }
    }
    { // Trees
        const size_t count = in.take<std::uint32_t>();
        data.m_trees.reserve(count);
        for (size_t k = 0; k < count; ++k)
        {
            TreeRecord tree;
            tree.m_east = in.take<float>();
            tree.m_north = in.take<float>();
            tree.m_height = in.take<float>();
            tree.m_crownRadius = in.take<float>();
            require(tree.m_height > 0.0 && tree.m_crownRadius > 0.0, "World file: size of a tree");
            data.m_trees.push_back(tree);
        }
    }
    require(in.atEnd(), "World file is longer than expected");
    return data;
}
