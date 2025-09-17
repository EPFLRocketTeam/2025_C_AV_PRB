// Last update: 05/09/2025
#include <Arduino.h>
#include <Wire.h>
#include "PRBComputer.h"
#include "wiring.h"

PRBComputer computer(IDLE);

// ================== I2C communication variables =================
volatile uint8_t received_buff[4];
volatile uint8_t received_cmd = 0;
volatile float resp_val_float = 0.0;
volatile uint32_t resp_val_int = 0x00;
volatile bool is_resp_int = false;

// ================= I2C event handlers =================

/**
 * @brief I2C event handler for receiving commands and data from the master device.
 *
 * This function is called automatically when data is received over the I2C bus (Wire1).
 * It reads the incoming command byte and up to 4 additional data bytes, then processes
 * the command accordingly. The function updates system state, controls hardware (such as valves
 * and igniters), and manages abort/passivation logic based on the received command and data.
 *
 * Command handling includes:
 * - AV_NET_PRB_TIMESTAMP: Updates status LED (WHITE).
 * - AV_NET_PRB_WAKE_UP: Reserved for wake-up logic. -> Deprived
 * - AV_NET_PRB_CLEAR_TO_IGNITE: Sets system state to CLEAR_TO_IGNITE if requested.
 * - AV_NET_PRB_RESET: Resets system state and deactivates MUX.
 * - AV_NET_PRB_VALVES_STATE: Opens or closes valves based on received states.
 * - AV_NET_PRB_IGNITER: Initiates ignition if system is clear to ignite.
 * - AV_NET_PRB_ABORT: Sets system to ABORT state, with optional passivation.
 * - AV_NET_PRB_PASSIVATE: Initiates passivation sequence if in ignition sequence.
 * - Default: Handles unknown or read commands.
 *
 * Debug output is available if DEBUG is defined.
 * 
 * @param numBytes Number of bytes received from the I2C master.
 *
 * @note This function should not be called directly; it is registered as an I2C event handler.
 * @note The function assumes global variables and objects such as Wire1, received_cmd, received_buff,
 *       computer, and various command/state constants are defined elsewhere.
 * @note The function flushes the I2C buffer at the end to ensure all data is processed.
 */
void receiveEvent(int numBytes) {
  for (int i = 0; i < 4; ++i) received_buff[i] = 0;
  int bytesRead = 0;

  if (numBytes >= 1) {

    if (Wire1.available()) {
      received_cmd = Wire1.read();
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
      // Serial.print(received_buff[i], HEX);
      if (i < 3) Serial.print(", ");
    }
    // Serial.println(); 
    #endif
    
    // Set responseValue according to command, but do not write here
    switch (received_cmd) {
      case AV_NET_PRB_TIMESTAMP: {
        status_led(WHITE);
        break;
      }

      case AV_NET_PRB_WAKE_UP:
        break;

      case AV_NET_PRB_CLEAR_TO_IGNITE:
        // Serial.println("Received AV_NET_PRB_CLEAR_TO_IGNITE command");
        if (received_buff[0] == AV_NET_CMD_ON) {
          computer.set_state(CLEAR_TO_IGNITE);
        }
        break;

      case AV_NET_PRB_RESET: {
        if (computer.get_state() == ABORT || computer.get_state() == PASSIVATION_SQ) {
          computer.set_state(IDLE);
          digitalWrite(RESET, LOW); // Deactivate MUX
        }
        break;
      }

      case AV_NET_PRB_VALVES_STATE: {
        status_led(PURPLE);
        // Serial.println("Received AV_NET_PRB_VALVES_STATE command");
        uint8_t valves_ME_State = received_buff[0];
        uint8_t valves_MO_State = received_buff[1];

        if (valves_ME_State == AV_NET_CMD_ON) {
          // Serial.println("Opening ME_b valve");
          computer.open_valve(ME_b);
          status_led(GREEN);
        } else if (valves_ME_State == AV_NET_CMD_OFF) {
          // Serial.println("Closing ME_b valve");
          computer.close_valve(ME_b);
        } else {
          Serial.print("Unknown state for ME_b valve: ");
          Serial.println(valves_ME_State, HEX);
        }

        if (valves_MO_State == AV_NET_CMD_ON) {
          // Serial.println("Opening MO_bC valve");
          computer.open_valve(MO_bC);
        } else if (valves_MO_State == AV_NET_CMD_OFF) {
          // Serial.println("Closing MO_bC valve");
          computer.close_valve(MO_bC);
        } else {
          Serial.print("Unknown state for MO_bC valve: ");
          Serial.println(valves_MO_State, HEX);
        }
        break;
      }

      case AV_NET_PRB_IGNITER: 
        // Serial.println("Received AV_NET_PRB_IGNITER command");
        if (computer.get_state() == CLEAR_TO_IGNITE && received_buff[0] == AV_NET_CMD_ON) {
          computer.ignite(millis());
        }
        break;

      case AV_NET_PRB_ABORT: {
        bool abort_passivation = (received_buff[0] == AV_NET_CMD_ON);
        if (abort_passivation) {
          // Serial.println("Received AV_NET_PRB_ABORT command - Passivation");
          computer.set_state(ABORT);
          computer.set_passivation(true);
        } else {
          // Serial.println("Received AV_NET_PRB_ABORT command - Abort");
          computer.set_state(ABORT);
          computer.set_passivation(false);
          // status_led(RED);
        }
        break;
      }

      case AV_NET_PRB_PASSIVATE: {
        // Serial.println("Received AV_NET_PRB_PASSIVATE command");
        if (computer.get_state() == IGNITION_SQ) {
          computer.set_state(PASSIVATION_SQ);
          computer.set_passivation_stage(PASSIVATION_ETH);
        }
        break;
      }

      default:
        // Serial.println("Unknown or read command received");
        break;
    }
    // Serial.println("End of command processing");
    Wire1.flush(); // Ensure all data is sent
  }
}


