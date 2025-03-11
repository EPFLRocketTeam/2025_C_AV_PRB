// Last update: 11/03/2025
#include <Arduino.h>

// Constantes
#define SLAVE_ADDR  0x0F
#define ME_b        0x01
#define MO_bC       0x02
#define EIN         0xDA        //or 0x6C
#define CCC         0xDA        //or 0x6C
#define CIG         0xDA        //or 0x6C
#define T_EIN       PIN_A17
#define T_OIN       PIN_A16
#define P_OIN       PIN_A13
#define MOSFET      33          //I-GP
#define VE_no       2
#define VO_noC      3
#define IE_nc       4
#define IO_ncC      5
#define SDA_SENSOR  25
#define SCL_SENSOR  24

enum systemState
{
    IDLE,
    WAKEUP,
    TEST,
    SETUP,
    WAIT,
    IGNITION_SQ1,
    IGNITION_SQ2,
    IGNITION_SQ3,
    IGNITION_SQ4,
    ABORT,
    ERROR
};