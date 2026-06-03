#include "evaluator.h"
#include <algorithm>
#include <iomanip>

EvalResult evaluatePath(const Maze& maze, int maxPaths) {
    EvalResult result;
    result.totalPaths = 0;
    result.shortestPath = Path();
    result.fewestTurnsPath = Path();

    int minSteps = INT_MAX;
    int minTurns = INT_MAX;

    dfsFindAllPathsCallback(maze, [&](const Path& p) {
        result.totalPaths++;

        if (p.steps < minSteps) {
            minSteps = p.steps;
            result.shortestPath = p;
        } else if (p.steps == minSteps && p.turns < result.shortestPath.turns) {
            result.shortestPath = p;
        }

        if (p.turns < minTurns) {
            minTurns = p.turns;
            result.fewestTurnsPath = p;
        } else if (p.turns == minTurns && p.steps < result.fewestTurnsPath.steps) {
            result.fewestTurnsPath = p;
        }

        if ((int)result.allPaths.size() < maxPaths) {
            result.allPaths.push_back(p);
        }
    }, 0, 0, maxPaths);

    return result;
}

void printPathDetails(const Path& path, const std::string& label) {
    if (path.points.empty()) {
        std::cout << "  " << label << ": 无路径" << std::endl;
        return;
    }

    std::cout << "\n  === " << label << " ===\n";
    std::cout << "  步数: " << path.steps << "  转弯次数: " << path.turns << "\n";
    std::cout << "  路径 (" << path.points.size() << " 个格子):\n   ";

    for (size_t i = 0; i < path.points.size(); i++) {
        if (i > 0) std::cout << " -> ";
        std::cout << "(" << path.points[i].x << "," << path.points[i].y << ")";
        if ((i + 1) % 5 == 0 && i + 1 < path.points.size())
            std::cout << "\n   ";
    }
    std::cout << std::endl;
}

void comparePaths(const Path& p1, const std::string& label1,
                  const Path& p2, const std::string& label2) {
    std::cout << "\n  ==== 路径对比 ====\n";
    std::cout << "  " << std::setw(20) << std::left << label1
              << std::setw(20) << std::left << label2 << "\n";
    std::cout << "  " << std::setw(20) << ("步数: " + std::to_string(p1.steps))
              << std::setw(20) << ("步数: " + std::to_string(p2.steps)) << "\n";
    std::cout << "  " << std::setw(20) << ("转弯: " + std::to_string(p1.turns))
              << std::setw(20) << ("转弯: " + std::to_string(p2.turns)) << "\n";
    std::cout << std::endl;
}
