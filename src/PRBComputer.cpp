// Last update: 11/03/2025
#include "PRBComputer.h"
#include "Wire.h"


#define TEST_WITHOUT_PRESSURE

PRBComputer::PRBComputer(systemState state)
{
    state = state;
    time_start_ignition = 0;
    ignition_stage = NOGO;
    shutdown_stage = SLEEP;
}

PRBComputer::~PRBComputer()
{
    state = ERROR;
}

// ========= valve and motor control =========
void PRBComputer::control_motor_angle(int motor, int angle)
{
    //control motor
}

void PRBComputer::open_valve(int valve)
{
    switch (valve)
    {
    case ME_b:
    case MO_bC:
    case MOSFET:
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
    case MOSFET:
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
    float voltage = 0.0, Rpt = 0.0, temp = 0.0;
    int DSP_T = 0;
    // float temp = 0.0;

    switch (sensor)
    {
    case T_OIN:
    case T_EIN:
        //read analog temperature
        value = analogRead(sensor);
        Serial.print("Value: ");
        Serial.println(value);
        //convert the value to voltage 
        voltage = (value * 3.3) / 4095.0; // Assuming a 12-bit ADC and 3V3 reference
        Serial.print("Voltage: ");
        Serial.println(voltage);
        //convert the voltage to temperature
        Rpt = (voltage * 1100.0)/(3.3 - voltage);
        Serial.print("Resistance measured: ");
        Serial.println(Rpt);
        temp = (Rpt-1000)/3.85;
        Serial.print("Temperature: ");
        Serial.println(temp);
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

bool PRBComputer::check_pressure(int sensor, float threshold)
{
    bool check = false;

    float pressure = read_pressure(sensor);
    if (pressure > threshold) {
        check = true;
    }
    
    return check;
}

void PRBComputer::set_time_start_sq(int time)
{
    time_start_ignition = time;
}


// ========= getter =========
int PRBComputer::get_time_start_sq() { return time_start_ignition;}

ignitionStage PRBComputer::get_ignition_stage() { return ignition_stage; }

shutdownStage PRBComputer::get_shutdown_stage() { return shutdown_stage; }

// ========= setter =========
void PRBComputer::set_state(systemState new_state) { state = new_state; }

void PRBComputer::set_ignition_stage(ignitionStage new_stage) { ignition_stage = new_stage; }

void PRBComputer::set_ignition_start_time(int time) { time_start_ignition = time; }

void PRBComputer::set_shutdown_stage(shutdownStage new_stage) { shutdown_stage = new_stage; }

// ========= ignition sequences =========
bool PRBComputer::ignition_sq(int time)
{
    //ignition sequence 1 (ISQ1)
    if (ignition_stage == GO)
    {
        ignition_stage = PRE_CHILL;
    }

    //Pre-chill : open MO-b
    if (ignition_stage == PRE_CHILL) 
    {
        open_valve(MO_bC);
        ignition_stage = POST_CHILL;
    }

    // after 10s
    // stop pre-chill : close MO-b
    if (ignition_stage == POST_CHILL && time - time_start_ignition >= time_sq_ignition[1])
    {
        close_valve(MO_bC);
        ignition_stage = IGNITION;
    }

    //after 15s
    //Activate : MOSFET (IGNITER)
    if (ignition_stage == IGNITION && time - time_start_ignition >= time_sq_ignition[2])
    {
        open_valve(MOSFET);
        ignition_stage = LIFTOFF;
    }

    //after 16s
    //Open MO-b and ME-b
    if (ignition_stage == LIFTOFF && time - time_start_ignition >= time_sq_ignition[3])
    {
        open_valve(MO_bC);
        open_valve(ME_b);
        ignition_stage = SANITY_CHECK;
    }

    //after 100ms
    if (ignition_stage == SANITY_CHECK && time - time_start_ignition >= time_sq_ignition[4])
    {
        // Perform sanity check
        float P_CCC_pres = read_pressure(CCC_CH);
        if (P_CCC_pres > -25) { // !!!!! CHANGE VALUE !!!!!!
            state = SHUTDOWN_SQ;
            ignition_stage = NOGO;
            shutdown_stage = CLOSE_MO_bC;
            time_start_shutdown = time;
        } else {
            // state = ABORT;
        }
    }
    return true;
}

bool PRBComputer::shutdown_sq(int time)
{
    //shutdown sequence
    if (shutdown_stage == CLOSE_MO_bC && time - time_start_shutdown >= time_sq_shutdown[0])
    {
        close_valve(MO_bC);
        shutdown_stage = CLOSE_ME_b;
    }

    if (shutdown_stage == CLOSE_ME_b && time - time_start_shutdown >= time_sq_shutdown[1])
    {
        close_valve(ME_b);
        shutdown_stage = OPEN_MO_bC;
    }

    if (shutdown_stage == OPEN_MO_bC && time - time_start_shutdown >= time_sq_shutdown[2])
    {
        open_valve(MO_bC);
        shutdown_stage = OPEN_ME_b;
    }

    if (shutdown_stage == OPEN_ME_b && time - time_start_shutdown >= time_sq_shutdown[3])
    {
        open_valve(ME_b);
        shutdown_stage = SHUTOFF;
    }
}

void PRBComputer::manual_abort()
{
    //manual abort

    //check pressure : P_CCC > 25 bar
    //MO-b to 0°
    //change state

    state = ABORT;
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
    switch (state)
    {
    case IDLE:
        // Do nothing
        break;
    case IGNITION_SQ:
        ignition_sq(time);
        break;
    case SHUTDOWN_SQ:
        shutdown_sq(time);
        break;
    case ABORT:
        manual_abort();
        break;
    default:
        break;
    }
}

void config_rgb_led_1(ignitionStage stage){
    switch (stage)
  {
  case PRE_CHILL:
    digitalWrite(RGB_GREEN, HIGH);
    digitalWrite(RGB_RED, LOW);
    digitalWrite(RGB_BLUE, LOW);
    break;

  case POST_CHILL:
    digitalWrite(RGB_BLUE, HIGH);
    digitalWrite(RGB_GREEN, LOW);
    digitalWrite(RGB_RED, LOW);
    break;

  case IGNITION:
    digitalWrite(RGB_RED, HIGH);
    digitalWrite(RGB_GREEN, LOW);
    digitalWrite(RGB_BLUE, LOW);
    break;

  case LIFTOFF:
    digitalWrite(RGB_RED, HIGH);
    digitalWrite(RGB_BLUE, HIGH);
    digitalWrite(RGB_GREEN, LOW);
    break;

  case SANITY_CHECK:
    digitalWrite(RGB_RED, LOW);
    digitalWrite(RGB_GREEN, HIGH);
    digitalWrite(RGB_BLUE, LOW);
    break;

  default:
    break;
  }
}

void config_rgb_led_2(shutdownStage stage){
    switch (stage)
  {
  case CLOSE_MO_bC:
    digitalWrite(RGB_GREEN, HIGH);
    digitalWrite(RGB_RED, HIGH);
    digitalWrite(RGB_BLUE, LOW);
    break;

  case CLOSE_ME_b:
    digitalWrite(RGB_GREEN, LOW);
    digitalWrite(RGB_RED, HIGH);
    digitalWrite(RGB_BLUE, HIGH);
    break;

  case OPEN_MO_bC:
  case OPEN_ME_b:
    digitalWrite(RGB_GREEN, HIGH);
    digitalWrite(RGB_RED, LOW);
    digitalWrite(RGB_BLUE, LOW);
    break;

  default:
    break;
  }
}

void test_read_sensata(PRBComputer* computer)
{
    // Test reading sensors
    float ein_temp = computer->read_temperature(EIN_CH);
    float ccc_temp = computer->read_temperature(CCC_CH);
    float ein_press = computer->read_pressure(EIN_CH);
    float ccc_press = computer->read_pressure(CCC_CH);

    Serial.print("EIN Temperature: ");
    Serial.println(ein_temp);
    Serial.print("EIN Pressure: ");
    Serial.println(ein_press);
    Serial.print("CCC Temperature: ");
    Serial.println(ccc_temp);
    Serial.print("CCC Pressure: ");
    Serial.println(ccc_press);
}

void test_read_pt1000(PRBComputer* computer)
{
    // Test reading PT1000 sensors
    float ein_temp = computer->read_temperature(T_EIN);
    float oin_temp = computer->read_temperature(T_OIN);

    Serial.print("EIN Temperature: ");
    Serial.println(ein_temp);
    Serial.print("OIN Temperature: ");
    Serial.println(oin_temp);
}

void test_read_kulite(PRBComputer* computer)
{
    // Test reading Kulite sensors
    float oin_press = computer->read_pressure(P_OIN);

    Serial.print("OIN Pressure: ");
    Serial.println(oin_press);
}

void stress_test(PRBComputer* computer, int cycles, int valve)
{
    int count_cycles = 0;
    bool valve_open = false;

    while (count_cycles < cycles)
    {
        if (valve_open) {
            computer->close_valve(valve);
            digitalWrite(RGB_GREEN, LOW);
            valve_open = false;
        } else {
            computer->open_valve(valve);
            count_cycles++;
            // Serial.print("Cycle count: ");
            // Serial.println(count_cycles);
            digitalWrite(RGB_GREEN, HIGH);
            valve_open = true;
        }
        delay(250);
    }
    Serial.print("Stress test completed. Iterations: ");
    Serial.println(count_cycles);
}