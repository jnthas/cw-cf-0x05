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
        0x0000, 0xce60, 0xce60, 0xce60, 0x0000, 
        0xce60, 0xce60, 0xffff, 0xce60, 0xce60, 
        0xce60, 0xce60, 0xce60, 0xce60, 0xce60, 
        0xce60, 0xce60, 0xce60, 0xce60, 0xce60, 
        0x0000, 0xce60, 0xce60, 0xce60, 0x0000
      }, 
      {
        // 'pacman2, 5x5px
        0x0000, 0xce60, 0xce60, 0xce60, 0xce60, 
        0xce60, 0xce60, 0xffff, 0xce60, 0x0000, 
        0xce60, 0xce60, 0xce60, 0x0000, 0x0000, 
        0xce60, 0xce60, 0xce60, 0xce60, 0x0000, 
        0x0000, 0xce60, 0xce60, 0xce60, 0xce60
      }
    };

    enum State {
      MOVING,
      STOPPED
    };

    Direction _direction = Direction::RIGHT;

    int _lastX;
    int _lastY;

    const unsigned short* _sprite;
    unsigned long lastMillis = 0;
    State _state = MOVING;

    bool _pacman_anim = true;

    void flip();
    void rotate();

    
  public:
    Pacman(int x, int y);
    void init();
    void move(Direction dir);
    void turn(Direction dir);
    void update();
    const char* name();
    void execute(EventType event, Sprite* caller);
    
};
