/*
 * File: PRBComputer.cpp
 * Author: C - AV Team
 * Last update: 05/09/2025
 *
 * Description:
 *  This file implements the PRBComputer class, which manages the core logic and control
 *  of the PRB (Propulsion Rocket Bench) system. It includes:
 *    - State machine management for ignition, passivation, and abort sequences
 *    - Valve control (open/close for main engine, oxidizer, and igniter)
 *    - Sensor reading functions for pressure and temperature (analog and I2C)
 *    - Memory structure for storing system state, sensor data, and control flags
 *    - Getters and setters for system state and memory
 *    - High-level ignition and shutdown sequence logic
 *
 *  The class interfaces with hardware via digital and analog I/O, as well as I2C communication.
 *  It is designed for embedded use in the PRB avionics system.
 */

// =================== Implementation ===================
//
// - PRBComputer constructor/destructor
// - Valve control methods
// - Sensor reading methods
// - State and memory accessors
// - Ignition and shutdown sequence logic
//
// ======================================================

#include "PRBComputer.h"
#include "Wire.h"

PRBComputer::PRBComputer(PRB_FSM state)
{
    state = state;
    memory.time_ignition = 0;
    memory.time_passivation = 0;
    ignition_phase = NOGO;
    passivation_phase = SLEEP;
    memory.status_led = false;
    memory.time_led = 0;
    memory.time_print = 0;
    memory.ME_state = false;
    memory.MO_state = false;
    memory.IGNITER_state = false;
    memory.integral = 0.0;
    memory.integral_past_time = 0;
    memory.engine_total_impulse = 0.0;
    memory.calculate_integral = false;
    memory.passivation = false;
    for (int i = 0; i < 5; i++) {
        memory.ccc_press_buffer[i] = 0.0;
    }
    memory.ccc_press_index = 0;
}

PRBComputer::~PRBComputer()
{
    state = ERROR;
}

// ========= valve control =========
/**
 * @brief Open the specified valve by updating its state and setting its control pin HIGH.
 *
 * This function sets the internal state of the specified valve to 'open' (true)
 * in the memory structure and sends a HIGH signal to the corresponding hardware pin
 * to physically open the valve.
 *
 * @param valve The identifier of the valve to open. Valid values are:
 *              - ME_b: Main Engine valve
 *              - MO_bC: Main Oxidizer valve
 *              - IGNITER: Igniter
 *
 * If an unknown valve identifier is provided, the function does nothing.
 */
void PRBComputer::open_valve(int valve)
{
    switch (valve)
    {
    case ME_b:
        memory.ME_state = true;
        break;
    case MO_bC:
        memory.MO_state = true;
        break;
    case IGNITER:
        memory.IGNITER_state = true;
        break;
    default:
        break;
    }
    digitalWrite(valve, HIGH);
}

/**
 * @brief Closes the specified valve by updating its state and setting its control pin LOW.
 *
 * This function sets the internal state of the specified valve to 'closed' (false)
 * in the memory structure and sends a LOW signal to the corresponding hardware pin
 * to physically close the valve.
 *
 * @param valve The identifier of the valve to close. Valid values are:
 *              - ME_b: Main Engine valve
 *              - MO_bC: Main Oxidizer valve
 *              - IGNITER: Igniter
 *
 * If an unknown valve identifier is provided, the function does nothing.
 */
void PRBComputer::close_valve(int valve)
{
    switch (valve)
    {
    case ME_b:
        memory.ME_state = false;
        break;
    case MO_bC:
        memory.MO_state = false;
        break;
    case IGNITER:
        memory.IGNITER_state = false;
        break;

    default:
        break;
    }
    digitalWrite(valve, LOW);
}


// ========= sensor reading =========
/**
 * @brief Reads the pressure value from the specified sensor.
 *
 * This function reads the pressure from either an analog or I2C-based sensor,
 * depending on the sensor identifier provided. For analog sensors (e.g., KULITE),
 * it reads the raw ADC value, converts it to voltage, and then calculates the pressure.
 * For I2C sensors, it selects the appropriate I2C channel, reads the digital sensor value,
 * and converts it to pressure in bar. The function handles sensor selection and
 * communication protocol internally.
 *
 * @param sensor The identifier of the pressure sensor to read from. Valid values include:
 *               - P_OIN: Analog or I2C sensor (depending on compilation flags)
 *               - EIN_CH: I2C sensor channel
 *               - CCC_CH: I2C sensor channel
 *
 * @return The measured pressure value in bar.
 *
 * @note If an unknown sensor identifier is provided, the function returns 0.0.
 * @note The function assumes a 12-bit ADC and 3.3V reference for analog sensors.
 * @note I2C communication is properly closed before returning.
 */
