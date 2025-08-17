// Last update: 11/03/2025
#include <Arduino.h>
#include "vector"

// Constantes
#define SLAVE_ADDR  0x0F
#define ME_b        37
#define MO_bC       36
#define SENS_ADDR   0x6C
#define EIN_CH      0x01        //or 0x6C
#define CCC_CH      0x02        //or 0x6C
#define CIG_CH      0x04        //or 0x6C
#define T_EIN       PIN_A12
#define T_OIN       PIN_A13
#define P_OIN       PIN_A6
#define MOSFET      35          //I-GP
#define SDA_SENSOR  25
#define SCL_SENSOR  24
#define RESET       9
#define RGB_RED     PIN_A7
#define RGB_GREEN   PIN_A8
#define RGB_BLUE    PIN_A9
#define BUZZER      PIN_A1

#define MUX_ADDR 0x70 // 0xE0

enum systemState
{
    IDLE,
    WAKEUP,
    TEST,
    SETUP,
    WAIT,
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
    SANITY_CHECK,
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