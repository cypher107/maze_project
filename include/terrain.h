#ifndef TERRAIN_H
#define TERRAIN_H

#include <string>

enum class Terrain : int {
    Grass = 0,
    Swamp = 1,
    Water = 2,
    Wall  = 3
};

constexpr int TERRAIN_COUNT = 4;
constexpr int TERRAIN_COST[] = { 1, 3, 5, 999999 };

inline bool isWalkable(Terrain t) { return t != Terrain::Wall; }
inline int terrainCost(Terrain t) { return TERRAIN_COST[static_cast<int>(t)]; }

const char* terrainName(Terrain t);
unsigned terrainColorRGB(Terrain t);

#endif
