#ifndef MAZE_GENERATOR_H
#define MAZE_GENERATOR_H

#include "maze.h"

// Generate random solvable maze using DFS backtracking
// rows/cols should be odd for proper wall carving
Maze generateMazeDFS(int rows, int cols, float wallDensity = 0.3f);

// Generate maze with weighted terrain distribution
// terrainProbs = {grass, swamp, water} weights (0-1 each)
Maze generateMazeWeighted(int rows, int cols,
                          float wallDensity = 0.3f,
                          float grassProb = 0.7f,
                          float swampProb = 0.2f,
                          float waterProb = 0.1f);

#endif
