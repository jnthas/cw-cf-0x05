#include "Clockface.h"

const int Clockface::MAP_SIZE; // Definition for static member
unsigned long lastMillis = 0;
unsigned long lastMillisTime = 0;
unsigned long lastMillisSec = 0;


Pacman *pacman;

Clockface::Clockface(Adafruit_GFX* display) {
  _display = display;
  Locator::provide(display);
}

void Clockface::setup(CWDateTime *dateTime) {
  this->_dateTime = dateTime;
  Locator::getDisplay()->setFont(&hourFont);
  randomSeed(dateTime->getMilliseconds() + millis());
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


  // Pacman
  if (millis() - lastMillis >= 75) { // Pacman update interval

    bool fullBlock = // Check if Pacman is aligned with the grid center
                     // X axis
                     ((pacman->_direction == Direction::LEFT || pacman->_direction == Direction::RIGHT) && (pacman->getX()-2) % 5 == 0) ||
                     // Y axis
                     ((pacman->_direction == Direction::UP || pacman->_direction == Direction::DOWN) && (pacman->getY()-2) % 5 == 0);


    if (fullBlock) { // Actions to take when Pacman reaches the center of a map block
      // Get coordinates of the block Pacman just fully entered
      int currentMapR = (pacman->getY() - 2) / 5;
      int currentMapC = (pacman->getX() - 2) / 5;

      // Check the content of the block *before* clearing it
      MapBlock currentBlockContent = static_cast<MapBlock>(_MAP[currentMapR][currentMapC]);

      // Now, change the block to empty (eat the food/superfood)
      _MAP[currentMapR][currentMapC] = MapBlock::EMPTY;

      // Check if Pacman just ate superfood
      if (currentBlockContent == MapBlock::SUPER_FOOD) {
        pacman->setState(Pacman::State::INVENCIBLE);
      }

      // Decide the next direction using BFS
      // The nextBlk parameter isn't strictly used by the BFS logic itself but kept for compatibility.
      MapBlock nextBlk = nextBlock(); // Determine block in current direction (used in fallback logic)
      directionDecision(nextBlk, (pacman->_direction == Direction::LEFT || pacman->_direction == Direction::RIGHT));


      // Check if all food (regular and super) is gone to reset map
      if (countBlocks(MapBlock::FOOD) == 0 && countBlocks(MapBlock::SUPER_FOOD) == 0) {
         resetMap();
      }
    } // end if(fullBlock)

    // Update Pacman's position/animation regardless of being on a full block
    pacman->update();

    // Reset the timer for the next Pacman update cycle
    lastMillis = millis();

  } // end if (millis() - lastMillis >= 75)

  
  
}


const char* Clockface::weekDayName(int weekday) {
  strncpy(weekDayTemp, _weekDayWords + (weekday*4), 4);
  return weekDayTemp;
}

