#include "Clockface.h"

const int Clockface::MAP_SIZE; // Definition for static member


Clockface::Clockface(Adafruit_GFX* display) {
  _display = display;
  Locator::provide(display);
  for (int i = 0; i < MAX_GHOSTS; i++) {
    ghosts[i] = nullptr;
    _ghostActive[i] = true;
    _ghostRespawnAt[i] = 0;
  }
}

Clockface::~Clockface() {
  if (pacman != nullptr) {
    delete pacman;
    pacman = nullptr;
  }
  for (int i = 0; i < MAX_GHOSTS; i++) {
    if (ghosts[i] != nullptr) {
      delete ghosts[i];
      ghosts[i] = nullptr;
    }
  }
  _ghostCount = 0;
}

void Clockface::setup(CWDateTime *dateTime) {
  this->_dateTime = dateTime;
  Locator::getDisplay()->setFont(&hourFont);
  randomSeed(dateTime->getMilliseconds() + millis());
  lastMillis = millis();
  lastMillisTime = millis();
  lastMillisSec = millis();
  memcpy(_MAP, _MAP_CONST, sizeof(_MAP_CONST));
  drawMap();
  updateClock();
}

void Clockface::update()
{

  // Seconds blink
  if ((millis() - lastMillisSec) >= 1000) {

    if (show_seconds) {
      Locator::getDisplay()->fillRect(31, 24, 2, 2, 0xFE40);
      Locator::getDisplay()->fillRect(31, 29, 2, 2, 0xFE40);
    } else  {
      Locator::getDisplay()->fillRect(31, 24, 2, 2, 0);
      Locator::getDisplay()->fillRect(31, 29, 2, 2, 0);
    }

    show_seconds = !show_seconds;
    lastMillisSec = millis();
  }

  // Clock
  if (millis() - lastMillisTime >= 60000) {

    updateClock();

    lastMillisTime = millis();
  }


  // Pacman & Ghost update interval (synchronized)
  if (millis() - lastMillis >= 75) {
    if (pacman != nullptr && _ghostCount > 0) {

      // 1. Pacman movement decision
      bool fullBlock = isAtGridJunction(pacman->getX(), pacman->getY());

      if (fullBlock) {
        int currentMapR = pixelToGrid(pacman->getY());
        int currentMapC = pixelToGrid(pacman->getX());

        // Eat food on current cell (invalidate cache — task 3.4)
        MapBlock currentBlockContent = static_cast<MapBlock>(_MAP[currentMapR][currentMapC]);
        _MAP[currentMapR][currentMapC] = MapBlock::EMPTY;

        bool ateAnyFood = (currentBlockContent == MapBlock::FOOD || currentBlockContent == MapBlock::SUPER_FOOD);
        if (ateAnyFood) {
          _pacmanPlan.active = false;
        }

        if (currentBlockContent == MapBlock::SUPER_FOOD) {
          pacman->setState(Pacman::State::INVENCIBLE);
        }

        // 3.5: Apply deferred cache invalidation at junction
        if (_pendingCacheInvalidation) {
          _pacmanPlan.active = false;
          _pendingCacheInvalidation = false;
        }

        // 3.4: Detect Pacman state change → invalidate cache
        if (pacman->_state != _lastPacmanState) {
          _pacmanPlan.active = false;
          _lastPacmanState = pacman->_state;
        }

        // 3.4: Detect ghost boundary crossing (enters/leaves ≤4 range) → invalidate cache
        int ghostsInRange = 0;
        for (int gi = 0; gi < _ghostCount; gi++) {
          int dist = manhattanToGhost(currentMapR, currentMapC, gi);
          if (dist <= 4) {
            ghostsInRange++;
          }
        }
        if (ghostsInRange != _lastGhostsInRange) {
          _pacmanPlan.active = false;
          _lastGhostsInRange = ghostsInRange;
        }

        // If normal food is immediately adjacent, drop any cached route and
        // re-evaluate so Pacman does not drift past a nearby pellet.
        if (_pacmanPlan.active && hasAdjacentNormalFood(currentMapR, currentMapC)) {
          _pacmanPlan.active = false;
        }

        // 3.3: Use cached plan if active — bypass tier evaluation,
        // but re-validate the cached direction at the current position
        // (the cell we cached for may be safe while the current cell is
        //  at a map boundary, e.g. RIGHT from c=11 → out of bounds).
        if (_pacmanPlan.active) {
          if (_pacmanPlan.nextDir != pacman->_direction) {
            int nextR = currentMapR + DIR_OFFSETS[static_cast<int>(_pacmanPlan.nextDir)].dRow;
            int nextC = currentMapC + DIR_OFFSETS[static_cast<int>(_pacmanPlan.nextDir)].dCol;
            if (isValid(nextR, nextC)) {
              if (canMove(pacman->getX(), pacman->getY(), _pacmanPlan.nextDir)) {
                pacman->turn(_pacmanPlan.nextDir);
              } else {
                _pacmanPlan.active = false;
              }
            } else {
              _pacmanPlan.active = false;
            }
          } else {
            // nextDir == currentDir: still re-validate at grid & pixel level.
            // Without this, a plan that points into a wall (because Pacman
            // arrived at a wall-adjacent junction) would not be caught until
            // the next wall-guard tick — one pixel after entering the wall.
            int nextR = currentMapR + DIR_OFFSETS[static_cast<int>(pacman->_direction)].dRow;
            int nextC = currentMapC + DIR_OFFSETS[static_cast<int>(pacman->_direction)].dCol;
            if (!isValid(nextR, nextC) ||
                !canMove(pacman->getX(), pacman->getY(), pacman->_direction)) {
              _pacmanPlan.active = false;
            }
          }
        }
        if (!_pacmanPlan.active) {
          // Three-tier Pacman decision logic (task 2.3)
          if (pacman->_state == Pacman::State::INVENCIBLE) {
            // Invincible: find and target nearest ghost
            int ghostR = 0, ghostC = 0;
            int nearestGhostIdx = nearestGhost(currentMapR, currentMapC, ghostR, ghostC);
            if (nearestGhostIdx >= 0) {
              Direction nextMove = pacman->_direction;
              bool pathToGhost = findPathTo(currentMapR, currentMapC, ghostR, ghostC, nextMove);
              if (pathToGhost && canMove(pacman->getX(), pacman->getY(), nextMove)) {
                if (nextMove != pacman->_direction) {
                  pacman->turn(nextMove);
                }
              } else {
                MapBlock nextBlk = nextBlock();
                directionDecision(nextBlk, (pacman->_direction == Direction::LEFT || pacman->_direction == Direction::RIGHT));
              }
            }
          } else {
            // Determine tier using effective distance (Task 4.2)
            int ghostR = 0, ghostC = 0;
            int nearestGhostIdx = nearestGhost(currentMapR, currentMapC, ghostR, ghostC);
            int nearestDist = (nearestGhostIdx >= 0) ? abs(currentMapR - ghostR) + abs(currentMapC - ghostC) : 999;

            bool tier1 = false;
            bool tier2 = false;
            if (nearestGhostIdx >= 0) {
              for (int gi = 0; gi < _ghostCount; gi++) {
                int gr = pixelToGrid(ghosts[gi]->getY());
                int gc = pixelToGrid(ghosts[gi]->getX());
                int rawDist = abs(currentMapR - gr) + abs(currentMapC - gc);
                int dirDot = ghostDirectionDotProduct(ghosts[gi]->_direction, currentMapR - gr, currentMapC - gc);
                int effectiveDist = rawDist - (dirDot > 0 ? 1 : 0) - (rawDist <= 2 ? 1 : 0);
                if (effectiveDist <= 3) tier1 = true;
                if (!tier1 && rawDist <= 4) tier2 = true;
              }
            }

            // TIER 1: Any ghost with effective distance ≤ 4 → multi-ghost flee (immediate danger)
            if (tier1) {
              // Task 5.2: Check escape plan cache
              bool useCachedEscape = false;
              Direction fleeDir = pacman->_direction;

              if (_pacmanPlan.escapePlanJunctionsLeft > 0) {
                // Check if any ghost moved ≥1 tile toward Pacman since cached
                bool ghostsChanged = false;
                for (int gi = 0; gi < _ghostCount && !ghostsChanged; gi++) {
                  int gr = pixelToGrid(ghosts[gi]->getY());
                  int gc = pixelToGrid(ghosts[gi]->getX());
                  int cachedR = _pacmanPlan.escapeGhostPositions[gi * 2];
                  int cachedC = _pacmanPlan.escapeGhostPositions[gi * 2 + 1];
                  if (cachedR >= 0) {
                    int oldDist = abs(currentMapR - cachedR) + abs(currentMapC - cachedC);
                    int newDist = abs(currentMapR - gr) + abs(currentMapC - gc);
                    if (newDist < oldDist) {
                      ghostsChanged = true;
                    }
                  }
                }
                if (!ghostsChanged) {
                  fleeDir = _pacmanPlan.escapeDir;
                  useCachedEscape = true;
                  _pacmanPlan.escapePlanJunctionsLeft--; // Task 5.3: Decrement junction counter
                } else {
                  _pacmanPlan.escapePlanJunctionsLeft = 0; // Task 5.3: Invalidate on ghost approach
                }
              }

              if (!useCachedEscape) {
                // Task 1.3: Allow U-turn in Tier 1 flee
                fleeDir = fleeDirectionMulti(currentMapR, currentMapC, pacman->_direction, /*allowUTurn=*/true);
              }

              fleeDir = preferFoodWhileFleeing(currentMapR, currentMapC, fleeDir, /*safetyMargin=*/1);

              int fleeNextR = currentMapR + DIR_OFFSETS[static_cast<int>(fleeDir)].dRow;
              int fleeNextC = currentMapC + DIR_OFFSETS[static_cast<int>(fleeDir)].dCol;
              if (isValid(fleeNextR, fleeNextC) && canMove(pacman->getX(), pacman->getY(), fleeDir)) {
                if (fleeDir != pacman->_direction) {
                  pacman->turn(fleeDir);
                }
                // Task 5.1: Cache freshly computed escape plan
                if (!useCachedEscape) {
                  _pacmanPlan.escapeDir = fleeDir;
                  _pacmanPlan.escapePlanCachedAt = millis();
                  for (int gi = 0; gi < _ghostCount; gi++) {
                    _pacmanPlan.escapeGhostPositions[gi * 2] = pixelToGrid(ghosts[gi]->getY());
                    _pacmanPlan.escapeGhostPositions[gi * 2 + 1] = pixelToGrid(ghosts[gi]->getX());
                  }
                  _pacmanPlan.escapePlanJunctionsLeft = 3;
                }
              } else {
                // Task 2.2: Flee direction blocked — use safestDirection() fallback
                Direction safeDir = safestDirection(currentMapR, currentMapC);
                safeDir = preferFoodWhileFleeing(currentMapR, currentMapC, safeDir, /*safetyMargin=*/1);
                if (safeDir != pacman->_direction &&
                    canMove(pacman->getX(), pacman->getY(), safeDir)) {
                  pacman->turn(safeDir);
                }
                // Also cache the fallback direction as an escape plan
                if (!useCachedEscape) {
                  _pacmanPlan.escapeDir = safeDir;
                  _pacmanPlan.escapePlanCachedAt = millis();
                  for (int gi = 0; gi < _ghostCount; gi++) {
                    _pacmanPlan.escapeGhostPositions[gi * 2] = pixelToGrid(ghosts[gi]->getY());
                    _pacmanPlan.escapeGhostPositions[gi * 2 + 1] = pixelToGrid(ghosts[gi]->getX());
                  }
                  _pacmanPlan.escapePlanJunctionsLeft = 3;
                }
              }
            }
            // TIER 2: Ghost within 6 tiles but none ≤ 4 (effective) → super food check (medium danger)
            else if (tier2) {
              Direction superMove = pacman->_direction;
              // Task 3.1: Increased BFS depth from 6 to 12
              bool superFound = superFoodReachable(currentMapR, currentMapC, 12, superMove);
              if (superFound && canMove(pacman->getX(), pacman->getY(), superMove)) {
                if (superMove != pacman->_direction) {
                  pacman->turn(superMove);
                }
              } else if (!superFound) {
                Direction fleeDir = fleeDirectionMulti(currentMapR, currentMapC, pacman->_direction);
                fleeDir = preferFoodWhileFleeing(currentMapR, currentMapC, fleeDir, /*safetyMargin=*/1);
                int nextR = currentMapR + DIR_OFFSETS[static_cast<int>(fleeDir)].dRow;
                int nextC = currentMapC + DIR_OFFSETS[static_cast<int>(fleeDir)].dCol;
                if (isValid(nextR, nextC) && canMove(pacman->getX(), pacman->getY(), fleeDir)) {
                  if (fleeDir != pacman->_direction) {
                    pacman->turn(fleeDir);
                  }
                } else {
                  // Task 2.3: Use safestDirection() in Tier 2 fallback
                  Direction safeDir = safestDirection(currentMapR, currentMapC);
                  if (safeDir != pacman->_direction &&
                      canMove(pacman->getX(), pacman->getY(), safeDir)) {
                    pacman->turn(safeDir);
                  }
                }
              }
            }
            // TIER 3: No ghost within 6 tiles → safe, BFS to food (cache if found — task 3.2)
            else {
              Direction nextMove = pacman->_direction;
              if (findShortestPath(currentMapR, currentMapC, nextMove) &&
                  canMove(pacman->getX(), pacman->getY(), nextMove)) {
                if (nextMove != pacman->_direction) {
                  pacman->turn(nextMove);
                }
                // 3.2: Initialize cache on valid BFS + pixel-level validation
                _pacmanPlan.active = true;
                _pacmanPlan.nextDir = nextMove;
                _pacmanPlan.targetR = currentMapR;
                _pacmanPlan.targetC = currentMapC;
                _pacmanPlan.cachedAt = millis();
              } else {
                MapBlock nextBlk = nextBlock();
                directionDecision(nextBlk, (pacman->_direction == Direction::LEFT || pacman->_direction == Direction::RIGHT));
              }
            }
          }
        }

        if (countBlocks(MapBlock::FOOD) == 0 && countBlocks(MapBlock::SUPER_FOOD) == 0) {
           resetMap();
           lastMillis = millis();
           return;
        }
      } else {
        // Not at grid junction — check for deferred cache invalidation (task 3.5)
        int currentR = pixelToGrid(pacman->getY());
        int currentC = pixelToGrid(pacman->getX());

        int nowGhostsInRange = 0;
        for (int gi = 0; gi < _ghostCount; gi++) {
          int dist = manhattanToGhost(currentR, currentC, gi);
          if (dist <= 6) nowGhostsInRange++;
        }
        if (nowGhostsInRange != _lastGhostsInRange) {
          _pendingCacheInvalidation = true;
          _lastGhostsInRange = nowGhostsInRange;
        }

        if (pacman->_state != _lastPacmanState) {
          _pendingCacheInvalidation = true;
          _lastPacmanState = pacman->_state;
        }

        // Wall guard: block movement if any part of the sprite would overlap a wall
        if (!canMove(pacman->getX(), pacman->getY(), pacman->_direction)) {
          _pacmanPlan.active = false;
          // Reverse direction to return to the last grid junction,
          // where the AI (with pixel-level canMove validation) will
          // choose a safe direction next tick.
          pacman->turn(oppositeDirection(pacman->_direction));
        }
      }

      // 2. Ghost respawn timer + movement decision for each active ghost
      for (int gi = 0; gi < _ghostCount; gi++) {
        if (!_ghostActive[gi] && _ghostRespawnAt[gi] != 0 && millis() >= _ghostRespawnAt[gi]) {
          respawnGhost(ghosts[gi]);
          _ghostActive[gi] = true;
          _ghostRespawnAt[gi] = 0;
        }
      }

      for (int gi = 0; gi < _ghostCount; gi++) {
        if (!_ghostActive[gi]) {
          continue;
        }
        Ghost* g = ghosts[gi];
        bool ghostFullBlock = isAtGridJunction(g->getX(), g->getY());
        if (ghostFullBlock) {
          ghostDirectionDecision(g);
        } else {
          // Wall guard: block movement if any part of the sprite would overlap a wall
          if (!canMove(g->getX(), g->getY(), g->_direction)) {
            // Reverse direction to return to the last grid junction
            g->turn(oppositeDirection(g->_direction));
          }
        }
      }

      // 3. Pre-move collision checks
      bool deathHappened = checkCollisions();

      if (!deathHappened) {
        int pacmanOldX = pacman->getX();
        int pacmanOldY = pacman->getY();

        int ghostOldX[MAX_GHOSTS], ghostOldY[MAX_GHOSTS];
        for (int gi = 0; gi < _ghostCount; gi++) {
          ghostOldX[gi] = ghosts[gi]->getX();
          ghostOldY[gi] = ghosts[gi]->getY();
        }

        pacman->update();

        unsigned long pacmanInvincibleTimeout = pacman->getInvincibleTimeout();
        for (int gi = 0; gi < _ghostCount; gi++) {
          if (!_ghostActive[gi]) {
            continue;
          }
          ghosts[gi]->updateWithInvincibleTimeout(pacmanInvincibleTimeout);
        }

        redrawFoodOverlap(pacmanOldX, pacmanOldY, pacman->getX(), pacman->getY());

        for (int gi = 0; gi < _ghostCount; gi++) {
          redrawFoodOverlap(ghostOldX[gi], ghostOldY[gi], ghosts[gi]->getX(), ghosts[gi]->getY());
        }

        // Post-move collision checks (ignore return — death already handled pre-move)
        checkCollisions();
      }
    }
    lastMillis = millis();
  }
}


