// Last update: 19/08/2025
#include <Arduino.h>
#include <Wire.h>
#include "PRBComputer.h"
#include "wiring.h"

PRBComputer computer(IDLE);

// Variables for communication
volatile uint8_t received_buff[4];
volatile uint8_t received_cmd = 0;

volatile float resp_val_float = 0.0; // Hardcoded response
volatile uint32_t resp_val_int = 0x00; // Hardcoded response
volatile bool is_resp_int = false;

// I2C receive handler
void receiveEvent(int numBytes) {
  for (int i = 0; i < 4; ++i) received_buff[i] = 0;

  int bytesRead = 0;
  if (numBytes >= 1) {
    if (Wire1.available()) {
      received_cmd = Wire1.read(); // Read the command
      bytesRead++;
    }

    #ifdef DEBUG
    Serial.print("Received I2C command, nb bytes:");
    Serial.println(numBytes);
    Serial.print("(receiveEvent) Received command: ");
    Serial.println(received_cmd);
    #endif

    if (numBytes == 1) return;

    for (int i = 0; i < 4 && bytesRead < numBytes && Wire1.available(); ++i) {
      received_buff[i] = Wire1.read();
      bytesRead++;
    }

    #ifdef DEBUG
    Serial.print("Received I2C bytes: 0x");
    for (int i = 0; i < 4; ++i) {
      Serial.print(received_buff[i], HEX);
      if (i < 3) Serial.print(", ");
    }
    Serial.println(); 
    #endif
    

    
    // Set responseValue according to command, but do not write here
    switch (received_cmd) {
      case AV_NET_PRB_TIMESTAMP: {
        status_led(WHITE);
        break;
      }

      case AV_NET_PRB_WAKE_UP:
        Serial.println("Received AV_NET_PRB_WAKE_UP command");
        status_led(TEAL);
        break;

      case AV_NET_PRB_CLEAR_TO_IGNITE:
        Serial.println("Received AV_NET_PRB_CLEAR_TO_IGNITE command");
        if (received_buff[0] == AV_NET_CMD_ON) {
          status_led(GREEN);
          computer.set_state(CLEAR_TO_IGNITE);
        } else {
          status_led(RED);
        }
        break;

      case AV_NET_PRB_VALVES_STATE: {
        status_led(PURPLE);
        Serial.println("Received AV_NET_PRB_VALVES_STATE command");
        uint8_t valves_ME_State = received_buff[0];
        uint8_t valves_MO_State = received_buff[1];

        if (valves_ME_State == AV_NET_CMD_ON) {
          computer.open_valve(ME_b);
          status_led(GREEN);
        } else if (valves_ME_State == AV_NET_CMD_OFF) {
          computer.close_valve(ME_b);
          status_led(ORANGE);
        } else {
          status_led(RED);
        }

        if (valves_MO_State == AV_NET_CMD_ON) {
          computer.open_valve(MO_bC);
          status_led(GREEN);
        } else if (valves_MO_State == AV_NET_CMD_OFF) {
          computer.close_valve(MO_bC);
          status_led(ORANGE);
        } else {
          status_led(RED);
        }
        break;
      }

      case AV_NET_PRB_IGNITER: 
        Serial.println("Received AV_NET_PRB_IGNITER command");
        if (computer.get_state() == CLEAR_TO_IGNITE && received_buff[0] == AV_NET_CMD_ON) {
          computer.ignite(millis());
        }
        break;

      case AV_NET_PRB_ABORT:
        computer.set_state(ABORT);
        status_led(RED);
        break;

      default:
        Serial.println("Unknown or read command received");
        break;
    }
    Serial.println("End of command processing");
    Wire1.flush(); // Ensure all data is sent
  }
}

