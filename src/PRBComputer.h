// Last update: 19/08/2025
#include "constant.h"
#include "./2024_C_AV_INTRANET/intranet_commands.h"
#include "PTE7300_I2C.h"

typedef struct prb_memory_t
{
    int time_ignition;              // time @ which ignition starts [ms]
    int time_start_passivation;        // time @ which shutdown starts [ms]
    int time_start_abort;           // time @ which abort starts [ms]
    bool status_led;                // status LED state
    bool ME_state;                  // ME valve state
    bool MO_state;                  // MO valve state
    bool IGNITER_state;             // IGNITER state
    int time_led;                   // time @ which LED state changes [ms]
    int time_print;                 // time @ which print occurs [ms]
    float oin_temp;                 // OIN temperature (PT1000) [°C]
    float ein_temp_pt1000;          // EIN temperature (PT1000) [°C]
    float oin_press;                // OIN pressure (Kulite) [bar]
    float ein_temp_sensata;         // EIN temperature (Sensata) [°C]
    float ein_press;                // EIN pressure (Sensata) [bar]
    float ccc_temp;                 // CCC temperature (Sensata) [°C]
    float ccc_press;                // CCC pressure (Sensata) [bar]
    float integral;                 // integral [bar.s]
    float engine_specific_impulse;  // engine specific impulse [N.s]
    int integral_past_time;         // past time for integral calculation [ms]
    bool calculate_integral;
    bool liftoff_detected;
}prb_memory_t;


class PRBComputer
{
private:
    PRB_FSM state;
    ignitionStage ignition_phase;
    passivationStage passivation_phase;
    abortStage abort_phase;

    PTE7300_I2C my_sensor;

    prb_memory_t memory;

    //sensor reading
    float read_pressure(int sensor);
    float read_temperature(int sensor);

    //valves sequences
    void ignition_sq();
    void passivation_sq(int time);
    void abort_on_gnd_sq(int time);
    
    // status LED configuration
    void status_led_ignition();
    void status_led_shutdown();

    // testing
    void stress_test(int cycles, int valve);

public:
    PRBComputer(PRB_FSM);
    ~PRBComputer();

    //valve control
    void open_valve(int valve);
    void close_valve(int valve);

    //getters
    prb_memory_t get_memory();
    PRB_FSM get_state();
    ignitionStage get_ignition_stage();
    passivationStage get_shutdown_stage();

    //setters
    void set_state(PRB_FSM new_state);

    void ignite(int time);

    void update(int time);
};


void selectI2CChannel(int channel); 

void status_led(RGBColor color);
void turn_on_sequence();