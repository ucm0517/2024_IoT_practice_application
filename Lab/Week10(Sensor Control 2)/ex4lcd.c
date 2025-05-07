#include <stdio.h>      
#include <wiringPi.h>   
#include <lcd.h>        
#include <softTone.h>    

// 핀 정의
#define TRIG_PIN 27       // 초음파 센서의 Trig 핀 번호
#define ECHO_PIN 22       // 초음파 센서의 Echo 핀 번호
#define BUZZER_PIN 17     // 부저 핀 번호
#define LCD_RS 2          // LCD RS 핀 번호
#define LCD_E 4           // LCD Enable 핀 번호
#define LCD_D4 20         // LCD 데이터 핀 D4
#define LCD_D5 21         // LCD 데이터 핀 D5
#define LCD_D6 12         // LCD 데이터 핀 D6
#define LCD_D7 16         // LCD 데이터 핀 D7

// 초음파 센서를 사용하여 거리 측정
float getDistance(void) {
    long startTime, endTime; // 초음파 송신 및 수신 시간 기록
    float distance;          // 계산된 거리 값

    // Trig 핀을 LOW로 설정하여 안정화
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2); // 안정화 시간 2µs 대기

    // Trig 핀을 HIGH로 설정하여 초음파 신호 송신
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10); // 초음파 신호 송신 10µs
    digitalWrite(TRIG_PIN, LOW); // 다시 LOW로 설정

    // Echo 핀이 LOW 상태에서 HIGH 상태로 변할 때까지 대기 (신호 송신 시작)
    while (digitalRead(ECHO_PIN) == LOW);
    startTime = micros(); // 송신 시작 시간 기록

    // Echo 핀이 HIGH 상태에서 LOW 상태로 변할 때까지 대기 (신호 수신 종료)
    while (digitalRead(ECHO_PIN) == HIGH);
    endTime = micros(); // 수신 종료 시간 기록

    // 거리 계산: 왕복 시간을 이용하여 거리(cm) 계산
    distance = (endTime - startTime) * 0.034 / 2.0;

    return distance; // 계산된 거리 반환
}

// 부저 초기화
void initBuzzer(void) {
    softToneCreate(BUZZER_PIN); // 부저 핀에 소리 제어 기능 활성화
}

// 일정 주파수 부저음 제어
void alertBuzzer(float distance) {
    int delayTime = 0; // 부저음 간격 설정 변수

    // 거리 값에 따라 부저음 간격 조정
    if (distance < 30) {
        delayTime = 30;  // 30cm 이하: 0.1초 간격
    } else if (distance < 50) {
        delayTime = 250; // 30~50cm: 0.25초 간격
    } else if (distance < 70) {
        delayTime = 500; // 50~70cm: 0.5초 간격
    } else if (distance < 100) {
        delayTime = 1000; // 70~100cm: 1초 간격
    } else {
        softToneWrite(BUZZER_PIN, 0); // 100cm 이상: 부저음 없음
        return;
    }

    // 부저음 출력: 일정 주파수로 소리 발생 (2093Hz)
    softToneWrite(BUZZER_PIN, 2093); // 부저음 발생
    delay(30);                       // 부저음 지속 시간
    softToneWrite(BUZZER_PIN, 0);    // 부저음 끔
    delay(delayTime - 30);           // 부저음 간격 유지
}

// 메인 함수
int main(void) {
    int lcdHandle; // LCD 핸들 (LCD를 제어하기 위한 변수)

    // GPIO 초기화
    if (wiringPiSetupGpio() == -1) {
        printf("GPIO setup failed!\n");
        return 1; // 초기화 실패 시 프로그램 종료
    }

    // 초음파 센서 핀 설정
    pinMode(TRIG_PIN, OUTPUT); // Trig 핀을 출력 모드로 설정
    pinMode(ECHO_PIN, INPUT);  // Echo 핀을 입력 모드로 설정

    // LCD 초기화
    lcdHandle = lcdInit(2, 16, 4, LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7, 0, 0, 0, 0);
    if (lcdHandle == -1) {
        printf("LCD initialization failed!\n");
        return 1; // LCD 초기화 실패 시 프로그램 종료
    }
    lcdClear(lcdHandle); // LCD 화면 초기화

    // 부저 초기화
    initBuzzer();

    // 거리 측정 및 출력 루프
    while (1) {
        float distance = getDistance(); // 초음파 센서를 통해 거리 측정

        // 측정된 거리 값을 LCD에 출력
        lcdClear(lcdHandle);
        lcdPosition(lcdHandle, 0, 0); // LCD의 첫 번째 줄, 첫 번째 칸
        lcdPrintf(lcdHandle, "Distance: ", distance); // 거리 출력
        lcdPosition(lcdHandle, 0, 1); // LCD의 첫 번째 줄, 첫 번째 칸
        lcdPrintf(lcdHandle, "%.2fcm", distance); // 거리 출력

        // 부저를 사용한 경고음 발생
        alertBuzzer(distance);

        delay(30); // 주기적으로 거리 갱신
    }

    return 0; // 프로그램 종료
}