#ifndef SOLVER_H
#define SOLVER_H

#include "maze.h"
#include <vector>
#include <functional>

// DFS 回溯 — 找到所有从入口到出口的路径
// maxPaths: 最多找到的路径数；maxStates: 最多探索的状态数（防止大迷宫卡死）
std::vector<Path> dfsFindAllPaths(const Maze& maze,
                                   int entryIndex = 0, int exitIndex = 0,
                                   int maxPaths = 50, int maxStates = 500000);

// BFS — 找到最短路径（步数最少）
Path bfsFindShortestPath(const Maze& maze,
                          int entryIndex = 0, int exitIndex = 0);

// BFS 变体 — 找到转弯最少的路径
Path bfsFindFewestTurnsPath(const Maze& maze,
                             int entryIndex = 0, int exitIndex = 0);

// 回调版本的 DFS，每找到一条路径就调用一次回调（避免内存爆炸）
// 返回实际找到的路径数；达到 maxPaths 或 maxStates 时提前终止
int dfsFindAllPathsCallback(const Maze& maze,
                             std::function<void(const Path&)> onPathFound,
                             int entryIndex = 0, int exitIndex = 0,
                             int maxPaths = 50, int maxStates = 500000);

#endif
