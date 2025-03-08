#include "classPRB.h"

classPRB::classPRB(/* args */)
{
    status = IDLE;
}

classPRB::~classPRB()
{
    status = ERROR;
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

bool classPRB::ignition_sq1(int time)
{
    //ignition sequence 1 (ISQ1)

    //Pre-chill : MO-b to 50°


    //after 5s
    //Pre-chill : open IO-nc


    //after 10s
    //Pre-chill : close IO-nc
    //Stop chill : MO-b to 0°
    //Fill channels : ME-b to 30°
    //Activate : MOSFET (I-GP)


    //after 12.5s
    //Stop fill : MO-b to 0°


    //after 15s
    //Ignition : Open I0-ncC
    //Ignition : Open IE-nc


    //after 15.2s
    //check pressure : P_CIG > 15 bar

    return true;
}

bool classPRB::ignition_sq2(int time)
{
    //ignition sequence 2 (ISQ2)

    //MO-b to 40°
    //ME-b to 40°


    //after 200ms
    //check pressure : P_CCC > 2 bar

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

    return true;
}
