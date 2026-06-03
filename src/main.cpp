#include "maze.h"
#include "solver.h"
#include "evaluator.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <climits>
#include <limits>
#include <windows.h>

#ifdef _WIN32
#define CLEAR_SCREEN "cls"
#else
#define CLEAR_SCREEN "clear"
#endif

void pauseScreen() {
    std::cout << "\n按 Enter 键继续...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

void showMenu() {
    std::cout << "\n";
    std::cout << "  ========================================\n";
    std::cout << "  |       迷 宫 求 解 系 统               |\n";
    std::cout << "  ========================================\n";
    std::cout << "  | 1. 从文件读取迷宫                    |\n";
    std::cout << "  | 2. 显示迷宫                          |\n";
    std::cout << "  | 3. DFS 回溯 — 查找所有路径           |\n";
    std::cout << "  | 4. BFS — 查找最短路径                |\n";
    std::cout << "  | 5. 路径评价 — 最短路径 & 最少转弯     |\n";
    std::cout << "  | 6. 查找最少转弯路径 (0-1 BFS)        |\n";
    std::cout << "  | 7. 修改入口/出口                     |\n";
    std::cout << "  | 0. 退出系统                          |\n";
    std::cout << "  ========================================\n";
    std::cout << "  请选择 (0-7): ";
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    Maze maze;
    bool mazeLoaded = false;

    while (true) {
        std::system(CLEAR_SCREEN);
        showMenu();

        int choice;
        std::cin >> choice;

        switch (choice) {
        case 0:
            std::cout << "\n  感谢使用迷宫求解系统，再见！\n" << std::endl;
            return 0;

        case 1: {
            std::string filename;
            std::cout << "\n  请输入迷宫文件名 (默认: mazes/maze1.txt): ";
            std::cin >> filename;
            if (filename.empty()) filename = "mazes/maze1.txt";
            mazeLoaded = maze.loadFromFile(filename);
            if (mazeLoaded) {
                maze.printMaze();
            }
            pauseScreen();
            break;
        }

        case 2:
            if (!mazeLoaded) {
                std::cout << "\n  请先加载迷宫文件！\n";
            } else {
                maze.printMaze();
            }
            pauseScreen();
            break;

        case 3: {
            if (!mazeLoaded) {
                std::cout << "\n  请先加载迷宫文件！\n";
                pauseScreen();
                break;
            }
            std::cout << "\n  正在使用 DFS 回溯查找所有路径...\n";

            std::vector<Path> allPaths = dfsFindAllPaths(maze);

            if (allPaths.empty()) {
                std::cout << "\n  该迷宫没有从入口到出口的通路！\n";
            } else {
                std::cout << "  共找到 " << allPaths.size() << " 条路径\n";
                int showCount = allPaths.size();
                if (showCount > 10) {
                    std::cout << "  路径过多，仅显示前10条，完整数据见上方统计。\n";
                    showCount = 10;
                }
                for (int i = 0; i < showCount; i++) {
                    printPathDetails(allPaths[i], "路径 " + std::to_string(i + 1));
                }

                // 找最短
                int minSteps = INT_MAX, idx = -1;
                for (int i = 0; i < (int)allPaths.size(); i++) {
                    if (allPaths[i].steps < minSteps) {
                        minSteps = allPaths[i].steps;
                        idx = i;
                    }
                }
                if (idx >= 0) {
                    std::cout << "\n  ★ 最短路径为 路径" << (idx + 1)
                              << " (步数=" << allPaths[idx].steps
                              << "，转弯=" << allPaths[idx].turns << ")\n";
                    maze.printMazeWithPath(allPaths[idx]);
                }
            }
            pauseScreen();
            break;
        }

        case 4: {
            if (!mazeLoaded) {
                std::cout << "\n  请先加载迷宫文件！\n";
                pauseScreen();
                break;
            }
            std::cout << "\n  正在使用 BFS 查找最短路径...\n";

            Path shortest = bfsFindShortestPath(maze);

            if (shortest.points.empty()) {
                std::cout << "\n  该迷宫没有从入口到出口的通路！\n";
            } else {
                printPathDetails(shortest, "BFS 最短路径");
                maze.printMazeWithPath(shortest);
            }
            pauseScreen();
            break;
        }

        case 5: {
            if (!mazeLoaded) {
                std::cout << "\n  请先加载迷宫文件！\n";
                pauseScreen();
                break;
            }
            std::cout << "\n  正在进行路径综合评价...\n";
            std::cout << "  (使用 DFS 遍历所有路径，分别比较步数和转弯数)\n";

            EvalResult result = evaluatePath(maze);

            if (result.totalPaths == 0) {
                std::cout << "\n  该迷宫没有从入口到出口的通路！\n";
            } else {
                std::cout << "\n  共找到 " << result.totalPaths << " 条路径\n";

                printPathDetails(result.shortestPath, "★ 最短路径（步数最少）");
                maze.printMazeWithPath(result.shortestPath);

                printPathDetails(result.fewestTurnsPath, "★ 最少转弯路径");
                maze.printMazeWithPath(result.fewestTurnsPath);

                comparePaths(result.shortestPath, "最短路径",
                             result.fewestTurnsPath, "最少转弯路径");
            }
            pauseScreen();
            break;
        }

        case 6: {
            if (!mazeLoaded) {
                std::cout << "\n  请先加载迷宫文件！\n";
                pauseScreen();
                break;
            }
            std::cout << "\n  正在使用 0-1 BFS 查找最少转弯路径...\n";

            Path fewestTurns = bfsFindFewestTurnsPath(maze);

            if (fewestTurns.points.empty()) {
                std::cout << "\n  该迷宫没有从入口到出口的通路！\n";
            } else {
                printPathDetails(fewestTurns, "0-1 BFS 最少转弯路径");
                maze.printMazeWithPath(fewestTurns);
            }
            pauseScreen();
            break;
        }

        case 7: {
            if (!mazeLoaded) {
                std::cout << "\n  请先加载迷宫文件！\n";
                pauseScreen();
                break;
            }
            int ex, ey;
            std::cout << "\n  当前入口: (" << maze.getEntry().x << ","
                      << maze.getEntry().y << ")\n";
            std::cout << "  当前出口: (" << maze.getExit().x << ","
                      << maze.getExit().y << ")\n";
            std::cout << "  请输入新入口坐标 (x y): ";
            std::cin >> ex >> ey;
            if (maze.isWalkable(ex, ey)) {
                maze.setEntry(Point(ex, ey));
                std::cout << "  入口已更新为 (" << ex << "," << ey << ")\n";
            } else {
                std::cout << "  坐标无效！该位置不是通路。\n";
            }
            std::cout << "  请输入新出口坐标 (x y): ";
            std::cin >> ex >> ey;
            if (maze.isWalkable(ex, ey)) {
                maze.setExit(Point(ex, ey));
                std::cout << "  出口已更新为 (" << ex << "," << ey << ")\n";
            } else {
                std::cout << "  坐标无效！该位置不是通路。\n";
            }
            pauseScreen();
            break;
        }

        default:
            std::cout << "\n  无效选择，请重试。\n";
            pauseScreen();
            break;
        }
    }
    return 0;
}
