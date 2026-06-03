#ifndef DIJKSTRA_SOLVER_H
#define DIJKSTRA_SOLVER_H

#include "maze.h"

// Find minimum-cost path using Dijkstra (weighted terrain)
Path dijkstraFindPath(const Maze& maze, int entryIndex = 0, int exitIndex = 0);

// Find shortest path from any entry to any exit
Path dijkstraMultiEntryExit(const Maze& maze);

// Single-source Dijkstra returning distance grid for segment solving
std::vector<std::vector<int>> dijkstraDistances(const Maze& maze, const Point& start);

#endif