const char* Clockface::weekDayName(int weekday) {
  strncpy(weekDayTemp, _weekDayWords + (weekday*4), 4);
  return weekDayTemp;
}

const char* Clockface::monthName(int month) {
  strncpy(monthTemp, _monthWords + ((month-1)*4), 4);
  return monthTemp;
}

// Helper function to check collisions between Pacman and ghosts
bool Clockface::checkCollisions() {
  bool deathHappened = false;
  for (int gi = 0; gi < _ghostCount; gi++) {
    if (!_ghostActive[gi]) {
      continue;
    }
    if (pacman->collidedWith(ghosts[gi])) {
      if (pacman->_state == Pacman::State::INVENCIBLE) {
        handleGhostEaten(ghosts[gi]);
      } else {
        handlePacmanDeath(ghosts[gi]);
        deathHappened = true;
        break;
      }
    }
  }
  return deathHappened;
}

// Helper function to get ghost grid position
void Clockface::ghostGridPosition(int ghostIndex, int &gridR, int &gridC) {
  if (ghostIndex >= 0 && ghostIndex < _ghostCount) {
    gridR = pixelToGrid(ghosts[ghostIndex]->getY());
    gridC = pixelToGrid(ghosts[ghostIndex]->getX());
  } else {
    gridR = 0;
    gridC = 0;
  }
}

