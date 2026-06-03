#ifndef ASTAR_SOLVER_H
#define ASTAR_SOLVER_H

#include "maze.h"

// A* search with Manhattan heuristic
// Heuristic is admissible: h(n) = manhattan(n, goal) * 1 <= true cost
Path astarFindPath(const Maze& maze, int entryIndex = 0, int exitIndex = 0);

Path astarMultiEntryExit(const Maze& maze);

#endif
