// Last update: 11/03/2025
#include "PRBComputer.h"
#include "Wire.h"


#define TEST_WITHOUT_PRESSURE

PRBComputer::PRBComputer(systemState state)
{
    state = state;
    time_start_sq = 0;
    stage_sq = 0;
    ignition_sq_ready = false;
}

PRBComputer::~PRBComputer()
{
    state = ERROR;
}

uint8_t PRBComputer::read_valve_state()
{
    uint8_t state = 0;
    state |= digitalRead(IE_nc) << 2;
    state |= digitalRead(IO_ncC) << 1;
    state |= digitalRead(MOSFET) << 0;
    return state;
}

void PRBComputer::set_valve_states(uint8_t state)
{
    digitalWrite(IE_nc, (state & (1<<2)) ? HIGH : LOW);
    digitalWrite(IO_ncC, (state & (1<<1)) ? HIGH : LOW);
    digitalWrite(MOSFET, (state & (1<<0)) ? HIGH : LOW);
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
    case VE_no:
    case VO_noC:
    case IE_nc:
    case IO_ncC:
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
    case VE_no:
    case VO_noC:
    case IE_nc:
    case IO_ncC:
    case MOSFET:
        digitalWrite(valve, LOW);
        break;
    
    default:
        break;
    }
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
        int rawValue = analogRead(P_OIN);
        float voltage = (rawValue / 4095.0) * 3.3; // Assuming a 12-bit ADC and 3.3V reference
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
    int i2cAddress = 0x00;
    bool I2C = false;
    int value = 0.0;
    float voltage = 0.0;
    int DSP_T = 0;
    float temp = 0.0;

    switch (sensor)
    {
    case T_OIN:


    case T_EIN:
        //read analog temperature
        value = analogRead(sensor);
        Serial.print("Value: ");
        Serial.println(value);
        //convert the value to voltage 
        voltage = (value * 3.3) / 4095.0; // Assuming a 12-bit ADC and 5V reference
        Serial.print("Voltage: ");
        Serial.println(voltage);
        //convert the voltage to temperature
        temp = 0.2667 * (voltage * 1100.0 / (5-voltage) - 266.7); // Example conversion formula
        Serial.print("Temperature: ");
        Serial.println(temp);
        return temp;
        break;

        //TODO to work on : how to differenciat between the 3 sensors
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
    if (sensor == P_OIN)
    {
        //read analog pressure
    } else if (sensor == EIN_CH || sensor == CIG_CH || sensor == CCC_CH)
    {
        float pressure = read_pressure(sensor);
        if (pressure > threshold)
        {
            return true;
        }
        else
        {
            return false;
        }
    } else
    {
        //error
    }
    
    return false;
}

void PRBComputer::set_time_start_sq(int time)
{
    time_start_sq = time;
}


// ========= getter =========
int PRBComputer::get_time_start_sq()
{
    return time_start_sq;
}

int PRBComputer::get_stage_sq()
{
    return stage_sq;
}


// ========= ignition sequences =========
bool PRBComputer::ignition_sq1(int time)
{
    //ignition sequence 1 (ISQ1)

    //Pre-chill : MO-b to 50°
    if (stage_sq == 0) 
    {
        control_motor_angle(MO_bC, 50);
        stage_sq = 1;
    }


    //after 5s
    //Pre-chill : open IO-nc
    if (time - time_start_sq >= 5000 && stage_sq == 1)
    {
        open_valve(IO_ncC);
        stage_sq += 1;
    }


    //after 10s
    //Pre-chill : close IO-nc
    //Stop chill : MO-b to 0°
    //Fill channels : ME-b to 30°
    //Activate : MOSFET (I-GP)
    if (time - time_start_sq >= 10000 && stage_sq == 2)
    {
        close_valve(IO_ncC);
        control_motor_angle(ME_b, 30);
        control_motor_angle(MO_bC, 0);
        open_valve(MOSFET);
        stage_sq += 1;
    }


    //after 12.5s
    //Stop fill : MO-b to 0°
    if (time - time_start_sq >= 12500 && stage_sq == 3)
    {
        control_motor_angle(MO_bC, 0);
        stage_sq += 1;
    }


    //after 15s
    //Ignition : Open I0-ncC
    //Ignition : Open IE-nc
    if (time - time_start_sq >= 15000 && stage_sq == 4)
    {
        open_valve(IO_ncC);
        open_valve(IE_nc);
        stage_sq += 1;
    }


    //after 15.2s
    //check pressure : P_CIG > 15 bar
    //change state
    if (time - time_start_sq >= 15200 && stage_sq == 5)
    {
        #ifdef TEST_WITHOUT_PRESSURE
        state = IGNITION_SQ2;
        stage_sq = 0;
        return true;
        #else
        if (check_pressure(CIG, 15))
        {
            state = IGNITION_SQ2;
            stage_sq = 0;
            return true;
        }
        else
        {
            state = ABORT;
            stage_sq = 0;
            return false;
        }
        #endif
    }

    return true;
}

