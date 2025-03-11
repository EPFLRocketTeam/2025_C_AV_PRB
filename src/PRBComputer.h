// Last update: 11/03/2025
#include "constant.h"

class PRBComputer
{
private:
    systemState state;
    int time_sart_sequence;
    int time_start_sq;
    int stage_sq;
public:
    PRBComputer(/* args */);
    ~PRBComputer();

    //valve and motor control
    void control_motor_angle(int motor, int angle);
    void open_valve(int valve);
    void close_valve(int valve);

    //sensor reading
    float read_pressure(int sensor);
    float read_temperature(int sensor);
    bool check_pressure(int sensor, float threshold);

    void set_time_start_sq(int time);

    //getter
    int get_time_start_sq();
    int get_stage_sq();

    //ignition sequences
    bool ignition_sq1(int time);
    bool ignition_sq2(int time);
    bool ignition_sq3(int time);
    bool ignition_sq4(int time);

    void manual_aboart();
};




