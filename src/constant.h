// Last update: 19/08/2025
#include <Arduino.h>
#include "vector"

// ================= ifdef defines =================
// #define DEBUG
#define TEST_WITHOUT_PRESSURE
#define INTEGRATE_CHAMBER_PRESSURE
// #define KULITE
#define VSTF_AND_COLD_FLOW

// PINs attribition on Teensy
#define ME_b        37
#define MO_bC       36
#define IGNITER     35          //I-GP

#define T_EIN       PIN_A12
#define T_OIN       PIN_A13

#define RESET       9
#define RGB_RED     PIN_A7
#define RGB_GREEN   PIN_A8
#define RGB_BLUE    PIN_A9
#define BUZZER      PIN_A1

// Adresses I2C
#define MUX_ADDR    0x70        // 0xE0
#define SENS_ADDR   0x6C        //or 0x6C
#define EIN_CH      0x01        // channel 1
#define CCC_CH      0x02        // channel 2

#ifdef KULITE
#define P_OIN       PIN_A6
#else
#define P_OIN       0x04        // channel 3
#endif

// Ignition sequence constants
// ================= Ignition sequence timing =================
#define PRECHILL_DURATION       200             // 200ms -> prechill duration
#define IGNITER_DURATION        4000            // 4s -> ignite
#define IGNITION_DELAY          250             // 250ms -> start burn
#define RAMPUP_DURATION         200             // 200ms -> pressure check
#define BURN_DURATION           4250            // 4.25s -> stop burn
#define CUTOFF_DELAY            250             // Close ME_b valve
#define PASSIVATION_DELAY       120000          // 120s -> max passivation

// ================= Shutdown sequence timing =================

// #define CLOSE_MO_bC_DELAY    0               // Close MO_bC valve
#define PASSIVATION_FUEL_DURATION   10000           // Open ME_b valve
#define PASSIVATION_OX_DURATION     10000           // Open MO_bC valve
// #define OPEN_VE_VO_DELAY     100000          // Open VE_VO valve

// ================= Abort sequence timing =================
#define ABORT_PASSIVATION_DELAY 5000           // 5s -> max passivation

// Pressure check
#ifdef TEST_WITHOUT_PRESSURE
#define RAMP_UP_CHECK_PRESSURE -1
#else
#define RAMP_UP_CHECK_PRESSURE 27.5          //for 5.38 kN of thrust
#endif

// cst to calculate motor power
#define G                       9.80665         //[m/s^2]
#define I_SP                    250.87          //[N.s] specific impulse
#define AREA_THROAT             0.001364411     //[m^2]
#define C_STAR                  2437.28         //[m/S]
#define BUILD_UP_POWER          750             //[N.s]
#define MIN_BURN_TIME           4500            //[ms] -> 4.5s min time burn
#define MAX_BURN_TIME           5000            //[ms] -> 5s max time burn

#define FLIGHT_IMPULSE          32939           //[N.s] flight impulse
#define CUTOFF_IMPULSE          374.292         //[N.s] cutoff impulse
#define I_TARGET                (FLIGHT_IMPULSE - CUTOFF_IMPULSE)           //[N.s] target impulse

// Status 
#define LED_TIMEOUT 1000 // 1 second

enum ignitionStage
{
    PRE_CHILL,
    IGNITION,
    BURN_START_ME,
    BURN_START_MO,
    PRESSURE_CHECK,
    BURN,
    BURN_STOP_MO,
    BURN_STOP_ME,
    WAIT_FOR_PASSIVATION,
    NOGO
};

enum passivationStage
{
    SLEEP,
    PASSIVATION_ETH,
    PASSIVATION_LOX,
    SHUTOFF
};

enum abortStage
{
    ABORT_OXYDANT,
    ABORT_ETHANOL,
    WAIT_FOR_PASSIVATION_ABORT,
};

struct RGBColor {
    int red;
    int green;
    int blue;
};

const RGBColor RED    = {HIGH, LOW, LOW};
const RGBColor GREEN  = {LOW, HIGH, LOW};
const RGBColor BLUE   = {LOW, LOW, HIGH};
const RGBColor WHITE  = {HIGH, HIGH, HIGH};
const RGBColor PURPLE = {HIGH, LOW, HIGH};
const RGBColor ORANGE = {HIGH, HIGH, LOW};
const RGBColor TEAL   = {LOW, HIGH, HIGH};
const RGBColor OFF    = {LOW, LOW, LOW};
