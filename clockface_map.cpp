#include "Clockface.h"

void Clockface::resetMap() {

  memcpy( _MAP, _MAP_CONST, sizeof(_MAP_CONST) );
  drawMap();
  updateClock();
}


int Clockface::countBlocks(Clockface::MapBlock elem) {
  int count = 0;
  for (int i = 0; i<MAP_SIZE; i++) {
    for (int j = 0; j<MAP_SIZE; j++) {
      if (_MAP[i][j] == elem)
        count++;
    }
  }

  return count;
}


void Clockface::turnRandom() {
  for (int i = 0; i < 4; i++) {
    Direction tryDir = static_cast<Direction>((random(4) + i) % 4);
    MapBlock nextBlk = nextBlock(tryDir);
    if (contains(nextBlk, PACMAN_MOVING_BLOCKS) && canMove(pacman->getX(), pacman->getY(), tryDir)) {
      pacman->turn(tryDir);
      Serial.print("New direction: ");
      Serial.println(pacman->_direction);
      return;
    }
  }
}


Clockface::MapBlock Clockface::nextBlock() {
  return nextBlock(pacman->_direction);
}

Clockface::MapBlock Clockface::nextBlock(Direction dir) {

  Clockface::MapBlock map_block = Clockface::MapBlock::OUT_OF_MAP;

  if (dir == Direction::RIGHT) {
    if (pacman->getX()+pacman->SPRITE_SIZE < MAP_MAX_POS) {
      map_block = static_cast<MapBlock>(_MAP[pixelToGrid(pacman->getY())][pixelToGrid(pacman->getX())+1]);
    }

  } else if (dir == Direction::DOWN) {
    if (pacman->getY()+pacman->SPRITE_SIZE < MAP_MAX_POS) {
      map_block = static_cast<MapBlock>(_MAP[pixelToGrid(pacman->getY())+1][pixelToGrid(pacman->getX())]);
    }
  } else if (dir == Direction::LEFT) {

    if ((pacman->getX()-2) >= 0) {
      map_block = static_cast<MapBlock>(_MAP[pixelToGrid(pacman->getY())][pixelToGrid(pacman->getX())-1]);
    }

  } else if (dir == Direction::UP) {
    if ((pacman->getY()-2) > 0) {
      map_block = static_cast<MapBlock>(_MAP[pixelToGrid(pacman->getY())-1][pixelToGrid(pacman->getX())]);
    }
  }

  return map_block;

}

bool Clockface::contains(int v, const int* values) {

  for (int i = 1; i<values[0]+1; i++) {
    if (v == values[i])
      return true;
  }

  return false;
}

void Clockface::drawMap()
{
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

  Locator::getDisplay()->fillRect(0, 0, 64, 64, 0x0000);

  uint16_t food_color = 0xB58C;
  uint16_t wall_color = 0x0016;
  uint16_t spcfood_color = 0xFBE0;

  Locator::getDisplay()->drawRect(0,0,64,64,wall_color);
  Locator::getDisplay()->drawRect(1,1,62,62,wall_color);

  for (int i=0; i<MAP_SIZE; i++) {
    for (int j=0; j<MAP_SIZE; j++) {
      if (_MAP[j][i] == MapBlock::FOOD) {
        Locator::getDisplay()->fillRect((i*5)+3,(j*5)+4,3,1,food_color);
      } else if (_MAP[j][i] == MapBlock::WALL) {
        Locator::getDisplay()->fillRect(gridToPixel(i),gridToPixel(j),5,5,wall_color);
      } else if (_MAP[j][i] == MapBlock::CLOCK) {
        Locator::getDisplay()->fillRect(gridToPixel(i),gridToPixel(j),5,5,wall_color);
      } else if (_MAP[j][i] == MapBlock::GATE) {
        //Locator::getDisplay()->fillRect((i*5)+((bool)i*2),gridToPixel(j),7,5,0x0000);
        Locator::getDisplay()->fillRect((i*5)+3,(j*5)+4,3,1,food_color);
      } else if (_MAP[j][i] == MapBlock::SUPER_FOOD) {
        Locator::getDisplay()->fillRect((i*5)+3,(j*5)+3,3,3,spcfood_color);
      } else if (_MAP[j][i] == MapBlock::PACMAN) {
        pacman = new Pacman(gridToPixel(i),gridToPixel(j));
        _MAP[j][i] = MapBlock::EMPTY;
      } else if (_MAP[j][i] == MapBlock::GHOST) {
        uint16_t ghostColors[MAX_GHOSTS] = {0xF800, 0x07FF}; // Blinky red, Pinky cyan
        if (_ghostCount < MAX_GHOSTS) {
          ghosts[_ghostCount] = new Ghost(gridToPixel(i), gridToPixel(j), ghostColors[_ghostCount]);
          ghosts[_ghostCount]->_personality = static_cast<Ghost::Personality>(_ghostCount);
          ghosts[_ghostCount]->init();
          _ghostCount++;
        }
        _MAP[j][i] = MapBlock::EMPTY;
      }
    }
  }
}

