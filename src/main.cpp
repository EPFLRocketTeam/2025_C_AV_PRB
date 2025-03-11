// 
#include <Arduino.h>
#include <Wire.h>
#include "constant.h"
#include "PRBComputer.h"

void setup() {

  //begin I2C communication with spine
  Wire1.begin(SLAVE_ADDR);

  //begin I2C communication with sensors
  Wire2.begin();
  
}

void loop() {

}