/**
 * @brief I2C event handler for responding to master device requests.
 *
 * This function is called automatically when the master device requests data over the I2C bus (Wire1).
 * It checks the last received command and prepares an appropriate response based on the current
 * state of the system and the requested information. The response can be either an integer or a float,
 * depending on the command.
 *
 * The function handles various commands, including:
 * - AV_NET_PRB_IS_WOKEN_UP: Responds with whether the system is awake.
 * - AV_NET_PRB_FSM_PRB: Responds with the current FSM state.
 * - AV_NET_PRB_P_OIN, AV_NET_PRB_T_OIN, AV_NET_PRB_P_EIN, AV_NET_PRB_T_EIN, AV_NET_PRB_P_CCC, AV_NET_PRB_T_CCC,
 *   AV_NET_PRB_T_EIN_PT1000: Responds with corresponding pressure or temperature readings.
 * - AV_NET_PRB_VALVES_STATE: Responds with the current state of the valves.
 * - AV_NET_PRB_SPECIFIC_IMP: Responds with the engine's specific impulse.
 *
 * Debug output is available if DEBUG is defined.
 * 
 * @note This function should not be called directly; it is registered as an I2C event handler.
 * @note The function assumes global variables and objects such as Wire1, received_cmd, resp_val_float,
 *       resp_val_int, is_resp_int, computer, prb_memory_t structure, and various command/state constants
 *       are defined elsewhere.
 * @note The function flushes the I2C buffer at the end to ensure all data is sent.
 */
void requestEvent() {
  if (Wire1.available()) {
    received_cmd = Wire1.read(); // Read the command
  }

  prb_memory_t memory = computer.get_memory();

  switch (received_cmd) {
    case AV_NET_PRB_IS_WOKEN_UP:
      resp_val_int = AV_NET_CMD_ON;
      is_resp_int = true;
      break;

    case AV_NET_PRB_FSM_PRB:
      resp_val_int = computer.get_state();
      is_resp_int = true;
      break;

    case AV_NET_PRB_P_OIN:
      resp_val_float = memory.oin_press;
      is_resp_int = false; // We are sending a float response
      break;

    case AV_NET_PRB_T_OIN:
      resp_val_float = memory.oin_temp;
      is_resp_int = false; // We are sending a float response
      break;

    case AV_NET_PRB_P_EIN:
      resp_val_float = memory.ein_press;
      is_resp_int = false; // We are sending a float response
      break;

    case AV_NET_PRB_T_EIN:
      resp_val_float = memory.ein_temp_sensata;
      is_resp_int = false; // We are sending a float response
      break;

    case AV_NET_PRB_P_CCC:
      resp_val_float = memory.ccc_press;
      is_resp_int = false; // We are sending a float response
      break;

    case AV_NET_PRB_T_CCC:
      resp_val_float = memory.ccc_temp;
      is_resp_int = false; // We are sending a float response
      break;

    case AV_NET_PRB_T_EIN_PT1000:
      resp_val_float = memory.ein_temp_pt1000;
      is_resp_int = false; // We are sending a float response
      break;

    case AV_NET_PRB_VALVES_STATE: {
      bool ME_state = memory.ME_state;
      bool MO_state = memory.MO_state;

      uint8_t response_ME = (ME_state) ? AV_NET_CMD_ON : AV_NET_CMD_OFF;
      uint8_t response_MO = (MO_state) ? AV_NET_CMD_ON : AV_NET_CMD_OFF;

      resp_val_int = (response_MO << 8) | response_ME;
      is_resp_int = true;
      break;
    }

    case AV_NET_PRB_SPECIFIC_IMP: {
      resp_val_float = memory.engine_total_impulse;
      is_resp_int = false; // We are sending a float response
      break;
    }

    default:
      break;
  }
  
  #ifdef DEBUG
  // Serial.print("Preparing to send response for command: ");
  // Serial.println(received_cmd);
  // if (is_resp_int) {
  //   Serial.print("Response is int: ");
  //   Serial.println(resp_val_int, HEX);
  // } else {
  //   Serial.print("Response is float: ");
  //   Serial.println(resp_val_float);
  // }
  #endif

  if (is_resp_int) {
    Wire1.write((uint8_t*)&resp_val_int, AV_NET_XFER_SIZE);
  } else {
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
#ifdef KULITE
  pinMode(P_OIN, INPUT);
#endif

  pinMode(RESET, OUTPUT);
  pinMode(RGB_RED, OUTPUT);
  pinMode(RGB_GREEN, OUTPUT);
  pinMode(RGB_BLUE, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  // Activate MUX
  digitalWrite(RESET, HIGH);

  // I2C with Raspberry Pi (use default Wire)
  Wire1.begin(AV_NET_ADDR_PRB);  // Set as I2C slave
  Wire1.onReceive(receiveEvent); // Register receive handler
  Wire1.onRequest(requestEvent); // Register request handler

  // Begin I2C communication with sensors
  Wire2.begin();

  // Analog sensor precision
  analogReadResolution(12);

  Serial.begin(115200); // For debugging
  Serial.println("PRB Computer started");

  turn_on_sequence();

  Serial.println("PRB Computer setup done");
}

void loop() {
  computer.update(millis());
}