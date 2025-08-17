// Last update: 11/03/2025
#include "PRBComputer.h"
#include "Wire.h"


#define TEST_WITHOUT_PRESSURE

PRBComputer::PRBComputer(prometheusFSM state)
{
    state = state;
    time_start_ignition = 0;
    time_start_shutdown = 0;
    ignition_stage = NOGO;
    shutdown_stage = SLEEP;
    status_led = false;
    time_led = 0;
}

PRBComputer::~PRBComputer()
{
    state = ERROR;
}

// ========= valve and motor control =========
void PRBComputer::open_valve(int valve)
{
    switch (valve)
    {
    case ME_b:
    case MO_bC:
    case IGNITER:
        digitalWrite(valve, HIGH);
        break;

    default:
        break;
    }
}

void PRBComputer::close_valve(int valve)
{
    switch (valve)
    {
    case ME_b:
    case MO_bC:
    case IGNITER:
        digitalWrite(valve, LOW);
        break;
    
    default:
        break;
    }
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
        int rawValue = analogRead(P_OIN);
        float voltage = (rawValue / 4095.0) * 3.3; // Assuming a 12-bit ADC and 3.3V reference
        float v_sensor = voltage / 33;
        press = (v_sensor * 1000.0) * (max_kulite_value / 100.0);
        Serial.println("voltage: " + String(voltage));
        //read analog pressure
        break;
    }

        //to work on : how to differenciat between the 3 sensors
    case EIN_CH:
    case CIG_CH:
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

        //to work on : how to differenciat between the 3 sensors
    case EIN_CH:
    case CIG_CH:
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

bool PRBComputer::check_pressure(int sensor)
{
    bool check = false;

    float pressure = read_pressure(sensor);
    if (pressure > PRESSURE_CHECK_THRESHOLD) {
        check = true;
    }
    
    return check;
}


// ========= getter =========
int PRBComputer::get_time_start_sq() { return time_start_ignition;}
prometheusFSM PRBComputer::get_state() { return state; }
ignitionStage PRBComputer::get_ignition_stage() { return ignition_stage; }
shutdownStage PRBComputer::get_shutdown_stage() { return shutdown_stage; }

// ========= setter =========
void PRBComputer::set_state(prometheusFSM new_state) { state = new_state; }
void PRBComputer::set_time_start_sq(int time) { time_start_ignition = time; }
void PRBComputer::set_ignition_stage(ignitionStage new_stage) { ignition_stage = new_stage; }
void PRBComputer::set_ignition_start_time(int time) { time_start_ignition = time; }
void PRBComputer::set_shutdown_stage(shutdownStage new_stage) { shutdown_stage = new_stage; }


// ========= ignition sequences =========
void PRBComputer::ignition_sq(int time)
{
    //ignition sequence 1 (ISQ1)
    if (ignition_stage == GO)
    {
        ignition_stage = PRE_CHILL;
    }

    //Pre-chill : open MO-b
    if (ignition_stage == PRE_CHILL && time - time_start_ignition >= PRE_CHILL_DELAY) 
    {
        open_valve(MO_bC);
        ignition_stage = POST_CHILL;
    }

    // after 10s
    // stop pre-chill : close MO-b
    if (ignition_stage == POST_CHILL && time - time_start_ignition >= STOP_CHILL_DELAY)
    {
        close_valve(MO_bC);
        ignition_stage = IGNITION;
    }

    //after 15s
    //Ignition : MOSFET (IGNITER)
    if (ignition_stage == IGNITION && time - time_start_ignition >= IGNITION_DELAY)
    {
        open_valve(IGNITER);
        ignition_stage = LIFTOFF;
    }

    //after 16s
    //Liftoff : Open MO-b and ME-b
    if (ignition_stage == LIFTOFF && time - time_start_ignition >= BURN_DELAY)
    {
        open_valve(MO_bC);
        open_valve(ME_b);
        ignition_stage = PRESSURE_CHECK;
    }

    //after 100ms
    if (ignition_stage == PRESSURE_CHECK && time - time_start_ignition >= PRESSURE_CHECK_DELAY)
    {
        // Perform pressure check
        bool test_result = check_pressure(CCC_CH);

        if (test_result) {
            state = SHUTDOWN_SQ;
            ignition_stage = NOGO;
            shutdown_stage = CLOSE_MO_bC;
            time_start_shutdown = time;
        } else {
            state = REQUEST_ABORT;
        }
    }
}

void PRBComputer::shutdown_sq(int time)
{
    switch (shutdown_stage)
    {
    case CLOSE_MO_bC:
        if (time - time_start_shutdown >= CLOSE_MO_bC_DELAY)
        {
            close_valve(MO_bC);
            shutdown_stage = CLOSE_ME_b;
        }
        break;

    case CLOSE_ME_b:
        if (time - time_start_shutdown >= CLOSE_ME_b_DELAY)
        {
            close_valve(ME_b);
            shutdown_stage = OPEN_MO_bC;
        }
        break;

    case OPEN_MO_bC:
        if (time - time_start_shutdown >= OPEN_MO_bC_DELAY)
        {
            open_valve(MO_bC);
            shutdown_stage = OPEN_ME_b;
        }
        break;

    case OPEN_ME_b:
        if (time - time_start_shutdown >= OPEN_ME_b_DELAY)
        {
            open_valve(ME_b);
            shutdown_stage = SHUTOFF;
        }
        break;

    default:
        break;
    }
}

void PRBComputer::request_manual_abort()
{
    //resquest aboart from FC
}

