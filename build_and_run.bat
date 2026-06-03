@echo off
cd /d "%~dp0"
echo Compiling...
g++ -std=c++17 -Wall -O2 main.cpp maze.cpp solver.cpp evaluator.cpp -o maze_solver.exe
if %errorlevel% equ 0 (
    echo Done! Running...
    echo.
    maze_solver.exe
) else (
    echo Compilation failed!
)
pause
