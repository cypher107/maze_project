#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QDockWidget>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QButtonGroup>
#include <QRadioButton>
#include <QTabWidget>
#include <QGroupBox>
#include <QTimer>
#include <QVector>
#include <QColor>

#include "maze.h"
#include "solver.h"
#include "dijkstra_solver.h"
#include "astar_solver.h"
#include "checkpoint_solver.h"
#include "maze_generator.h"

// ============================================================
// MazeCanvas: Custom-painted maze display widget
// ============================================================
class MazeCanvas : public QWidget {
    Q_OBJECT
public:
    explicit MazeCanvas(QWidget* parent = nullptr);

    void setMaze(const Maze* maze);
    void setPath(const Path& path, const QColor& color = Qt::red);
    void clearAllPaths();
    void addComparePath(const Path& path, const QColor& color, const QString& label);

    enum EditMode { None, SetTerrain, SetEntry, SetExit, SetCheckpoint };
    void setEditMode(EditMode mode);
    void setActiveTerrain(Terrain t);
    void setAnimateSpeed(int ms);

    void animatePath(const Path& path, const QColor& color);
    void stopAnimation();

    Point cellFromPixel(const QPoint& pixelPos) const;
    int cellSize() const { return mCellSize; }

signals:
    void cellClicked(int x, int y);
    void animationFinished();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    const Maze* mMaze;
    QVector<QPair<Path, QColor>> mPaths;
    QStringList mPathLabels;
    EditMode mEditMode;
    Terrain mActiveTerrain;
    int mCellSize;

    // Animation
    QTimer* mAnimTimer;
    QVector<QPointF> mAnimPoints;
    QColor mAnimColor;
    int mAnimIndex;

    void updateCellSize();
    QColor terrainQColor(Terrain t) const;
    void drawCell(QPainter* p, int row, int col, Terrain t);
    void drawSpecialPoint(QPainter* p, int row, int col, const QColor& border, const QString& label);
    bool isEntry(int x, int y) const;
    bool isExit(int x, int y) const;
    bool isCheckpoint(int x, int y) const;
    int checkpointIndex(int x, int y) const;

private slots:
    void animTick();
};

// ============================================================
// ControlPanel: Dock widget with all controls
// ============================================================
class ControlPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit ControlPanel(QWidget* parent = nullptr);

    void updateEntryExitCombos(const Maze& maze);
    void setStats(const QString& status, int steps, int turns, int cost, int pathCount);
    void clearStats();

signals:
    void runSolver(int algorithm, int entryIdx, int exitIdx, bool animated);
    void generateMaze(int rows, int cols, float wallDensity,
                      float grassP, float swampP, float waterP);
    void loadFile(const QString& path, bool extended);
    void saveFile(const QString& path);
    void editModeChanged(int mode);      // 0=None, 1=Terrain, 2=Entry, 3=Exit, 4=Checkpoint
    void activeTerrainChanged(int t);    // 0=Grass, 1=Swamp, 2=Water, 3=Wall
    void speedChanged(int ms);

private:
    QComboBox* mAlgoCombo;
    QComboBox* mEntryCombo;
    QComboBox* mExitCombo;
    QCheckBox* mAnimateCheck;
    QSlider* mSpeedSlider;
    QPushButton* mRunBtn;
    QPushButton* mStopAnimBtn;

    QSpinBox* mGenRows;
    QSpinBox* mGenCols;
    QDoubleSpinBox* mGenWallDensity;
    QDoubleSpinBox* mGenGrassProb;
    QDoubleSpinBox* mGenSwampProb;
    QDoubleSpinBox* mGenWaterProb;
    QPushButton* mGenerateBtn;

    QButtonGroup* mTerrainGroup;
    QButtonGroup* mEditModeGroup;

    QLabel* mStatusLabel;
    QLabel* mStepsLabel;
    QLabel* mTurnsLabel;
    QLabel* mCostLabel;
    QLabel* mPathCountLabel;
};

// ============================================================
// MainWindow
// ============================================================
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onRunSolver(int algorithm, int entryIdx, int exitIdx, bool animated);
    void onGenerate(int rows, int cols, float wallDensity,
                    float grassP, float swampP, float waterP);
    void onLoadFile(const QString& path, bool extended);
    void onSaveFile(const QString& path);
    void onEditModeChanged(int mode);
    void onActiveTerrainChanged(int terrain);
    void onCellClicked(int x, int y);
    void onAnimationFinished();

private:
    Maze mMaze;
    Path mCurrentPath;

    MazeCanvas* mCanvas;
    ControlPanel* mPanel;

    // Edit state
    int mEditMode;     // 0-4
    Terrain mEditTerrain;

    void setupMenuBar();
    void runAlgorithm(int algo, int entryIdx, int exitIdx, bool animated);
    void updatePanelStats(const Path& p, int pathCount = -1);
};

#endif
