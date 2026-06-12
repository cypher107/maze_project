#include "mainwindow.h"

#include <QPainter>
#include <QMouseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QMenuBar>
#include <QStatusBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QAction>
#include <QApplication>
#include <cmath>

// ============================================================
// MazeCanvas implementation
// ============================================================

MazeCanvas::MazeCanvas(QWidget* parent)
    : QWidget(parent), mMaze(nullptr), mEditMode(None),
      mActiveTerrain(Terrain::Grass), mCellSize(40), mAnimIndex(0)
{
    setMinimumSize(400, 300);
    setMouseTracking(true);

    mAnimTimer = new QTimer(this);
    connect(mAnimTimer, &QTimer::timeout, this, &MazeCanvas::animTick);
}

void MazeCanvas::setMaze(const Maze* maze) {
    mMaze = maze;
    clearAllPaths();
    updateCellSize();
    update();
}

void MazeCanvas::setPath(const Path* path, const QColor& color) {
    clearAllPaths();
    if (path) mPaths.append({path, color});
    update();
}

void MazeCanvas::clearAllPaths() {
    mPaths.clear();
    mPathLabels.clear();
    stopAnimation();
    update();
}

void MazeCanvas::addComparePath(const Path* path, const QColor& color, const QString& label) {
    if (path && !path->points.empty()) {
        mPaths.append({path, color});
        mPathLabels.append(label);
    }
}

void MazeCanvas::setEditMode(EditMode mode) { mEditMode = mode; }
void MazeCanvas::setActiveTerrain(Terrain t) { mActiveTerrain = t; }
void MazeCanvas::setAnimateSpeed(int ms) { mAnimTimer->setInterval(ms); }

Point MazeCanvas::cellFromPixel(const QPoint& pos) const {
    if (!mMaze) return Point(-1, -1);
    int x = (pos.y() - 1) / mCellSize;
    int y = (pos.x() - 1) / mCellSize;
    if (x < 0 || x >= mMaze->getRows() || y < 0 || y >= mMaze->getCols())
        return Point(-1, -1);
    return Point(x, y);
}

void MazeCanvas::mousePressEvent(QMouseEvent* event) {
    if (!mMaze) return;
    Point p = cellFromPixel(event->pos());
    if (p.x < 0) return;

    switch (mEditMode) {
    case SetTerrain:
        emit cellClicked(p.x, p.y);
        break;
    default:
        emit cellClicked(p.x, p.y);
        break;
    }
}

QColor MazeCanvas::terrainQColor(Terrain t) const {
    switch (t) {
        case Terrain::Grass: return QColor(144, 238, 144);
        case Terrain::Swamp: return QColor(139, 90, 43);
        case Terrain::Water: return QColor(70, 130, 180);
        case Terrain::Wall:  return QColor(50, 50, 55);
    }
    return Qt::gray;
}

bool MazeCanvas::isEntry(int x, int y) const {
    if (!mMaze) return false;
    for (const auto& e : mMaze->getEntries())
        if (e.x == x && e.y == y) return true;
    return false;
}

bool MazeCanvas::isExit(int x, int y) const {
    if (!mMaze) return false;
    for (const auto& e : mMaze->getExits())
        if (e.x == x && e.y == y) return true;
    return false;
}

bool MazeCanvas::isCheckpoint(int x, int y) const {
    if (!mMaze) return false;
    for (const auto& c : mMaze->getCheckpoints())
        if (c.x == x && c.y == y) return true;
    return false;
}

int MazeCanvas::checkpointIndex(int x, int y) const {
    if (!mMaze) return -1;
    const auto& cps = mMaze->getCheckpoints();
    for (size_t i = 0; i < cps.size(); i++)
        if (cps[i].x == x && cps[i].y == y) return (int)i;
    return -1;
}