void Clockface::handlePacmanDeath(Ghost* g) {
    // Invalidate all caches on death so tier decision recalculates from spawn position
    _pacmanPlan.active = false;
    _pacmanPlan.escapePlanJunctionsLeft = 0;
    _pendingCacheInvalidation = false;
    _lastPacmanState = pacman->_state;
    _lastGhostsInRange = -1; // Forces a re-count on next tick

    int px = pacman->getX();
    int py = pacman->getY();
    int gx = g->getX();
    int gy = g->getY();

    Locator::getDisplay()->fillRect(px, py, 5, 5, 0);
    Locator::getDisplay()->fillRect(gx, gy, 5, 5, 0);

    pacman->resetToStart();
    scheduleGhostRespawn(g);

    redrawFoodAt(px, py);
    redrawFoodAt(gx, gy);
}

void Clockface::handleGhostEaten(Ghost* g) {
    int gx = g->getX();
    int gy = g->getY();

    Locator::getDisplay()->fillRect(gx, gy, 5, 5, 0);

    scheduleGhostRespawn(g);

    redrawFoodAt(gx, gy);
}

void Clockface::scheduleGhostRespawn(Ghost* g) {
    for (int gi = 0; gi < _ghostCount; gi++) {
        if (ghosts[gi] == g) {
            _ghostActive[gi] = false;
            _ghostRespawnAt[gi] = millis() + GHOST_RESPAWN_DELAY_MS;
            return;
        }
    }
}

void Clockface::respawnGhost(Ghost* g) {
    Point validPositions[MAP_SIZE * MAP_SIZE];
    int validCount = 0;

    int pacmanR = pixelToGrid(pacman->getY());
    int pacmanC = pixelToGrid(pacman->getX());

    for (int r = 0; r < MAP_SIZE; r++) {
        for (int c = 0; c < MAP_SIZE; c++) {
            if (isValid(r, c)) {
                // Check if any other ghost occupies this tile
                bool otherGhostHere = false;
                for (int gi = 0; gi < _ghostCount; gi++) {
                    if (ghosts[gi] == g) continue; // Skip self
                    int gr = pixelToGrid(ghosts[gi]->getY());
                    int gc = pixelToGrid(ghosts[gi]->getX());
                    if (gr == r && gc == c) {
                        otherGhostHere = true;
                        break;
                    }
                }
                if (otherGhostHere) continue;

                // Avoid spawning directly on Pacman or too close to him (within 2 tiles) to avoid instant death/collision
                if (abs(r - pacmanR) + abs(c - pacmanC) > 2) {
                    validPositions[validCount++] = {r, c};
                }
            }
        }
    }

    // Fallback: If no position is found far enough, avoid only Pacman's exact cell and other ghosts
    if (validCount == 0) {
        for (int r = 0; r < MAP_SIZE; r++) {
            for (int c = 0; c < MAP_SIZE; c++) {
                if (isValid(r, c)) {
                    bool otherGhostHere = false;
                    for (int gi = 0; gi < _ghostCount; gi++) {
                        if (ghosts[gi] == g) continue;
                        int gr = pixelToGrid(ghosts[gi]->getY());
                        int gc = pixelToGrid(ghosts[gi]->getX());
                        if (gr == r && gc == c) {
                            otherGhostHere = true;
                            break;
                        }
                    }
                    if (otherGhostHere) continue;
                    if (r != pacmanR || c != pacmanC) {
                        validPositions[validCount++] = {r, c};
                    }
                }
            }
        }
    }

    // Ultimate fallback: Just spawn on Pacman's position if there are absolutely no other valid cells
    if (validCount == 0) {
        validPositions[validCount++] = {pacmanR, pacmanC};
    }

    int index = random(validCount);
    Point spawnPoint = validPositions[index];

    int spawnX = gridToPixel(spawnPoint.y);
    int spawnY = gridToPixel(spawnPoint.x);

    // Determine a valid initial direction for the ghost
    Direction initialDir = Direction::LEFT;
    for (int i = 0; i < 4; i++) {
        if (isValid(spawnPoint.x + DIR_OFFSETS[i].dRow, spawnPoint.y + DIR_OFFSETS[i].dCol)) {
            initialDir = static_cast<Direction>(i);
            break;
        }
    }

    g->resetToPosition(spawnX, spawnY);
    g->turn(initialDir);
}

