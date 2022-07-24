#include "pacman.h"

Pacman::Pacman(int x, int y) {
  _x = x;
  _y = y;
}

void Pacman::turn(Direction dir) {

  // set to right  
  memcpy( _PACMAN, _PACMAN_CONST, sizeof(_PACMAN) );
  
  if (dir == Direction::LEFT) {
    flip();
  } else if (dir == Direction::DOWN) {
    flip();
    rotate();    
    flip();
  } else if (dir == Direction::UP) {
    rotate();
  }

  _direction = dir;

}

void Pacman::move(Direction dir) {
  
  if (dir == Direction::RIGHT) {
    _x += 1;
  } else if (dir == Direction::LEFT) {
    _x -= 1;
  } else if (dir == Direction::DOWN) {
    _y += 1;
  } else if (dir == Direction::UP) {
    _y -= 1;
  }

}


void Pacman::init() {
  //Locator::getEventBus()->subscribe(this);
  Locator::getDisplay()->drawRGBBitmap(_x, _y, _PACMAN[int(_pacman_anim)], SPRITE_SIZE, SPRITE_SIZE);
}

void Pacman::update() { 
  
  if (_state == MOVING) {
    Locator::getDisplay()->fillRect(_x,_y, SPRITE_SIZE, SPRITE_SIZE, 0);
    this->move(_direction);
  }

  _pacman_anim = !_pacman_anim;
  Locator::getDisplay()->drawRGBBitmap(_x, _y, _PACMAN[int(_pacman_anim)], SPRITE_SIZE, SPRITE_SIZE);

}


//TODO move to the gfx-engine lib
void Pacman::rotate() {
  int n = SPRITE_SIZE;

  uint16_t temp0[2][25] = {};  
  memcpy( temp0, _PACMAN, sizeof(temp0) );
 
  for(int i = 0; i < n; i++) {
    for(int j = 0; j < n; j++) {
      _PACMAN[0][(i*n)+j] = temp0[0][(n-1-i)+(j*n)];
      _PACMAN[1][(i*n)+j] = temp0[1][(n-1-i)+(j*n)];
    }
  }
}

//TODO move to the gfx-engine lib
void Pacman::flip() {
  int n = SPRITE_SIZE;

  for(int i = 0; i < n; i++) {
    for(int j = 0; j < floor(n/2)+1; j++) {
      std::swap(_PACMAN[0][j + (i*n)], _PACMAN[0][(i*n)+n-j-1]);
      std::swap(_PACMAN[1][j + (i*n)], _PACMAN[1][(i*n)+n-j-1]);
    }
  }

}

void Pacman::execute(EventType event, Sprite* caller) {
  if (event == EventType::COLLISION) {
    //Serial.println("MARIO - Collision detected");
    _direction = DOWN;
  }
}


int Pacman::getX() {
  return this->_x;
}
int Pacman::getY() {
  return this->_y;
}

const char* Pacman::name() {
  return "PACMAN";
}