void MazeCanvas::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background
    p.fillRect(rect(), QColor(30, 30, 40));

    if (!mMaze) {
        p.setPen(Qt::gray);
        p.drawText(rect(), Qt::AlignCenter, "请加载或生成迷宫");
        return;
    }

    int rows = mMaze->getRows(), cols = mMaze->getCols();

    // Draw terrain cells
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            Terrain t = mMaze->getTerrain(i, j);
            p.fillRect(j * mCellSize + 1, i * mCellSize + 1,
                       mCellSize - 2, mCellSize - 2, terrainQColor(t));
        }
    }

    // Draw path overlays - collect path cells per path
    for (int pi = 0; pi < mPaths.size(); pi++) {
        const Path* path = mPaths[pi].first;
        if (!path || path->points.empty()) continue;
        QColor col = mPaths[pi].second;

        // Build set for this path
        for (size_t k = 0; k < path->points.size(); k++) {
            const Point& pt = path->points[k];
            int cx = pt.y * mCellSize + mCellSize / 2;
            int cy = pt.x * mCellSize + mCellSize / 2;

            // Draw dot
            p.setPen(Qt::NoPen);
            p.setBrush(col);
            int r = std::max(3, mCellSize / 8);
            p.drawEllipse(QPoint(cx, cy), r, r);
        }

        // Draw line segments between consecutive points
        if (path->points.size() > 1) {
            QPen linePen(col);
            linePen.setWidth(std::max(1, mCellSize / 20));
            p.setPen(linePen);
            for (size_t k = 0; k < path->points.size() - 1; k++) {
                const Point& a = path->points[k];
                const Point& b = path->points[k + 1];
                int ax = a.y * mCellSize + mCellSize / 2;
                int ay = a.x * mCellSize + mCellSize / 2;
                int bx = b.y * mCellSize + mCellSize / 2;
                int by = b.x * mCellSize + mCellSize / 2;
                p.drawLine(ax, ay, bx, by);
            }
        }
    }

    // Draw animated segment
    if (mAnimTimer->isActive() && mAnimIndex > 0 && mAnimIndex < (int)mAnimPoints.size()) {
        QPen animPen(mAnimColor, std::max(2, mCellSize / 12));
        p.setPen(animPen);
        for (int k = 0; k < mAnimIndex - 1; k++) {
            int ax = (int)mAnimPoints[k].x(), ay = (int)mAnimPoints[k].y();
            int bx = (int)mAnimPoints[k+1].x(), by = (int)mAnimPoints[k+1].y();
            p.drawLine(ax, ay, bx, by);
        }
    }

    // Draw special points on top
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (isEntry(i, j))
                drawSpecialPoint(&p, i, j, QColor(78, 204, 163), "入");
            else if (isExit(i, j))
                drawSpecialPoint(&p, i, j, QColor(240, 165, 0), "出");
            else if (isCheckpoint(i, j)) {
                int idx = checkpointIndex(i, j);
                drawSpecialPoint(&p, i, j, QColor(160, 80, 220),
                                 QString("C%1").arg(idx + 1));
            }
        }
    }

    // Grid lines for larger cells
    if (mCellSize >= 15) {
        p.setPen(QPen(QColor(60, 60, 70), 1));
        for (int i = 0; i <= rows; i++)
            p.drawLine(0, i * mCellSize, cols * mCellSize, i * mCellSize);
        for (int j = 0; j <= cols; j++)
            p.drawLine(j * mCellSize, 0, j * mCellSize, rows * mCellSize);
    }
}

void MazeCanvas::drawSpecialPoint(QPainter* p, int row, int col,
                                   const QColor& border, const QString& label) {
    int x = col * mCellSize;
    int y = row * mCellSize;
    int s = mCellSize;

    // Border rectangle
    QPen pen(border, std::max(2, s / 10));
    p->setPen(pen);
    p->setBrush(Qt::NoBrush);
    p->drawRect(x + 2, y + 2, s - 4, s - 4);

    // Label
    if (s >= 20) {
        QFont f = p->font();
        f.setPixelSize(std::max(8, s * 4 / 10));
        f.setBold(true);
        p->setFont(f);
        p->setPen(border.lighter(150));
        p->drawText(QRect(x, y, s, s), Qt::AlignCenter, label);
    }
}

