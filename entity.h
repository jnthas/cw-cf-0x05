#pragma once

#include <Arduino.h>
#include <Game.h>
#include <Locator.h>
#include <EventBus.h>

// Direction offset lookup table (from Clockface.h)
struct DirectionOffset {
  int dRow;
  int dCol;
};

// Base class for game entities (Pacman, Ghost)
class Entity: public Sprite, public EventTask {
protected:
  int _startX;
  int _startY;
  byte _iteration = 0;
  bool _anim = true;

  // Common move implementation
  void moveInDirection(Direction dir);

  // Common reset logic
  void resetToStartPosition();

public:
  Entity(int x, int y);
  virtual ~Entity() = default;

  // Common accessors
  int getX();
  int getY();

  // Grid position helpers (assuming 5x5 sprite grid)
  int getGridRow();
  int getGridCol();

  // Direction helpers
  static void getDirectionOffset(Direction dir, int &dRow, int &dCol);

  // Animation helper
  void updateAnimation(int interval = 3);

  // Pure virtual methods (must be implemented by subclasses)
  virtual void init() = 0;
  virtual void turn(Direction dir) = 0;
  virtual void update() = 0;
  virtual const char* name() = 0;
  virtual void execute(EventType event, Sprite* caller) = 0;

  // Public member variables
  Direction _direction = Direction::RIGHT;
  const int SPRITE_SIZE = 5;
};