float PRBComputer::read_pressure(int sensor)
{
    bool I2C = false;
    int DSP_S = 0;
    float press = 0.0;

    switch (sensor)
    {
    case P_OIN: {
        #ifdef KULITE
        int max_kulite_value = 100;
        int rawValue = analogRead(P_OIN);
        float voltage = (rawValue / 4095.0) * 3.3; // Assuming a 12-bit ADC and 3.3V reference
        float v_sensor = voltage / 33;
        press = (v_sensor * 1000.0) * (max_kulite_value / 100.0);
        #else 
        selectI2CChannel(sensor);
        I2C = true;
        #endif
        break;
    }

    case EIN_CH:
    case CCC_CH:
        selectI2CChannel(sensor);
        I2C = true;
        break;
    
    default:
        //error
        break;
    }
    
    if (I2C)
    {
        DSP_S = my_sensor.readDSP_S();
        press = ((DSP_S - (-16000.0)) * (100.0) / (16000.0 - (-16000.0))); // in bar
    }

    endI2CCommunication();

    return press;
}


/**
 * @brief Reads the temperature from the specified sensor.
 *
 * This function reads the temperature value from either an analog or I2C-based sensor,
 * depending on the sensor identifier provided. For analog sensors (e.g., T_OIN, T_EIN),
 * it reads the analog value, converts it to voltage, calculates the resistance of a PT1000
 * sensor using a resistor divider formula, and then computes the temperature in Celsius.
 * For I2C-based sensors (e.g., EIN_CH, CCC_CH), it selects the appropriate I2C channel,
 * reads the digital temperature value, and converts it to Celsius using a sensor-specific formula.
 *
 * @param sensor The identifier of the sensor to read from.
 *               - For analog sensors: T_OIN, T_EIN
 *               - For I2C sensors: EIN_CH, CCC_CH
 * @return The measured temperature in degrees Celsius.
 */
float PRBComputer::read_temperature(int sensor)
{
    bool I2C = false;
    int value = 0.0;
    float voltage = 0.0, resistance_pt1000 = 0.0, temp = 0.0;
    int DSP_T = 0;

    switch (sensor)
    {
    case T_OIN:
    case T_EIN:
        //read analog temperature
        value = analogRead(sensor);
        voltage = (value * 3.3) / 4095.0; // Assuming a 12-bit ADC and 3V3 reference
        resistance_pt1000 = (voltage * 1100.0)/(3.3 - voltage); // formula from a resistor divider
        temp = (resistance_pt1000-1000)/3.85;
        break;

    case EIN_CH:
    case CCC_CH:
        selectI2CChannel(sensor);
        I2C = true;
        break;
    
    default:
        break;
    }
    
    if (I2C)
    {
        DSP_T = my_sensor.readDSP_T();
        temp = DSP_T * 82.5 / 16000 + 42.5;
    }

    endI2CCommunication();

    return temp;
}

// ========= getter =========
prb_memory_t PRBComputer::get_memory() { return memory; }
PRB_FSM PRBComputer::get_state() { return state; }
ignitionStage PRBComputer::get_ignition_stage() { return ignition_phase; }
passivationStage PRBComputer::get_shutdown_stage() { return passivation_phase; }


// ========= setter =========
void PRBComputer::set_state(PRB_FSM new_state) { state = new_state; }
void PRBComputer::set_passivation(bool passiv) { memory.passivation = passiv; }
void PRBComputer::set_passivation_stage(passivationStage new_stage) { passivation_phase = new_stage; };


// ============================ ignition sequences ============================

/**
 * @brief Manages the ignition sequence state machine for the PRB computer.
 *
 * This function controls the ignition process by transitioning through various phases,
 * operating valves, and monitoring system parameters such as pressure and timing.
 * The sequence includes pre-chill, ignition, burn start, pressure check, burn, and shutdown phases.
 * It also handles abort and passivation logic based on system state and sensor feedback.
 *
 * The function should be called periodically (e.g., in a main loop or timer interrupt)
 * to ensure proper sequencing and timing of ignition events.
 *
 * Phases handled:
 * - PRE_CHILL: Opens oxidizer valve for pre-chilling.
 * - IGNITION: Closes pre-chill valve, opens igniter, and waits for ignition duration.
 * - BURN_START_ME: Opens main engine valve, closes igniter, and waits for ignition delay.
 * - BURN_START_MO: Opens oxidizer valve, transitions to pressure check.
 * - PRESSURE_CHECK: Monitors chamber pressure and aborts if insufficient.
 * - BURN: Integrates chamber pressure for total impulse or burns for a fixed duration.
 * - BURN_STOP_MO: Closes oxidizer valve.
 * - BURN_STOP_ME: Closes main engine valve after cutoff delay.
 * - WAIT_FOR_PASSIVATION: Waits before transitioning to passivation sequence.
 * - Handles abort and passivation transitions as needed.
 *
 * Uses timing functions (millis()) and updates internal memory/state variables.
 * Relies on pre-defined constants for durations, pressures, and thresholds.
 *
 */
