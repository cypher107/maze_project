#ifndef CHECKPOINT_SOLVER_H
#define CHECKPOINT_SOLVER_H

#include "maze.h"

// Solve path visiting all checkpoints in stored order
Path solveWithCheckpoints(const Maze& maze, int entryIndex = 0, int exitIndex = 0);

// Find optimal checkpoint visit order (exhaustive for <=8, greedy for >8)
Path solveOptimalCheckpoints(const Maze& maze, int entryIndex = 0, int exitIndex = 0);

#endif
