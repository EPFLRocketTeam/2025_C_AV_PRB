// Last update: 19/08/2025
#include "PRBComputer.h"
#include "Wire.h"

PRBComputer::PRBComputer(PRB_FSM state)
{
    state = state;
    memory.time_start_ignition = 0;
    memory.time_start_shutdown = 0;
    ignition_stage = NOGO;
    shutdown_stage = SLEEP;
    memory.status_led = false;
    memory.time_led = 0;
    memory.time_print = 0;
    memory.ME_state = false;
    memory.MO_state = false;
    memory.IGNITER_state = false;
    memory.integral = 0.0;
    memory.integral_past_time = 0;
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

    int max_kulite_value = 100;

    switch (sensor)
    {
    case P_OIN: {
        #ifdef KULITE
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
        press = DSP_S * 5.0 / 1600 + 50;
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
    // float temp = 0.0;

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

bool PRBComputer::check_pressure(int sensor) { 
    return (read_pressure(sensor) > PRESSURE_CHECK_THRESHOLD); 
}


// ========= getter =========
prb_memory_t PRBComputer::get_memory() { return memory; }
PRB_FSM PRBComputer::get_state() { return state; }
ignitionStage PRBComputer::get_ignition_stage() { return ignition_stage; }
shutdownStage PRBComputer::get_shutdown_stage() { return shutdown_stage; }


// ========= setter =========
void PRBComputer::set_state(PRB_FSM new_state) { state = new_state; }


// ========= ignition sequences =========
void PRBComputer::ignition_sq(int time)
{
    switch (ignition_stage)
    {
    case GO:
        ignition_stage = PRE_CHILL;
        break;
    
    case PRE_CHILL:
        if (time - memory.time_start_ignition >= PRE_CHILL_DELAY) {
            open_valve(MO_bC);
            ignition_stage = STOP_CHILL;
        }
        break;
    
    case STOP_CHILL:
        if (time - memory.time_start_ignition >= STOP_CHILL_DELAY) {
            close_valve(MO_bC);
            ignition_stage = IGNITION;
        }
        break;
    
    case IGNITION:
        if (time - memory.time_start_ignition >= IGNITION_DELAY) {
            open_valve(IGNITER);
            ignition_stage = BURN_START;
        }
        break;

    case BURN_START:
        if (time - memory.time_start_ignition >= BURN_START_DELAY) {
            open_valve(MO_bC);
            open_valve(ME_b);
            ignition_stage = PRESSURE_CHECK;
        }
        break;

    case PRESSURE_CHECK:
        if (time - memory.time_start_ignition >= PRESSURE_CHECK_DELAY) {
            if (check_pressure(CCC_CH)) {
                ignition_stage = PRESSURE_OK;
            } else {
                state = ABORT_PRB;
                memory.time_start_abort = time;
                abort_stage = ABORT_MO;
            }
        }
        break;
    
    case PRESSURE_OK:
        if (time - memory.time_start_ignition >= STOP_IGNITER_DELAY) {
            close_valve(IGNITER);
            ignition_stage = BURN;
        }
        break;
    
    case BURN:

        #ifdef INTEGRATE_CHAMBER_PRESSURE

        if (memory.integral_past_time == 0) {
            memory.integral_past_time = time;
            break;
        }
        
        memory.integral += memory.ccc_press * (time - memory.integral_past_time); // in Pa.s
        memory.integral_past_time = time;

        memory.engine_specific_impulse = I_SP * G * (AREA_EXHAUST/C_STAR) * memory.integral;

        if (time - memory.time_start_ignition <= (BURN_START_DELAY + MIN_BURN_TIME)) {
            break;
        }

        if (memory.integral >= (I_TARGET * C_STAR) / (I_SP * G * AREA_EXHAUST) ||
            time - memory.time_start_ignition >= (BURN_START_DELAY + MAX_BURN_TIME)) {
            state = SHUTDOWN_SQ;
            memory.time_start_shutdown = time;
            shutdown_stage = CLOSE_MO_bC;
            ignition_stage = NOGO;
        }

        #else

        if (time - memory.time_start_ignition >= IGNITION_ENDED) {
            state = SHUTDOWN_SQ;
            memory.time_start_shutdown = time;
            shutdown_stage = CLOSE_MO_bC;
            ignition_stage = NOGO;
        }
        break;

        #endif

    default:
        break;
    }
    //ignition sequence 1 (ISQ1)
    // if (ignition_stage == GO)
    // {
    //     ignition_stage = PRE_CHILL;
    // }

    // //Pre-chill : open MO-b
    // if (ignition_stage == PRE_CHILL && time - memory.time_start_ignition >= PRE_CHILL_DELAY) 
    // {
    //     open_valve(MO_bC);
    //     ignition_stage = STOP_CHILL;
    // }

    // // after 10s
    // // stop pre-chill : close MO-b
    // if (ignition_stage == STOP_CHILL && time - memory.time_start_ignition >= STOP_CHILL_DELAY)
    // {
    //     close_valve(MO_bC);
    //     ignition_stage = IGNITION;
    // }

    // //after 15s
    // //Ignition : MOSFET (IGNITER)
    // if (ignition_stage == IGNITION && time - memory.time_start_ignition >= IGNITION_DELAY)
    // {
    //     open_valve(IGNITER);
    //     ignition_stage = BURN_START;
    // }

    // //after 16s
    // //Liftoff : Open MO-b and ME-b
    // if (ignition_stage == BURN_START && time - memory.time_start_ignition >= BURN_START_DELAY)
    // {
    //     open_valve(MO_bC);
    //     open_valve(ME_b);
    //     // close_valve(IGNITER);
    //     ignition_stage = PRESSURE_CHECK;
    // }

    // //after 500ms
    // if (ignition_stage == PRESSURE_CHECK && time - memory.time_start_ignition >= PRESSURE_CHECK_DELAY)
    // {
    //     if (check_pressure(CCC_CH)) {
    //         ignition_stage = PRESSURE_OK;
    //     } else {
    //         state = ABORT;
    //         memory.time_start_abort = time;
    //         abort_stage = ABORT_MO;
    //     }
    // }


}

void PRBComputer::shutdown_sq(int time)
{
    switch (shutdown_stage)
    {
    case CLOSE_MO_bC:
        if (time - memory.time_start_shutdown >= CLOSE_MO_bC_DELAY)
        {
            close_valve(MO_bC);
            shutdown_stage = CLOSE_ME_b;
        }
        break;

    case CLOSE_ME_b:
        if (time - memory.time_start_shutdown >= CLOSE_ME_b_DELAY)
        {
            close_valve(ME_b);
            shutdown_stage = OPEN_MO_bC;
        }
        break;

    case OPEN_MO_bC:
        if (time - memory.time_start_shutdown >= OPEN_MO_bC_DELAY)
        {
            open_valve(MO_bC);
            shutdown_stage = OPEN_ME_b;
        }
        break;

    case OPEN_ME_b:
        if (time - memory.time_start_shutdown >= OPEN_ME_b_DELAY)
        {
            open_valve(ME_b);
            shutdown_stage = SHUTOFF;
        }
        break;

    default:
        break;
    }
}

void PRBComputer::abort_sq(int time)
{
    // Handle abort sequence
    switch (abort_stage)
    {
        case ABORT_MO:
            if (time - memory.time_start_abort >= ABORT_MO_DELAY)
            {
                close_valve(MO_bC);
                abort_stage = ABORT_ME;
            }
            break;

        case ABORT_ME:
            if (time - memory.time_start_abort >= ABORT_ME_DELAY)
            {
                close_valve(ME_b);
            }
            break;

    }
}

void selectI2CChannel(int channel) {
    Wire2.beginTransmission(MUX_ADDR);
    Wire2.write(channel); // Enable only the selected channel
    Wire2.endTransmission();
}

void PRBComputer::ignite(int time)
{
    // Start the ignition sequence
    state = IGNITION_SQ;
    memory.time_start_ignition = time;
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
            ignition_stage = GO;
            break;

        case IGNITION_SQ:
            ignition_sq(time);
            status_led_ignition();
            break;

        case SHUTDOWN_SQ:
            shutdown_sq(time);
            status_led_shutdown();
            break;

        case ABORT_PRB:
            abort_sq(time);
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

    #ifdef DEBUG
    if (time - memory.time_print >= LED_TIMEOUT) {
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
void PRBComputer::status_led_ignition(){
    switch (ignition_stage)
    {
    case PRE_CHILL:
        status_led(GREEN);
        break;

    case STOP_CHILL:
        status_led(BLUE);
        break;

    case IGNITION:
        status_led(RED);
        break;

    case BURN_START:
        status_led(TEAL);
        break;

    case PRESSURE_CHECK:
        status_led(ORANGE);
        break;

    default:
        break;
    }
}

void PRBComputer::status_led_shutdown(){
    switch (shutdown_stage)
    {
    case CLOSE_MO_bC:
        status_led(BLUE);
        break;

    case CLOSE_ME_b:
        status_led(PURPLE);
        break;

    case OPEN_MO_bC:
    case OPEN_ME_b:
        status_led(GREEN);
        break;

    default:
        break;
    }
}

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
