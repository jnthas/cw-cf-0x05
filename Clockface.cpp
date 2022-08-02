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
  randomSeed(dateTime->getMilliseconds() + millis());
  drawMap();
}

void Clockface::update()
{

  if (millis() - lastMillis >= 75) {

    int nextBlk = nextBlock();


    // X axis
    if ((pacman->_direction == Direction::LEFT || pacman->_direction == Direction::RIGHT) && (pacman->getX()-2) % 5 == 0) {

      if (_MAP[(pacman->getY()-2)/5][(pacman->getX()-2)/5] == 0)
        _MAP[(pacman->getY()-2)/5][(pacman->getX()-2)/5] = 4;


      //Serial.print(pacman->getX());
      //Serial.print(" Next block: "); Serial.println(nextBlk);

      if (nextBlk == 1 || nextBlk == -1) {
        turnRandom();
      } else if ((nextBlock(Direction::DOWN) == 0 || nextBlock(Direction::DOWN) == 4) && random(100) % 2 == 0) {
        pacman->turn(Direction::DOWN);
      } else if ((nextBlock(Direction::UP) == 0 || nextBlock(Direction::UP) == 4) && random(100) % 2 == 0) {
        pacman->turn(Direction::UP);
      }

      if (nextBlk == 3) {
        pacman->_state = Pacman::State::INVENCIBLE;
      }


      // Serial.print("X=");
      // Serial.print((pacman->getX()-2)/5);
      // Serial.print(" Y=");
      // Serial.println((pacman->getY()-2)/5);


      // for (int i = 0; i<12; i++) {
      //   for (int j = 0; j<12; j++) {
      //     Serial.print(_MAP[i][j]);
      //     Serial.print(",");
      //   }
      //   Serial.println("");
      // }


     
    
    // Y axis
    } else if ((pacman->_direction == Direction::UP || pacman->_direction == Direction::DOWN) && (pacman->getY()-2) % 5 == 0) {

      if (_MAP[(pacman->getY()-2)/5][(pacman->getX()-2)/5] == 0)
        _MAP[(pacman->getY()-2)/5][(pacman->getX()-2)/5] = 4;


      //Serial.print(pacman->getX());
      //Serial.print(" Next block: "); Serial.println(nextBlk);

      if (nextBlk == 1 || nextBlk == -1) {
        turnRandom();
      } else if ((nextBlock(Direction::LEFT) == 0 || nextBlock(Direction::LEFT) == 4) && random(100) % 2 == 0) {
        pacman->turn(Direction::LEFT);
      } else if ((nextBlock(Direction::RIGHT) == 0 || nextBlock(Direction::RIGHT) == 4) && random(100) % 2 == 0) {
        pacman->turn(Direction::RIGHT);
      }

      
      if (nextBlk == 3) {
        pacman->_state = Pacman::State::INVENCIBLE;
      }



      // for (int i = 0; i<12; i++) {
      //   for (int j = 0; j<12; j++) {
      //     Serial.print(_MAP[i][j]);
      //     Serial.print(",");
      //   }
      //   Serial.println("");
      // }

      // Serial.print("X=");
      // Serial.print((pacman->getX()-2)/5);
      // Serial.print(" Y=");
      // Serial.println((pacman->getY()-2)/5);


    }

    pacman->update();
    
    if (!checkBlocks(0)) {
      // Serial.println("DRAW MAP");
      // memcpy( _MAP, _MAP_CONST, sizeof(_MAP));
      for (int i = 0; i<12; i++) {
        for (int j = 0; j<12; j++) {
          if (_MAP[i][j] == 4)
            _MAP[i][j] = 0;
        }
      }


      drawMap();  
    }


    lastMillis = millis();
  }
  
}

bool Clockface::checkBlocks(int elem) {
  for (int i = 0; i<12; i++) {
    for (int j = 0; j<12; j++) {
      if (_MAP[i][j] == elem)
        return true;
    }
  }

  return false;
}


void Clockface::turnRandom() {
  int dir = random(4);
  //int dir = 3;
  //pacman->_state = Pacman::State::TURNING;

  do {
    pacman->turn(static_cast<Direction>(dir));
    dir = random(4);
    //dir++;

    
  } while (nextBlock() != 0 && nextBlock() != 4);

  Serial.print("New direction: ");
  Serial.println(pacman->_direction);
}


int Clockface::nextBlock() {
  return nextBlock(pacman->_direction);
}

int Clockface::nextBlock(Direction dir) {

  int map_block = 0;

  if (dir == Direction::RIGHT) {
    if (pacman->getX()+pacman->SPRITE_SIZE >= MAP_MAX_POS) {
      map_block = -1;
    } else {
      map_block = _MAP[(pacman->getY()-2)/5][((pacman->getX()-2)/5)+1];
    }
    
  } else if (dir == Direction::DOWN) {
    if (pacman->getY()+pacman->SPRITE_SIZE >= MAP_MAX_POS) {
      map_block = -1;
    } else {
      map_block = _MAP[((pacman->getY()-2)/5)+1][(pacman->getX()-2)/5];
    }
  } else if (dir == Direction::LEFT) {

    if ((pacman->getX()-2) <= 0) {
      map_block = -1;      
    } else {
      map_block = _MAP[(pacman->getY()-2)/5][((pacman->getX()-2)/5)-1];
    }

  } else if (dir == Direction::UP) {
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

  Locator::getDisplay()->fillRect(0, 0, 64, 64, 0x0000);



  uint16_t food_color = 0xB58C;
  uint16_t wall_color = 0x0016;
  uint16_t spcfood_color = 0xFBE0;



  Locator::getDisplay()->drawRect(0,0,64,64,wall_color);
  Locator::getDisplay()->drawRect(1,1,62,62,wall_color);

  // 0: empty space
  // 1: obstacle
  // 2: teleport
  // 3: special food
  // 8: clock space
  // 9: pacman


  for (int i=0; i<12; i++) {
    for (int j=0; j<12; j++) {
      if (_MAP[j][i] == 0) {        // 0: empty space with food
        Locator::getDisplay()->fillRect((i*5)+3,(j*5)+4,3,1,food_color);
      } else if (_MAP[j][i] == 1) { // 1: obstacle
        Locator::getDisplay()->fillRect((i*5)+2,(j*5)+2,5,5,wall_color);
      } else if (_MAP[j][i] == 2) { // 2: teleport
        Locator::getDisplay()->fillRect((i*5)+((bool)i*2),(j*5)+2,7,5,0x0000);
      } else if (_MAP[j][i] == 3) { // 3: special food
        Locator::getDisplay()->fillRect((i*5)+3,(j*5)+3,3,3,spcfood_color);
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