void MazeCanvas::resizeEvent(QResizeEvent*) {
    updateCellSize();
}

void MazeCanvas::updateCellSize() {
    if (!mMaze || mMaze->getRows() == 0) return;
    int maxW = width() - 4;
    int maxH = height() - 4;
    mCellSize = std::min(maxW / mMaze->getCols(), maxH / mMaze->getRows());
    mCellSize = std::max(mCellSize, 6);
    mCellSize = std::min(mCellSize, 80);
    update();
}

void MazeCanvas::animatePath(const Path& path, const QColor& color) {
    stopAnimation();
    mAnimPoints.clear();
    mAnimColor = color;
    mAnimIndex = 0;

    for (const auto& pt : path.points) {
        mAnimPoints.append(QPointF(
            pt.y * mCellSize + mCellSize / 2.0,
            pt.x * mCellSize + mCellSize / 2.0));
    }
    mAnimTimer->start();
}

void MazeCanvas::stopAnimation() {
    mAnimTimer->stop();
    mAnimIndex = 0;
    mAnimPoints.clear();
}

void MazeCanvas::animTick() {
    mAnimIndex++;
    update();
    if (mAnimIndex >= (int)mAnimPoints.size()) {
        mAnimTimer->stop();
        emit animationFinished();
    }
}

// ============================================================
// ControlPanel implementation
// ============================================================