void Clockface::drawFoodTile(int r, int c) {
  if (_MAP[r][c] == MapBlock::FOOD) {
    uint16_t food_color = 0xB58C;
    int fx = (c * 5) + 3;
    int fy = (r * 5) + 4;
    for (int dx = 0; dx < 3; dx++) {
      int px = fx + dx;
      int py = fy;
      bool pacmanOverlap = (pacman != nullptr) && (px >= pacman->getX() && px < pacman->getX() + 5 && py >= pacman->getY() && py < pacman->getY() + 5);
      bool ghostOverlap = false;
      for (int gi = 0; gi < _ghostCount && !ghostOverlap; gi++) {
        ghostOverlap = (px >= ghosts[gi]->getX() && px < ghosts[gi]->getX() + 5 && py >= ghosts[gi]->getY() && py < ghosts[gi]->getY() + 5);
      }
      if (!pacmanOverlap && !ghostOverlap) {
        Locator::getDisplay()->drawPixel(px, py, food_color);
      }
    }
  } else if (_MAP[r][c] == MapBlock::SUPER_FOOD) {
    uint16_t spcfood_color = 0xFBE0;
    int fx = (c * 5) + 3;
    int fy = (r * 5) + 3;
    for (int dy = 0; dy < 3; dy++) {
      for (int dx = 0; dx < 3; dx++) {
        int px = fx + dx;
        int py = fy + dy;
        bool pacmanOverlap = (pacman != nullptr) && (px >= pacman->getX() && px < pacman->getX() + 5 && py >= pacman->getY() && py < pacman->getY() + 5);
        bool ghostOverlap = false;
        for (int gi = 0; gi < _ghostCount && !ghostOverlap; gi++) {
          ghostOverlap = (px >= ghosts[gi]->getX() && px < ghosts[gi]->getX() + 5 && py >= ghosts[gi]->getY() && py < ghosts[gi]->getY() + 5);
        }
        if (!pacmanOverlap && !ghostOverlap) {
          Locator::getDisplay()->drawPixel(px, py, spcfood_color);
        }
      }
    }
  }
}

void Clockface::redrawFoodOverlap(int oldX, int oldY, int newX, int newY) {
  int r_start1 = pixelToGrid(oldY);
  int r_end1 = (oldY + 2) / 5;
  int c_start1 = pixelToGrid(oldX);
  int c_end1 = (oldX + 2) / 5;

  int r_start2 = pixelToGrid(newY);
  int r_end2 = (newY + 2) / 5;
  int c_start2 = pixelToGrid(newX);
  int c_end2 = (newX + 2) / 5;

  int r_min = std::min(r_start1, r_start2);
  int r_max = std::max(r_end1, r_end2);
  int c_min = std::min(c_start1, c_start2);
  int c_max = std::max(c_end1, c_end2);

  for (int r = r_min; r <= r_max; r++) {
    for (int c = c_min; c <= c_max; c++) {
      if (r >= 0 && r < MAP_SIZE && c >= 0 && c < MAP_SIZE) {
        drawFoodTile(r, c);
      }
    }
  }
}

void Clockface::redrawFoodAt(int x, int y) {
  int r_start = pixelToGrid(y);
  int r_end = (y + 2) / 5;
  int c_start = pixelToGrid(x);
  int c_end = (x + 2) / 5;

  for (int r = r_start; r <= r_end; r++) {
    for (int c = c_start; c <= c_end; c++) {
      if (r >= 0 && r < MAP_SIZE && c >= 0 && c < MAP_SIZE) {
        drawFoodTile(r, c);
      }
    }
  }
}
