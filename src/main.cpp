// 
#include <Arduino.h>
#include <Wire.h>
#include "PRBComputer.h"

PRBComputer computer(IDLE);

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

  //Analog sensor precision
  analogReadResolution(12);

  Serial.begin(115200); //for debugging
  Serial.println("PRB Computer started");

  digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)
  delay(1000); // wait for a second
  digitalWrite(LED_BUILTIN, LOW); // turn the LED off by making the voltage LOW
  
}

PTE7300_I2C mySensor; // attach sensor
int16_t DSP_T1;

void loop() {
  float temp = 0.0;
  float press = 0.0;

  temp = computer.read_temperature(EIN_CH);
  Serial.print("Temperature EIN: ");
  Serial.print(temp);
  Serial.print(" °C\t");
  press = computer.read_pressure(EIN_CH);
  Serial.print("Pressure EIN: ");
  Serial.println(press);

  temp = computer.read_temperature(CCC_CH);
  Serial.print("Temperature CCC: ");
  Serial.print(temp);
  Serial.print(" °C\t");
  press = computer.read_pressure(CCC_CH);
  Serial.print("Pressure CCC: ");
  Serial.println(press);

  delay(100);
}