ControlPanel::ControlPanel(QWidget* parent)
    : QDockWidget("控制面板", parent)
{
    setMinimumWidth(280);
    setFeatures(DockWidgetMovable | DockWidgetFloatable);

    QWidget* container = new QWidget;
    QVBoxLayout* mainLayout = new QVBoxLayout(container);

    QTabWidget* tabs = new QTabWidget;

    // ---------- Tab 1: Solve ----------
    QWidget* solveTab = new QWidget;
    QVBoxLayout* solveLayout = new QVBoxLayout(solveTab);

    QGroupBox* algoGroup = new QGroupBox("求解算法");
    QFormLayout* algoForm = new QFormLayout(algoGroup);
    mAlgoCombo = new QComboBox;
    mAlgoCombo->addItems({
        "DFS - 所有路径",
        "BFS - 最短步数",
        "0-1 BFS - 最少转弯",
        "Dijkstra - 最小代价",
        "A* - 最小代价(启发式)",
        "综合对比 (4种算法)"
    });
    algoForm->addRow("算法:", mAlgoCombo);

    mEntryCombo = new QComboBox;
    mExitCombo = new QComboBox;
    algoForm->addRow("入口:", mEntryCombo);
    algoForm->addRow("出口:", mExitCombo);
    solveLayout->addWidget(algoGroup);

    QGroupBox* animGroup = new QGroupBox("动画设置");
    QFormLayout* animForm = new QFormLayout(animGroup);
    mAnimateCheck = new QCheckBox("启用动画");
    mAnimateCheck->setChecked(true);

    mSpeedSlider = new QSlider(Qt::Horizontal);
    mSpeedSlider->setRange(5, 200);
    mSpeedSlider->setValue(40);
    mSpeedSlider->setTickPosition(QSlider::TicksBelow);
    animForm->addRow(mAnimateCheck);
    animForm->addRow("速度(ms):", mSpeedSlider);

    QHBoxLayout* btnRow = new QHBoxLayout;
    mRunBtn = new QPushButton("运行求解");
    mRunBtn->setStyleSheet("QPushButton{background:#e94560;color:white;font-weight:bold;padding:6px;}");
    mStopAnimBtn = new QPushButton("停止动画");
    btnRow->addWidget(mRunBtn);
    btnRow->addWidget(mStopAnimBtn);
    animForm->addRow(btnRow);
    solveLayout->addWidget(animGroup);

    tabs->addTab(solveTab, "求解");

    // ---------- Tab 2: Generate ----------
    QWidget* genTab = new QWidget;
    QVBoxLayout* genLayout = new QVBoxLayout(genTab);

    QGroupBox* sizeGroup = new QGroupBox("迷宫尺寸");
    QFormLayout* sizeForm = new QFormLayout(sizeGroup);
    mGenRows = new QSpinBox;
    mGenRows->setRange(5, 99); mGenRows->setValue(21); mGenRows->setSingleStep(2);
    mGenCols = new QSpinBox;
    mGenCols->setRange(5, 99); mGenCols->setValue(21); mGenCols->setSingleStep(2);
    mGenWallDensity = new QDoubleSpinBox;
    mGenWallDensity->setRange(0.0, 0.5); mGenWallDensity->setValue(0.2); mGenWallDensity->setSingleStep(0.05);
    sizeForm->addRow("行数:", mGenRows);
    sizeForm->addRow("列数:", mGenCols);
    sizeForm->addRow("额外墙密度:", mGenWallDensity);
    genLayout->addWidget(sizeGroup);

    QGroupBox* terrainGroup = new QGroupBox("地形概率");
    QFormLayout* terrainForm = new QFormLayout(terrainGroup);
    mGenGrassProb = new QDoubleSpinBox;
    mGenGrassProb->setRange(0.0, 1.0); mGenGrassProb->setValue(0.7); mGenGrassProb->setSingleStep(0.05);
    mGenSwampProb = new QDoubleSpinBox;
    mGenSwampProb->setRange(0.0, 1.0); mGenSwampProb->setValue(0.2); mGenSwampProb->setSingleStep(0.05);
    mGenWaterProb = new QDoubleSpinBox;
    mGenWaterProb->setRange(0.0, 1.0); mGenWaterProb->setValue(0.1); mGenWaterProb->setSingleStep(0.05);
    terrainForm->addRow("草地:", mGenGrassProb);
    terrainForm->addRow("沼泽:", mGenSwampProb);
    terrainForm->addRow("水面:", mGenWaterProb);
    genLayout->addWidget(terrainGroup);

    mGenerateBtn = new QPushButton("生成随机迷宫");
    mGenerateBtn->setStyleSheet("QPushButton{background:#0f3460;color:#e94560;font-weight:bold;padding:8px;}");
    genLayout->addWidget(mGenerateBtn);
    genLayout->addStretch();

    tabs->addTab(genTab, "生成");

    // ---------- Tab 3: Edit ----------
    QWidget* editTab = new QWidget;
    QVBoxLayout* editLayout = new QVBoxLayout(editTab);

    QGroupBox* modeGroup = new QGroupBox("编辑模式");
    QVBoxLayout* modeLayout = new QVBoxLayout(modeGroup);
    mEditModeGroup = new QButtonGroup(this);
    QStringList modes = {"无(查看)", "设置地形", "添加入口", "添加出口", "添加关卡"};
    for (int i = 0; i < modes.size(); i++) {
        QRadioButton* rb = new QRadioButton(modes[i]);
        mEditModeGroup->addButton(rb, i);
        modeLayout->addWidget(rb);
        if (i == 0) rb->setChecked(true);
    }
    editLayout->addWidget(modeGroup);

    QGroupBox* terrainSelGroup = new QGroupBox("当前地形");
    QVBoxLayout* terrainSelLayout = new QVBoxLayout(terrainSelGroup);
    mTerrainGroup = new QButtonGroup(this);
    QStringList terrains = {"草地 (代价1)", "沼泽 (代价3)", "水面 (代价5)", "墙壁"};
    for (int i = 0; i < terrains.size(); i++) {
        QRadioButton* rb = new QRadioButton(terrains[i]);
        mTerrainGroup->addButton(rb, i);
        terrainSelLayout->addWidget(rb);
        if (i == 0) rb->setChecked(true);
    }
    editLayout->addWidget(terrainSelGroup);
    editLayout->addStretch();

    tabs->addTab(editTab, "编辑");

    mainLayout->addWidget(tabs);

    // ---------- Stats ----------
    QGroupBox* statsGroup = new QGroupBox("路径统计");
    QFormLayout* statsForm = new QFormLayout(statsGroup);
    mStatusLabel = new QLabel("就绪");
    mStepsLabel = new QLabel("-");
    mTurnsLabel = new QLabel("-");
    mCostLabel = new QLabel("-");
    mPathCountLabel = new QLabel("-");
    statsForm->addRow("状态:", mStatusLabel);
    statsForm->addRow("步数:", mStepsLabel);
    statsForm->addRow("转弯:", mTurnsLabel);
    statsForm->addRow("总代价:", mCostLabel);
    statsForm->addRow("路径数:", mPathCountLabel);
    mainLayout->addWidget(statsGroup);

    setWidget(container);

    // Connections
    connect(mRunBtn, &QPushButton::clicked, [this]() {
        emit runSolver(mAlgoCombo->currentIndex(),
                       mEntryCombo->currentIndex(),
                       mExitCombo->currentIndex(),
                       mAnimateCheck->isChecked());
    });

    connect(mStopAnimBtn, &QPushButton::clicked, [this]() {
        emit speedChanged(-1); // signal to stop animation
    });

    connect(mGenerateBtn, &QPushButton::clicked, [this]() {
        emit generateMaze(mGenRows->value(), mGenCols->value(),
                          (float)mGenWallDensity->value(),
                          (float)mGenGrassProb->value(),
                          (float)mGenSwampProb->value(),
                          (float)mGenWaterProb->value());
    });

    connect(mEditModeGroup, &QButtonGroup::idClicked,
            [this](int id) { emit editModeChanged(id); });

    connect(mTerrainGroup, &QButtonGroup::idClicked,
            [this](int id) { emit activeTerrainChanged(id); });

    connect(mSpeedSlider, &QSlider::valueChanged, [this](int v) {
        emit speedChanged(v);
    });
}

