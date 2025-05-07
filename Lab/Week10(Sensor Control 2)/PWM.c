#include <stdio.h>
#include <wiringPi.h>
#include <lcd.h>

// 핀 정의
#define TRIG_PIN 27       // 초음파 센서 Trig 핀
#define ECHO_PIN 22       // 초음파 센서 Echo 핀
#define BUZZER_PIN 18     // PWM으로 제어할 부저 핀 (GPIO 18 - PWM 핀)
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

// PWM 초기화
void initPWM(void) {
    pinMode(BUZZER_PIN, PWM_OUTPUT); // 부저 핀을 PWM 출력으로 설정
    pwmSetMode(PWM_MODE_MS);         // 마크-스페이스 모드 설정?
    pwmSetRange(1024);               // PWM 범위 설정 (0~2048 중 적당한 범위 설정)
    pwmSetClock(9);                // PWM 주파수 설정 (주파수 계산: 기본 클럭 / 설정 값) => 2093Hz 주파수 소리 출력
}

// 일정 주파수 부저음 제어
void alertBuzzer(float distance) {
    int delayTime = 0;
    int pwmValue = 800; // 듀티 사이클 80% (0~1024 범위 중)

    if (distance < 30) {
        delayTime = 30;  // 30cm 이하: 소리 간격 0.03초
    } else if (distance < 50) {
        delayTime = 250; // 30~50cm: 소리 간격 0.25초
    } else if (distance < 70) {
        delayTime = 500; // 50~70cm: 소리 간격 0.5초
    } else if (distance < 100) {
        delayTime = 1000; // 70~100cm: 소리 간격 1초
    } else {
        pwmWrite(BUZZER_PIN, 0); // 100cm 이상: 소리 없음
        return;
    }

    // PWM 신호로 부저음 발생 (2093Hz)
    pwmWrite(BUZZER_PIN, pwmValue); // PWM 듀티 사이클 설정
    delay(30);                      // 부저 음 지속 시간
    pwmWrite(BUZZER_PIN, 0);        // PWM 신호 끔
    delay(delayTime - 30);          // 간격
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

    // PWM 초기화
    initPWM();

    while (1) {
        float distance = getDistance(); // 거리 측정

        // 거리 출력 (LCD)
        lcdClear(lcdHandle);
        lcdPosition(lcdHandle, 0, 0);
        lcdPrintf(lcdHandle, "Distance: %.2fcm", distance);

        // 경고음 제어
        alertBuzzer(distance);

        delay(30); // T-LCD 거리 갱신 주기
    }

    return 0;
}
