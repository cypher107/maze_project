#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "maze.h"
#include "solver.h"
#include <vector>

// 路径评价结果
struct EvalResult {
    Path shortestPath;       // 步数最短的路径
    Path fewestTurnsPath;    // 转弯最少的路径
    int totalPaths;          // 路径总数
    std::vector<Path> allPaths; // 所有路径（用于展示）
};

// 综合评价：同时计算最短路径和最少转弯路径
EvalResult evaluatePath(const Maze& maze, int maxPaths = 100);

// 打印路径详情
void printPathDetails(const Path& path, const std::string& label);

// 比较两条路径
void comparePaths(const Path& p1, const std::string& label1,
                  const Path& p2, const std::string& label2);

#endif
