#include "classPRB.h"

classPRB::classPRB(/* args */)
{
    state = IDLE;
}

classPRB::~classPRB()
{
    state = ERROR;
}

void classPRB::control_motor_angle(int motor, int angle)
{
    //control motor
}

void classPRB::open_valve(int valve)
{
    //open valve
}

void classPRB::close_valve(int valve)
{
    //close valve
}

float classPRB::read_pressure(int sensor)
{
    //read pressure
    return 0.0;
}

float classPRB::read_temperature(int sensor)
{
    //read temperature
    return 0.0;
}

bool classPRB::check_pressure(int sensor, float threshold)
{
    //check pressure
    return false;
}

void classPRB::set_time_start_sq(int time)
{
    time_start_sq = time;
}

int classPRB::get_time_start_sq()
{
    return time_start_sq;
}

bool classPRB::ignition_sq1(int time)
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
            return false;
        }
    }

    return true;
}

bool classPRB::ignition_sq2(int time)
{
    //ignition sequence 2 (ISQ2)

    //MO-b to 40°
    //ME-b to 40°


    //after 200ms
    //check pressure : P_CCC > 2 bar
    //change state

    return true;
}

bool classPRB::ignition_sq3(int time)
{
    //ignition sequence 3 (ISQ3)

    //MO-b to 90°
    //ME-b to 90°
    //Close : IO-ncC
    //Close : IE-nc
    //Deactivate : MOSFET (I-GP)

    //after 100ms
    //check pressure : P_CCC > 25 bar
    //change state

    return true;
}

bool classPRB::ignition_sq4(int time)
{
    //ignition sequence 4 (ISQ4)

    //after 15s
    //MO-b to 0°


    //after 20s
    //ME-b to 0°


    //after 22s
    //MO-b to 90°

    
    //after 50s 
    //ME-b to 90°

    //after 100s
    //Open : VO-noC
    //Open : VE-no
    //change state

    return true;
}

void classPRB::manual_aboart()
{
    //manual abort

    //check pressure : P_CCC > 25 bar
    //MO-b to 0°
    //change state
}
