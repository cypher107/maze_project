#include "terrain.h"

const char* terrainName(Terrain t) {
    switch (t) {
        case Terrain::Grass: return "草地";
        case Terrain::Swamp: return "沼泽";
        case Terrain::Water: return "水面";
        case Terrain::Wall:  return "墙壁";
    }
    return "未知";
}

unsigned terrainColorRGB(Terrain t) {
    switch (t) {
        case Terrain::Grass: return 0x90EE90;
        case Terrain::Swamp: return 0x8B5A2B;
        case Terrain::Water: return 0x4682B4;
        case Terrain::Wall:  return 0x323237;
    }
    return 0x000000;
}
