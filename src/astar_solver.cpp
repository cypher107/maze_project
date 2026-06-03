#include "astar_solver.h"
#include <queue>
#include <climits>
#include <algorithm>
#include <cstdlib>

static const int DX[] = {-1, 1, 0, 0};
static const int DY[] = {0, 0, -1, 1};

inline int manhattan(int x1, int y1, int x2, int y2) {
    return std::abs(x1 - x2) + std::abs(y1 - y2);
}

struct AStarState {
    int f;     // f = g + h
    int g;
    int x, y;
    bool operator>(const AStarState& o) const { return f > o.f; }
};

Path reconstructAStar(const std::vector<std::vector<Point>>& parent,
                      const Point& start, const Point& goal,
                      const Maze& maze) {
    Path path;
    Point cur = goal;
    std::vector<Point> reversed;
    while (cur.x != -1) {
        reversed.push_back(cur);
        if (cur == start) break;
        cur = parent[cur.x][cur.y];
    }
    std::reverse(reversed.begin(), reversed.end());
    for (const auto& p : reversed) {
        path.addPointWithCost(p, maze.getCost(p.x, p.y));
    }
    return path;
}

// Find the minimum Manhattan distance from (x,y) to any exit
static int minHeuristicToExits(const Maze& maze, int x, int y) {
    int best = INT_MAX;
    for (const auto& e : maze.getExits())
        best = std::min(best, manhattan(x, y, e.x, e.y));
    return best;
}

Path astarFindPath(const Maze& maze, int entryIndex, int exitIndex) {
    int rows = maze.getRows(), cols = maze.getCols();
    Point start = maze.getEntries()[entryIndex];
    Point goal  = maze.getExits()[exitIndex];

    std::vector<std::vector<int>> gScore(rows, std::vector<int>(cols, INT_MAX));
    std::vector<std::vector<Point>> parent(rows, std::vector<Point>(cols, Point(-1, -1)));

    std::priority_queue<AStarState, std::vector<AStarState>, std::greater<AStarState>> open;

    int h0 = manhattan(start.x, start.y, goal.x, goal.y);
    gScore[start.x][start.y] = 0;
    open.push({h0, 0, start.x, start.y});

    while (!open.empty()) {
        AStarState s = open.top(); open.pop();
        if (s.g > gScore[s.x][s.y]) continue;
        if (s.x == goal.x && s.y == goal.y) break;

        for (int k = 0; k < 4; k++) {
            int nx = s.x + DX[k], ny = s.y + DY[k];
            if (!maze.isWalkable(nx, ny)) continue;
            int ng = s.g + maze.getCost(nx, ny);
            if (ng < gScore[nx][ny]) {
                gScore[nx][ny] = ng;
                parent[nx][ny] = Point(s.x, s.y);
                int h = manhattan(nx, ny, goal.x, goal.y);
                open.push({ng + h, ng, nx, ny});
            }
        }
    }

    if (gScore[goal.x][goal.y] == INT_MAX) return Path();
    return reconstructAStar(parent, start, goal, maze);
}

Path astarMultiEntryExit(const Maze& maze) {
    int rows = maze.getRows(), cols = maze.getCols();

    std::vector<std::vector<int>> gScore(rows, std::vector<int>(cols, INT_MAX));
    std::vector<std::vector<Point>> parent(rows, std::vector<Point>(cols, Point(-1, -1)));

    std::priority_queue<AStarState, std::vector<AStarState>, std::greater<AStarState>> open;

    for (const auto& entry : maze.getEntries()) {
        int h = minHeuristicToExits(maze, entry.x, entry.y);
        gScore[entry.x][entry.y] = 0;
        open.push({h, 0, entry.x, entry.y});
    }

    Point goal(-1, -1);
    while (!open.empty()) {
        AStarState s = open.top(); open.pop();
        if (s.g > gScore[s.x][s.y]) continue;

        bool isExit = false;
        for (const auto& e : maze.getExits()) {
            if (s.x == e.x && s.y == e.y) { goal = e; isExit = true; break; }
        }
        if (isExit) break;

        for (int k = 0; k < 4; k++) {
            int nx = s.x + DX[k], ny = s.y + DY[k];
            if (!maze.isWalkable(nx, ny)) continue;
            int ng = s.g + maze.getCost(nx, ny);
            if (ng < gScore[nx][ny]) {
                gScore[nx][ny] = ng;
                parent[nx][ny] = Point(s.x, s.y);
                int h = minHeuristicToExits(maze, nx, ny);
                open.push({ng + h, ng, nx, ny});
            }
        }
    }

    if (goal.x == -1) return Path();

    Point start = goal;
    while (parent[start.x][start.y].x != -1)
        start = parent[start.x][start.y];
    return reconstructAStar(parent, start, goal, maze);
}
