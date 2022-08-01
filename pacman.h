#pragma once

#include <Arduino.h>
#include <Game.h>
#include <Locator.h>
#include <EventBus.h>
#include <ImageUtils.h>
#include "assets.h"


class Pacman: public Sprite, public EventTask {
  private:

    uint16_t _PACMAN [2][25] = {
      {
        // 'pacman1, 5x5px
        0x0000, 0xFE40, 0xFE40, 0xFE40, 0x0000, 
        0xFE40, 0xFE40, 0x0000, 0xFE40, 0xFE40, 
        0xFE40, 0xFE40, 0xFE40, 0xFE40, 0xFE40, 
        0xFE40, 0xFE40, 0xFE40, 0xFE40, 0xFE40, 
        0x0000, 0xFE40, 0xFE40, 0xFE40, 0x0000
      }, 
      {
        // 'pacman2, 5x5px
        0x0000, 0xFE40, 0xFE40, 0xFE40, 0xFE40, 
        0xFE40, 0xFE40, 0x0000, 0xFE40, 0x0000, 
        0xFE40, 0xFE40, 0xFE40, 0x0000, 0x0000, 
        0xFE40, 0xFE40, 0xFE40, 0xFE40, 0x0000, 
        0x0000, 0xFE40, 0xFE40, 0xFE40, 0xFE40
      }
    };

    byte _iteration = 0;

    int _lastX;
    int _lastY;

    const unsigned short* _sprite;
    unsigned long lastMillis = 0;
    

    bool _pacman_anim = true;

    void flip();
    void rotate();
    void replaceColor(byte sprite_index, uint16_t oldcolor, uint16_t newcolor);

    
  public:
    enum State {
      MOVING,
      STOPPED,
      TURNING,
      INVENCIBLE
    };
    Pacman(int x, int y);
    void init();
    void move(Direction dir);
    void turn(Direction dir);    
    int nextBlock();
    int getX();
    int getY();
    void update();
    const char* name();
    void execute(EventType event, Sprite* caller);
    Direction _direction = Direction::RIGHT;
    State _state = MOVING;
    const int SPRITE_SIZE = 5;

    
};
