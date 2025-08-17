// Last update: 11/03/2025
#include "constant.h"
#include "intranet_commands.h"
#include "PTE7300_I2C.h"


class PRBComputer
{
private:
    prometheusFSM state;
    ignitionStage ignition_stage;
    shutdownStage shutdown_stage;
    int time_start_ignition;
    int time_start_shutdown;
    PTE7300_I2C my_sensor;
    int16_t value_sensor;
    bool status_led;
    int time_led;
public:
    PRBComputer(prometheusFSM);
    ~PRBComputer();

    //valve and motor control
    void open_valve(int valve);
    void close_valve(int valve);

    //sensor reading
    float read_pressure(int sensor);
    float read_temperature(int sensor);
    bool check_pressure(int sensor);

    
    //getters
    int get_time_start_sq();
    prometheusFSM get_state();
    ignitionStage get_ignition_stage();
    shutdownStage get_shutdown_stage();

    //setters
    void set_time_start_sq(int time);
    void set_state(prometheusFSM new_state);
    void set_ignition_stage(ignitionStage new_stage);
    void set_ignition_start_time(int time);
    void set_shutdown_stage(shutdownStage new_stage);

    //valves sequences
    void ignition_sq(int time);
    void shutdown_sq(int time);

    void request_manual_abort();

    void update(int time);
    void send_update();

    // testing
    std::vector<float> test_read_sensors();
    std::vector<float> test_read_pt1000();
    float test_read_kulite();
    void stress_test(int cycles, int valve);
    void test_valves();

    // status LED configuration
    void status_led_ignition();
    void status_led_shutdown();
};


void selectI2CChannel(int channel); 

void status_led_off();
void status_led_blue();
void status_led_green();
void status_led_red();
void status_led_orange();
void status_led_purple();
void status_led_teal();
void status_led_white();