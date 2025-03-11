// Last update: 11/03/2025
#include "PRBComputer.h"

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


// ========= sensor reading =========
float PRBComputer::read_pressure(int sensor)
{
    if (sensor == P_OIN || sensor == T_OIN || sensor == T_EIN)
    {
        //read analog pressure
    } else if (sensor == EIN || sensor == CIG || sensor == CCC)
    {
        //read I2C pressure
    } else {
        //error
    }
    
    return 0.0;
}

float PRBComputer::read_temperature(int sensor)
{
    //read temperature
    return 0.0;
}

bool PRBComputer::check_pressure(int sensor, float threshold)
{
    if (sensor == P_OIN)
    {
        //read analog pressure
    } else if (sensor == EIN || sensor == CIG || sensor == CCC)
    {
        //read I2C pressure
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
