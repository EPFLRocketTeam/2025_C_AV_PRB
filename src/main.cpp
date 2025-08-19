// 
#include <Arduino.h>
#include <Wire.h>
#include "PRBComputer.h"
#include "wiring.h"

// // Define I2C slave address for Raspberry Pi
// #define SLAVE_ADDR 0x08

PRBComputer computer(IDLE);

// Variables for communication
volatile uint8_t received_buffer[4];
volatile uint8_t received_command = 0;

volatile float responseValue_float = 0.0; // Hardcoded response
volatile uint32_t responseValue_int = 0x00; // Hardcoded response
volatile bool sent_int_response = false;

// I2C receive handler
void receiveEvent(int numBytes) {
  // Wire2.endTransmission();
  // Clear buffer
  for (int i = 0; i < 4; ++i) received_buffer[i] = 0;

  Serial.print("Received I2C command, nb bytes:");
  Serial.println(numBytes);

  int bytesRead = 0;
  if (numBytes >= 1) {
    if (Wire1.available()) {
      received_command = Wire1.read(); // Read the command
      bytesRead++;
    }

    Serial.print("(receiveEvent) Received command: ");
    Serial.println(received_command);

    if (numBytes == 1) return;

    for (int i = 0; i < 4 && bytesRead < numBytes && Wire1.available(); ++i) {
      received_buffer[i] = Wire1.read();
      bytesRead++;
    }

    Serial.print("Received I2C bytes: 0x");
    for (int i = 0; i < 4; ++i) {
      Serial.print(received_buffer[i], HEX);
      if (i < 3) Serial.print(", ");
    }
    Serial.println(); 

    
    // Set responseValue according to command, but do not write here
    switch (received_command) {
      case AV_NET_PRB_TIMESTAMP: {
        status_led_white();
        break;
      }

      case AV_NET_PRB_WAKE_UP:
        Serial.println("Received AV_NET_PRB_WAKE_UP command");
        status_led_teal();
        computer.set_state(WAKEUP);
        break;

      case AV_NET_PRB_CLEAR_TO_IGNITE:
        Serial.println("Received AV_NET_PRB_CLEAR_TO_IGNITE command");
        if (received_buffer[0] == AV_NET_CMD_ON) {
          status_led_green();
          computer.set_state(CLEAR_TO_IGNITE);
        } else {
          status_led_red();
        }
        break;

      case AV_NET_PRB_VALVES_STATE: {
        status_led_purple();
        Serial.println("Received AV_NET_PRB_VALVES_STATE command");
        uint8_t valves_ME_State = received_buffer[0];
        uint8_t valves_MO_State = received_buffer[1];

        if (valves_ME_State == AV_NET_CMD_ON) {
          computer.open_valve(ME_b);
          status_led_green();
        } else if (valves_ME_State == AV_NET_CMD_OFF) {
          computer.close_valve(ME_b);
          status_led_orange();
        } else {
          status_led_red();
        }

        if (valves_MO_State == AV_NET_CMD_ON) {
          computer.open_valve(MO_bC);
          status_led_green();
        } else if (valves_MO_State == AV_NET_CMD_OFF) {
          computer.close_valve(MO_bC);
          status_led_orange();
        } else {
          status_led_red();
        }
        break;
      }

      case AV_NET_PRB_IGNITER: {
        Serial.println("Received AV_NET_PRB_IGNITER command");
        prometheusFSM state = computer.get_state();
        if (state == CLEAR_TO_IGNITE && received_buffer[0] == AV_NET_CMD_ON) {
          computer.set_state(IGNITION_SQ);
          computer.set_time_start_sq(millis());
          computer.set_ignition_stage(GO);
        }
        break;
      }

      case AV_NET_PRB_ABORT:
        computer.set_state(ABORT);
        status_led_red();
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
  // float woken_up = 0.0;
  tone(BUZZER, 440, 1000); // Indicate request received
  
  if (Wire1.available()) {
    received_command = Wire1.read(); // Read the command
  }

  Serial.print("(requestEvent) Received command: ");
  Serial.println(received_command);


  switch (received_command)
  {
  case AV_NET_PRB_IS_WOKEN_UP:
    Serial.println("Received AV_NET_PRB_IS_WOKEN_UP command");
    status_led_purple();
    if (computer.get_state() == WAKEUP) {
      responseValue_int = AV_NET_CMD_ON;
    } else {
      responseValue_int = AV_NET_CMD_OFF;
    }
    sent_int_response = true;
    break;

  case AV_NET_PRB_P_OIN:
      Serial.println("Received AV_NET_PRB_P_OIN command");
      responseValue_float = computer.get_sensor_value(P_OIN_SENS);
      sent_int_response = false; // We are sending a float response
      break;

  case AV_NET_PRB_T_OIN:
      Serial.println("Received AV_NET_PRB_T_OIN command");
      responseValue_float = computer.get_sensor_value(T_OIN_SENS);
      sent_int_response = false; // We are sending a float response
      break;

    case AV_NET_PRB_P_EIN:
      Serial.println("Received AV_NET_PRB_P_EIN command");
      responseValue_float = computer.get_sensor_value(P_EIN_SENS);
      sent_int_response = false; // We are sending a float response
      break;

    case AV_NET_PRB_T_EIN: {
      Serial.println("Received AV_NET_PRB_T_EIN command");
      responseValue_float = computer.get_sensor_value(T_EIN_SENS);
      sent_int_response = false; // We are sending a float response
      break;
    }

    case AV_NET_PRB_P_CCC:
      Serial.println("Received AV_NET_PRB_P_CCC command");
      responseValue_float = computer.get_sensor_value(P_CCC_SENS);
      sent_int_response = false; // We are sending a float response
      break;

    case AV_NET_PRB_T_CCC:
      Serial.println("Received AV_NET_PRB_T_CCC command");
      responseValue_float = computer.get_sensor_value(T_CCC_SENS);
      sent_int_response = false; // We are sending a float response
      break;

    case AV_NET_PRB_VALVES_STATE: {
      status_led_green();
      Serial.println("Received AV_NET_PRB_VALVES_STATE read command");
      bool ME_state = computer.get_valve_state(ME_b);
      bool MO_state = computer.get_valve_state(MO_bC);

      uint8_t response_ME = (ME_state) ? AV_NET_CMD_ON : AV_NET_CMD_OFF;
      uint8_t response_MO = (MO_state) ? AV_NET_CMD_ON : AV_NET_CMD_OFF;

      responseValue_int = (response_MO << 8) | response_ME;
      sent_int_response = true;
      break;
    }

    case AV_NET_PRB_FSM_PRB:
      responseValue_int = computer.get_state();
      sent_int_response = true;
      status_led_blue();
      break;
  
  default:
    break;
  }
  
  if (sent_int_response) {
    Serial.print("Sending int response value (HEX): ");
    Serial.println(responseValue_int, HEX);
    Wire1.write((uint8_t*)&responseValue_int, AV_NET_XFER_SIZE);
  } else {
    Serial.print("Sending float response value (HEX): ");
    Serial.println(responseValue_float, HEX);
    Wire1.write((uint8_t*)&responseValue_float, AV_NET_XFER_SIZE);
  }
  Wire1.flush(); // Ensure all data is sent
}

void turn_on_sequence()
{
  digitalWrite(LED_BUILTIN, HIGH);

  status_led_blue();
  delay(500);
  status_led_green();
  delay(500);
  status_led_red();
  delay(500);
  status_led_white();
  tone(BUZZER, 440, 1000);
  delay(1000);
  noTone(BUZZER);
  status_led_off();

  digitalWrite(LED_BUILTIN, LOW);
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

  // computer.set_state(IGNITION_SQ);
  // computer.set_time_start_sq(millis());
  // computer.set_ignition_stage(GO);
}

// PTE7300_I2C mySensor; // attach sensor
// int16_t DSP_T1;

void loop() {

  computer.update(millis());

}