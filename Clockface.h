#pragma once

#include <Arduino.h>
#include <functional>


#include "hour_font.h"
#include "../cw-commons/picopixel.h"

#include <Adafruit_GFX.h>
#include <Tile.h>
#include <Locator.h>
#include <Game.h>
#include <Object.h>
#include <ImageUtils.h>
#include <ColorUtil.h>
#include "IClockface.h"

//sprites
#include "pacman.h"
#include "ghost.h"


// Simple coordinate struct for BFS
struct Point {
  int x, y;
};

// Coordinate conversion utilities
static constexpr int pixelToGrid(int p) {
    // Floor division: C++ truncates toward zero, so (p-2)/5 for p<2
    // gives 0 instead of -1.  Subtract 4 from negative dividends to
    // make truncation equal floor for /5.
    int val = p - 2;
    return val >= 0 ? val / 5 : (val - 4) / 5;
}
static constexpr int gridToPixel(int g) { return (g * 5) + 2; }

// Direction utility
constexpr Direction oppositeDirection(Direction dir) {
    switch (dir) {
        case Direction::UP: return Direction::DOWN;
        case Direction::DOWN: return Direction::UP;
        case Direction::LEFT: return Direction::RIGHT;
        case Direction::RIGHT: return Direction::LEFT;
    }
    return Direction::RIGHT; // fallback
}

// Direction offset lookup — matches Direction enum order:
//   Direction::RIGHT=0, LEFT=1, UP=2, DOWN=3
struct DirOffset { int dRow, dCol; };
static constexpr DirOffset DIR_OFFSETS[4] = {
    {0, 1},   // RIGHT  (index 0)
    {0, -1},  // LEFT   (index 1)
    {-1, 0},  // UP     (index 2)
    {1, 0}    // DOWN   (index 3)
};

// Cached path plan for Pacman (task 3.1)
struct PacmanPlan {
  bool active = false;         // Is the cache valid?
  Direction nextDir;           // Direction to turn at the next junction
  int targetR = -1, targetC = -1; // The grid target we're heading to
  unsigned long cachedAt = 0;  // millis() when cached

  // Flee path cache — Tier 1 escape plan (tasks 1.1, 5.1-5.3)
  Direction escapeDir;                         // Cached escape direction
  int escapeGhostPositions[4] = {-1, -1, -1, -1}; // Ghost positions at cache time (r0,c0,r1,c1)
  int escapePlanJunctionsLeft = 0;             // Validity counter (decrements each junction, max 3)
  unsigned long escapePlanCachedAt = 0;        // millis() when escape plan was cached
};


class Clockface: public IClockface {
  private:
    static const int MAP_SIZE = 12;
    static const int MAX_GHOSTS = 2;
    static const unsigned long GHOST_RESPAWN_DELAY_MS = 3000;
    Adafruit_GFX* _display;
    CWDateTime* _dateTime;
    bool pacmanState = true;
    bool show_seconds = true;

    unsigned long lastMillis = 0;
    unsigned long lastMillisTime = 0;
    unsigned long lastMillisSec = 0;

    Pacman* pacman = nullptr;
    Ghost* ghosts[MAX_GHOSTS];
    bool _ghostActive[MAX_GHOSTS];
    unsigned long _ghostRespawnAt[MAX_GHOSTS];
    int _ghostCount = 0;

    const char* _weekDayWords = "SUN\0MON\0TUE\0WED\0THU\0FRI\0SAT\0";
    const char* _monthWords = "JAN\0FEB\0MAR\0APR\0MAY\0JUN\0JUL\0AUG\0SEP\0OCT\0NOV\0DEC\0";
    char weekDayTemp[4]= "\0";
    char monthTemp[4]= "\0";

   // Shared wall collision helpers
   bool canMove(int x, int y, Direction dir);
   static bool isAtGridJunction(int x, int y);

   // BFS related members
   static const int MAX_QUEUE_SIZE = MAP_SIZE * MAP_SIZE;
   Point queue[MAX_QUEUE_SIZE];
   int queueFront, queueRear;
   bool visited[MAP_SIZE][MAP_SIZE];
   Point parent[MAP_SIZE][MAP_SIZE]; // Stores the predecessor point in the path

   // Path caching (task 3.1)
   PacmanPlan _pacmanPlan;
   // Track whether cache invalidation is pending (deferred between cells, task 3.5)
   bool _pendingCacheInvalidation = false;
   // Track count of ghosts within 6 tiles to detect boundary crossings (task 3.4)
   int _lastGhostsInRange = 0;
   // Track previous Pacman state to detect state changes (task 3.4)
   Pacman::State _lastPacmanState = Pacman::State::MOVING;



    enum MapBlock {
      EMPTY = 0,
      FOOD = 1,
      WALL = 2,
      GATE = 3,
      SUPER_FOOD = 4,
      CLOCK = 5,
      GHOST = 6,
      PACMAN = 7,
      OUT_OF_MAP = 99
    };


