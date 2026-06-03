#include "checkpoint_solver.h"
#include "dijkstra_solver.h"
#include <algorithm>
#include <climits>
#include <queue>

static const int DX[] = {-1, 1, 0, 0};
static const int DY[] = {0, 0, -1, 1};

// BFS to find actual path between two points on a weighted maze
// (returns steps-only path, used for segment concatenation)
static Path bfsSegment(const Maze& maze, const Point& start, const Point& goal) {
    int rows = maze.getRows(), cols = maze.getCols();
    std::vector<std::vector<bool>> visited(rows, std::vector<bool>(cols, false));
    std::vector<std::vector<Point>> parent(rows, std::vector<Point>(cols, Point(-1, -1)));

    std::queue<Point> q;
    q.push(start);
    visited[start.x][start.y] = true;

    while (!q.empty()) {
        Point cur = q.front(); q.pop();
        if (cur == goal) break;
        for (int k = 0; k < 4; k++) {
            int nx = cur.x + DX[k], ny = cur.y + DY[k];
            if (maze.isWalkable(nx, ny) && !visited[nx][ny]) {
                visited[nx][ny] = true;
                parent[nx][ny] = cur;
                q.push(Point(nx, ny));
            }
        }
    }

    Path path;
    if (!visited[goal.x][goal.y]) return path;

    Point cur = goal;
    std::vector<Point> reversed;
    while (true) {
        reversed.push_back(cur);
        if (cur == start) break;
        cur = parent[cur.x][cur.y];
    }
    std::reverse(reversed.begin(), reversed.end());
    for (const auto& p : reversed)
        path.addPointWithCost(p, maze.getCost(p.x, p.y));
    return path;
}

Path solveWithCheckpoints(const Maze& maze, int entryIndex, int exitIndex) {
    const auto& checkpoints = maze.getCheckpoints();
    const auto& entries = maze.getEntries();
    const auto& exits = maze.getExits();

    if (entries.empty() || exits.empty()) return Path();
    Point start = entries[entryIndex];
    Point goal  = exits[exitIndex];

    Path fullPath;
    Point cur = start;

    // Entry -> each checkpoint in order
    for (const auto& cp : checkpoints) {
        Path seg = bfsSegment(maze, cur, cp);
        if (seg.points.empty()) return Path(); // no path
        for (size_t i = (cur == start ? 0 : 1); i < seg.points.size(); i++)
            fullPath.addPointWithCost(seg.points[i], maze.getCost(seg.points[i].x, seg.points[i].y));
        cur = cp;
    }

    // Last checkpoint -> exit
    Path seg = bfsSegment(maze, cur, goal);
    if (seg.points.empty()) return Path();
    for (size_t i = 1; i < seg.points.size(); i++)
        fullPath.addPointWithCost(seg.points[i], maze.getCost(seg.points[i].x, seg.points[i].y));

    return fullPath;
}

Path solveOptimalCheckpoints(const Maze& maze, int entryIndex, int exitIndex) {
    const auto& checkpoints = maze.getCheckpoints();
    const auto& entries = maze.getEntries();
    const auto& exits = maze.getExits();

    if (entries.empty() || exits.empty()) return Path();
    if (checkpoints.empty()) return bfsSegment(maze, entries[entryIndex], exits[exitIndex]);

    int k = (int)checkpoints.size();
    Point start = entries[entryIndex];
    Point goal  = exits[exitIndex];

    if (k <= 8) {
        // Exhaustive permutation
        std::vector<int> order(k);
        for (int i = 0; i < k; i++) order[i] = i;

        Path bestPath;
        int bestCost = INT_MAX;

        do {
            Path cand;
            Point cur = start;
            int cost = 0;
            bool ok = true;

            for (int idx : order) {
                Path seg = bfsSegment(maze, cur, checkpoints[idx]);
                if (seg.points.empty()) { ok = false; break; }
                cost += seg.totalCost;
                for (size_t i = (cur == start ? 0 : 1); i < seg.points.size(); i++)
                    cand.addPointWithCost(seg.points[i], maze.getCost(seg.points[i].x, seg.points[i].y));
                cur = checkpoints[idx];
            }

            if (ok) {
                Path seg = bfsSegment(maze, cur, goal);
                if (!seg.points.empty()) {
                    cost += seg.totalCost;
                    for (size_t i = 1; i < seg.points.size(); i++)
                        cand.addPointWithCost(seg.points[i], maze.getCost(seg.points[i].x, seg.points[i].y));
                    if (cost < bestCost) {
                        bestCost = cost;
                        bestPath = cand;
                    }
                }
            }
        } while (std::next_permutation(order.begin(), order.end()));

        return bestPath;
    }

    // Greedy nearest-neighbor for >8 checkpoints
    std::vector<bool> visited(k, false);
    Path fullPath;
    Point cur = start;
    bool first = true;

    for (int step = 0; step < k; step++) {
        int bestIdx = -1, bestDist = INT_MAX;
        for (int i = 0; i < k; i++) {
            if (visited[i]) continue;
            int d = std::abs(cur.x - checkpoints[i].x) + std::abs(cur.y - checkpoints[i].y);
            if (d < bestDist) { bestDist = d; bestIdx = i; }
        }
        visited[bestIdx] = true;
        Path seg = bfsSegment(maze, cur, checkpoints[bestIdx]);
        if (seg.points.empty()) return Path();
        for (size_t i = (first ? 0 : 1); i < seg.points.size(); i++)
            fullPath.addPointWithCost(seg.points[i], maze.getCost(seg.points[i].x, seg.points[i].y));
        cur = checkpoints[bestIdx];
        first = false;
    }

    Path seg = bfsSegment(maze, cur, goal);
    if (seg.points.empty()) return Path();
    for (size_t i = 1; i < seg.points.size(); i++)
        fullPath.addPointWithCost(seg.points[i], maze.getCost(seg.points[i].x, seg.points[i].y));

    return fullPath;
}