void PRBComputer::ignition_sq()
{
    switch (ignition_phase)
    {
    case PRE_CHILL:
        open_valve(MO_bC);
        ignition_phase = IGNITION;
        memory.time_ignition = millis();
        break;
    
    case IGNITION:
        if (millis() - memory.time_ignition >= PRECHILL_DURATION) {
            close_valve(MO_bC);
            open_valve(IGNITER);
            ignition_phase = BURN_START_ME;
            memory.time_ignition = millis();
        }
        break;

    case BURN_START_ME:
        if (millis() - memory.time_ignition >= IGNITER_DURATION) {
            open_valve(ME_b);
            close_valve(IGNITER);
            ignition_phase = BURN_START_MO;
            memory.time_ignition = millis();
        }
        break;

    case BURN_START_MO:
        if (millis() - memory.time_ignition >= IGNITION_DELAY) {
            open_valve(MO_bC);
            ignition_phase = PRESSURE_CHECK;
            memory.time_ignition = millis();
        }
        break;

    case PRESSURE_CHECK: {
        memory.ccc_press_buffer[memory.ccc_press_index] = memory.ccc_press; // in Pa
        memory.ccc_press_index = (memory.ccc_press_index + 1) % 5;
        float sum_ccc_press = 0.0;
        for (int i = 0; i < 5; i++) {
            sum_ccc_press += memory.ccc_press_buffer[i];
        }
        float mean_ccc_press = sum_ccc_press / 5.0;

        if (millis() - memory.time_ignition >= RAMPUP_DURATION) {
            if (mean_ccc_press >= RAMP_UP_CHECK_PRESSURE) {
                ignition_phase = BURN;
                memory.time_ignition = millis();
            } else {
                state = ABORT;
                abort_phase = ABORT_OXYDANT;
                memory.time_abort = millis();
            }
        }
        break;
        }
    
    case BURN: {
        #ifdef INTEGRATE_CHAMBER_PRESSURE

        if (memory.integral_past_time == 0) {
            memory.integral_past_time = millis();
            break;
        }

        float chamber_pressure_bar = memory.ccc_press / 100000.0; // Convert Pa to bar
        memory.integral += chamber_pressure_bar * (millis() - memory.integral_past_time); // in Pa.s
        memory.integral_past_time = millis();

        memory.engine_total_impulse = I_SP * G * (AREA_THROAT/C_STAR) * memory.integral;

        if (millis() - memory.time_ignition <= (MIN_BURN_TIME)) {
            break;
        }

        if (memory.integral >= (I_TARGET * C_STAR) / (I_SP * G * AREA_THROAT) ||
            millis() - memory.time_ignition >= (MAX_BURN_TIME)) {
            ignition_phase = BURN_STOP_MO;
            memory.time_ignition = millis();
        }

        #else

        if (millis() - memory.time_ignition >= BURN_DURATION) {
            ignition_phase = BURN_STOP_MO;
            memory.time_ignition = millis();
        }
        break;

        #endif
        }

        case BURN_STOP_MO:
            close_valve(MO_bC);
            ignition_phase = BURN_STOP_ME;
            memory.time_ignition = millis();

        case BURN_STOP_ME:
            if (millis() - memory.time_ignition >= CUTOFF_DELAY) {
                close_valve(ME_b);
                ignition_phase = WAIT_FOR_PASSIVATION;
                memory.time_ignition = millis();
            }
            break;

        case WAIT_FOR_PASSIVATION:
        {
            if (millis() - memory.time_ignition >= PASSIVATION_DELAY) {
                state = PASSIVATION_SQ;
                memory.time_passivation = millis();
                passivation_phase = PASSIVATION_ETH;
                ignition_phase = NOGO;
            }
            break;
        }

    default:
        break;
    }
}


// ========================== passivation sequence =============================

