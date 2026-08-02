#include "Clockface.h"


// Reconstructs the path to find the immediate next move
void Clockface::reconstructPath(Point start, Point end, Direction& nextMove) {
    if (start.x == end.x && start.y == end.y) return;
    Point current = end;
    Point prev = parent[current.x][current.y];

    // Trace back until we find the step immediately after the start
    while (!(prev.x == start.x && prev.y == start.y)) {
        current = prev;
        prev = parent[current.x][current.y];
         // Safety break in case something goes wrong
        if (current.x == -1 || current.y == -1) return;
    }

    // Determine direction from start to 'current' (the next step)
    if (current.x > start.x) nextMove = Direction::DOWN;
    else if (current.x < start.x) nextMove = Direction::UP;
    else if (current.y > start.y) nextMove = Direction::RIGHT;
    else if (current.y < start.y) nextMove = Direction::LEFT;
}


// Unified BFS using a goal predicate
bool Clockface::bfs(int startR, int startC, Direction& nextMove, std::function<bool(int,int)> isGoal) {
    // Initialize BFS structures
    queueFront = 0;
    queueRear = -1;
    for (int i = 0; i < MAP_SIZE; ++i) {
        for (int j = 0; j < MAP_SIZE; ++j) {
            visited[i][j] = false;
            parent[i][j] = {-1, -1};
        }
    }

    Point startPoint = {startR, startC};
    visited[startR][startC] = true;
    queue[++queueRear] = startPoint;

    while (queueFront <= queueRear) {
        Point current = queue[queueFront++];

        // Check if the current cell is a goal (excluding the starting cell)
        if (isGoal(current.x, current.y) && !(current.x == startPoint.x && current.y == startPoint.y)) {
            reconstructPath(startPoint, current, nextMove);
            return true;
        }

        // Explore neighbors in order UP, DOWN, LEFT, RIGHT
        for (int i = 0; i < 4; ++i) {
            int nextR = current.x + DIR_OFFSETS[i].dRow;
            int nextC = current.y + DIR_OFFSETS[i].dCol;

            if (isValid(nextR, nextC) && !visited[nextR][nextC]) {
                if (queueRear >= MAX_QUEUE_SIZE - 1) {
                    return false; // Prevent overflow
                }
                visited[nextR][nextC] = true;
                parent[nextR][nextC] = current;
                queue[++queueRear] = {nextR, nextC};
            }
        }
    }

    return false;
}

// Finds the shortest path to food using BFS
bool Clockface::findShortestPath(int startR, int startC, Direction& nextMove) {
    // In normal collection mode, clear regular pellets before taking a power
    // pellet. Keep power pellets as a fallback once the regular food is gone.
    if (bfs(startR, startC, nextMove,
            [this](int r, int c) { return isNormalFoodTarget(r, c); })) {
        return true;
    }
    return bfs(startR, startC, nextMove, [this](int r, int c) { return isTarget(r, c); });
}

bool Clockface::chooseFoodDirectionFromNeighbors(int currentMapR, int currentMapC, Direction& nextMove) {
    for (int i = 0; i < 4; ++i) {
        Direction tryDir = static_cast<Direction>(i);
        int nextR = currentMapR + DIR_OFFSETS[i].dRow;
        int nextC = currentMapC + DIR_OFFSETS[i].dCol;

        if (!isValid(nextR, nextC)) {
            continue;
        }
        if (!canMove(pacman->getX(), pacman->getY(), tryDir)) {
            continue;
        }

        Direction dummy = tryDir;
        if (findShortestPath(nextR, nextC, dummy)) {
            nextMove = tryDir;
            return true;
        }
    }
    return false;
}


bool Clockface::findPathTo(int startR, int startC, int targetR, int targetC, Direction& nextMove) {
    return bfs(startR, startC, nextMove, [=](int r, int c) { return r == targetR && c == targetC; });
}

// Finds the ghost nearest to (fromR, fromC) by Manhattan distance
// Returns ghost index (0-based) or -1 if no ghosts.
// Sets ghostR/ghostC to the grid position of the nearest ghost.
int Clockface::nearestGhost(int fromR, int fromC, int& ghostR, int& ghostC) {
    int bestIdx = -1;
    int nearestDist = 999;
    for (int i = 0; i < _ghostCount; i++) {
        int gr = pixelToGrid(ghosts[i]->getY());
        int gc = pixelToGrid(ghosts[i]->getX());
        int dist = abs(fromR - gr) + abs(fromC - gc);
        if (dist < nearestDist) {
            nearestDist = dist;
            bestIdx = i;
            ghostR = gr;
            ghostC = gc;
        }
    }
    return bestIdx;
}

// Super food reachability: BFS-limited to maxSteps, checks if super food is reachable (task 2.2)
bool Clockface::superFoodReachable(int entityR, int entityC, int maxSteps, Direction& nextMove) {
    // Quick check: no super food on the map
    if (countBlocks(MapBlock::SUPER_FOOD) == 0) {
        return false;
    }

    // BFS with step-count limiting
    queueFront = 0;
    queueRear = -1;
    for (int i = 0; i < MAP_SIZE; ++i) {
        for (int j = 0; j < MAP_SIZE; ++j) {
            visited[i][j] = false;
            parent[i][j] = {-1, -1};
        }
    }

    struct StepPoint {
        int r, c, dist;
    };
    // Use the shared queue for BFS (storing step distance inline in visited isn't enough)
    // We'll use a local approach: enqueue StepPoints but store them in queue + separate dist array
    int dist[MAP_SIZE][MAP_SIZE];

    visited[entityR][entityC] = true;
    dist[entityR][entityC] = 0;
    queue[++queueRear] = {entityR, entityC};

    while (queueFront <= queueRear) {
        Point current = queue[queueFront++];
        int currentDist = dist[current.x][current.y];

        if (static_cast<MapBlock>(_MAP[current.x][current.y]) == MapBlock::SUPER_FOOD &&
            !(current.x == entityR && current.y == entityC)) {
            reconstructPath({entityR, entityC}, current, nextMove);
            return true;
        }

        // A target exactly maxSteps away is reachable, but do not explore past it.
        if (currentDist >= maxSteps) continue;

        for (int i = 0; i < 4; ++i) {
            int nextR = current.x + DIR_OFFSETS[i].dRow;
            int nextC = current.y + DIR_OFFSETS[i].dCol;

            if (isValid(nextR, nextC) && !visited[nextR][nextC]) {
                if (queueRear >= MAX_QUEUE_SIZE - 1) {
                    return false;
                }
                visited[nextR][nextC] = true;
                dist[nextR][nextC] = currentDist + 1;
                parent[nextR][nextC] = current;
                queue[++queueRear] = {nextR, nextC};
            }
        }
    }

    return false;
}
