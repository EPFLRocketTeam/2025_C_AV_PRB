// 
#include <Arduino.h>
#include <Wire.h>
#include "PRBComputer.h"

// Define I2C slave address for Raspberry Pi
#define SLAVE_ADDR 0x08

PRBComputer computer(IDLE);

// Variables for communication
volatile uint8_t receivedCommand = 0;
volatile uint32_t responseValue = 0xDEADBEEF; // Hardcoded response

// I2C receive handler
void receiveEvent(int numBytes) {
  if (numBytes >= 1) {
    receivedCommand = Wire.read(); // Read the command
    Serial.print("Received I2C command: 0x");
    Serial.println(receivedCommand, HEX);

    // TODO: add missing functions and check if current calls are correct
    switch (receivedCommand) {
      case AV_NET_PRB_TIMESTAMP_MAIN:
        Serial.println("Received AV_NET_PRB_TIMESTAMP_MAIN command");
        break;
      case AV_NET_PRB_WAKE_UP:
        Serial.println("Received AV_NET_PRB_WAKE_UP command");
        break;
      case AV_NET_PRB_IS_WOKEN_UP:
        Serial.println("Received AV_NET_PRB_IS_WOKEN_UP command");
        break;
      case AV_NET_PRB_CLEAR_TO_IGNITE:
        Serial.println("Received AV_NET_PRB_CLEAR_TO_IGNITE command");
        break;
      case AV_NET_PRB_FSM_PRB:
        Serial.println("Received AV_NET_PRB_FSM_PRB command");
        // responseValue = computer.get_stage_sq();
        break;
      case AV_NET_PRB_P_OIN:
        Serial.println("Received AV_NET_PRB_P_OIN command");
        // responseValue = computer.read_pressure(P_OIN);
        break;
      case AV_NET_PRB_T_OIN:
        Serial.println("Received AV_NET_PRB_T_OIN command");
        // responseValue = computer.read_temperature(T_OIN);
        break;
      case AV_NET_PRB_P_EIN:
        Serial.println("Received AV_NET_PRB_P_EIN command");
        // responseValue = computer.read_pressure(EIN_CH);    
        break;
      case AV_NET_PRB_T_EIN:
        Serial.println("Received AV_NET_PRB_T_EIN command");
        // responseValue = computer.read_temperature(T_EIN);
        break;

      case AV_NET_PRB_P_CCC:
        Serial.println("Received AV_NET_PRB_P_CCC command");
        // responseValue = computer.read_pressure(CCC_CH);
        break;
      case AV_NET_PRB_T_CCC:
        Serial.println("Received PRB_T_CCC command");
        // responseValue = computer.read_temperature(CCC_CH);
        break;

      case AV_NET_PRB_P_CIG:
        Serial.println("Received AV_NET_PRB_P_CIG command");
        // responseValue = computer.read_pressure(CIG_CH);
        break;
      case AV_NET_PRB_T_CIG:
        Serial.println("Received AV_NET_PRB_T_CIG command");
        // responseValue = computer.read_temperature(CIG_CH);
        break;
      case AV_NET_PRB_VALVES_STATE:
        Serial.println("Received AV_NET_PRB_VALVES_STATE command");
        break;
      case AV_NET_PRB_ABORT:
        computer.manual_abort();
      case AV_NET_PRB_FSM_PRB:
        responseValue = computer.get_system_state();
        Wire.write((uint8_t*)&responseValue, sizeof(responseValue));
      default:
        Serial.println("Unknown command received");
        break;
    }
  }
}


// I2C request handler
void requestEvent() {
  Wire.write((uint8_t*)&responseValue, sizeof(responseValue));
}

void setup() {
  // PIN configuration
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

  // I2C with Raspberry Pi (use default Wire)
  Wire1.begin(SLAVE_ADDR);       // Set as I2C slave
  Wire1.onReceive(receiveEvent); // Register receive handler
  Wire1.onRequest(requestEvent); // Register request handler

  // Begin I2C communication with spine
  Wire1.begin(SLAVE_ADDR);

  // Begin I2C communication with sensors
  Wire2.begin();

  // Begin UART communication with EPOS4
  Serial7.begin(9600); //baudrate to determine

  // Analog sensor precision
  analogReadResolution(12);

  Serial.begin(115200); // For debugging
  Serial.println("PRB Computer started");

  digitalWrite(LED_BUILTIN, HIGH); // Turn the LED on (HIGH is the voltage level)
  delay(1000); // Wait for a second
  digitalWrite(LED_BUILTIN, LOW); // Turn the LED off by making the voltage LOW
}

PTE7300_I2C mySensor; // attach sensor
int16_t DSP_T1;

void loop() {
  delay(100);
}

// float temp = 0.0;
//   float press = 0.0;

//   temp = computer.read_temperature(EIN_CH);
//   Serial.print("Temperature EIN: ");
//   Serial.print(temp);
//   Serial.print(" °C\t");
//   press = computer.read_pressure(EIN_CH);
//   Serial.print("Pressure EIN: ");
//   Serial.println(press);

//   temp = computer.read_temperature(CCC_CH);
//   Serial.print("Temperature CCC: ");
//   Serial.print(temp);
//   Serial.print(" °C\t");
//   press = computer.read_pressure(CCC_CH);
//   Serial.print("Pressure CCC: ");
//   Serial.println(press);