#include <wiringPi.h>

#define FAN_MT_P_PIN    20
#define FAN_MT_N_PIN    21

void FanOn(void);
void FanOff(void);

int main(void)
{
    if (wiringPiSetupGpio() == -1)
        return 1;
    
    pinMode(FAN_MT_P_PIN, OUTPUT);
    pinMode(FAN_MT_N_PIN, OUTPUT);

    while(1)
    {
        FanOn();
        delay(2000);

        FanOff();
        delay(2000);
    }
    return 0;
}

void FanOn(void)
{
    digitalWrite(FAN_MT_P_PIN, HIGH);
    digitalWrite(FAN_MT_N_PIN, LOW);
}

void FanOff(void)
{
    digitalWrite(FAN_MT_P_PIN, LOW);
    digitalWrite(FAN_MT_N_PIN, LOW);
}