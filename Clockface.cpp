#include "Clockface.h"


unsigned long lastMillis = 0;

Pacman *pacman;


Clockface::Clockface(Adafruit_GFX* display) {
  _display = display;
  Locator::provide(display);
}

void Clockface::setup(CWDateTime *dateTime) {
  this->_dateTime = dateTime;
  Locator::getDisplay()->setFont(&hourFont);
  Locator::getDisplay()->fillRect(0, 0, 64, 64, 0x0000);  
  randomSeed(dateTime->getMilliseconds() + millis());
  drawMap();
}

void Clockface::update()
{

  if (millis() - lastMillis >= 25) {

    // if (pacman->getX() >= MAP_MAX_POS-pacman->SPRITE_SIZE && pacman->_direction == Direction::RIGHT ) {
    //   pacman->turn(Direction::DOWN);
    // } else if (pacman->getY() >= MAP_MAX_POS-pacman->SPRITE_SIZE && pacman->_direction == Direction::DOWN) {
    //   pacman->turn(Direction::LEFT);
    // } else if (pacman->getX() <= MAP_MIN_POS && pacman->_direction == Direction::LEFT) {
    //   pacman->turn(Direction::UP);
    // } else if (pacman->getY() <= MAP_MIN_POS && pacman->_direction == Direction::UP) {
    //   pacman->turn(Direction::RIGHT);
    // }

        
    Serial.print("X=");
    Serial.print(pacman->getX()-2);
    Serial.print(" Y=");
    Serial.println(pacman->getY()-2);


    // X axis
    if ((pacman->_direction == Direction::LEFT || pacman->_direction == Direction::RIGHT) && (pacman->getX()-2) % 5 == 0) {
      //Serial.print(pacman->getX());
      Serial.print(" Next block: "); Serial.println(nextBlock());

      if (nextBlock() == 1 || nextBlock() == -1) {
        turnRandom();
      // } else if (nextBlock() == 0 && random(100) % 2 == 0) {
      //   pacman->turn(Direction::DOWN);
      // } else if (nextBlock() == 0 && random(100) % 2 == 0) {
      //   pacman->turn(Direction::DOWN);
      }


    
    // Y axis
    } else if ((pacman->_direction == Direction::UP || pacman->_direction == Direction::DOWN) && (pacman->getY()-2) % 5 == 0) {
      //Serial.print(pacman->getX());
      Serial.print(" Next block: "); Serial.println(nextBlock());

      if (nextBlock() == 1 || nextBlock() == -1) {
        turnRandom();
      }

      
    }

    if (pacman->_state == Pacman::State::MOVING) {
      pacman->update();
    }
    
    lastMillis = millis();
  }
  
}

void Clockface::turnRandom() {
  int dir = random(4);
  //int dir = 3;
  //pacman->_state = Pacman::State::TURNING;

  do {
    pacman->turn(static_cast<Direction>(dir));
    dir = random(4);
    //dir++;

    
  } while (nextBlock() != 0);

  Serial.print("New direction: ");
  Serial.println(pacman->_direction);
}

int Clockface::nextBlock() {

  int map_block = 0;

  if (pacman->_direction == Direction::RIGHT) {
    if (pacman->getX()+pacman->SPRITE_SIZE >= MAP_MAX_POS) {
      map_block = -1;
    } else {
      map_block = _MAP[(pacman->getY()-2)/5][((pacman->getX()-2)/5)+1];
    }
    
  } else if (pacman->_direction == Direction::DOWN) {
    if (pacman->getY()+pacman->SPRITE_SIZE >= MAP_MAX_POS) {
      map_block = -1;
    } else {
      map_block = _MAP[((pacman->getY()-2)/5)+1][(pacman->getX()-2)/5];
    }
  } else if (pacman->_direction == Direction::LEFT) {

    if ((pacman->getX()-2) <= 0) {
      map_block = -1;      
    } else {
      map_block = _MAP[(pacman->getY()-2)/5][((pacman->getX()-2)/5)-1];
    }

  } else if (pacman->_direction == Direction::UP) {
    if ((pacman->getY()-2) <= 0) {
      map_block = -1;
    } else {
      map_block = _MAP[((pacman->getY()-2)/5)-1][((pacman->getX()-2)/5)];
    }
  }

  return map_block;

}


void Clockface::drawMap() 
{
  //Locator::getDisplay()->drawRGBBitmap(0, 0, _PACMAN_MAP, 64, 64);
  //Locator::getDisplay()->getPixel(0, 0);

  Locator::getDisplay()->drawRect(0,0,64,64,0x0016);
  Locator::getDisplay()->drawRect(1,1,62,62,0x0016);

  // 0: empty space
  // 1: obstacle
  // 2: teleport
  // 3: special food
  // 8: clock space
  // 9: pacman


  for (int i=0; i<12; i++) {
    for (int j=0; j<12; j++) {
      if (_MAP[j][i] == 0) {        // 0: empty space with food
        Locator::getDisplay()->fillRect((i*5)+3,(j*5)+4,3,1,0xB58C);
      } else if (_MAP[j][i] == 1) { // 1: obstacle
        Locator::getDisplay()->fillRect((i*5)+2,(j*5)+2,5,5,0x0016);
      } else if (_MAP[j][i] == 2) { // 2: teleport
        Locator::getDisplay()->fillRect((i*5)+((bool)i*2),(j*5)+2,7,5,0x00E1);
      } else if (_MAP[j][i] == 3) { // 3: special food
        Locator::getDisplay()->fillRect((i*5)+3,(j*5)+3,3,3,0xFBE0);
      } else if (_MAP[j][i] == 9) { // 9: pacman
        pacman = new Pacman((i*5)+2,(j*5)+2);

        // Locator::getDisplay()->drawRGBBitmap((i*5)+2,(j*5)+2, _PACMAN_2, 5, 5);
        // pacmanState = !pacmanState;
      }
    }
  }

  Locator::getDisplay()->setTextColor(0xFE40);
  Locator::getDisplay()->setCursor(15, 33);
  Locator::getDisplay()->print("23:45");
}
