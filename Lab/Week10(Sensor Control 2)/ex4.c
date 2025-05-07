#include <stdio.h>
#include <wiringPi.h>
#include <lcd.h>
#include <softTone.h>

// 핀 정의
#define TRIG_PIN 27       // 초음파 센서 Trig 핀
#define ECHO_PIN 22       // 초음파 센서 Echo 핀
#define BUZZER_PIN 17     // 부저 핀
#define LCD_RS 2          // LCD RS 핀
#define LCD_E 4           // LCD Enable 핀
#define LCD_D4 20         // LCD D4 핀
#define LCD_D5 21         // LCD D5 핀
#define LCD_D6 12         // LCD D6 핀
#define LCD_D7 16         // LCD D7 핀

// 초음파 센서로 거리 측정
float getDistance(void) {
    long startTime, endTime;
    float distance;

    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    while (digitalRead(ECHO_PIN) == LOW);
    startTime = micros();

    while (digitalRead(ECHO_PIN) == HIGH);
    endTime = micros();

    distance = (endTime - startTime) * 0.034 / 2.0;
    return distance;
}

// 부저 초기화
void initBuzzer(void) {
    softToneCreate(BUZZER_PIN);
}

// 일정 주파수 부저음 제어
void alertBuzzer(float distance) {
    int delayTime = 0;

    if (distance < 30) {
        delayTime = 30; 
    } else if (distance < 50) {
        delayTime = 250; 
    } else if (distance < 70) {
        delayTime = 500; 
    } else if (distance < 100) {
        delayTime = 1000;
    } else {
        softToneWrite(BUZZER_PIN, 0);
        return;
    }

    // 부저음 출력 (주파수 2093Hz)
    softToneWrite(BUZZER_PIN, 2093);
    delay(30);              // 부저 음 지속 시간
    softToneWrite(BUZZER_PIN, 0); // 부저 음 끔
    delay(delayTime - 30);  // 30cm 미만에 설정한 딜레이와 동일하게 시간 설정
}

// 메인 함수
int main(void) {
    int lcdHandle;

    // GPIO 초기화
    if (wiringPiSetupGpio() == -1) {
        printf("GPIO setup failed!\n");
        return 1;
    }

    // 초음파 센서 핀 설정
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    // LCD 초기화
    lcdHandle = lcdInit(2, 16, 4, LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7, 0, 0, 0, 0);
    if (lcdHandle == -1) {
        printf("LCD initialization failed!\n");
        return 1;
    }
    lcdClear(lcdHandle);

    // 부저 초기화
    initBuzzer();

    while (1) {
        float distance = getDistance(); // 거리 측정

        // 거리 출력 (LCD)
        lcdClear(lcdHandle);
        lcdPosition(lcdHandle, 0, 0);
        lcdPrintf(lcdHandle, "Distance: %.2fcm", distance);

        // 경고음 제어
        alertBuzzer(distance);

        delay(30); // 위에 30cm 미만으로 설정한 딜레이와 동일하게 거리 갱신하며 T-LCD에 출력
    }

    return 0;
}