// I2C request handler
void requestEvent() {
  tone(BUZZER, 440, 500); // Indicate request received
  status_led(GREEN);
  
  if (Wire1.available()) {
    received_cmd = Wire1.read(); // Read the command
  }

  Serial.print("(requestEvent) Received command: ");
  Serial.println(received_cmd);

  prb_memory_t memory = computer.get_memory();

  switch (received_cmd)
  {
  case AV_NET_PRB_IS_WOKEN_UP:
    Serial.println("Received AV_NET_PRB_IS_WOKEN_UP command");
    resp_val_int = AV_NET_CMD_ON;
    is_resp_int = true;
    break;

  case AV_NET_PRB_FSM_PRB:
    resp_val_int = computer.get_state();
    is_resp_int = true;
    break;

  case AV_NET_PRB_P_OIN:
      Serial.println("Received AV_NET_PRB_P_OIN command");
      resp_val_float = memory.oin_press;
      is_resp_int = false; // We are sending a float response
      break;

  case AV_NET_PRB_T_OIN:
      Serial.println("Received AV_NET_PRB_T_OIN command");
      resp_val_float = memory.oin_temp;
      is_resp_int = false; // We are sending a float response
      break;

    case AV_NET_PRB_P_EIN:
      Serial.println("Received AV_NET_PRB_P_EIN command");
      resp_val_float = memory.ein_press;
      is_resp_int = false; // We are sending a float response
      break;

    case AV_NET_PRB_T_EIN:
      Serial.println("Received AV_NET_PRB_T_EIN command");
      resp_val_float = memory.ein_temp_sensata;
      is_resp_int = false; // We are sending a float response
      break;

    case AV_NET_PRB_P_CCC:
      Serial.println("Received AV_NET_PRB_P_CCC command");
      resp_val_float = memory.ccc_press;
      is_resp_int = false; // We are sending a float response
      break;

    case AV_NET_PRB_T_CCC:
      Serial.println("Received AV_NET_PRB_T_CCC command");
      resp_val_float = memory.ccc_temp;
      is_resp_int = false; // We are sending a float response
      break;

    case AV_NET_PRB_T_EIN_PT1000:
      Serial.println("Received AV_NET_PRB_T_EIN_PT1000 command");
      resp_val_float = memory.ein_temp_pt1000;
      is_resp_int = false; // We are sending a float response
      break;

    case AV_NET_PRB_VALVES_STATE: {
      Serial.println("Received AV_NET_PRB_VALVES_STATE read command");
      bool ME_state = memory.ME_state;
      bool MO_state = memory.MO_state;

      uint8_t response_ME = (ME_state) ? AV_NET_CMD_ON : AV_NET_CMD_OFF;
      uint8_t response_MO = (MO_state) ? AV_NET_CMD_ON : AV_NET_CMD_OFF;

      resp_val_int = (response_MO << 8) | response_ME;
      is_resp_int = true;
      break;
    }
  
  default:
    break;
  }
  
  if (is_resp_int) {
    Serial.print("Sending int response value (HEX): ");
    Serial.println(resp_val_int, HEX);
    Wire1.write((uint8_t*)&resp_val_int, AV_NET_XFER_SIZE);
  } else {
    Serial.print("Sending float response value (HEX): ");
    Serial.println(resp_val_float, HEX);
    Wire1.write((uint8_t*)&resp_val_float, AV_NET_XFER_SIZE);
  }
  Wire1.flush(); // Ensure all data is sent

  status_led(OFF);
}

void setup() {

  //PIN configuration
  pinMode(ME_b, OUTPUT);
  pinMode(MO_bC, OUTPUT);
  pinMode(IGNITER, OUTPUT);

  pinMode(T_EIN, INPUT);
  pinMode(T_OIN, INPUT);
  pinMode(P_OIN, INPUT);

  pinMode(RESET, OUTPUT);
  pinMode(RGB_RED, OUTPUT);
  pinMode(RGB_GREEN, OUTPUT);
  pinMode(RGB_BLUE, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  // Activate MUX
  digitalWrite(RESET, HIGH);

  // I2C with Raspberry Pi (use default Wire)
  Wire1.begin(SLAVE_ADDR);       // Set as I2C slave
  Wire1.onReceive(receiveEvent); // Register receive handler
  Wire1.onRequest(requestEvent); // Register request handler

  // Begin I2C communication with spine
  // Wire1.begin(SLAVE_ADDR);

  // Begin I2C communication with sensors
  Wire2.begin();

  // Analog sensor precision
  analogReadResolution(12);

  Serial.begin(115200); // For debugging
  Serial.println("PRB Computer started");

  turn_on_sequence();

  Serial.println("PRB Computer setup done");
}

// PTE7300_I2C mySensor; // attach sensor
// int16_t DSP_T1;

void loop() {

  computer.update(millis());

}