    const byte _MAP_CONST[12][12] = {
      {4,1,1,1,1,1,7,1,1,1,1,4},
      {1,2,2,1,2,2,2,2,1,2,2,1},
      {1,1,1,1,1,2,2,1,1,1,1,1},
      {2,1,5,5,5,5,5,5,5,5,1,2},
      {2,1,5,5,5,5,5,5,5,5,1,2},
      {1,1,5,5,5,5,5,5,5,5,1,1},
      {1,1,5,5,5,5,5,5,5,5,1,1},
      {2,1,5,5,5,5,5,5,5,5,1,2},
      {2,1,5,5,5,5,5,5,5,5,1,2},
      {1,1,1,1,1,2,2,1,1,1,1,1},
      {1,2,2,1,2,2,2,2,1,2,2,1},
      {4,1,1,1,1,6,1,1,6,1,1,4}
    };


    byte _MAP[12][12] = {
      {4,1,1,1,1,1,7,1,1,1,1,4},
      {1,2,2,1,2,2,2,2,1,2,2,1},
      {1,1,1,1,1,2,2,1,1,1,1,1},
      {2,1,5,5,5,5,5,5,5,5,1,2},
      {2,1,5,5,5,5,5,5,5,5,1,2},
      {1,1,5,5,5,5,5,5,5,5,1,1},
      {1,1,5,5,5,5,5,5,5,5,1,1},
      {2,1,5,5,5,5,5,5,5,5,1,2},
      {2,1,5,5,5,5,5,5,5,5,1,2},
      {1,1,1,1,1,2,2,1,1,1,1,1},
      {1,2,2,1,2,2,2,2,1,2,2,1},
      {4,1,1,1,1,6,1,1,6,1,1,4}
    };

    const byte MAP_BORDER_SIZE = 2;
    const byte MAP_MIN_POS = 0 + MAP_BORDER_SIZE;
    const byte MAP_MAX_POS = 64 - MAP_BORDER_SIZE;

    // first elem is the size
    const int PACMAN_MOVING_BLOCKS[4] = {3, MapBlock::EMPTY, MapBlock::FOOD, MapBlock::GATE};
    const int PACMAN_BLOCKING_BLOCKS[4] = {3, MapBlock::OUT_OF_MAP, MapBlock::WALL, MapBlock::CLOCK};

    void drawMap();
    Clockface::MapBlock nextBlock(Direction dir);
    Clockface::MapBlock nextBlock();
    void turnRandom();
    int countBlocks(Clockface::MapBlock elem);
    bool contains(int v, const int* values);
    void resetMap();
    void directionDecision(MapBlock nextBlk, bool moving_axis_x);
    bool isValid(int r, int c); // Check if a cell is valid for BFS traversal
    bool isTarget(int r, int c); // Check if a cell contains food/superfood
    bool isNormalFoodTarget(int r, int c); // Check if a cell contains normal food
    bool hasAdjacentNormalFood(int r, int c); // Check for normal food in the 4-neighborhood
    bool findShortestPath(int startX, int startY, Direction& nextMove); // BFS implementation
    bool findPathTo(int startR, int startC, int targetR, int targetC, Direction& nextMove);
    bool chooseFoodDirectionFromNeighbors(int currentMapR, int currentMapC, Direction& nextMove);
    bool bfs(int startR, int startC, Direction& nextMove, std::function<bool(int,int)> isGoal); // Unified BFS
    void reconstructPath(Point start, Point end, Direction& nextMove); // Determine next move from path
    int nearestGhost(int fromR, int fromC, int& ghostR, int& ghostC);
    Direction fleeDirection(int entityR, int entityC, Direction currentDir, int threatR, int threatC, bool allowUTurn = false);
    Direction fleeDirectionMulti(int entityR, int entityC, Direction currentDir, bool allowUTurn = false);
    int directionSafetyScore(int entityR, int entityC, Direction dir);
    Direction preferFoodWhileFleeing(int entityR, int entityC, Direction fleeDir, int safetyMargin = 1);
    Direction safestDirection(int entityR, int entityC);
    int ghostDirectionDotProduct(Direction ghostDir, int dr, int dc);
    bool superFoodReachable(int entityR, int entityC, int maxSteps, Direction& nextMove);
    void ghostDirectionDecision(Ghost* g);
    void pinkyAmbushTarget(int pacmanR, int pacmanC, Direction pacmanDir, int& targetR, int& targetC);
    void handlePacmanDeath(Ghost* g);
    void handleGhostEaten(Ghost* g);
    void scheduleGhostRespawn(Ghost* g);
    void respawnGhost(Ghost* g);
    void drawFoodTile(int r, int c);
    void redrawFoodOverlap(int oldX, int oldY, int newX, int newY);
    void redrawFoodAt(int x, int y);
    void updateClock();
    const char* weekDayName(int weekday);
    const char* monthName(int month);

    // Helper functions for common patterns
    bool checkCollisions();
    void ghostGridPosition(int ghostIndex, int &gridR, int &gridC);
    int manhattanToGhost(int entityR, int entityC, int ghostIndex);
    
    
  public:
    Clockface(Adafruit_GFX* display);
    ~Clockface();
    void setup(CWDateTime *dateTime);
    void update();
};
