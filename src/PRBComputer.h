
#define ME_b        0x01
#define MO_bC       0x02
#define EIN         0x00
#define CCC         0x00
#define CIG         0x00
#define T_EIN       0x00
#define T_OIN       0x00
#define P_OIN       0x00 
#define MOSFET      0x00 //I-GP
#define VE_no       0x00
#define VO_noC      0x00
#define IE_nc       0x00
#define IO_ncC      0x00

enum systemState
{
    IDLE,
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

class PRBComputer
{
private:
    systemState state;
    int time_sart_sequence;
    int time_start_sq;
    int stage_sq;
public:
    PRBComputer(/* args */);
    ~PRBComputer();

    //valve and motor control
    void control_motor_angle(int motor, int angle);
    void open_valve(int valve);
    void close_valve(int valve);
    float read_pressure(int sensor);
    float read_temperature(int sensor);
    bool check_pressure(int sensor, float threshold);

    void set_time_start_sq(int time);
    int get_time_start_sq();

    bool ignition_sq1(int time);
    bool ignition_sq2(int time);
    bool ignition_sq3(int time);
    bool ignition_sq4(int time);

    void manual_aboart();
};