/**
 * @brief Handles the sequential passivation process for the PRBComputer.
 *
 * This function manages the passivation sequence by transitioning through different
 * phases: PASSIVATION_ETH, PASSIVATION_LOX, and SHUTOFF. In each phase, it controls
 * the opening and closing of specific valves and updates timing and state variables
 * accordingly. The transitions between phases are based on elapsed time and are used
 * to safely passivate the system.
 *
 * Phases:
 * - PASSIVATION_ETH: Opens the ME_b valve (unless VSTF_AND_COLD_FLOW is defined),
 *   sets the next phase to PASSIVATION_LOX, and records the current time.
 * - PASSIVATION_LOX: After PASSIVATION_FUEL_DURATION has elapsed, closes ME_b,
 *   opens MO_bC, sets the next phase to SHUTOFF, and records the current time.
 * - SHUTOFF: After PASSIVATION_OX_DURATION has elapsed, closes MO_bC and sets the
 *   phase to SLEEP.
 *
 * The function is intended to be called periodically to advance the passivation
 * sequence based on timing and system state.
 */
void PRBComputer::passivation_sq()
{
    switch (passivation_phase)
    {
    case PASSIVATION_ETH:
        #ifndef VSTF_AND_COLD_FLOW
            open_valve(ME_b);
        #endif
        passivation_phase = PASSIVATION_LOX;
        memory.time_passivation = millis();
        break;

    case PASSIVATION_LOX:
        if (millis() - memory.time_passivation >= PASSIVATION_FUEL_DURATION)
        {
            close_valve(ME_b);
            open_valve(MO_bC);
            passivation_phase = SHUTOFF;
            memory.time_passivation = millis();
        }
        break;

    case SHUTOFF:
        if (millis() - memory.time_passivation >= PASSIVATION_OX_DURATION)
        {
            close_valve(MO_bC);
            passivation_phase = SLEEP;
        }
        break;

    default:
        break;
    }
}


// ============================ abort sequence ===============================

/**
 * @brief Handles the abort sequence for the PRBComputer system.
 *
 * This function manages the different phases of the abort sequence by closing valves
 * and transitioning between abort states based on elapsed time and system memory.
 * The abort sequence consists of multiple phases:
 *   - ABORT_OXYDANT: Closes the oxidant and igniter valves, records the abort time,
 *     and transitions to the ethanol abort phase.
 *   - ABORT_ETHANOL: After a defined cutoff delay, closes the ethanol valve.
 *   - WAIT_FOR_PASSIVATION_ABORT: After a passivation delay, checks if passivation is required.
 *     If so, transitions to the passivation sequence.
 * The function uses the system's memory and timing functions to ensure safe and orderly
 * shutdown of the relevant components during an abort event.
 */
void PRBComputer::abort_sq()
{
    switch (abort_phase)
    {
        case ABORT_OXYDANT:
            close_valve(MO_bC);
            close_valve(IGNITER);
            memory.time_abort = millis();
            abort_phase = ABORT_ETHANOL;
            break;

        case ABORT_ETHANOL:
            if (millis() - memory.time_abort >= CUTOFF_DELAY)
            {
                close_valve(ME_b);
            }
            break;

        case WAIT_FOR_PASSIVATION_ABORT:
            if (millis() - memory.time_abort >= ABORT_PASSIVATION_DELAY)
            {
                if (memory.passivation) {
                    state = PASSIVATION_SQ;
                    passivation_phase = PASSIVATION_ETH;
                    memory.time_passivation = millis();
                }
            }
            break;

        default:
            break;
    }
}

// ============================ I2C multiplexer control ===============================

/**
 * @brief Selects a specific I2C channel on the multiplexer.
 *
 * This function resets the I2C multiplexer by toggling the RESET pin,
 * then enables only the specified channel by sending the channel number
 * to the multiplexer via the I2C bus (Wire2).
 *
 * @param channel The channel number to enable on the I2C multiplexer.
 */
void selectI2CChannel(int channel) {
    digitalWrite(RESET, LOW);
    delay(10);
    digitalWrite(RESET, HIGH);

    noInterrupts();
    Wire2.beginTransmission(MUX_ADDR);
    Wire2.write(channel); // Enable only the selected channel
    Wire2.endTransmission();
    interrupts();
}


/**
 * @brief Ends I2C communication by disabling all channels on the I2C multiplexer.
 *
 * This function sends a command to the I2C multiplexer at address MUX_ADDR
 * to disable all its channels, effectively ending any ongoing I2C communication
 * through the multiplexer. It uses the Wire2 interface for communication.
 */
void endI2CCommunication() {
    noInterrupts();
    Wire2.beginTransmission(MUX_ADDR);
    Wire2.write(0); // Disable all channels
    Wire2.endTransmission();
    interrupts();
}

 /**
 * @brief Initiates the ignition sequence for the PRBComputer.
 *
 * This function sets the internal state to indicate that the ignition sequence has started.
 * It also sets the ignition phase to the pre-chill stage and records the ignition time in memory.
 *
 * @param time The current time (in appropriate units) at which ignition is initiated.
 */
