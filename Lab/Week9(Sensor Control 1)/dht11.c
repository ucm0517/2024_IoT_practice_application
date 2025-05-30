#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MAXTIMING 83
#define DHTPIN 26

int dht11_dat[5] = {0, };

void read_dht11_dat() 
{
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;

    dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;

    pinMode(DHTPIN, OUTPUT);
    digitalWrite(DHTPIN, LOW);
    delay(18);

    digitalWrite(DHTPIN, HIGH);
    delayMicroseconds(30);
    pinMode(DHTPIN, INPUT);

    for (i = 0; i < MAXTIMING; i++) {

        counter = 0;
        while (digitalRead(DHTPIN) == laststate) {
            counter++;
            delayMicroseconds(1);
            if (counter == 200) break;
        } 
        
        laststate = digitalRead(DHTPIN);

        if (counter == 200) break;
        if ((i >= 4) && (i % 2 == 0)) {
            dht11_dat[j / 8] <<= 1;
            if (counter > 50) dht11_dat[j / 8] |= 1;
            j++;
        }
    }

    if ((j >= 40) && (dht11_dat[4] == ((dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xff))) {
        printf("humidity = %d.%d %% Temperature = %d.%d *C \n", dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3]);
    } else {
        printf("Data get failed\n");
    }
}

int main(void)
{
    printf("dht11 Raspberry pi\n");
    if (wiringPiSetupGpio() == -1) return 1;

    while (1) {
        read_dht11_dat();
        delay(2000);
    }

    return 0;
}