#include <stdio.h>       // 표준 입출력을 위한 헤더
#include <wiringPi.h>    // Raspberry Pi의 GPIO 제어를 위한 헤더

// 핀 정의
#define TP 12            // 초음파 센서 Trig 핀 (Trigger Pin)
#define EP 16            // 초음파 센서 Echo 핀 (Echo Pin)

// 초음파 센서를 통해 거리 측정
float getDistance(void)
{
    float fDistance;     // 계산된 거리 값 (단위: cm)
    int nStartTime, nEndTime;  // 초음파 신호의 시작 및 끝 시간을 저장할 변수

    // Trig 핀을 LOW로 설정하여 안정화
    digitalWrite(TP, LOW);
    delayMicroseconds(2);  // 2µs 대기 (안정화 시간)

    // Trig 핀을 HIGH로 설정하여 초음파 신호 발생
    digitalWrite(TP, HIGH);
    delayMicroseconds(10); // 10µs 동안 신호를 유지
    digitalWrite(TP, LOW); // Trig 핀을 다시 LOW로 설정

    // Echo 핀이 LOW 상태에서 HIGH로 변할 때까지 대기 (초음파 신호 송출 시간 시작)
    while (digitalRead(EP) == LOW); // Ep = Echo Pulse
    nStartTime = micros();  // 송신 시작 시간 기록

    // Echo 핀이 HIGH 상태에서 LOW로 변할 때까지 대기 (초음파 신호 반사 시간 종료)
    while (digitalRead(EP) == HIGH);
    nEndTime = micros();    // 수신 종료 시간 기록

    // 거리 계산: 시간(µs)을 거리(cm)로 변환
    // 초음파의 속도는 340m/s, 따라서 1cm 이동에 약 29.412µs 필요
    fDistance = (nEndTime - nStartTime) * 0.034 / 2.0;

    return fDistance;  // 계산된 거리 반환
}

// 메인 함수
int main(void)
{
    // GPIO 초기화
    if (wiringPiSetupGpio() == -1) {
        return 1;  // 초기화 실패 시 프로그램 종료
    }

    // Trig 핀을 출력 모드로 설정
    pinMode(TP, OUTPUT);
    // Echo 핀을 입력 모드로 설정
    pinMode(EP, INPUT);

    // 거리 측정을 반복적으로 수행
    while (1) 
    {
        float fDistance = getDistance();  // 거리 측정 함수 호출
        printf("Distance: %.2f cm\n", fDistance);  // 측정된 거리 출력
        delay(1000);  // 1초 대기 후 다시 측정
    }

    return 0;  // 프로그램 종료
}