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

void scanI2C() {
  byte error, address;
  int nDevices = 0;

  Serial.println("Scanning...");

  for (address = 1; address < 127; address++ ) {
    Wire2.beginTransmission(address);
    error = Wire2.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      Serial.println(address, HEX);
      nDevices++;
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found");
  else
    Serial.println("Scan done");
}

PTE7300_I2C mySensor; // attach sensor
int16_t DSP_T1;

void loop() {
  int16_t DSP_S1, DSP_T1, DSP_S2, DSP_T2;
  float pct1, pct2;

  delay(100);
  Wire2.beginTransmission(0x70); // 0x70 is the 7-bit address of the PCA9548A
  Wire2.write(0x01); // Enable channel 0 (bit 0 = 1)
  Wire2.endTransmission();

  //print out pressure in %. Modify for desired format
  //-16000 to 16000, 0 to 100%
  Serial.print("Pressure sensor 1:");  
  DSP_S1 = mySensor.readDSP_S();
  pct1 = DSP_S1 * 5.0 / 1600 + 50;
  Serial.print(pct1,2);
  Serial.print("%\t");

  //print out bridge temperature in °C
  //-16000 to 16000, -40 to 125°C
  Serial.print("\tBridge Temp:");
  DSP_T1 = mySensor.readDSP_T();
  pct1 = DSP_T1 * 82.5 / 16000 + 42.5;
  Serial.print(pct1,1);
  Serial.print("°C");

  Serial.print("\t\t");

  Wire2.beginTransmission(0x70); // 0x70 is the 7-bit address of the PCA9548A
  Wire2.write(0x02); // Enable channel 0 (bit 0 = 1)
  Wire2.endTransmission();

      //print out pressure in %. Modify for desired format
  //-16000 to 16000, 0 to 100%
  Serial.print("Pressure sensor 2:");  
  DSP_S2 = mySensor.readDSP_S();
  pct2 = DSP_S2 * 5.0 / 1600 + 50;
  Serial.print(pct2,2);
  Serial.print("%\t");

//print out bridge temperature in °C
//-16000 to 16000, -40 to 125°C
  Serial.print("\tBridge Temp:");
  DSP_T2 = mySensor.readDSP_T();
  pct2 = DSP_T2 * 82.5 / 16000 + 42.5;
  Serial.print(pct2,1);
  Serial.print("°C");

    Serial.println("");
}
