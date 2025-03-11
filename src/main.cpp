// 
#include <Arduino.h>
#include <Wire.h>
#include "constant.h"
#include "PRBComputer.h"

void setup() {

  //PIN configuration 
  pinMode(VE_no, OUTPUT);
  pinMode(VO_noC, OUTPUT);
  pinMode(IE_nc, OUTPUT);
  pinMode(IO_ncC, OUTPUT);
  pinMode(MOSFET, OUTPUT);

  pinMode(T_EIN, INPUT);
  pinMode(T_OIN, INPUT);
  pinMode(P_OIN, INPUT);

  pinMode(RESET, OUTPUT);
  digitalWrite(RESET, HIGH);

  //begin I2C communication with spine
  Wire1.begin(SLAVE_ADDR);

  //begin I2C communication with sensors
  Wire2.begin();

  //begin UART communication with EPOS4
  Serial7.begin(9600); //baudrate to determine
  
}

void loop() {

}