void ControlPanel::updateEntryExitCombos(const Maze& maze) {
    mEntryCombo->clear();
    mExitCombo->clear();
    const auto& entries = maze.getEntries();
    const auto& exits = maze.getExits();
    for (size_t i = 0; i < entries.size(); i++)
        mEntryCombo->addItem(QString("入口%1 (%2,%3)").arg(i+1).arg(entries[i].x).arg(entries[i].y), (int)i);
    for (size_t i = 0; i < exits.size(); i++)
        mExitCombo->addItem(QString("出口%1 (%2,%3)").arg(i+1).arg(exits[i].x).arg(exits[i].y), (int)i);
}

void ControlPanel::setStats(const QString& status, int steps, int turns, int cost, int pathCount) {
    mStatusLabel->setText(status);
    mStepsLabel->setText(steps >= 0 ? QString::number(steps) : "-");
    mTurnsLabel->setText(turns >= 0 ? QString::number(turns) : "-");
    mCostLabel->setText(cost >= 0 ? QString::number(cost) : "-");
    mPathCountLabel->setText(pathCount >= 0 ? QString::number(pathCount) : "-");
}

void ControlPanel::clearStats() {
    setStats("就绪", -1, -1, -1, -1);
}

// ============================================================
// MainWindow implementation
// ============================================================

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), mEditMode(0), mEditTerrain(Terrain::Grass)
{
    setWindowTitle("迷宫求解系统 - 算法与数据结构B");
    resize(1200, 750);

    // Central canvas
    mCanvas = new MazeCanvas(this);
    setCentralWidget(mCanvas);

    // Control panel dock
    mPanel = new ControlPanel(this);
    addDockWidget(Qt::RightDockWidgetArea, mPanel);

    // Menu bar
    setupMenuBar();

    // Status bar
    statusBar()->showMessage("就绪 - 请加载迷宫文件或生成新迷宫");

    // Connect signals
    connect(mPanel, &ControlPanel::runSolver,
            this, &MainWindow::onRunSolver);
    connect(mPanel, &ControlPanel::generateMaze,
            this, &MainWindow::onGenerate);
    connect(mPanel, &ControlPanel::loadFile,
            this, &MainWindow::onLoadFile);
    connect(mPanel, &ControlPanel::saveFile,
            this, &MainWindow::onSaveFile);
    connect(mPanel, &ControlPanel::editModeChanged,
            this, &MainWindow::onEditModeChanged);
    connect(mPanel, &ControlPanel::activeTerrainChanged,
            this, &MainWindow::onActiveTerrainChanged);
    connect(mCanvas, &MazeCanvas::cellClicked,
            this, &MainWindow::onCellClicked);
    connect(mCanvas, &MazeCanvas::animationFinished,
            this, &MainWindow::onAnimationFinished);
    connect(mPanel, &ControlPanel::speedChanged, [this](int ms) {
        if (ms < 0) mCanvas->stopAnimation();
        else mCanvas->setAnimateSpeed(ms);
    });
}

