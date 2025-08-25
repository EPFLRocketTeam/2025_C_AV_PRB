// Last update: 19/08/2025
#include "constant.h"
#include "./2024_C_AV_INTRANET/intranet_commands.h"
#include "PTE7300_I2C.h"

typedef struct prb_memory_t
{
    int time_start_ignition;
    int time_start_shutdown;
    bool status_led;
    bool ME_state;
    bool MO_state;
    bool IGNITER_state;
    int time_led;
    int time_print;
    float oin_temp;
    float oin_press;
    float ein_temp;
    float ein_press;
    float ccc_temp;
    float ccc_press;
}prb_memory_t;


class PRBComputer
{
private:
    prometheusFSM state;
    ignitionStage ignition_stage;
    shutdownStage shutdown_stage;

    PTE7300_I2C my_sensor;

    prb_memory_t memory;

    //sensor reading
    float read_pressure(int sensor);
    float read_temperature(int sensor);
    bool check_pressure(int sensor);

    //valves sequences
    void ignition_sq(int time);
    void shutdown_sq(int time);
    
    // status LED configuration
    void status_led_ignition();
    void status_led_shutdown();

    // testing
    std::vector<float> test_read_sensors();
    std::vector<float> test_read_pt1000();
    float test_read_kulite();
    void stress_test(int cycles, int valve);
    void test_valves();

public:
    PRBComputer(prometheusFSM);
    ~PRBComputer();

    //valve control
    void open_valve(int valve);
    void close_valve(int valve);

    //getters
    prb_memory_t get_memory();
    prometheusFSM get_state();
    ignitionStage get_ignition_stage();
    shutdownStage get_shutdown_stage();

    //setters
    void set_state(prometheusFSM new_state);

    void ignite(int time);

    void request_manual_abort();

    void update(int time);
};


void selectI2CChannel(int channel); 

void status_led(RGBColor color);
void turn_on_sequence();