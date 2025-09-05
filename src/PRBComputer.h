#ifndef PRB_COMPUTER_H
#define PRB_COMPUTER_H
/*
 * File: PRBComputer.h
 * Author: C - AV Team
 * Last update: 05/09/2025
 *
 * Description:
 *  This header file declares the PRBComputer class, which manages the core logic and control
 *  of the PRB (Propulsion Rocket Bench) system. It provides:
 *    - State machine management for ignition, passivation, and abort sequences
 *    - Valve control methods (open/close for main engine, oxidizer, and igniter)
 *    - Sensor reading functions for pressure and temperature (analog and I2C)
 *    - Memory structure for storing system state, sensor data, and control flags
 *    - Getters and setters for system state and memory
 *    - High-level ignition and shutdown sequence logic
 *
 *  The class interfaces with hardware via digital and analog I/O, as well as I2C communication.
 *  It is designed for embedded use in the PRB avionics system.
 */

#include "constant.h"
#include "./2024_C_AV_INTRANET/intranet_commands.h"
#include "PTE7300_I2C.h"

typedef struct prb_memory_t
{
    int time_ignition;              // time @ which ignition starts [ms]
    int time_passivation;           // time @ which shutdown starts [ms]
    int time_abort;                 // time @ which abort starts [ms]
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
    float ccc_press_buffer[5];      // CCC pressure buffer for moving average [bar]
    int ccc_press_index;            // Index for circular buffer
    float integral;                 // integral [bar.s]
    float engine_total_impulse;     // engine specific impulse [N.s]
    int integral_past_time;         // past time for integral calculation [ms]
    bool calculate_integral;        // flag to start/stop integral calculation
    bool passivation;               // flag to start/stop passivation sequence
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
    void passivation_sq();
    void abort_sq();

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
    void set_passivation(bool passiv);
    void set_passivation_stage(passivationStage new_stage);

    void ignite(int time);

    void update(int time);
};


void selectI2CChannel(int channel); 
void endI2CCommunication();

void status_led(RGBColor color);
void turn_on_sequence();

#endif // PRB_COMPUTER_H