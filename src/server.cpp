#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define closesocket close
#endif

#include "maze.h"
#include "solver.h"
#include "evaluator.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <sstream>

Maze g_maze;
bool g_loaded = false;

std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::string jsonEscape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}

std::string pathToJson(const Path& p) {
    std::ostringstream js;
    js << "{\"steps\":" << p.steps << ",\"turns\":" << p.turns << ",\"points\":[";
    for (size_t i = 0; i < p.points.size(); i++) {
        if (i) js << ",";
        js << "[" << p.points[i].x << "," << p.points[i].y << "]";
    }
    js << "]}";
    return js.str();
}

std::string mazeToJson() {
    std::ostringstream js;
    js << "{\"rows\":" << g_maze.getRows()
       << ",\"cols\":" << g_maze.getCols()
       << ",\"entry\":[" << g_maze.getEntry().x << "," << g_maze.getEntry().y << "]"
       << ",\"exit\":[" << g_maze.getExit().x << "," << g_maze.getExit().y << "]"
       << ",\"grid\":[";
    const auto& g = g_maze.getGrid();
    for (int i = 0; i < g_maze.getRows(); i++) {
        if (i) js << ",";
        js << "[";
        for (int j = 0; j < g_maze.getCols(); j++) {
            if (j) js << ",";
            js << (isWalkable(g[i][j]) ? 0 : 1);
        }
        js << "]";
    }
    js << "]}";
    return js.str();
}

std::string handleApi(const std::string& path, const std::string& body) {
    std::ostringstream resp;

    if (path == "/api/maze") {
        if (!g_loaded) return "{\"error\":\"no maze loaded\"}";
        return mazeToJson();
    }

    if (path == "/api/load") {
        std::string file = body;
        size_t p = file.find("\"file\"");
        if (p != std::string::npos) {
            p = file.find("\"", p + 6) + 1;
            size_t e = file.find("\"", p);
            file = file.substr(p, e - p);
        }
        bool ok = g_maze.loadFromFile(file);
        g_loaded = ok;
        return ok ? mazeToJson() : "{\"error\":\"failed to load\"}";
    }

    if (path == "/api/solve/dfs") {
        if (!g_loaded) return "{\"error\":\"no maze loaded\"}";
        auto paths = dfsFindAllPaths(g_maze);
        resp << "{\"paths\":[";
        int show = std::min((int)paths.size(), 50);
        for (int i = 0; i < show; i++) {
            if (i) resp << ",";
            resp << pathToJson(paths[i]);
        }
        resp << "],\"total\":" << paths.size() << "}";
        return resp.str();
    }

    if (path == "/api/solve/bfs") {
        if (!g_loaded) return "{\"error\":\"no maze loaded\"}";
        Path p = bfsFindShortestPath(g_maze);
        return p.points.empty() ? "{\"error\":\"no path\"}" : pathToJson(p);
    }

    if (path == "/api/solve/fewest-turns") {
        if (!g_loaded) return "{\"error\":\"no maze loaded\"}";
        Path p = bfsFindFewestTurnsPath(g_maze);
        return p.points.empty() ? "{\"error\":\"no path\"}" : pathToJson(p);
    }

    if (path == "/api/evaluate") {
        if (!g_loaded) return "{\"error\":\"no maze loaded\"}";
        EvalResult r = evaluatePath(g_maze);
        resp << "{\"totalPaths\":" << r.totalPaths
             << ",\"shortest\":" << pathToJson(r.shortestPath)
             << ",\"fewestTurns\":" << pathToJson(r.fewestTurnsPath) << "}";
        return resp.str();
    }

    return "{\"error\":\"unknown api\"}";
}

std::string getContentType(const std::string& path) {
    if (path.rfind(".html") != std::string::npos) return "text/html; charset=utf-8";
    if (path.rfind(".css") != std::string::npos) return "text/css; charset=utf-8";
    if (path.rfind(".js") != std::string::npos) return "application/javascript; charset=utf-8";
    if (path.rfind(".json") != std::string::npos) return "application/json; charset=utf-8";
    return "text/plain; charset=utf-8";
}

void handleRequest(SOCKET client) {
    char buf[8192] = {};
    int n = recv(client, buf, sizeof(buf) - 1, 0);
    if (n <= 0) { closesocket(client); return; }
    buf[n] = 0;

    std::string req(buf);
    std::string method, path, body;

    std::istringstream rs(req);
    rs >> method >> path;

    if (method.empty()) { closesocket(client); return; }

    // find body after \r\n\r\n
    size_t hdrEnd = req.find("\r\n\r\n");
    if (hdrEnd != std::string::npos) {
        body = req.substr(hdrEnd + 4);
    }

    std::string response;

    if (path == "/" || path == "/index.html") {
        std::string html = readFile("static/index.html");
        response = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: "
                 + std::to_string(html.size()) + "\r\nConnection: close\r\n\r\n" + html;
    } else if (path.rfind("/api/", 0) == 0) {
        std::string json = handleApi(path, body);
        response = "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: "
                 + std::to_string(json.size()) + "\r\nConnection: close\r\nAccess-Control-Allow-Origin: *\r\n\r\n" + json;
    } else {
        std::string file = readFile("static" + path);
        if (file.empty()) {
            response = std::string("HTTP/1.1 404 Not Found\r\nContent-Length: 13\r\nConnection: close\r\n\r\n404 Not Found");
        } else {
            std::string ct = getContentType(path);
            response = "HTTP/1.1 200 OK\r\nContent-Type: " + ct + "\r\nContent-Length: "
                     + std::to_string(file.size()) + "\r\nConnection: close\r\n\r\n" + file;
        }
    }

    send(client, response.c_str(), response.size(), 0);
    closesocket(client);
}

int main() {
    SetConsoleOutputCP(CP_UTF8);

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }
#endif

    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == INVALID_SOCKET) {
        std::cerr << "socket failed" << std::endl;
        return 1;
    }

    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "bind failed" << std::endl;
        return 1;
    }

    listen(server, 5);
    std::cout << "\n  迷宫求解 Web 服务器已启动!\n";
    std::cout << "  请打开浏览器访问: http://localhost:8080\n";
    std::cout << "  按 Ctrl+C 停止服务器\n" << std::endl;

    while (true) {
        SOCKET client = accept(server, nullptr, nullptr);
        if (client != INVALID_SOCKET) {
            handleRequest(client);
        }
    }

    closesocket(server);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