// ========= send update =========
void PRBComputer::send_update()
{
    //send update
}


void selectI2CChannel(int channel) {
    Wire2.beginTransmission(MUX_ADDR);
    Wire2.write(channel); // Enable only the selected channel
    Wire2.endTransmission();
}

void PRBComputer::update(int time)
{
    // Update the state machine
    // Declare variables outside the switch to avoid bypassing initialization
    std::vector<float> sensor_values;
    std::vector<float> pt1000_values;
    float kulite_value = 0.0;

    switch (state)
    {
        case IDLE:
            if (!status_led && time - time_led >= LED_TIMEOUT) {
                status_led_teal();
                time_led = time;
                status_led = true;
            } else if (status_led && time - time_led >= LED_TIMEOUT) {
                status_led_off();
                time_led = time;
                status_led = false;
            }
            break;
        case WAKEUP:
            tone(BUZZER, 480, 1000);
            // Handle WAKEUP state if needed, otherwise do nothing
            break;
        case TEST:
            status_led_orange();
            test_valves();
            sensor_values = test_read_sensors();
            pt1000_values = test_read_pt1000();
            kulite_value = test_read_kulite();
            Serial.println("Test done");
            state = IDLE; // Return to IDLE after test
            status_led_off();
            break;
        
        case ARM:
            if (!status_led && time - time_led >= LED_TIMEOUT) {
                status_led_orange();
                time_led = time;
                status_led = true;
            } else if (status_led && time - time_led >= LED_TIMEOUT) {
                status_led_off();
                time_led = time;
                status_led = false;
            }
            break;

        case IGNITION_SQ:
            ignition_sq(time);
            break;
        case SHUTDOWN_SQ:
            shutdown_sq(time);
            break;
        case REQUEST_ABORT:
            status_led_red();
            tone(BUZZER, 440, 2500);
            request_manual_abort();
            break;

        default:
            break;
    }
}

// ================ testing ================
std::vector<float> PRBComputer::test_read_sensors()
{
    // Test reading sensors
    float ein_temp = read_temperature(EIN_CH);
    float ccc_temp = read_temperature(CCC_CH);
    float ein_press = read_pressure(EIN_CH);
    float ccc_press = read_pressure(CCC_CH);

    return { ein_temp, ccc_temp, ein_press, ccc_press };
}

std::vector<float> PRBComputer::test_read_pt1000()
{
    // Test reading PT1000 sensors
    float ein_temp = read_temperature(T_EIN);
    float oin_temp = read_temperature(T_OIN);

    return { ein_temp, oin_temp };
}

float PRBComputer::test_read_kulite()
{
    // Test reading Kulite sensors
    float oin_press = read_pressure(P_OIN);

    return oin_press;
}

void PRBComputer::stress_test(int cycles, int valve)
{
    int count_cycles = 0;
    bool valve_open = false;

    while (count_cycles < cycles)
    {
        if (valve_open) {
            close_valve(valve);
            status_led_off();
            valve_open = false;
        } else {
            open_valve(valve);
            count_cycles++;
            status_led_green();
            valve_open = true;
        }
        delay(250);
    }
    Serial.print("Stress test completed. Iterations: ");
    Serial.println(count_cycles);
}

void PRBComputer::test_valves()
{
    // Test opening and closing valves
    open_valve(ME_b);
    Serial.println("ME_b valve opened");
    delay(500);
    open_valve(MO_bC);
    Serial.println("MO_bC valve opened");
    delay(1000);
    close_valve(ME_b);
    Serial.println("ME_b valve closed");
    delay(500);
    close_valve(MO_bC);
    Serial.println("MO_bC valve closed");
}


// =============== status LED configuration ===============
void PRBComputer::status_led_ignition(){
    switch (ignition_stage)
    {
    case PRE_CHILL:
        status_led_green();
        break;

    case POST_CHILL:
        status_led_blue();
        break;

    case IGNITION:
        status_led_red();
        break;

    case LIFTOFF:
        status_led_teal();
        break;

    case PRESSURE_CHECK:
        status_led_orange();
        break;

    default:
        break;
    }
}

void PRBComputer::status_led_shutdown(){
    switch (shutdown_stage)
    {
    case CLOSE_MO_bC:
        status_led_blue();
        break;

    case CLOSE_ME_b:
        status_led_purple();
        break;

    case OPEN_MO_bC:
    case OPEN_ME_b:
        status_led_green();
        break;

    default:
        break;
    }
}

void status_led_off() {
    digitalWrite(RGB_GREEN, LOW);
    digitalWrite(RGB_RED, LOW);
    digitalWrite(RGB_BLUE, LOW);
}

void status_led_blue() {
    digitalWrite(RGB_BLUE, HIGH);
}

void status_led_green() {
    digitalWrite(RGB_GREEN, HIGH);
}

void status_led_red() {
    digitalWrite(RGB_RED, HIGH);
}

void status_led_white() {
    digitalWrite(RGB_RED, HIGH);
    digitalWrite(RGB_GREEN, HIGH);
    digitalWrite(RGB_BLUE, HIGH);
}

void status_led_orange() {
    digitalWrite(RGB_RED, HIGH);
    digitalWrite(RGB_GREEN, HIGH);
}

void status_led_purple() {
    digitalWrite(RGB_RED, HIGH);
    digitalWrite(RGB_BLUE, HIGH);
}

void status_led_teal() {
    digitalWrite(RGB_GREEN, HIGH);
    digitalWrite(RGB_BLUE, HIGH);
}
