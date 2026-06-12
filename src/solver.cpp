#include "solver.h"
#include <queue>
#include <stack>
#include <algorithm>
#include <climits>
#include <unordered_set>

static const int DX[] = {-1, 1, 0, 0};
static const int DY[] = {0, 0, -1, 1};

// ============ DFS 回溯 — 非递归实现，防止栈溢出 ============

int dfsFindAllPathsCallback(const Maze& maze,
                             std::function<void(const Path&)> onPathFound,
                             int entryIndex, int exitIndex,
                             int maxPaths, int maxStates) {
    const auto& entries = maze.getEntries();
    const auto& exits = maze.getExits();
    if (entries.empty() || exits.empty()) return 0;

    entryIndex = std::min(entryIndex, (int)entries.size() - 1);
    Point start = entries[entryIndex];

    // Build exit set for fast lookup
    std::unordered_set<int> exitSet;
    for (const auto& e : exits) {
        exitSet.insert(e.x * maze.getCols() + e.y);
    }

    int pathCount = 0;
    int statesVisited = 0;
    int rows = maze.getRows(), cols = maze.getCols();

    // Iterative DFS state: (x, y, direction_index)
    struct State {
        int x, y, dir;
    };

    std::vector<std::vector<bool>> visited(rows, std::vector<bool>(cols, false));
    std::vector<State> stk;
    Path current;

    stk.push_back({start.x, start.y, 0});
    visited[start.x][start.y] = true;
    current.points.push_back(start);
    statesVisited++;

    while (!stk.empty() && pathCount < maxPaths && statesVisited < maxStates) {
        State& top = stk.back();
        int x = top.x, y = top.y;

        // Check if we reached any exit
        if (exitSet.count(x * cols + y)) {
            Path found;
            for (const auto& p : current.points)
                found.points.push_back(p);
            found.steps = (int)found.points.size() - 1;
            found.calculateTurns();
            onPathFound(found);
            pathCount++;

            // Backtrack — don't expand further from exit
            visited[x][y] = false;
            current.points.pop_back();
            stk.pop_back();
            continue;
        }

        // Try next direction
        bool moved = false;
        while (top.dir < 4 && !moved && statesVisited < maxStates) {
            int nx = x + DX[top.dir];
            int ny = y + DY[top.dir];
            top.dir++;
            if (maze.isWalkable(nx, ny) && !visited[nx][ny]) {
                visited[nx][ny] = true;
                current.points.push_back(Point(nx, ny));
                stk.push_back({nx, ny, 0});
                statesVisited++;
                moved = true;
            }
        }

        if (!moved) {
            // Dead end — backtrack
            visited[x][y] = false;
            current.points.pop_back();
            stk.pop_back();
        }
    }

    return pathCount;
}

std::vector<Path> dfsFindAllPaths(const Maze& maze,
                                   int entryIndex, int exitIndex,
                                   int maxPaths, int maxStates) {
    std::vector<Path> allPaths;
    allPaths.reserve(maxPaths);
    dfsFindAllPathsCallback(maze, [&](const Path& p) {
        allPaths.push_back(p);
    }, entryIndex, exitIndex, maxPaths, maxStates);
    return allPaths;
}

// ============ BFS — 最短路径（支持多出口）============

Path bfsFindShortestPath(const Maze& maze, int entryIndex, int exitIndex) {
    int rows = maze.getRows();
    int cols = maze.getCols();
    const auto& entries = maze.getEntries();
    const auto& exits = maze.getExits();
    if (entries.empty() || exits.empty()) return Path();

    entryIndex = std::min(entryIndex, (int)entries.size() - 1);
    exitIndex = std::min(exitIndex, (int)exits.size() - 1);

    Point entry = entries[entryIndex];
    Point target = exits[exitIndex];

    std::vector<std::vector<bool>> visited(rows, std::vector<bool>(cols, false));
    std::vector<std::vector<Point>> parent(rows, std::vector<Point>(cols, Point(-1, -1)));

    std::queue<Point> q;
    q.push(entry);
    visited[entry.x][entry.y] = true;

    bool found = false;
    while (!q.empty() && !found) {
        Point cur = q.front();
        q.pop();

        for (int d = 0; d < 4; d++) {
            int nx = cur.x + DX[d];
            int ny = cur.y + DY[d];
            if (maze.isWalkable(nx, ny) && !visited[nx][ny]) {
                visited[nx][ny] = true;
                parent[nx][ny] = cur;
                q.push(Point(nx, ny));
                if (nx == target.x && ny == target.y) {
                    found = true;
                    break;
                }
            }
        }
    }

    Path path;
    if (!found) return path;

    Point cur = target;
    std::vector<Point> reversed;
    while (true) {
        reversed.push_back(cur);
        if (cur == entry) break;
        cur = parent[cur.x][cur.y];
    }
    std::reverse(reversed.begin(), reversed.end());
    for (const auto& p : reversed) {
        path.points.push_back(p);
    }
    path.steps = (int)path.points.size() - 1;
    path.calculateTurns();
    return path;
}

// ============ 0-1 BFS — 最少转弯路径 ============

Path bfsFindFewestTurnsPath(const Maze& maze, int entryIndex, int exitIndex) {
    int rows = maze.getRows();
    int cols = maze.getCols();
    const auto& entries = maze.getEntries();
    const auto& exits = maze.getExits();
    if (entries.empty() || exits.empty()) return Path();

    entryIndex = std::min(entryIndex, (int)entries.size() - 1);
    exitIndex = std::min(exitIndex, (int)exits.size() - 1);

    Point entry = entries[entryIndex];
    Point target = exits[exitIndex];

    std::vector<std::vector<int>> minTurns(rows, std::vector<int>(cols, INT_MAX));
    std::vector<std::vector<Point>> parent(rows, std::vector<Point>(cols, Point(-1, -1)));
    std::vector<std::vector<int>> cameFromDir(rows, std::vector<int>(cols, -1));

    std::deque<Point> dq;
    dq.push_back(entry);
    minTurns[entry.x][entry.y] = 0;

    while (!dq.empty()) {
        Point cur = dq.front();
        dq.pop_front();

        if (cur == target) break;

        int curDir = cameFromDir[cur.x][cur.y];

        for (int d = 0; d < 4; d++) {
            int nx = cur.x + DX[d];
            int ny = cur.y + DY[d];
            if (!maze.isWalkable(nx, ny)) continue;

            int extraCost = (curDir == -1 || curDir == d) ? 0 : 1;
            int newTurns = minTurns[cur.x][cur.y] + extraCost;

            if (newTurns < minTurns[nx][ny]) {
                minTurns[nx][ny] = newTurns;
                parent[nx][ny] = cur;
                cameFromDir[nx][ny] = d;
                if (extraCost == 0)
                    dq.push_front(Point(nx, ny));
                else
                    dq.push_back(Point(nx, ny));
            }
        }
    }

    Path path;
    if (minTurns[target.x][target.y] == INT_MAX) return path;

    Point cur = target;
    std::vector<Point> reversed;
    while (true) {
        reversed.push_back(cur);
        if (cur == entry) break;
        cur = parent[cur.x][cur.y];
    }
    std::reverse(reversed.begin(), reversed.end());
    for (const auto& p : reversed) {
        path.points.push_back(p);
    }
    path.steps = (int)path.points.size() - 1;
    path.calculateTurns();
    return path;
}
