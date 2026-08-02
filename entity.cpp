#include "entity.h"

Entity::Entity(int x, int y) {
  _x = x;
  _y = y;
  _startX = x;
  _startY = y;
  _width = SPRITE_SIZE;
  _height = SPRITE_SIZE;
}

void Entity::moveInDirection(Direction dir) {
  int dRow, dCol;
  getDirectionOffset(dir, dRow, dCol);
  _x += dCol;
  _y += dRow;
}

void Entity::resetToStartPosition() {
  _x = _startX;
  _y = _startY;
}

int Entity::getX() {
  return _x;
}

int Entity::getY() {
  return _y;
}

int Entity::getGridRow() {
  // Convert pixel position to grid position (assuming 5x5 sprites on 12x12 grid)
  return _y / SPRITE_SIZE;
}

int Entity::getGridCol() {
  // Convert pixel position to grid position (assuming 5x5 sprites on 12x12 grid)
  return _x / SPRITE_SIZE;
}

void Entity::getDirectionOffset(Direction dir, int &dRow, int &dCol) {
  switch (dir) {
    case Direction::UP:
      dRow = -1;
      dCol = 0;
      break;
    case Direction::DOWN:
      dRow = 1;
      dCol = 0;
      break;
    case Direction::LEFT:
      dRow = 0;
      dCol = -1;
      break;
    case Direction::RIGHT:
      dRow = 0;
      dCol = 1;
      break;
    default:
      dRow = 0;
      dCol = 0;
      break;
  }
}

void Entity::updateAnimation(int interval) {
  if (_iteration % interval == 0) {
    _anim = !_anim;
  }
}