void MainWindow::setupMenuBar() {
    QMenu* fileMenu = menuBar()->addMenu("文件(&F)");

    QAction* openAct = fileMenu->addAction("打开迷宫 (.txt)");
    openAct->setShortcut(QKeySequence("Ctrl+O"));
    connect(openAct, &QAction::triggered, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "打开迷宫文件",
            "", "迷宫文件 (*.txt *.maze);;所有文件 (*)");
        if (!path.isEmpty()) {
            bool ext = path.endsWith(".maze");
            onLoadFile(path, ext);
        }
    });

    QAction* saveAct = fileMenu->addAction("保存扩展迷宫 (.maze)");
    saveAct->setShortcut(QKeySequence("Ctrl+S"));
    connect(saveAct, &QAction::triggered, this, [this]() {
        if (mMaze.getRows() == 0) return;
        QString path = QFileDialog::getSaveFileName(this, "保存扩展迷宫",
            "maze_export.maze", "迷宫文件 (*.maze)");
        if (!path.isEmpty()) onSaveFile(path);
    });

    fileMenu->addSeparator();

    QAction* quitAct = fileMenu->addAction("退出(&Q)");
    quitAct->setShortcut(QKeySequence("Ctrl+Q"));
    connect(quitAct, &QAction::triggered, this, &QWidget::close);

    QMenu* helpMenu = menuBar()->addMenu("帮助(&H)");
    QAction* aboutAct = helpMenu->addAction("关于");
    connect(aboutAct, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "关于",
            "迷宫求解系统 v2.0\n\n"
            "算法与数据结构B 期末作业\n"
            "功能：随机生成、加权地形、多入口/出口、关卡点\n"
            "算法：DFS / BFS / 0-1 BFS / Dijkstra / A*");
    });
}

void MainWindow::onRunSolver(int algorithm, int entryIdx, int exitIdx, bool animated) {
    if (mMaze.getRows() == 0) {
        statusBar()->showMessage("请先加载或生成迷宫!");
        return;
    }
    if (mMaze.getEntries().empty() || mMaze.getExits().empty()) {
        statusBar()->showMessage("迷宫缺少入口或出口!");
        return;
    }

    mCanvas->stopAnimation();
    mCanvas->clearAllPaths();
    statusBar()->showMessage("正在求解...");
    QApplication::processEvents();

    if (algorithm == 5) {
        // Compare mode — 有关卡时统一用关卡求解
        if (!mMaze.getCheckpoints().empty()) {
            Path cpPath = solveOptimalCheckpoints(mMaze, entryIdx, exitIdx);
            mCanvas->addComparePath(&cpPath, QColor(233, 69, 96), "关卡最优路径");
            mCanvas->update();
            mPanel->setStats("关卡对比完成",
                cpPath.steps, cpPath.turns, cpPath.totalCost, 1);
            statusBar()->showMessage("关卡对比完成 - 红色:关卡最优路径");
            return;
        }

        Path bfsPath = bfsFindShortestPath(mMaze, entryIdx, exitIdx);
        Path turnsPath = bfsFindFewestTurnsPath(mMaze, entryIdx, exitIdx);
        Path dijPath = dijkstraFindPath(mMaze, entryIdx, exitIdx);
        Path aPath = astarFindPath(mMaze, entryIdx, exitIdx);

        mCanvas->addComparePath(&bfsPath, QColor(233, 69, 96), "BFS最短");
        mCanvas->addComparePath(&turnsPath, QColor(78, 204, 163), "最少转弯");
        mCanvas->addComparePath(&dijPath, QColor(240, 165, 0), "Dijkstra");
        mCanvas->addComparePath(&aPath, QColor(83, 52, 131), "A*");
        mCanvas->update();

        mPanel->setStats("对比完成",
            bfsPath.steps, bfsPath.turns, dijPath.totalCost, 4);
        statusBar()->showMessage("综合对比完成 - 红:BFS 绿:转弯 金:Dijkstra 紫:A*");
        return;
    }

    runAlgorithm(algorithm, entryIdx, exitIdx, animated);
}