bool PRBComputer::ignition_sq2(int time)
{
    //ignition sequence 2 (ISQ2)

    //MO-b to 40°
    //ME-b to 40°
    if (stage_sq == 0)
    {
        control_motor_angle(MO_bC, 40);
        control_motor_angle(ME_b, 40);
        stage_sq += 1;
    }


    //after 200ms
    //check pressure : P_CCC > 2 bar
    //change state
    if (time - time_start_sq >= 200 && stage_sq == 1)
    {
        #ifdef TEST_WITHOUT_PRESSURE
        state = IGNITION_SQ3;
        stage_sq = 0;
        return true;
        #else
        if (check_pressure(CCC, 2))
        {
            state = IGNITION_SQ3;
            stage_sq = 0;
            return true;
        }
        else
        {
            state = ABORT;
            stage_sq = 0;
            return false;
        }
        #endif
    }

    return true;
}

bool PRBComputer::ignition_sq3(int time)
{
    //ignition sequence 3 (ISQ3)

    //MO-b to 90°
    //ME-b to 90°
    //Close : IO-ncC
    //Close : IE-nc
    //Deactivate : MOSFET (I-GP)
    if (stage_sq == 0)
    {
        control_motor_angle(MO_bC, 90);
        control_motor_angle(ME_b, 90);
        close_valve(IO_ncC);
        close_valve(IE_nc);
        close_valve(MOSFET);
        stage_sq += 1;
    }

    //after 100ms
    //check pressure : P_CCC > 25 bar
    //change state
    if (time - time_start_sq >= 100 && stage_sq == 1)
    {
        #ifdef TEST_WITHOUT_PRESSURE
        state = IGNITION_SQ4;
        stage_sq = 0;
        return true;
        #else
        if (check_pressure(CCC, 25))
        {
            state = IGNITION_SQ4;
            stage_sq = 0;
            return true;
        }
        else
        {
            state = ABORT;
            stage_sq = 0;
            return false;
        }
        #endif
    }

    return true;
}

bool PRBComputer::ignition_sq4(int time)
{
    //ignition sequence 4 (ISQ4)

    //after 15s
    //MO-b to 0°
    if (time - time_start_sq >= 15000 && stage_sq == 0)
    {
        control_motor_angle(MO_bC, 0);
        stage_sq += 1;
    }


    //after 20s
    //ME-b to 0°
    if (time - time_start_sq >= 20000 && stage_sq == 1)
    {
        control_motor_angle(ME_b, 0);
        stage_sq += 1;
    }


    //after 22s
    //MO-b to 90°
    if (time - time_start_sq >= 22000 && stage_sq == 2)
    {
        control_motor_angle(MO_bC, 90);
        stage_sq += 1;
    }

    
    //after 50s 
    //ME-b to 90°
    if (time - time_start_sq >= 50000 && stage_sq == 3)
    {
        control_motor_angle(ME_b, 90);
        stage_sq += 1;
    }

    //after 100s
    //Open : VO-noC
    //Open : VE-no
    //change state
    if (time - time_start_sq >= 100000 && stage_sq == 4)
    {
        open_valve(VO_noC);
        open_valve(VE_no);
        state = IDLE;
        stage_sq = 0;
        return true;
    }

    return true;
}

void PRBComputer::manual_aboart()
{
    //manual abort

    //check pressure : P_CCC > 25 bar
    //MO-b to 0°
    //change state
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