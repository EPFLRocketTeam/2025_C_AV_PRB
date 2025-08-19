// Last update: 11/03/2025
#include <Arduino.h>
#include "vector"

// PINs attribition on Teensy
#define ME_b        37
#define MO_bC       36
#define IGNITER     35          //I-GP

#define T_EIN       PIN_A12
#define T_OIN       PIN_A13
#define P_OIN       PIN_A6

#define RESET       9
#define RGB_RED     PIN_A7
#define RGB_GREEN   PIN_A8
#define RGB_BLUE    PIN_A9
#define BUZZER      PIN_A1

// Adresses I2C
#define SLAVE_ADDR  0x0F
#define MUX_ADDR    0x70 // 0xE0
#define SENS_ADDR   0x6C        //or 0x6C
#define EIN_CH      0x01        // channel 1
#define CCC_CH      0x02        // channel 2
#define CIG_CH      0x04        // channel 3

// Ignition sequence constants
// ================= Before Burn =================
#define PRE_CHILL_DELAY 0
#define STOP_CHILL_DELAY 10000
#define IGNITION_DELAY 15000
#define BURN_DELAY 16000
#define PRESSURE_CHECK_DELAY 16100

// ================= After Burn =================
#define CLOSE_MO_bC_DELAY 15000
#define CLOSE_ME_b_DELAY 20000
#define OPEN_MO_bC_DELAY 22000
#define OPEN_ME_b_DELAY 50000
#define OPEN_VE_VO_DELAY 150000

// Pressure check
#define PRESSURE_CHECK_THRESHOLD 25

// Status 
#define LED_TIMEOUT 1000 // 1 second

enum prometheusFSM
{
    IDLE,
    WAKEUP,
    TEST,
    SETUP,
    WAIT,
    ARM,
    IGNITION_SQ,
    SHUTDOWN_SQ,
    ABORT,
    ERROR
};

enum ignitionStage
{
    GO,
    PRE_CHILL,
    POST_CHILL,
    IGNITION,
    LIFTOFF,
    PRESSURE_CHECK,
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

const std::vector<int> time_sq_ignition = { 
    0,
    10000,
    15000,
    16000,
    16100,
};

const std::vector<int> time_sq_shutdown = {
    15000,
    20000,
    22000,
    50000,
};