void MainWindow::runAlgorithm(int algo, int entryIdx, int exitIdx, bool animated) {
    Path result;

    // 如果有关卡点，先求解关卡顺序（最优排列），再跑指定算法
    if (!mMaze.getCheckpoints().empty()) {
        result = solveOptimalCheckpoints(mMaze, entryIdx, exitIdx);
        QStringList algoNames = {"DFS", "BFS", "0-1 BFS", "Dijkstra", "A*"};
        mPanel->setStats("关卡+" + algoNames[algo] + "完成",
                         result.steps, result.turns, result.totalCost, 1);
    } else {
        switch (algo) {
        case 0: { // DFS
            std::vector<Path> all = dfsFindAllPaths(mMaze, entryIdx, exitIdx, 50);
            if (!all.empty()) {
                result = all[0];
                for (const auto& p : all) {
                    if (p.steps < result.steps) result = p;
                }
            }
            mPanel->setStats("DFS完成", result.steps, result.turns, result.totalCost, (int)all.size());
            break;
        }
        case 1: // BFS
            result = bfsFindShortestPath(mMaze, entryIdx, exitIdx);
            mPanel->setStats("BFS完成", result.steps, result.turns, result.totalCost, 1);
            break;
        case 2: // 0-1 BFS
            result = bfsFindFewestTurnsPath(mMaze, entryIdx, exitIdx);
            mPanel->setStats("0-1 BFS完成", result.steps, result.turns, result.totalCost, 1);
            break;
        case 3: // Dijkstra
            result = dijkstraFindPath(mMaze, entryIdx, exitIdx);
            mPanel->setStats("Dijkstra完成", result.steps, result.turns, result.totalCost, 1);
            break;
        case 4: // A*
            result = astarFindPath(mMaze, entryIdx, exitIdx);
            mPanel->setStats("A*完成", result.steps, result.turns, result.totalCost, 1);
            break;
        }
    }

    if (result.points.empty()) {
        statusBar()->showMessage("未找到路径!");
        mPanel->setStats("无路径", -1, -1, -1, -1);
        return;
    }

    mCurrentPath = result;

    if (animated) {
        mCanvas->setPath(nullptr);
        QColor col = (algo == 3 || algo == 4) ? QColor(240, 165, 0)
                    : (algo == 2) ? QColor(78, 204, 163)
                    : QColor(233, 69, 96);
        mCanvas->animatePath(result, col);
        statusBar()->showMessage("动画播放中...");
    } else {
        QColor col = (algo == 3 || algo == 4) ? QColor(240, 165, 0)
                    : (algo == 2) ? QColor(78, 204, 163)
                    : QColor(233, 69, 96);
        mCanvas->setPath(&result, col);
        statusBar()->showMessage(QString("求解完成 - 步数:%1 转弯:%2 代价:%3")
            .arg(result.steps).arg(result.turns).arg(result.totalCost));
    }
}

