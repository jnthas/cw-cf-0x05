#pragma once

#include <Arduino.h>
#include <Game.h>
#include <Locator.h>
#include <EventBus.h>
#include <ImageUtils.h>
#include "assets.h"
#include "entity.h"


class Pacman: public Entity {
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

    long current_color = 0xFE40;

    const unsigned short* _sprite;
    unsigned long invencibleTimeout = 0;

    void flip();
    void rotate();
    void changePacmanColor(uint16_t newcolor);


  public:
    enum State {
      MOVING,
      STOPPED,
      TURNING,
      INVENCIBLE
    };
    Pacman(int x, int y);
    void init() override;
    void move(Direction dir);
    void turn(Direction dir) override;
    void setState(State state);
    int nextBlock();
    void update() override;
    const char* name() override;
    void execute(EventType event, Sprite* caller) override;
    void resetToStart();
    unsigned long getInvincibleTimeout() {
      return _state == INVENCIBLE ? invencibleTimeout + INVINCIBILITY_DURATION_MS : 0;
    }
    State _state = MOVING;
    static constexpr unsigned long INVINCIBILITY_DURATION_MS = 7000;

    
};