// Helper function to calculate Manhattan distance to a ghost
int Clockface::manhattanToGhost(int entityR, int entityC, int ghostIndex) {
  int ghostR, ghostC;
  ghostGridPosition(ghostIndex, ghostR, ghostC);
  return abs(entityR - ghostR) + abs(entityC - ghostC);
}



void Clockface::updateClock() {

    Locator::getDisplay()->fillRect(14, 19, 36, 26, 0x0000);

    Locator::getDisplay()->setFont(&Picopixel);
    Locator::getDisplay()->setTextColor(0xAD55);
    Locator::getDisplay()->setCursor(15, 41);
    Locator::getDisplay()->print(monthName(this->_dateTime->getMonth()));
    Locator::getDisplay()->print(" ");
    Locator::getDisplay()->print(this->_dateTime->getDay());
    Locator::getDisplay()->print(" ");
    Locator::getDisplay()->print(weekDayName(this->_dateTime->getWeekday()));

    Locator::getDisplay()->setFont(&hourFont);

    Locator::getDisplay()->setTextColor(0xFE40);
    Locator::getDisplay()->setCursor(15, 28);

    Locator::getDisplay()->print(this->_dateTime->getHour("00"));
    Locator::getDisplay()->print(" ");
    Locator::getDisplay()->print(this->_dateTime->getMinute("00"));
}


// Modified direction decision logic
void Clockface::directionDecision(MapBlock nextBlk, bool moving_axis_x) {

    int currentMapR = pixelToGrid(pacman->getY());
    int currentMapC = pixelToGrid(pacman->getX());
    Direction nextMove = pacman->_direction; // Default to current direction

    bool bfsOk = findShortestPath(currentMapR, currentMapC, nextMove);
    bool pixelOk = bfsOk && canMove(pacman->getX(), pacman->getY(), nextMove);

    if (pixelOk) {
        // Path found and pixel-validated, turn Pacman if needed
        if (nextMove != pacman->_direction) {
            pacman->turn(nextMove);
        }
    } else {
        // BFS direction is blocked or path not found. Try a reachable food direction
        // from the neighboring cells before using the fallback.
        if (chooseFoodDirectionFromNeighbors(currentMapR, currentMapC, nextMove)) {
            if (nextMove != pacman->_direction) {
                pacman->turn(nextMove);
            }
            return;
        }

        if (!contains(nextBlock(), PACMAN_MOVING_BLOCKS)) {
            turnRandom();
        }
        MapBlock immediateNext = nextBlock();
        if (contains(immediateNext, PACMAN_BLOCKING_BLOCKS)) {
            turnRandom();
        }
    }
}
