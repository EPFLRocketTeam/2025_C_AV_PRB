// Last update: 19/08/2025
#include <Arduino.h>
#include "vector"

// ================= ifdef defines =================
#define DEBUG
#define TEST_WITHOUT_PRESSURE
#define INTEGRATE_CHAMBER_PRESSURE
#define KULITE

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
#define SLAVE_ADDR  0x0F
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
#define PRE_CHILL_DELAY         0              // start pre-chill
#define STOP_CHILL_DELAY        4000           // 4s -> stop pre-chill
#define IGNITION_DELAY          9000           // 5s -> ignite
#define BURN_START_DELAY        10000          // 1s -> start burn
#define PRESSURE_CHECK_DELAY    10100          // 100ms -> pressure check
#define STOP_IGNITER_DELAY      13000          // 3s -> stop igniter
#define IGNITION_ENDED          16000          // 6s -> stop burn

// ================= Shutdown sequence timing =================

#define CLOSE_MO_bC_DELAY       0               // Close MO_bC valve
#define CLOSE_ME_b_DELAY        5000            // Close ME_b valve
#define OPEN_MO_bC_DELAY        7000            // Open MO_bC valve
#define OPEN_ME_b_DELAY         35000           // Open ME_b valve
#define OPEN_VE_VO_DELAY        150000          // Open VE_VO valve

// ================= Abort sequence timing =================
#define ABORT_MO_DELAY          0
#define ABORT_ME_DELAY          1000

// Pressure check
#define PRESSURE_CHECK_THRESHOLD 25

// cst to calculate motor power
#define G                       9.81    //[m/s^2]
#define I_SP                    1.0     //[N.s] specific impulse
#define I_TARGET                2.0     //[N.s] target impulse
#define AREA_EXHAUST            0.01    //[m^2]
#define C_STAR                  1.0     //[]
#define MIN_BURN_TIME           5000    //[ms]
#define MAX_BURN_TIME           6000    //[ms]

// Status 
#define LED_TIMEOUT 1000 // 1 second

enum ignitionStage
{
    GO,
    PRE_CHILL,
    STOP_CHILL,
    IGNITION,
    BURN_START,
    PRESSURE_CHECK,
    PRESSURE_OK,
    BURN,
    NOGO
};

enum shutdownStage
{
    SLEEP,
    CLOSE_MO_bC,
    CLOSE_ME_b,
    OPEN_MO_bC,
    OPEN_ME_b,
    SHUTOFF
};


enum abortStage
{
    ABORT_MO,
    ABORT_ME
};

// enum sensorName
// {
//     P_OIN_SENS,
//     T_OIN_PT1000,
//     T_EIN_PT1000,
//     P_EIN_SENS,
//     T_EIN_SENS,
//     P_CCC_SENS,
//     T_CCC_SENS,
// };

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
