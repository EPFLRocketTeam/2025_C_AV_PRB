// Last update: 19/08/2025
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
float PRBComputer::read_pressure(int sensor)
{
    // Serial.println("Reading pressure...");
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

    return press;
}


float PRBComputer::read_temperature(int sensor)
{
    //read temperature
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
        //error
        break;
    }
    
    if (I2C)
    {
        DSP_T = my_sensor.readDSP_T();
        temp = DSP_T * 82.5 / 16000 + 42.5;
    }

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


// ========= ignition sequences =========
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
        // mobile mean on the pressure over 5 values
        memory.ccc_press_buffer[memory.ccc_press_index] = memory.ccc_press; // in Pa
        memory.ccc_press_index = (memory.ccc_press_index + 1) % 5;
        float sum_ccc_press = 0.0;
        for (int i = 0; i < 5; i++) {
            sum_ccc_press += memory.ccc_press_buffer[i];
        }
        float mean_ccc_press = sum_ccc_press / 5.0;

        if (millis() - memory.time_ignition >= RAMPUP_DURATION) {
            if (mean_ccc_press >= PRESSURE_CHECK_THRESHOLD) { // CHECK 5 value before
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
            memory.time_abort = millis();
            passivation_phase = SLEEP;
        }
        break;

    default:
        break;
    }
}

void PRBComputer::abort_sq()
{
    // Handle abort sequence
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
                } else {
                    state = IDLE;
                }
            }
            break;

        default:
            break;
    }
}

void selectI2CChannel(int channel) {
    Wire2.beginTransmission(MUX_ADDR);
    noInterrupts();
    Wire2.write(channel); // Enable only the selected channel
    interrupts();
    Wire2.endTransmission();
}

void PRBComputer::ignite(int time)
{
    // Start the ignition sequence
    state = IGNITION_SQ;
    ignition_phase = PRE_CHILL;
    memory.time_ignition = time;
}

void PRBComputer::update(int time)
{
    // Update the state machine
    // Declare variables outside the switch to avoid bypassing initialization

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

    memory.ein_temp_sensata = read_temperature(EIN_CH);
    memory.ein_press = read_pressure(EIN_CH);
    memory.ccc_temp = read_temperature(CCC_CH);
    memory.ccc_press = read_pressure(CCC_CH);
    memory.oin_temp = read_temperature(T_OIN);
    memory.ein_temp_pt1000 = read_temperature(T_EIN);
    memory.oin_press = read_pressure(P_OIN);

    #ifdef INTEGRATE_CHAMBER_PRESSURE
    if (memory.calculate_integral) {
        if (memory.integral_past_time == 0) {
            memory.integral_past_time = time;
        } else {
            memory.integral += memory.ccc_press * (time - memory.integral_past_time); // in bar.s
            memory.integral_past_time = time;
            memory.engine_total_impulse = I_SP * G * (AREA_THROAT/C_STAR) * memory.integral;
        }
    }
    #endif

    // if (time - memory.time_print >= LED_TIMEOUT) {
    //     Serial.print("State : ");
    //     Serial.println(state);
    //     memory.time_print = time;
    // }

    // #ifdef DEBUG
    if (time - memory.time_print >= LED_TIMEOUT) {
        /*Serial.print("State : ");
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
        Serial.println(memory.oin_press);*/
        memory.time_print = time;
    }
    // #endif
}

// ================ Stress test ================

void PRBComputer::stress_test(int cycles, int valve)
{
    int count_cycles = 0;
    bool valve_open = false;

    while (count_cycles < cycles)
    {
        if (valve_open) {
            close_valve(valve);
            status_led(OFF);
            valve_open = false;
        } else {
            open_valve(valve);
            count_cycles++;
            status_led(GREEN);
            valve_open = true;
        }
        delay(250);
    }
    Serial.print("Stress test completed. Iterations: ");
    Serial.println(count_cycles);
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
