#include "dijkstra_solver.h"
#include <queue>
#include <climits>
#include <algorithm>

static const int DX[] = {-1, 1, 0, 0};
static const int DY[] = {0, 0, -1, 1};

struct PQState {
    int dist;
    int x, y;
    bool operator>(const PQState& o) const { return dist > o.dist; }
};

Path reconstructPath(const std::vector<std::vector<Point>>& parent,
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

Path dijkstraFindPath(const Maze& maze, int entryIndex, int exitIndex) {
    int rows = maze.getRows(), cols = maze.getCols();
    Point start = maze.getEntries()[entryIndex];
    Point goal  = maze.getExits()[exitIndex];

    std::vector<std::vector<int>> dist(rows, std::vector<int>(cols, INT_MAX));
    std::vector<std::vector<Point>> parent(rows, std::vector<Point>(cols, Point(-1, -1)));

    std::priority_queue<PQState, std::vector<PQState>, std::greater<PQState>> pq;

    dist[start.x][start.y] = 0;
    pq.push({0, start.x, start.y});

    while (!pq.empty()) {
        PQState s = pq.top(); pq.pop();
        if (s.dist > dist[s.x][s.y]) continue;
        if (s.x == goal.x && s.y == goal.y) break;

        for (int k = 0; k < 4; k++) {
            int nx = s.x + DX[k], ny = s.y + DY[k];
            if (!maze.isWalkable(nx, ny)) continue;
            int nd = s.dist + maze.getCost(nx, ny);
            if (nd < dist[nx][ny]) {
                dist[nx][ny] = nd;
                parent[nx][ny] = Point(s.x, s.y);
                pq.push({nd, nx, ny});
            }
        }
    }

    if (dist[goal.x][goal.y] == INT_MAX) return Path();
    return reconstructPath(parent, start, goal, maze);
}

Path dijkstraMultiEntryExit(const Maze& maze) {
    int rows = maze.getRows(), cols = maze.getCols();
    std::vector<std::vector<int>> dist(rows, std::vector<int>(cols, INT_MAX));
    std::vector<std::vector<Point>> parent(rows, std::vector<Point>(cols, Point(-1, -1)));

    std::priority_queue<PQState, std::vector<PQState>, std::greater<PQState>> pq;

    for (const auto& entry : maze.getEntries()) {
        dist[entry.x][entry.y] = 0;
        pq.push({0, entry.x, entry.y});
    }

    Point goal(-1, -1);
    while (!pq.empty()) {
        PQState s = pq.top(); pq.pop();
        if (s.dist > dist[s.x][s.y]) continue;

        bool isExit = false;
        for (const auto& e : maze.getExits()) {
            if (s.x == e.x && s.y == e.y) { goal = e; isExit = true; break; }
        }
        if (isExit) break;

        for (int k = 0; k < 4; k++) {
            int nx = s.x + DX[k], ny = s.y + DY[k];
            if (!maze.isWalkable(nx, ny)) continue;
            int nd = s.dist + maze.getCost(nx, ny);
            if (nd < dist[nx][ny]) {
                dist[nx][ny] = nd;
                parent[nx][ny] = Point(s.x, s.y);
                pq.push({nd, nx, ny});
            }
        }
    }

    if (goal.x == -1) return Path();

    Point start = goal;
    while (parent[start.x][start.y].x != -1)
        start = parent[start.x][start.y];
    return reconstructPath(parent, start, goal, maze);
}

std::vector<std::vector<int>> dijkstraDistances(const Maze& maze, const Point& start) {
    int rows = maze.getRows(), cols = maze.getCols();
    std::vector<std::vector<int>> dist(rows, std::vector<int>(cols, INT_MAX));

    std::priority_queue<PQState, std::vector<PQState>, std::greater<PQState>> pq;

    dist[start.x][start.y] = 0;
    pq.push({0, start.x, start.y});

    while (!pq.empty()) {
        PQState s = pq.top(); pq.pop();
        if (s.dist > dist[s.x][s.y]) continue;
        for (int k = 0; k < 4; k++) {
            int nx = s.x + DX[k], ny = s.y + DY[k];
            if (!maze.isWalkable(nx, ny)) continue;
            int nd = s.dist + maze.getCost(nx, ny);
            if (nd < dist[nx][ny]) {
                dist[nx][ny] = nd;
                pq.push({nd, nx, ny});
            }
        }
    }
    return dist;
}
