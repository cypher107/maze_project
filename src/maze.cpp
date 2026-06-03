#include "maze.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

void Path::addPoint(const Point& p) {
    points.push_back(p);
    steps = (int)points.size() - 1;
    calculateTurns();
}

void Path::addPointWithCost(const Point& p, int cost) {
    points.push_back(p);
    steps = (int)points.size() - 1;
    totalCost += cost;
    calculateTurns();
}

void Path::calculateTurns() {
    turns = 0;
    if (points.size() < 3) return;
    for (size_t i = 1; i < points.size() - 1; i++) {
        int dx1 = points[i].x - points[i - 1].x;
        int dy1 = points[i].y - points[i - 1].y;
        int dx2 = points[i + 1].x - points[i].x;
        int dy2 = points[i + 1].y - points[i].y;
        if (dx1 != dx2 || dy1 != dy2) turns++;
    }
}

bool Maze::inBounds(int x, int y) const {
    return x >= 0 && x < rows && y >= 0 && y < cols;
}

bool Maze::isWalkable(int x, int y) const {
    return inBounds(x, y) && ::isWalkable(grid[x][y]);
}

int Maze::getCost(int x, int y) const {
    if (!inBounds(x, y)) return 999999;
    return terrainCost(grid[x][y]);
}

Terrain Maze::getTerrain(int x, int y) const {
    if (!inBounds(x, y)) return Terrain::Wall;
    return grid[x][y];
}

void Maze::setTerrain(int x, int y, Terrain t) {
    if (inBounds(x, y)) grid[x][y] = t;
}

void Maze::resize(int newRows, int newCols) {
    rows = newRows;
    cols = newCols;
    grid.assign(rows, std::vector<Terrain>(cols, Terrain::Grass));
    entries.clear();
    exits.clear();
    checkpoints.clear();
}

void Maze::addEntry(const Point& p)   { entries.push_back(p); }
void Maze::addExit(const Point& p)    { exits.push_back(p); }
void Maze::addCheckpoint(const Point& p) { checkpoints.push_back(p); }

void Maze::removeEntry(const Point& p) {
    entries.erase(std::remove(entries.begin(), entries.end(), p), entries.end());
}
void Maze::removeExit(const Point& p) {
    exits.erase(std::remove(exits.begin(), exits.end(), p), exits.end());
}
void Maze::removeCheckpoint(const Point& p) {
    checkpoints.erase(std::remove(checkpoints.begin(), checkpoints.end(), p), checkpoints.end());
}

void Maze::clearSpecialPoints() {
    entries.clear();
    exits.clear();
    checkpoints.clear();
}

void Maze::setEntry(const Point& p) {
    entries.clear();
    entries.push_back(p);
}
void Maze::setExit(const Point& p) {
    exits.clear();
    exits.push_back(p);
}

bool Maze::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return false;
    }

    file >> rows >> cols;
    if (rows <= 0 || cols <= 0) {
        std::cerr << "迷宫尺寸无效: " << rows << "x" << cols << std::endl;
        return false;
    }

    grid.assign(rows, std::vector<Terrain>(cols, Terrain::Grass));
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            int val;
            if (!(file >> val)) {
                std::cerr << "读取迷宫数据失败，行 " << i << " 列 " << j << std::endl;
                return false;
            }
            grid[i][j] = (val == 0) ? Terrain::Grass : Terrain::Wall;
        }
    }

    int ex, ey;
    file >> ex >> ey;
    entries.clear(); entries.push_back(Point(ex, ey));
    file >> ex >> ey;
    exits.clear(); exits.push_back(Point(ex, ey));

    if (!isWalkable(entries[0])) {
        std::cerr << "入口位置无效" << std::endl;
        return false;
    }
    if (!isWalkable(exits[0])) {
        std::cerr << "出口位置无效" << std::endl;
        return false;
    }

    checkpoints.clear();
    file.close();
    std::cout << "成功加载迷宫: " << rows << "x" << cols << std::endl;
    return true;
}

bool Maze::loadExtendedFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    file >> rows >> cols;
    if (rows <= 0 || cols <= 0) return false;

    grid.assign(rows, std::vector<Terrain>(cols, Terrain::Grass));
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            int val;
            if (!(file >> val)) return false;
            grid[i][j] = static_cast<Terrain>(val);
        }
    }

    auto readPoints = [&](std::vector<Point>& vec) {
        int n; file >> n;
        vec.clear();
        for (int i = 0; i < n; i++) {
            int x, y; file >> x >> y;
            vec.push_back(Point(x, y));
        }
    };

    readPoints(entries);
    readPoints(exits);
    readPoints(checkpoints);

    file.close();
    std::cout << "加载扩展迷宫: " << rows << "x" << cols
              << ", 入口" << entries.size() << "个, 出口" << exits.size()
              << "个, 关卡" << checkpoints.size() << "个" << std::endl;
    return true;
}

bool Maze::saveExtendedFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) return false;

    file << rows << " " << cols << "\n";
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            file << static_cast<int>(grid[i][j]);
            if (j < cols - 1) file << " ";
        }
        file << "\n";
    }

    auto writePoints = [&](const std::vector<Point>& vec) {
        file << vec.size() << "\n";
        for (const auto& p : vec) file << p.x << " " << p.y << "\n";
    };

    writePoints(entries);
    writePoints(exits);
    writePoints(checkpoints);

    file.close();
    return true;
}

void Maze::printMaze(const Path* highlight) const {
    std::vector<std::vector<bool>> onPath(rows, std::vector<bool>(cols, false));
    if (highlight) {
        for (const auto& p : highlight->points) onPath[p.x][p.y] = true;
    }

    std::cout << "\n  ";
    for (int j = 0; j < cols; j++) std::cout << std::setw(2) << j;
    std::cout << "\n";

    for (int i = 0; i < rows; i++) {
        std::cout << std::setw(2) << i << " ";
        for (int j = 0; j < cols; j++) {
            bool isEntry = std::find(entries.begin(), entries.end(), Point(i,j)) != entries.end();
            bool isExit  = std::find(exits.begin(), exits.end(), Point(i,j)) != exits.end();
            if (isEntry)
                std::cout << "入";
            else if (isExit)
                std::cout << "出";
            else if (highlight && onPath[i][j])
                std::cout << " *";
            else if (grid[i][j] == Terrain::Wall)
                std::cout << "▓▓";
            else if (grid[i][j] == Terrain::Swamp)
                std::cout << "沼";
            else if (grid[i][j] == Terrain::Water)
                std::cout << "水";
            else
                std::cout << "  ";
        }
        std::cout << "\n";
    }
}

void Maze::printMazeWithPath(const Path& path) const {
    printMaze(&path);
}
