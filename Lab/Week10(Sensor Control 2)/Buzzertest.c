#include <stdio.h>
#include <wiringPi.h>
#include <softTone.h>

#define BUZZER_PIN 17

int main() {
    if (wiringPiSetupGpio() == -1) {
        printf("GPIO setup failed!\n");
        return 1;
    }

    softToneCreate(BUZZER_PIN);

    while (1) {
        softToneWrite(BUZZER_PIN, 2093); // 2093Hz 소리 발생
        delay(1000);                    // 1초 동안 소리
        softToneWrite(BUZZER_PIN, 0);   // 소리 끔
        delay(1000);                    // 1초 대기
    }

    return 0;
}