void MainWindow::onGenerate(int rows, int cols, float wallDensity,
                             float grassP, float swampP, float waterP) {
    statusBar()->showMessage("正在生成迷宫...");
    QApplication::processEvents();

    mMaze = generateMazeWeighted(rows, cols, wallDensity, grassP, swampP, waterP);

    mCanvas->setMaze(&mMaze);
    mPanel->updateEntryExitCombos(mMaze);
    mPanel->clearStats();
    mCurrentPath = Path();

    statusBar()->showMessage(QString("迷宫生成完成 %1x%2").arg(mMaze.getRows()).arg(mMaze.getCols()));
}

void MainWindow::onLoadFile(const QString& path, bool extended) {
    bool ok;
    if (extended)
        ok = mMaze.loadExtendedFile(path.toLocal8Bit().constData());
    else
        ok = mMaze.loadFromFile(path.toLocal8Bit().constData());

    if (ok) {
        mCanvas->setMaze(&mMaze);
        mPanel->updateEntryExitCombos(mMaze);
        mPanel->clearStats();
        mCurrentPath = Path();
        statusBar()->showMessage("迷宫加载成功: " + path);
    } else {
        QMessageBox::warning(this, "加载失败", "无法加载迷宫文件:\n" + path);
        statusBar()->showMessage("加载失败");
    }
}

void MainWindow::onSaveFile(const QString& path) {
    if (mMaze.saveExtendedFile(path.toLocal8Bit().constData())) {
        statusBar()->showMessage("已保存: " + path);
    } else {
        QMessageBox::warning(this, "保存失败", "无法保存到:\n" + path);
    }
}

void MainWindow::onEditModeChanged(int mode) {
    mEditMode = mode;
    MazeCanvas::EditMode canvasMode;
    switch (mode) {
        case 1: canvasMode = MazeCanvas::SetTerrain; break;
        case 2: canvasMode = MazeCanvas::SetEntry; break;
        case 3: canvasMode = MazeCanvas::SetExit; break;
        case 4: canvasMode = MazeCanvas::SetCheckpoint; break;
        default: canvasMode = MazeCanvas::None; break;
    }
    mCanvas->setEditMode(canvasMode);
    statusBar()->showMessage(QString("编辑模式: %1").arg(
        QStringList{"查看", "地形", "入口", "出口", "关卡"}[mode]));
}

void MainWindow::onActiveTerrainChanged(int terrain) {
    mEditTerrain = static_cast<Terrain>(terrain);
    mCanvas->setActiveTerrain(mEditTerrain);
}

void MainWindow::onCellClicked(int x, int y) {
    switch (mEditMode) {
    case 1: // Set terrain
        mMaze.setTerrain(x, y, mEditTerrain);
        mCanvas->update();
        break;
    case 2: // Set entry
        mMaze.addEntry(Point(x, y));
        mPanel->updateEntryExitCombos(mMaze);
        mCanvas->update();
        statusBar()->showMessage(QString("已添加入口 (%1,%2)").arg(x).arg(y));
        break;
    case 3: // Set exit
        mMaze.addExit(Point(x, y));
        mPanel->updateEntryExitCombos(mMaze);
        mCanvas->update();
        statusBar()->showMessage(QString("已添加出口 (%1,%2)").arg(x).arg(y));
        break;
    case 4: // Set checkpoint
        mMaze.addCheckpoint(Point(x, y));
        mCanvas->update();
        statusBar()->showMessage(QString("已添加关卡 (%1,%2)").arg(x).arg(y));
        break;
    default:
        break;
    }
}

void MainWindow::onAnimationFinished() {
    QColor col = (mEditMode == 0) ? QColor(233, 69, 96) : QColor(240, 165, 0);
    mCanvas->setPath(&mCurrentPath, col);
    statusBar()->showMessage(QString("求解完成 - 步数:%1 转弯:%2 代价:%3")
        .arg(mCurrentPath.steps).arg(mCurrentPath.turns).arg(mCurrentPath.totalCost));
}