void PRBComputer::ignite(int time)
{
    state = IGNITION_SQ;
    ignition_phase = PRE_CHILL;
    memory.time_ignition = time;
}

/**
 * @brief Updates the PRBComputer state and sensor readings.
 *
 * This function manages the state machine of the PRBComputer, handling different states
 * such as IDLE, CLEAR_TO_IGNITE, IGNITION_SQ, PASSIVATION_SQ, and ABORT. It also reads
 * various sensors (temperature and pressure) and updates the internal memory with the
 * latest readings. The function includes optional debug output to print the current state
 * and sensor values at regular intervals.
 *
 * @param time The current time (in milliseconds) used for timing operations.
 */
void PRBComputer::update(int time)
{
    switch (state)
    {
        case IDLE:
            if (!memory.status_led && time - memory.time_led >= LED_TIMEOUT) {
                status_led(TEAL);
                memory.time_led = time;
                memory.status_led = true;
            } else if (memory.status_led && time - memory.time_led >= LED_TIMEOUT) {
                status_led(OFF);
                memory.time_led = time;
                memory.status_led = false;
            }

            break;
        
        case CLEAR_TO_IGNITE:
            if (!memory.status_led && time - memory.time_led >= LED_TIMEOUT) {
                status_led(ORANGE);
                memory.time_led = time;
                memory.status_led = true;
            } else if (memory.status_led && time - memory.time_led >= LED_TIMEOUT) {
                status_led(OFF);
                memory.time_led = time;
                memory.status_led = false;
            }
            break;

        case IGNITION_SQ:
            ignition_sq();
            status_led(BLUE);
            break;

        case PASSIVATION_SQ:
            passivation_sq();
            status_led(PURPLE);
            break;

        case ABORT:
            abort_sq();
            status_led(RED);
            break;

        default:
            break;
    }

    if(time - memory.time_sensors_update > SENSORS_POLLING_RATE_MS) {
        memory.ein_temp_sensata = read_temperature(EIN_CH);
        memory.ein_press = read_pressure(EIN_CH);
        memory.ccc_temp = read_temperature(CCC_CH);
        memory.ccc_press = read_pressure(CCC_CH);
        memory.oin_temp = read_temperature(T_OIN);
        memory.ein_temp_pt1000 = read_temperature(T_EIN);
        memory.oin_press = read_pressure(P_OIN);
        memory.time_sensors_update = time;

        #ifdef INTEGRATE_CHAMBER_PRESSURE
        if (memory.calculate_integral) {
            if (memory.integral_past_time == 0) {
                memory.integral_past_time = time;
            } else {
                float chamber_pressure_bar = memory.ccc_press / 100000.0; // Convert Pa to bar
                memory.integral += chamber_pressure_bar * (time - memory.integral_past_time); // in bar.ms
                memory.integral_past_time = time;
                memory.engine_total_impulse = I_SP * G * (AREA_THROAT/C_STAR) * memory.integral;
            }
        }
    }
    #endif

    #ifdef DEBUG
    if (time - memory.time_print >= LED_TIMEOUT) {
        Serial.print("State : ");
        Serial.println(state);
        Serial.print("EIN T°: ");
        Serial.println(memory.ein_temp_sensata);
        Serial.print("EIN P: ");
        Serial.println(memory.ein_press);
        Serial.print("CCC T°: ");
        Serial.println(memory.ccc_temp);
        Serial.print("CCC P: ");
        Serial.println(memory.ccc_press);
        Serial.print("OIN T°: ");
        Serial.println(memory.oin_temp);
        Serial.print("EIN T° (PT1000): ");
        Serial.println(memory.ein_temp_pt1000);
        Serial.print("OIN P: ");
        Serial.println(memory.oin_press);
        memory.time_print = time;
    }
    #endif
}

// =============== status LED configuration ===============

void status_led(RGBColor color) {
    digitalWrite(RGB_RED, color.red);
    digitalWrite(RGB_GREEN, color.green);
    digitalWrite(RGB_BLUE, color.blue);
}

void turn_on_sequence()
{
  digitalWrite(LED_BUILTIN, HIGH);

  status_led(BLUE);
  delay(500);
  status_led(GREEN);
  delay(500);
  status_led(RED);
  delay(500);
  status_led(WHITE);
  tone(BUZZER, 440, 1000);
  delay(1000);
  noTone(BUZZER);
  status_led(OFF);

  digitalWrite(LED_BUILTIN, LOW);
}
