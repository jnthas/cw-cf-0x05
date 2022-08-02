#pragma once

#include <Arduino.h>


#include "hour_font.h"


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


class Clockface: public IClockface {
  private:
    Adafruit_GFX* _display;
    CWDateTime* _dateTime;
    boolean pacmanState = true;



    // 390000000003
    // 011011110110
    // 000001100000
    // 101111111101
    // 101888888101
    // 201888888102
    // 201888888102
    // 101888888101
    // 101111111101
    // 000001100000
    // 011011110110
    // 300000000003
    // 59 food blocks

    // 0: empty space
    // 1: obstacle
    // 2: teleport
    // 3: special food
    // 8: clock space
    // 9: pacman

    const byte _MAP_CONST[12][12] = {
      {3,0,0,0,0,0,9,0,0,0,0,3},
      {0,1,1,0,1,1,1,1,0,1,1,0},
      {0,0,0,0,0,1,1,0,0,0,0,0},
      {1,0,1,1,1,1,1,1,1,1,0,1},
      {1,0,1,8,8,8,8,8,8,1,0,1},
      {2,0,1,8,8,8,8,8,8,1,0,2},
      {2,0,1,8,8,8,8,8,8,1,0,2},
      {1,0,1,8,8,8,8,8,8,1,0,1},
      {1,0,1,1,1,1,1,1,1,1,0,1},
      {0,0,0,0,0,1,1,0,0,0,0,0},
      {0,1,1,0,1,1,1,1,0,1,1,0},
      {3,0,0,0,0,0,0,0,0,0,0,3}
    };

    
    byte _MAP[12][12] = {
      {3,0,0,0,0,0,9,0,0,0,0,3},
      {0,1,1,0,1,1,1,1,0,1,1,0},
      {0,0,0,0,0,1,1,0,0,0,0,0},
      {1,0,1,1,1,1,1,1,1,1,0,1},
      {1,0,1,8,8,8,8,8,8,1,0,1},
      {2,0,1,8,8,8,8,8,8,1,0,2},
      {2,0,1,8,8,8,8,8,8,1,0,2},
      {1,0,1,8,8,8,8,8,8,1,0,1},
      {1,0,1,1,1,1,1,1,1,1,0,1},
      {0,0,0,0,0,1,1,0,0,0,0,0},
      {0,1,1,0,1,1,1,1,0,1,1,0},
      {3,0,0,0,0,0,0,0,0,0,0,3}
    };


    const byte MAP_BORDER_SIZE = 2;
    const byte MAP_MIN_POS = 0 + MAP_BORDER_SIZE;
    const byte MAP_MAX_POS = 64 - MAP_BORDER_SIZE;

    void drawMap();
    int nextBlock(Direction dir);
    int nextBlock();
    void turnRandom();
    bool checkBlocks(int elem);
    
    
  public:
    Clockface(Adafruit_GFX* display);
    void setup(CWDateTime *dateTime);
    void update();
};
