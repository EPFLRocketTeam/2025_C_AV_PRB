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