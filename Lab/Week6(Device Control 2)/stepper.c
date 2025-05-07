#include <stdio.h> 
#include <wiringPi.h> 

#define PINA 20
#define PINB 21 
#define PINC 19 
#define PIND 26 

void setsteps(int w1, int w2, int w3, int w4) 
{
    digitalWrite(PINA,w1); 
    digitalWrite(PINB,w2); 
    digitalWrite(PINC,w3); 
    digitalWrite(PIND,w4); 
}




void forward(int del,int steps)
{
    int i;
        for(i=0; i<steps; i++)
        {
            setsteps(1,1,0,0); 
            delay(del); 
            setsteps(0,1,1,0); 
            delay(del); 
            setsteps(0,0,1,1); 
            delay(del); 
            setsteps(1,0,0,1); 
            delay(del); 
        }
}

void backward (int del, int steps)
{
    int k;
        for(k=0; k<steps; k++)
        {
            setsteps(1,0,0,1);
            delay(del);
            setsteps(0,0,1,1);
            delay(del);
            setsteps(0,1,1,0);
            delay(del);
            setsteps(1,1,0,0);
            delay(del);

        }
}

int main(void)
{
    if (wiringPiSetupGpio() == -1) return 1;
    pinMode(PINA, OUTPUT);
    pinMode(PINB, OUTPUT);
    pinMode(PINC, OUTPUT);
    pinMode(PIND, OUTPUT);

    for(;;)
    {
        forward(5, 512);
        delay(1000);
        backward(5, 512);
        delay(1000);
    }

    return 0;
}