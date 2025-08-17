// Last update: 11/03/2025
#include "constant.h"
#include "intranet_commands.h"
#include "PTE7300_I2C.h"


class PRBComputer
{
private:
    systemState state;
    int time_start_ignition;
    int time_start_shutdown;
    ignitionStage ignition_stage;
    shutdownStage shutdown_stage;
    PTE7300_I2C my_sensor;
    int16_t value_sensor;
public:
    PRBComputer(systemState);
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

    //getters
    int get_time_start_sq();
    ignitionStage get_ignition_stage();
    shutdownStage get_shutdown_stage();

    //setters
    void set_state(systemState new_state);
    void set_ignition_stage(ignitionStage new_stage);
    void set_ignition_start_time(int time);
    void set_shutdown_stage(shutdownStage new_stage);

    //ignition sequences
    bool ignition_sq(int time);

    bool shutdown_sq(int time);

    void manual_abort();

    void send_update();



    void update(int time);
};


void selectI2CChannel(int channel); 

void config_rgb_led_1(ignitionStage);
void config_rgb_led_2(shutdownStage);

void test_read_sensata(PRBComputer*);
void test_read_pt1000(PRBComputer*);
void test_read_kulite(PRBComputer*);

void stress_test(PRBComputer*, int cycles, int valve);