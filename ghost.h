#pragma once

#include <Arduino.h>
#include <Game.h>
#include <Locator.h>
#include <EventBus.h>
#include <ImageUtils.h>
#include "entity.h"

class Ghost: public Entity {
  private:
    // 0: Transparent/Black
    // 1: Body color
    // 2: Eye color
    const byte _GHOST_TEMPLATE[2][25] = {
      {
        0, 1, 1, 1, 0,
        1, 2, 1, 2, 1,
        1, 1, 1, 1, 1,
        1, 1, 1, 1, 1,
        1, 0, 1, 0, 1
      },
      {
        0, 1, 1, 1, 0,
        1, 2, 1, 2, 1,
        1, 1, 1, 1, 1,
        1, 1, 1, 1, 1,
        0, 1, 0, 1, 0
      }
    };

    uint16_t _GHOST [2][25];

    void updateSprite(unsigned long invincibleTimeout);

  public:
    enum State {
      CHASING,
      FRIGHTENED
    };

    enum Personality {
      BLINKY,  // Direct chaser — targets Pacman's current cell
      PINKY    // Ambusher — targets 4 tiles ahead of Pacman's direction
    };

    Ghost(int x, int y, uint16_t bodyColor = 0xF800);
    void init() override;
    void move(Direction dir);
    void turn(Direction dir) override;
    void resetToStart();
    void resetToPosition(int x, int y);
    void update() override;
    void updateWithInvincibleTimeout(unsigned long invincibleTimeout);
    const char* name() override;
    void execute(EventType event, Sprite* caller) override;

    uint16_t _bodyColor;
    State _state = CHASING;
    Personality _personality = BLINKY;
};