const char* Clockface::monthName(int month) {
  strncpy(monthTemp, _monthWords + ((month-1)*4), 4);
  return monthTemp;
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

// Helper function to check if a cell is within bounds and movable
bool Clockface::isValid(int r, int c) {
    // Check bounds
    if (r < 0 || r >= MAP_SIZE || c < 0 || c >= MAP_SIZE) {
        return false;
    }
    // Check if the block type is movable
    MapBlock block = static_cast<MapBlock>(_MAP[r][c]);
    // Allow moving onto EMPTY, FOOD, GATE (via contains) OR SUPER_FOOD
    return contains(block, PACMAN_MOVING_BLOCKS) || block == MapBlock::SUPER_FOOD;
}

// Helper function to check if a cell contains a target (food or superfood)
bool Clockface::isTarget(int r, int c) {
    // Check bounds (although isValid should handle this)
    if (r < 0 || r >= MAP_SIZE || c < 0 || c >= MAP_SIZE) {
        return false;
    }
    MapBlock block = static_cast<MapBlock>(_MAP[r][c]);
    return block == MapBlock::FOOD || block == MapBlock::SUPER_FOOD;
}


// Reconstructs the path to find the immediate next move
void Clockface::reconstructPath(Point start, Point end, Direction& nextMove) {
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


// Finds the shortest path using BFS and determines the next move
bool Clockface::findShortestPath(int startR, int startC, Direction& nextMove) {
    // Initialize BFS structures
    queueFront = 0;
    queueRear = -1;
    for (int i = 0; i < MAP_SIZE; ++i) {
        for (int j = 0; j < MAP_SIZE; ++j) {
            visited[i][j] = false;
            parent[i][j] = {-1, -1}; // Initialize parent pointers
        }
    }

    // Starting point
    Point startPoint = {startR, startC};
    visited[startR][startC] = true;
    queue[++queueRear] = startPoint;

    // Possible moves (row and column offsets)
    int dRow[] = {-1, 1, 0, 0}; // Up, Down
    int dCol[] = {0, 0, -1, 1}; // Left, Right

    while (queueFront <= queueRear) {
        Point current = queue[queueFront++];

        // Check if the current cell is a target
        if (isTarget(current.x, current.y)) {
            reconstructPath(startPoint, current, nextMove);
            return true; // Path found
        }

        // Explore neighbors
        for (int i = 0; i < 4; ++i) {
            int nextR = current.x + dRow[i];
            int nextC = current.y + dCol[i];

            if (isValid(nextR, nextC) && !visited[nextR][nextC]) {
                visited[nextR][nextC] = true;
                parent[nextR][nextC] = current; // Record parent
                queue[++queueRear] = {nextR, nextC};

                 // Check queue bounds (simple circular queue might be better)
                if (queueRear >= MAX_QUEUE_SIZE -1) {
                    Serial.println("BFS Queue Overflow!");
                    return false; // Prevent overflow
                }
            }
        }
    }

    return false; // No path found
}


// Modified direction decision logic
void Clockface::directionDecision(MapBlock nextBlk, bool moving_axis_x) {

    int currentMapR = (pacman->getY() - 2) / 5;
    int currentMapC = (pacman->getX() - 2) / 5;
    Direction nextMove = pacman->_direction; // Default to current direction

    if (findShortestPath(currentMapR, currentMapC, nextMove)) {
        // Path found, turn Pacman
        if (nextMove != pacman->_direction) {
             pacman->turn(nextMove);
        }
        // If the next move is the current direction, just continue straight.
        // No explicit action needed here as Pacman continues in its current direction by default.

    } else {
        // No path found (e.g., trapped or no food left)
        // Fallback to random turning if the immediate next block is invalid
         if (!contains(nextBlock(), PACMAN_MOVING_BLOCKS)) {
            turnRandom();
         }
         // Otherwise, continue straight if possible, or turn randomly if blocked.
         // The original random logic might still be useful as a fallback.
         // For simplicity now, if blocked, turn random. If not blocked, continue.
         // If the next block in the current direction is a wall, turn randomly.
         MapBlock immediateNext = nextBlock();
         if (contains(immediateNext, PACMAN_BLOCKING_BLOCKS)) {
             turnRandom();
         }
         // If the pathfinding failed but the next block is movable,
         // let Pacman continue straight. This might happen if pathfinding
         // fails due to queue overflow or other unexpected issues.
    }
}


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
  int dir = random(4);
  //int dir = 3;
  //pacman->_state = Pacman::State::TURNING;

  do {
    pacman->turn(static_cast<Direction>(dir));
    dir = random(4);
    //dir++;

    
  } while (!contains(nextBlock(), PACMAN_MOVING_BLOCKS));

  Serial.print("New direction: ");
  Serial.println(pacman->_direction);
}


Clockface::MapBlock Clockface::nextBlock() {
  return nextBlock(pacman->_direction);
}

Clockface::MapBlock Clockface::nextBlock(Direction dir) {

  Clockface::MapBlock map_block = Clockface::MapBlock::OUT_OF_MAP;

  if (dir == Direction::RIGHT) {
    if (pacman->getX()+pacman->SPRITE_SIZE < MAP_MAX_POS) {
      map_block = static_cast<MapBlock>(_MAP[(pacman->getY()-2)/5][((pacman->getX()-2)/5)+1]);
    }
    
  } else if (dir == Direction::DOWN) {
    if (pacman->getY()+pacman->SPRITE_SIZE < MAP_MAX_POS) {
      map_block = static_cast<MapBlock>(_MAP[((pacman->getY()-2)/5)+1][(pacman->getX()-2)/5]);
    }
  } else if (dir == Direction::LEFT) {

    if ((pacman->getX()-2) > 0) {
      map_block = static_cast<MapBlock>(_MAP[(pacman->getY()-2)/5][((pacman->getX()-2)/5)-1]);
    }

  } else if (dir == Direction::UP) {
    if ((pacman->getY()-2) > 0) {
      map_block = static_cast<MapBlock>(_MAP[((pacman->getY()-2)/5)-1][((pacman->getX()-2)/5)]);
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
  //Locator::getDisplay()->drawRGBBitmap(0, 0, _PACMAN_MAP, 64, 64);
  //Locator::getDisplay()->getPixel(0, 0);

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
        Locator::getDisplay()->fillRect((i*5)+2,(j*5)+2,5,5,wall_color);
      } else if (_MAP[j][i] == MapBlock::CLOCK) {
        Locator::getDisplay()->fillRect((i*5)+2,(j*5)+2,5,5,wall_color);
      } else if (_MAP[j][i] == MapBlock::GATE) {
        //Locator::getDisplay()->fillRect((i*5)+((bool)i*2),(j*5)+2,7,5,0x0000);
        Locator::getDisplay()->fillRect((i*5)+3,(j*5)+4,3,1,food_color);
      } else if (_MAP[j][i] == MapBlock::SUPER_FOOD) {
        Locator::getDisplay()->fillRect((i*5)+3,(j*5)+3,3,3,spcfood_color);
      } else if (_MAP[j][i] == MapBlock::PACMAN) {
        pacman = new Pacman((i*5)+2,(j*5)+2);

        // Locator::getDisplay()->drawRGBBitmap((i*5)+2,(j*5)+2, _PACMAN_2, 5, 5);
        // pacmanState = !pacmanState;
      }
    }
  }

  
}
