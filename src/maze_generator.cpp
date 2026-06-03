#include "maze_generator.h"
#include <stack>
#include <algorithm>
#include <random>
#include <ctime>

static const int DX[] = {-2, 2, 0, 0};
static const int DY[] = {0, 0, -2, 2};

static std::mt19937& rng() {
    static std::mt19937 gen(static_cast<unsigned>(std::time(nullptr)));
    return gen;
}

static Terrain pickTerrain(float grassProb, float swampProb, float waterProb) {
    float r = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng());
    if (r < grassProb) return Terrain::Grass;
    if (r < grassProb + swampProb) return Terrain::Swamp;
    return Terrain::Water;
}

Maze generateMazeDFS(int rows, int cols, float wallDensity) {
    // Force odd dimensions
    int r = (rows % 2 == 0) ? rows + 1 : rows;
    int c = (cols % 2 == 0) ? cols + 1 : cols;

    Maze maze;
    maze.resize(r, c);
    // Fill with walls
    for (int i = 0; i < r; i++)
        for (int j = 0; j < c; j++)
            maze.setTerrain(i, j, Terrain::Wall);

    // DFS backtracking from (1,1)
    std::vector<std::vector<bool>> visited(r, std::vector<bool>(c, false));
    std::stack<Point> stk;

    maze.setTerrain(1, 1, Terrain::Grass);
    visited[1][1] = true;
    stk.push(Point(1, 1));

    while (!stk.empty()) {
        Point cur = stk.top();
        std::vector<int> dirs = {0, 1, 2, 3};
        std::shuffle(dirs.begin(), dirs.end(), rng());

        bool carved = false;
        for (int d : dirs) {
            int nx = cur.x + DX[d], ny = cur.y + DY[d];
            int mx = cur.x + DX[d] / 2, my = cur.y + DY[d] / 2;
            if (mx >= 0 && mx < r && my >= 0 && my < c && !visited[mx][my]) {
                maze.setTerrain(mx, my, Terrain::Grass);
                visited[mx][my] = true;
                // Carve the wall between
                maze.setTerrain((cur.x + mx) / 2, (cur.y + my) / 2, Terrain::Grass);
                stk.push(Point(mx, my));
                carved = true;
                break;
            }
        }
        if (!carved) stk.pop();
    }

    // Remove extra walls for multiple paths
    for (int i = 1; i < r - 1; i++) {
        for (int j = 1; j < c - 1; j++) {
            if (maze.getTerrain(i, j) == Terrain::Wall) continue;
            // Randomly open adjacent walls
            if (std::uniform_real_distribution<float>(0, 1)(rng()) < wallDensity) {
                for (int d = 0; d < 4; d++) {
                    int nx = i + DX[d] / 2, ny = j + DY[d] / 2;
                    if (maze.inBounds(nx, ny) && maze.getTerrain(nx, ny) == Terrain::Wall) {
                        if (std::uniform_real_distribution<float>(0, 1)(rng()) < 0.3f) {
                            maze.setTerrain(nx, ny, Terrain::Grass);
                        }
                    }
                }
            }
        }
    }

    maze.setEntry(Point(1, 1));
    maze.setExit(Point(r - 2, c - 2));
    return maze;
}

Maze generateMazeWeighted(int rows, int cols, float wallDensity,
                          float grassProb, float swampProb, float waterProb) {
    Maze maze = generateMazeDFS(rows, cols, wallDensity);

    // Randomly assign terrain to non-wall cells
    for (int i = 0; i < maze.getRows(); i++) {
        for (int j = 0; j < maze.getCols(); j++) {
            if (maze.getTerrain(i, j) != Terrain::Wall) {
                maze.setTerrain(i, j, pickTerrain(grassProb, swampProb, waterProb));
            }
        }
    }

    // Ensure entry and exit are grass
    maze.setTerrain(maze.getEntry().x, maze.getEntry().y, Terrain::Grass);
    maze.setTerrain(maze.getExit().x, maze.getExit().y, Terrain::Grass);

    return maze;
}
