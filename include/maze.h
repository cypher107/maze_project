#ifndef MAZE_H
#define MAZE_H

#include "terrain.h"
#include <vector>
#include <string>
#include <iostream>
#include <functional>

struct Point {
    int x, y;
    Point(int x = 0, int y = 0) : x(x), y(y) {}
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
    bool operator!=(const Point& other) const {
        return !(*this == other);
    }
    struct Hash {
        std::size_t operator()(const Point& p) const {
            return std::hash<int>()(p.x) ^ (std::hash<int>()(p.y) << 16);
        }
    };
};

struct Path {
    std::vector<Point> points;
    int steps;
    int turns;
    int totalCost;

    Path() : steps(0), turns(0), totalCost(0) {}
    void addPoint(const Point& p);
    void addPointWithCost(const Point& p, int cost);
    void calculateTurns();
};

class Maze {
private:
    std::vector<std::vector<Terrain>> grid;
    int rows, cols;
    std::vector<Point> entries;
    std::vector<Point> exits;
    std::vector<Point> checkpoints;

public:
    Maze() : rows(0), cols(0) {}

    // File I/O
    bool loadFromFile(const std::string& filename);
    bool loadExtendedFile(const std::string& filename);
    bool saveExtendedFile(const std::string& filename) const;

    // Grid access
    bool isWalkable(int x, int y) const;
    bool isWalkable(const Point& p) const { return isWalkable(p.x, p.y); }
    bool inBounds(int x, int y) const;
    int getCost(int x, int y) const;
    Terrain getTerrain(int x, int y) const;
    void setTerrain(int x, int y, Terrain t);
    void resize(int newRows, int newCols);

    int getRows() const { return rows; }
    int getCols() const { return cols; }

    // Entry / Exit / Checkpoint management
    const std::vector<Point>& getEntries() const { return entries; }
    const std::vector<Point>& getExits() const { return exits; }
    const std::vector<Point>& getCheckpoints() const { return checkpoints; }

    void addEntry(const Point& p);
    void addExit(const Point& p);
    void addCheckpoint(const Point& p);
    void removeEntry(const Point& p);
    void removeExit(const Point& p);
    void removeCheckpoint(const Point& p);
    void clearSpecialPoints();

    // Backward-compatible single entry/exit API
    Point getEntry() const { return entries.empty() ? Point(-1,-1) : entries[0]; }
    Point getExit() const { return exits.empty() ? Point(-1,-1) : exits[0]; }
    void setEntry(const Point& p);
    void setExit(const Point& p);

    const std::vector<std::vector<Terrain>>& getGrid() const { return grid; }

    // Display
    void printMaze(const Path* highlight = nullptr) const;
    void printMazeWithPath(const Path& path) const;
};

#endif
