#include "ghost.h"

Ghost::Ghost(int x, int y, uint16_t bodyColor) : Entity(x, y) {
  _bodyColor = bodyColor;
}

void Ghost::init() {
  updateSprite(0);
  Locator::getDisplay()->drawRGBBitmap(_x, _y, _GHOST[int(_anim)], SPRITE_SIZE, SPRITE_SIZE);
}

void Ghost::move(Direction dir) {
  moveInDirection(dir);
}

void Ghost::turn(Direction dir) {
  _direction = dir;
}

void Ghost::resetToStart() {
  resetToStartPosition();
  _direction = Direction::LEFT;
  _state = CHASING;
}

void Ghost::resetToPosition(int x, int y) {
  _x = x;
  _y = y;
  _direction = Direction::LEFT;
  _state = CHASING;
}

void Ghost::update() {
  // Clear old position before moving
  Locator::getDisplay()->fillRect(_x, _y, SPRITE_SIZE, SPRITE_SIZE, 0);

  // Update position
  this->move(_direction);

  // Update animation frame
  updateAnimation(3);

  // Update dynamic sprite colors/eyes (no invincible timeout in base update)
  updateSprite(0);

  // Draw at new position
  Locator::getDisplay()->drawRGBBitmap(_x, _y, _GHOST[int(_anim)], SPRITE_SIZE, SPRITE_SIZE);

  _iteration++;
}

void Ghost::updateWithInvincibleTimeout(unsigned long invincibleTimeout) {
  // Clear old position before moving
  Locator::getDisplay()->fillRect(_x, _y, SPRITE_SIZE, SPRITE_SIZE, 0);

  // Update position
  this->move(_direction);

  // Update animation frame
  updateAnimation(3);

  // Update dynamic sprite colors/eyes
  updateSprite(invincibleTimeout);

  // Draw at new position
  Locator::getDisplay()->drawRGBBitmap(_x, _y, _GHOST[int(_anim)], SPRITE_SIZE, SPRITE_SIZE);

  _iteration++;
}

void Ghost::updateSprite(unsigned long invincibleTimeout) {
  uint16_t bodyColor = _bodyColor;
  uint16_t eyeColor = 0xFFFF;  // White

  unsigned long currentMillis = millis();

  if (invincibleTimeout > currentMillis) {
    _state = FRIGHTENED;
    unsigned long timeLeft = invincibleTimeout - currentMillis;
    if (timeLeft < 2000) {
      // Flash blue and white in the last 2 seconds
      bool flashWhite = (timeLeft / 250) % 2 == 0;
      bodyColor = flashWhite ? 0xFFFF : 0x001F;
      eyeColor = flashWhite ? 0xF800 : 0xFE40; // Red/Yellow eyes when flashing
    } else {
      bodyColor = 0x001F; // Solid blue
      eyeColor = 0xFE40;  // Yellow eyes
    }
  } else {
    _state = CHASING;
  }

  // Build the sprite from template
  for (int f = 0; f < 2; f++) {
    for (int i = 0; i < 25; i++) {
      byte val = _GHOST_TEMPLATE[f][i];
      if (val == 0) {
        _GHOST[f][i] = 0x0000;
      } else if (val == 1) {
        _GHOST[f][i] = bodyColor;
      } else if (val == 2) {
        _GHOST[f][i] = eyeColor;
      }
    }

    // Custom eye look direction when chasing
    if (_state == CHASING) {
      if (_direction == Direction::LEFT) {
        _GHOST[f][5] = eyeColor;
        _GHOST[f][6] = bodyColor;
        _GHOST[f][7] = eyeColor;
        _GHOST[f][8] = bodyColor;
        _GHOST[f][9] = bodyColor;
      } else if (_direction == Direction::RIGHT) {
        _GHOST[f][5] = bodyColor;
        _GHOST[f][6] = bodyColor;
        _GHOST[f][7] = eyeColor;
        _GHOST[f][8] = bodyColor;
        _GHOST[f][9] = eyeColor;
      } else if (_direction == Direction::UP) {
        // Eyes shift to row 0, eye row becomes body color
        _GHOST[f][1] = eyeColor;
        _GHOST[f][3] = eyeColor;
        _GHOST[f][5] = bodyColor;
        _GHOST[f][6] = bodyColor;
        _GHOST[f][7] = bodyColor;
        _GHOST[f][8] = bodyColor;
        _GHOST[f][9] = bodyColor;
      } else if (_direction == Direction::DOWN) {
        // Eyes at default row 1, add highlights on row 2
        _GHOST[f][11] = eyeColor;
        _GHOST[f][13] = eyeColor;
      }
    }
  }
}

const char* Ghost::name() {
  return "GHOST";
}

void Ghost::execute(EventType event, Sprite* caller) {
  // Event callback implementation if needed
}
